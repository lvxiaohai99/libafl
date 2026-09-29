#pragma once

/**
 * @file Asn1Cpp.h
 * @brief asn1c 生成类型的 C++11 RAII 封装（编解码 / 约束检查 / BIT·OCTET 辅助）
 *
 * 依赖：先包含 asn1c 生成的头（会带入 asn_application.h），再包含本文件。
 *
 * 示例：
 * @code
 *   #include "MessageFrame.h"
 *   #include "afl/asn1/Asn1Cpp.h"
 *   AFL_DECLARE_ASN1_TYPE(MessageFrame)
 *   afl::asn1::MessageFramePtr mf;
 *   mf->present = MessageFrame_PR_bsmFrame;
 *   std::string uper = mf.encode<afl::asn1::B_UPER>();
 *   mf.decode<afl::asn1::B_XER>(xml);
 * @endcode
 *
 * 源自 RSU asn1cpp 思路，作为 afl::asn1 新接口使用（不保留旧命名空间兼容）。
 *
 * 内存所有权规则（违反即泄漏或 double free）：
 *   1. 树上所有节点必须来自 malloc/calloc（fragment / fragmentEnsure / Asn1List::push），
 *      不能用 new、栈或全局变量；根节点由 Asn1Ptr 析构时 ASN_STRUCT_FREE 整棵释放。
 *   2. 释放或清空子结构只能走类型描述符：freeField / resetField / Asn1List::clear / remove。
 *      禁止 asn_sequence_empty / asn_sequence_del(..,1) / 裸 free：生成列表的 list.free
 *      为空，它们只释放指针数组，元素及其子树全部泄漏。
 *   3. 同一子节点不能同时挂在两棵树上；要转移就把原指针置空（Asn1List::pick 同理）。
 *   4. Asn1Ptr(p, false) 只借用，不释放；调用方仍负责 ASN_STRUCT_FREE。
 */

#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <type_traits>

#ifndef ASN_APPLICATION_H
#error "Include asn1c generated headers (e.g. MessageFrame.h) before afl/asn1/Asn1Cpp.h"
#endif

namespace afl {
namespace asn1 {

/** 传输语法（与 asn_transfer_syntax 对齐） */
enum CodecType {
    NONE = ATS_INVALID,
    PLAIN = ATS_NONSTANDARD_PLAINTEXT,
    RANDOM = ATS_RANDOM,
    BER = ATS_BER,
    DER = ATS_DER,
    CER = ATS_CER, /* 仅解码 */

    B_OER = ATS_BASIC_OER,
    C_OER = ATS_CANONICAL_OER,

    B_UPER = ATS_UNALIGNED_BASIC_PER,
    C_UPER = ATS_UNALIGNED_CANONICAL_PER,

    B_XER = ATS_BASIC_XER,
    C_XER = ATS_CANONICAL_XER
};

/** 类型 → asn_DEF_xxx 映射；由 AFL_DECLARE_ASN1_TYPE 特化 */
template <typename T>
struct Asn1TypeTraits {
    static_assert(sizeof(T) == 0,
                  "missing AFL_DECLARE_ASN1_TYPE(Xxx) for this asn1c type");
};

template <typename T>
inline const asn_TYPE_descriptor_t& getTypeDescriptor()
{
    return Asn1TypeTraits<T>::descriptor();
}

/**
 * asn1c 结构体的智能指针封装。
 * - 默认用 ASN_STRUCT_FREE 释放
 * - encode/decode 支持 UPER / XER / BER / OER 等
 */
template <typename T>
class Asn1Ptr : public std::shared_ptr<T> {
public:
    /** 分配一个清零的根节点，析构时整棵释放 */
    Asn1Ptr()
        : std::shared_ptr<T>(static_cast<T*>(calloc(1, sizeof(T))), &Asn1Ptr::deleter)
    {
    }

    /** useDeleter=true 接管 p（须为 calloc/解码产物）；false 仅借用 */
    explicit Asn1Ptr(T* p, bool useDeleter = true)
        : std::shared_ptr<T>(p, useDeleter ? &Asn1Ptr::deleter : &Asn1Ptr::noDeleter)
    {
    }

    const asn_TYPE_descriptor_t& getDescriptor() const
    {
        return getTypeDescriptor<T>();
    }

    /** 编码到内部缓存并返回引用（非 const 对象） */
    template <CodecType CT>
    const std::string& encode()
    {
        m_serialized.clear();
        if (!this->operator bool()) {
            return m_serialized;
        }
        const asn_enc_rval_t enc =
            asn_encode(0, static_cast<asn_transfer_syntax>(CT), &getDescriptor(),
                       this->get(), &Asn1Ptr::appendToString, &m_serialized);
        if (enc.encoded < 0) {
            m_serialized.clear();
        }
        return m_serialized;
    }

    /** 编码到新 string */
    template <CodecType CT>
    std::string encode() const
    {
        std::string out;
        if (!this->operator bool()) {
            return out;
        }
        const asn_enc_rval_t enc =
            asn_encode(0, static_cast<asn_transfer_syntax>(CT), &getDescriptor(),
                       this->get(), &Asn1Ptr::appendToString, &out);
        if (enc.encoded < 0) {
            out.clear();
        }
        return out;
    }

    /** 编码到用户缓冲区；成功时 len 为实际字节数 */
    template <CodecType CT>
    bool encode(char* ptr, int& len)
    {
        if (!this->operator bool() || ptr == 0 || len <= 0) {
            return false;
        }
        const asn_enc_rval_t enc = asn_encode_to_buffer(
            0, static_cast<asn_transfer_syntax>(CT), &getDescriptor(), this->get(), ptr,
            static_cast<size_t>(len));
        if (enc.encoded < 0 || enc.encoded > static_cast<ssize_t>(len)) {
            if (enc.encoded > static_cast<ssize_t>(len)) {
                len = -1;
            }
            return false;
        }
        len = static_cast<int>(enc.encoded);
        return true;
    }

    template <CodecType CT>
    bool decode(const std::string& data, bool record = false)
    {
        return decode<CT>(data.data(), static_cast<int>(data.size()), record);
    }

    template <CodecType CT>
    bool decode(const char* ptr, int len, bool record = false)
    {
        if (ptr == 0 || len <= 0) {
            return false;
        }
        T* m = 0;
        const asn_dec_rval_t dec =
            asn_decode(0, static_cast<asn_transfer_syntax>(CT), &getDescriptor(),
                       reinterpret_cast<void**>(&m), ptr, static_cast<size_t>(len));
        if (dec.code != RC_OK) {
            ASN_STRUCT_FREE(getDescriptor(), m);
            return false;
        }
        this->reset(m, &Asn1Ptr::deleter);
        if (record) {
            m_serialized.assign(ptr, static_cast<size_t>(len));
        }
        return true;
    }

    /** 编码并写入文件（UPER 等二进制 / XER 文本均可） */
    template <CodecType CT>
    bool encodeToFile(const std::string& path)
    {
        const std::string bytes = encode<CT>();
        if (bytes.empty()) {
            return false;
        }
        std::ofstream ofs(path.c_str(), std::ios::binary);
        if (!ofs) {
            return false;
        }
        ofs.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        return static_cast<bool>(ofs);
    }

    /** 从文件解码（.uper → B_UPER，.xml → B_XER） */
    template <CodecType CT>
    bool decodeFromFile(const std::string& path, bool record = false)
    {
        std::ifstream ifs(path.c_str(), std::ios::binary);
        if (!ifs) {
            return false;
        }
        std::string data((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());
        return decode<CT>(data, record);
    }

    /** 经 BER 深拷贝出裸指针（调用方负责 ASN_STRUCT_FREE） */
    T* dragOut()
    {
        T* retval = 0;
        const std::string enc = encode<BER>();
        if (enc.empty()) {
            return 0;
        }
        const asn_dec_rval_t dec =
            asn_decode(0, static_cast<asn_transfer_syntax>(BER), &getDescriptor(),
                       reinterpret_cast<void**>(&retval), enc.data(), enc.size());
        if (dec.code != RC_OK) {
            ASN_STRUCT_FREE(getDescriptor(), retval);
            return 0;
        }
        return retval;
    }

    bool fillRandom(size_t approxSize)
    {
        T* m = 0;
        if (asn_random_fill(&getDescriptor(), reinterpret_cast<void**>(&m), approxSize) == 0) {
            this->reset(m, &Asn1Ptr::deleter);
            return true;
        }
        return false;
    }

    bool check() const
    {
        std::string err;
        return check(err);
    }

    template <int BufSize = 256>
    bool check(std::string& err) const
    {
        char errbuf[BufSize];
        size_t errlen = static_cast<size_t>(BufSize);
        const int ret =
            asn_check_constraints(&getDescriptor(), this->get(), errbuf, &errlen);
        if (ret) {
            err.assign(errbuf, errlen);
            return false;
        }
        return true;
    }

    bool printTo(char* ptr, size_t& len) const
    {
        if (ptr == 0 || len == 0 || !this->operator bool()) {
            return false;
        }
        char* memPtr = 0;
        size_t memSize = 0;
        FILE* tf = open_memstream(&memPtr, &memSize);
        if (!tf) {
            return false;
        }
        if (asn_fprint(tf, &getDescriptor(), this->get()) < 0) {
            fclose(tf);
            free(memPtr);
            return false;
        }
        fflush(tf);
        const size_t n = (memSize < len) ? memSize : len;
        if (n > 0 && memPtr) {
            memcpy(ptr, memPtr, n);
        }
        len = n;
        fclose(tf);
        free(memPtr);
        return true;
    }

    template <size_t N = 4096>
    std::string print() const
    {
        std::string out(N, '\0');
        size_t buflen = N;
        if (!printTo(&out[0], buflen)) {
            return std::string();
        }
        out.resize(buflen);
        return out;
    }

    const std::string& getSerialized() const { return m_serialized; }

private:
    static void deleter(void* ptr)
    {
        ASN_STRUCT_FREE(getTypeDescriptor<T>(), ptr);
    }

    static void noDeleter(void*) {}

    static int appendToString(const void* buffer, size_t size, void* key)
    {
        std::string* str = static_cast<std::string*>(key);
        if (!str) {
            return -1;
        }
        str->append(static_cast<const char*>(buffer), size);
        return 0;
    }

    std::string m_serialized;
};

/** calloc 片段；要求指针当前为 null，非空时不覆盖（避免 Release 下静默泄漏旧子树） */
template <typename T>
T fragment(T& t, int nr = 1)
{
    static_assert(std::is_pointer<T>::value, "T must be a pointer type");
    assert(t == 0);
    if (t != 0) {
        return t;
    }
    return (t = static_cast<T>(calloc(static_cast<size_t>(nr), sizeof(*t))));
}

/** 若指针为空则 calloc */
template <typename T>
T fragmentEnsure(T& t, int nr = 1)
{
    static_assert(std::is_pointer<T>::value, "T must be a pointer type");
    return (t = ((t == 0) ? static_cast<T>(calloc(static_cast<size_t>(nr), sizeof(*t))) : t));
}

/** 释放 OPTIONAL 指针字段指向的整棵子结构并置空 */
template <typename T>
void freeField(const asn_TYPE_descriptor_t& td, T*& p)
{
    ASN_STRUCT_FREE(td, p);
    p = 0;
}

template <typename T>
void freeField(T*& p)
{
    freeField(getTypeDescriptor<T>(), p);
}

/** 释放子结构内容、保留结构体本身并清零，可直接复用（如整表清空后重新装填） */
template <typename T>
void resetField(const asn_TYPE_descriptor_t& td, T& s)
{
    ASN_STRUCT_RESET(td, &s);
}

template <typename T>
void resetField(T& s)
{
    resetField(getTypeDescriptor<T>(), s);
}

class BitString {
public:
    explicit BitString(BIT_STRING_t& bs) : m_bitString(bs)
    {
        assert(m_bitString.buf != 0);
        assert(m_bitString.size > 0);
    }

    BitString(BIT_STRING_t& bs, size_t bitCount) : m_bitString(bs)
    {
        assert(bitCount > 0);
        free(m_bitString.buf);
        m_bitString.size = (bitCount + 7) / 8;
        m_bitString.buf =
            static_cast<uint8_t*>(calloc(m_bitString.size, sizeof(uint8_t)));
        m_bitString.bits_unused =
            static_cast<int>(8 * m_bitString.size - bitCount);
    }

    void set(size_t pos)
    {
        assert(m_bitString.buf != 0);
        assert(pos < 8 * m_bitString.size - static_cast<size_t>(m_bitString.bits_unused));
        m_bitString.buf[pos / 8] =
            static_cast<uint8_t>(m_bitString.buf[pos / 8] | (1u << (7 - (pos % 8))));
    }

    bool test(size_t pos) const
    {
        if (m_bitString.buf == 0
            || pos >= 8 * m_bitString.size - static_cast<size_t>(m_bitString.bits_unused)) {
            return false;
        }
        return (m_bitString.buf[pos / 8] & (1u << (7 - (pos % 8)))) != 0;
    }

    void clear(size_t pos)
    {
        assert(m_bitString.buf != 0);
        assert(pos < 8 * m_bitString.size - static_cast<size_t>(m_bitString.bits_unused));
        m_bitString.buf[pos / 8] =
            static_cast<uint8_t>(m_bitString.buf[pos / 8] & ~(1u << (7 - (pos % 8))));
    }

    void clearAll() { memset(m_bitString.buf, 0, m_bitString.size); }

private:
    BIT_STRING_t& m_bitString;
};

class Asn1String {
public:
    explicit Asn1String(OCTET_STRING_t& s) : m_string(s) {}

    std::string get() const
    {
        if (m_string.buf && m_string.size) {
            return std::string(reinterpret_cast<const char*>(m_string.buf), m_string.size);
        }
        return std::string();
    }

    void set(const std::string& s)
    {
        free(m_string.buf);
        m_string.buf = static_cast<uint8_t*>(malloc(s.size() + 1));
        m_string.size = s.size();
        if (m_string.buf) {
            memset(m_string.buf, 0, s.size() + 1);
            memcpy(m_string.buf, s.data(), m_string.size);
        }
    }

private:
    OCTET_STRING_t& m_string;
};

class Asn1Integer {
public:
    explicit Asn1Integer(OCTET_STRING_t& i) : m_integer(i) {}

    template <typename I>
    void set(I h)
    {
        if (m_integer.size != sizeof(I)) {
            free(m_integer.buf);
            m_integer.buf = 0;
            fragment(m_integer.buf, static_cast<int>(sizeof(I)));
            m_integer.size = sizeof(I);
        }
        hostToNet(&h, m_integer.buf, sizeof(I));
    }

    uint64_t get() const
    {
        if (!m_integer.buf || !m_integer.size) {
            return 0;
        }
        switch (m_integer.size) {
        case 1: {
            uint8_t h8 = 0;
            netToHost(m_integer.buf, &h8, m_integer.size);
            return h8;
        }
        case 2: {
            uint16_t h16 = 0;
            netToHost(m_integer.buf, &h16, m_integer.size);
            return h16;
        }
        case 4: {
            uint32_t h32 = 0;
            netToHost(m_integer.buf, &h32, m_integer.size);
            return h32;
        }
        case 8: {
            uint64_t h64 = 0;
            netToHost(m_integer.buf, &h64, m_integer.size);
            return h64;
        }
        default:
            return 0;
        }
    }

private:
    static bool isLittleEndian()
    {
        union {
            int i;
            char c;
        } u;
        u.i = 1;
        return u.c == 1;
    }

    static void reverseBytes(const void* source, void* result, size_t length)
    {
        const char* src = static_cast<const char*>(source);
        char* dstEnd = static_cast<char*>(result) + length;
        for (size_t i = 0; i < length; ++i) {
            *(--dstEnd) = src[i];
        }
    }

    static void hostToNet(const void* source, void* result, size_t length)
    {
        if (isLittleEndian()) {
            reverseBytes(source, result, length);
        } else {
            memcpy(result, source, length);
        }
    }

    static void netToHost(const void* source, void* result, size_t length)
    {
        hostToNet(source, result, length);
    }

    OCTET_STRING_t& m_integer;
};

/**
 * SEQUENCE OF 列表操作。push/at/back/size 无额外要求；
 * clear/remove/freeElement 需要释放元素子树，须对列表类型做 AFL_DECLARE_ASN1_TYPE（缺失则编译报错）。
 */
template <typename T>
class Asn1List {
    typedef typename std::remove_pointer<decltype(std::declval<T>().list.array)>::type ETP;
    typedef typename std::remove_pointer<ETP>::type ET;

public:
    explicit Asn1List(T& t) : m_list(t) {}

    /** 追加元素（t 为空则 calloc 一个）；返回挂上的元素，失败返回 0 且 t 仍归调用方 */
    ET* push(ET* t = 0)
    {
        fragmentEnsure(t);
        if (t == 0 || asn_sequence_add(&(m_list.list), t) != 0) {
            return 0;
        }
        return t;
    }

    /** 删除并整棵释放第 idx 个元素；末尾元素会被移到 idx（不保序） */
    void remove(size_t idx) { freeElement(pick(idx)); }

    /** 整棵释放一个已脱离列表的元素（配合 pick 使用） */
    void freeElement(ET* e)
    {
        if (e != 0) {
            ASN_STRUCT_FREE(*getTypeDescriptor<T>().elements->type, e);
        }
    }

    /** 取出第 idx 个元素，所有权交给调用方（须 freeElement 或挂到别处）；不保序 */
    ET* pick(size_t idx)
    {
        ET* retv = at(idx);
        asn_sequence_del(&(m_list.list), static_cast<int>(idx), 0);
        return retv;
    }

    ET* at(size_t idx)
    {
        return (idx < static_cast<size_t>(m_list.list.count)) ? m_list.list.array[idx] : 0;
    }

    ET* back()
    {
        if (m_list.list.count <= 0) {
            return 0;
        }
        return at(static_cast<size_t>(m_list.list.count - 1));
    }

    size_t size() const { return static_cast<size_t>(m_list.list.count); }

    /** 整棵释放所有元素，列表清零后可继续 push */
    void clear() { resetField(getTypeDescriptor<T>(), m_list); }

private:
    T& m_list;
};

} // namespace asn1
} // namespace afl

/**
 * 声明类型描述符特化，并生成 XxxPtr（在 afl::asn1 内）。
 * 用法：AFL_DECLARE_ASN1_TYPE(MessageFrame) → afl::asn1::MessageFramePtr
 */
#define AFL_DECLARE_ASN1_TYPE(type)                                                        \
    namespace afl {                                                                         \
    namespace asn1 {                                                                       \
    template <>                                                                            \
    struct Asn1TypeTraits<type##_t> {                                                      \
        static const asn_TYPE_descriptor_t& descriptor() { return asn_DEF_##type; }        \
    };                                                                                     \
    typedef Asn1Ptr<type##_t> type##Ptr;                                                   \
    }                                                                                      \
    }
