#pragma once

/**
 * @file Asn1Json.h
 * @brief asn1c 生成结构 ⇄ JSON（按 ITU-T X.697 JER 的基本映射，双向）
 *
 * asn1c 0.9.29 没有 JER，这里遍历 asn_TYPE_descriptor_t 实现，与具体消息集无关：
 *   SEQUENCE / SET      → 对象，缺省的 OPTIONAL 不输出（输入时缺省或 null 视为不存在）
 *   CHOICE              → {"分支名": 值}
 *   开放类型            → 直接输出实际类型的值；输入时由前面的字段（如 messageId）选择类型
 *   SEQUENCE OF / SET OF→ 数组
 *   NativeInteger       → 数字；NativeEnumerated → 枚举名（输入也接受数字）
 *   BOOLEAN / NULL      → true|false / null
 *   IA5String 等字符串  → 字符串
 *   OCTET STRING        → 大写 hex 字符串
 *   BIT STRING          → 定长（SIZE(n)）为 hex 字符串；变长为 {"value": hex, "length": 位数}
 *   其它类型：输出退回 asn1c 打印文本；输入报 "unsupported type"
 *
 * 限制：依赖 asn1c 0.9.29 的描述符布局；-fwide-types 生成的 INTEGER_t 大整数不支持输入。
 * 依赖：先包含 asn1c 生成的头，再包含本文件（通常经 Asn1Cpp.h 间接包含）；
 *       输入解析用 thirdparty/nlohmann/json.hpp。
 */

#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#ifndef ASN_APPLICATION_H
#error "Include asn1c generated headers (e.g. MessageFrame.h) before afl/asn1/Asn1Json.h"
#endif

#include "BIT_STRING.h"
#include "BOOLEAN.h"
#include "INTEGER.h"
#include "OCTET_STRING.h"
#include "constr_CHOICE.h"
#include "constr_SEQUENCE.h"
#include "constr_SET.h"
#include "constr_SET_OF.h"
#include "per_support.h"

#include "afl/string/Hex2String.h"
#include "nlohmann/json.hpp"

/* 弱引用：工程未链接某运行库类型时地址为 0，只是不会匹配到该分支 */
extern "C" {
extern asn_TYPE_operation_t asn_OP_SEQUENCE __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_SET __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_CHOICE __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_OPEN_TYPE __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_SEQUENCE_OF __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_SET_OF __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_NativeInteger __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_NativeEnumerated __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_BOOLEAN __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_NULL __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_OCTET_STRING __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_BIT_STRING __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_IA5String __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_UTF8String __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_VisibleString __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_PrintableString __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_NumericString __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_ISO646String __attribute__((weak));
}

namespace afl {
namespace asn1 {
namespace json_detail {

/* ------------------------------------------------------------------ 公共 */

inline bool opIs(const asn_TYPE_operation_t* op, const asn_TYPE_operation_t* ref)
{
    return ref != 0 && op == ref;
}

inline bool isConstructed(const asn_TYPE_operation_t* op)
{
    return opIs(op, &asn_OP_SEQUENCE) || opIs(op, &asn_OP_SET);
}

inline bool isChoiceLike(const asn_TYPE_operation_t* op)
{
    return opIs(op, &asn_OP_CHOICE) || opIs(op, &asn_OP_OPEN_TYPE);
}

inline bool isList(const asn_TYPE_operation_t* op)
{
    return opIs(op, &asn_OP_SEQUENCE_OF) || opIs(op, &asn_OP_SET_OF);
}

inline bool isCharString(const asn_TYPE_operation_t* op)
{
    return opIs(op, &asn_OP_IA5String) || opIs(op, &asn_OP_UTF8String)
           || opIs(op, &asn_OP_VisibleString) || opIs(op, &asn_OP_PrintableString)
           || opIs(op, &asn_OP_NumericString) || opIs(op, &asn_OP_ISO646String);
}

inline const asn_per_constraints_t* effectiveConstraints(const asn_TYPE_descriptor_t* td,
                                                         const asn_per_constraints_t* memberPc)
{
    return memberPc ? memberPc : td->encoding_constraints.per_constraints;
}

/** BIT STRING 是否为不可扩展的定长 SIZE(n)；是则 bits = n */
inline bool fixedSizeBits(const asn_per_constraints_t* pc, long& bits)
{
    if (pc && (pc->size.flags & asn_per_constraint_t::APC_CONSTRAINED)
        && !(pc->size.flags & asn_per_constraint_t::APC_EXTENSIBLE)
        && pc->size.lower_bound == pc->size.upper_bound) {
        bits = pc->size.lower_bound;
        return true;
    }
    return false;
}

inline const void* memberPtr(const void* base, const asn_TYPE_member_t* elm)
{
    const char* p = static_cast<const char*>(base) + elm->memb_offset;
    if (elm->flags & ATF_POINTER) {
        return *reinterpret_cast<const void* const*>(p);
    }
    return p;
}

inline unsigned choicePresent(const void* base, const asn_CHOICE_specifics_t* sp)
{
    const char* p = static_cast<const char*>(base) + sp->pres_offset;
    switch (sp->pres_size) {
    case 1: {
        uint8_t v;
        memcpy(&v, p, sizeof(v));
        return v;
    }
    case 2: {
        uint16_t v;
        memcpy(&v, p, sizeof(v));
        return v;
    }
    case 4: {
        uint32_t v;
        memcpy(&v, p, sizeof(v));
        return v;
    }
    default:
        return 0;
    }
}

inline bool setChoicePresent(void* base, const asn_CHOICE_specifics_t* sp, unsigned present)
{
    char* p = static_cast<char*>(base) + sp->pres_offset;
    switch (sp->pres_size) {
    case 1: {
        const uint8_t v = static_cast<uint8_t>(present);
        memcpy(p, &v, sizeof(v));
        return true;
    }
    case 2: {
        const uint16_t v = static_cast<uint16_t>(present);
        memcpy(p, &v, sizeof(v));
        return true;
    }
    case 4: {
        const uint32_t v = present;
        memcpy(p, &v, sizeof(v));
        return true;
    }
    default:
        return false;
    }
}

/* ------------------------------------------------------------------ 输出 */

inline void newline(std::string& out, int indent, int level)
{
    if (indent > 0) {
        out += '\n';
        out.append(static_cast<size_t>(indent * level), ' ');
    }
}

inline void appendEscaped(std::string& out, const char* s, size_t n)
{
    static const char kHex[] = "0123456789abcdef";
    out += '"';
    for (size_t i = 0; i < n; ++i) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        switch (c) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (c < 0x20) {
                out += "\\u00";
                out += kHex[c >> 4];
                out += kHex[c & 0x0F];
            } else {
                out += static_cast<char>(c);
            }
        }
    }
    out += '"';
}

inline int appendToString(const void* buf, size_t size, void* key)
{
    static_cast<std::string*>(key)->append(static_cast<const char*>(buf), size);
    return 0;
}

/** 用 asn1c 自带 print_struct 输出；能完整解析成有限数字则原样输出，否则作为字符串 */
inline void appendFallback(std::string& out, const asn_TYPE_descriptor_t* td, const void* ptr)
{
    std::string text;
    if (td->op->print_struct(td, ptr, 0, &appendToString, &text) < 0) {
        out += "null";
        return;
    }
    if (!text.empty()) {
        char* end = 0;
        const double v = strtod(text.c_str(), &end);
        if (end && *end == '\0' && v == v && v - v == 0) {
            out += text;
            return;
        }
    }
    appendEscaped(out, text.data(), text.size());
}

inline void appendHex(std::string& out, const uint8_t* buf, size_t n)
{
    static const char kHex[] = "0123456789ABCDEF";
    out += '"';
    for (size_t i = 0; buf && i < n; ++i) {
        out += kHex[buf[i] >> 4];
        out += kHex[buf[i] & 0x0F];
    }
    out += '"';
}

inline void appendBitString(std::string& out, const BIT_STRING_t* bs,
                            const asn_per_constraints_t* pc, int indent)
{
    long fixed = 0;
    if (fixedSizeBits(pc, fixed)) {
        appendHex(out, bs->buf, bs->size);
        return;
    }
    const long bits = static_cast<long>(bs->size) * 8 - bs->bits_unused;
    char len[32];
    snprintf(len, sizeof(len), "%ld", bits);
    const char* sep = indent > 0 ? ", " : ",";
    const char* colon = indent > 0 ? ": " : ":";
    out += "{\"value\"";
    out += colon;
    appendHex(out, bs->buf, bs->size);
    out += sep;
    out += "\"length\"";
    out += colon;
    out += len;
    out += '}';
}

inline void appendValue(std::string& out, const asn_TYPE_descriptor_t* td,
                        const asn_per_constraints_t* memberPc, const void* ptr, int indent,
                        int level);

inline void appendKey(std::string& out, const char* name, int indent)
{
    appendEscaped(out, name, strlen(name));
    out += indent > 0 ? ": " : ":";
}

inline void appendSequence(std::string& out, const asn_TYPE_descriptor_t* td, const void* ptr,
                           int indent, int level)
{
    out += '{';
    bool first = true;
    for (unsigned i = 0; i < td->elements_count; ++i) {
        const asn_TYPE_member_t* elm = &td->elements[i];
        const void* m = memberPtr(ptr, elm);
        if (m == 0) {
            continue;
        }
        if (!first) {
            out += ',';
        }
        first = false;
        newline(out, indent, level + 1);
        appendKey(out, elm->name, indent);
        appendValue(out, elm->type, elm->encoding_constraints.per_constraints, m, indent,
                    level + 1);
    }
    if (!first) {
        newline(out, indent, level);
    }
    out += '}';
}

inline void appendChoice(std::string& out, const asn_TYPE_descriptor_t* td, const void* ptr,
                         int indent, int level)
{
    const asn_CHOICE_specifics_t* sp = static_cast<const asn_CHOICE_specifics_t*>(td->specifics);
    const unsigned present = sp ? choicePresent(ptr, sp) : 0;
    if (present == 0 || present > td->elements_count) {
        out += "null";
        return;
    }
    const asn_TYPE_member_t* elm = &td->elements[present - 1];
    if (opIs(td->op, &asn_OP_OPEN_TYPE)) {
        appendValue(out, elm->type, elm->encoding_constraints.per_constraints,
                    memberPtr(ptr, elm), indent, level);
        return;
    }
    out += '{';
    newline(out, indent, level + 1);
    appendKey(out, elm->name, indent);
    appendValue(out, elm->type, elm->encoding_constraints.per_constraints, memberPtr(ptr, elm),
                indent, level + 1);
    newline(out, indent, level);
    out += '}';
}

inline void appendList(std::string& out, const asn_TYPE_descriptor_t* td, const void* ptr,
                       int indent, int level)
{
    const asn_anonymous_set_* list = _A_CSET_FROM_VOID(ptr);
    const asn_TYPE_member_t* elm = td->elements;
    out += '[';
    for (int i = 0; i < list->count; ++i) {
        if (i > 0) {
            out += ',';
        }
        newline(out, indent, level + 1);
        appendValue(out, elm->type, elm->encoding_constraints.per_constraints, list->array[i],
                    indent, level + 1);
    }
    if (list->count > 0) {
        newline(out, indent, level);
    }
    out += ']';
}

inline void appendValue(std::string& out, const asn_TYPE_descriptor_t* td,
                        const asn_per_constraints_t* memberPc, const void* ptr, int indent,
                        int level)
{
    if (ptr == 0) {
        out += "null";
        return;
    }
    const asn_TYPE_operation_t* op = td->op;

    if (isConstructed(op)) {
        appendSequence(out, td, ptr, indent, level);
    } else if (isChoiceLike(op)) {
        appendChoice(out, td, ptr, indent, level);
    } else if (isList(op)) {
        appendList(out, td, ptr, indent, level);
    } else if (opIs(op, &asn_OP_NativeInteger)) {
        const asn_INTEGER_specifics_t* sp =
            static_cast<const asn_INTEGER_specifics_t*>(td->specifics);
        const long v = *static_cast<const long*>(ptr);
        char buf[32];
        if (sp && sp->field_unsigned) {
            snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(v));
        } else {
            snprintf(buf, sizeof(buf), "%ld", v);
        }
        out += buf;
    } else if (opIs(op, &asn_OP_NativeEnumerated)) {
        const asn_INTEGER_specifics_t* sp =
            static_cast<const asn_INTEGER_specifics_t*>(td->specifics);
        const long v = *static_cast<const long*>(ptr);
        for (int i = 0; sp && i < sp->map_count; ++i) {
            if (sp->value2enum[i].nat_value == v) {
                appendEscaped(out, sp->value2enum[i].enum_name, sp->value2enum[i].enum_len);
                return;
            }
        }
        char buf[32];
        snprintf(buf, sizeof(buf), "%ld", v);
        out += buf;
    } else if (opIs(op, &asn_OP_BOOLEAN)) {
        out += *static_cast<const BOOLEAN_t*>(ptr) ? "true" : "false";
    } else if (opIs(op, &asn_OP_NULL)) {
        out += "null";
    } else if (opIs(op, &asn_OP_BIT_STRING)) {
        appendBitString(out, static_cast<const BIT_STRING_t*>(ptr),
                        effectiveConstraints(td, memberPc), indent);
    } else if (opIs(op, &asn_OP_OCTET_STRING)) {
        const OCTET_STRING_t* os = static_cast<const OCTET_STRING_t*>(ptr);
        appendHex(out, os->buf, os->size);
    } else if (isCharString(op)) {
        const OCTET_STRING_t* os = static_cast<const OCTET_STRING_t*>(ptr);
        appendEscaped(out, reinterpret_cast<const char*>(os->buf), os->size);
    } else {
        appendFallback(out, td, ptr);
    }
}

/* ------------------------------------------------------------------ 输入 */

typedef nlohmann::json Json;

/** 类型在内存中的结构体大小；不支持的类型返回 0 */
inline size_t structSize(const asn_TYPE_descriptor_t* td)
{
    const asn_TYPE_operation_t* op = td->op;
    if (!td->specifics
        && (isConstructed(op) || isChoiceLike(op) || isList(op))) {
        return 0;
    }
    if (opIs(op, &asn_OP_SEQUENCE)) {
        return static_cast<const asn_SEQUENCE_specifics_t*>(td->specifics)->struct_size;
    }
    if (opIs(op, &asn_OP_SET)) {
        return static_cast<const asn_SET_specifics_t*>(td->specifics)->struct_size;
    }
    if (isChoiceLike(op)) {
        return static_cast<const asn_CHOICE_specifics_t*>(td->specifics)->struct_size;
    }
    if (isList(op)) {
        return static_cast<const asn_SET_OF_specifics_t*>(td->specifics)->struct_size;
    }
    if (opIs(op, &asn_OP_BIT_STRING)) {
        return td->specifics
                   ? static_cast<const asn_OCTET_STRING_specifics_t*>(td->specifics)->struct_size
                   : sizeof(BIT_STRING_t);
    }
    if (opIs(op, &asn_OP_OCTET_STRING) || isCharString(op)) {
        return td->specifics
                   ? static_cast<const asn_OCTET_STRING_specifics_t*>(td->specifics)->struct_size
                   : sizeof(OCTET_STRING_t);
    }
    if (opIs(op, &asn_OP_NativeInteger) || opIs(op, &asn_OP_NativeEnumerated)) {
        return sizeof(long);
    }
    if (opIs(op, &asn_OP_BOOLEAN)) {
        return sizeof(BOOLEAN_t);
    }
    if (opIs(op, &asn_OP_NULL)) {
        return sizeof(int);
    }
    return 0;
}

class Reader {
public:
    std::string error;

    bool fill(const asn_TYPE_descriptor_t* td, const asn_per_constraints_t* memberPc, void* ptr,
              const Json& j, const std::string& path)
    {
        const asn_TYPE_operation_t* op = td->op;
        if (isConstructed(op)) {
            return fillSequence(td, ptr, j, path);
        }
        if (opIs(op, &asn_OP_CHOICE)) {
            return fillChoice(td, ptr, j, path);
        }
        if (isList(op)) {
            return fillList(td, ptr, j, path);
        }
        if (opIs(op, &asn_OP_NativeInteger)) {
            return fillInteger(td, ptr, j, path);
        }
        if (opIs(op, &asn_OP_NativeEnumerated)) {
            return fillEnumerated(td, ptr, j, path);
        }
        if (opIs(op, &asn_OP_BOOLEAN)) {
            if (!j.is_boolean()) {
                return fail(path, "expected true/false");
            }
            *static_cast<BOOLEAN_t*>(ptr) = j.get<bool>() ? 1 : 0;
            return true;
        }
        if (opIs(op, &asn_OP_NULL)) {
            return j.is_null() ? true : fail(path, "expected null");
        }
        if (opIs(op, &asn_OP_BIT_STRING)) {
            return fillBitString(static_cast<BIT_STRING_t*>(ptr),
                                 effectiveConstraints(td, memberPc), j, path);
        }
        if (opIs(op, &asn_OP_OCTET_STRING)) {
            std::string bytes;
            if (!j.is_string() || !afl::str::parseHexBytes(j.get<std::string>(), bytes)) {
                return fail(path, "expected hex string");
            }
            return setOctets(static_cast<OCTET_STRING_t*>(ptr), bytes, path);
        }
        if (isCharString(op)) {
            if (!j.is_string()) {
                return fail(path, "expected string");
            }
            return setOctets(static_cast<OCTET_STRING_t*>(ptr), j.get<std::string>(), path);
        }
        return fail(path, std::string("unsupported type ") + td->name);
    }

private:
    bool fail(const std::string& path, const std::string& msg)
    {
        if (error.empty()) {
            error = (path.empty() ? std::string("<root>") : path) + ": " + msg;
        }
        return false;
    }

    static std::string join(const std::string& path, const char* name)
    {
        return path.empty() ? std::string(name) : path + "." + name;
    }

    /** 取成员存储；指针成员按需 calloc 并先挂到父结构上，失败时由根节点统一释放 */
    void* memberStorage(void* base, const asn_TYPE_member_t* elm, const std::string& path)
    {
        char* p = static_cast<char*>(base) + elm->memb_offset;
        if (!(elm->flags & ATF_POINTER)) {
            return p;
        }
        void** slot = reinterpret_cast<void**>(p);
        if (*slot == 0) {
            const size_t n = structSize(elm->type);
            if (n == 0) {
                fail(path, std::string("unsupported type ") + elm->type->name);
                return 0;
            }
            *slot = calloc(1, n);
            if (*slot == 0) {
                fail(path, "out of memory");
                return 0;
            }
        }
        return *slot;
    }

    bool fillSequence(const asn_TYPE_descriptor_t* td, void* ptr, const Json& j,
                      const std::string& path)
    {
        if (!j.is_object()) {
            return fail(path, "expected object");
        }
        for (Json::const_iterator it = j.begin(); it != j.end(); ++it) {
            bool known = false;
            for (unsigned i = 0; i < td->elements_count && !known; ++i) {
                known = (it.key() == td->elements[i].name);
            }
            if (!known) {
                return fail(path, "unknown member \"" + it.key() + "\"");
            }
        }
        for (unsigned i = 0; i < td->elements_count; ++i) {
            const asn_TYPE_member_t* elm = &td->elements[i];
            const std::string childPath = join(path, elm->name);
            Json::const_iterator it = j.find(elm->name);
            const bool absent =
                it == j.end() || (it->is_null() && !opIs(elm->type->op, &asn_OP_NULL));
            if (absent) {
                if (elm->optional) {
                    continue;
                }
                return fail(childPath, "missing mandatory member");
            }
            if (elm->flags & ATF_OPEN_TYPE) {
                if (!fillOpenType(td, ptr, elm, *it, childPath)) {
                    return false;
                }
                continue;
            }
            void* mp = memberStorage(ptr, elm, childPath);
            if (!mp || !fill(elm->type, elm->encoding_constraints.per_constraints, mp, *it,
                             childPath)) {
                return false;
            }
        }
        return true;
    }

    bool fillChoice(const asn_TYPE_descriptor_t* td, void* ptr, const Json& j,
                    const std::string& path)
    {
        if (!j.is_object() || j.size() != 1) {
            return fail(path, "expected object with exactly one alternative");
        }
        const std::string key = j.begin().key();
        for (unsigned i = 0; i < td->elements_count; ++i) {
            const asn_TYPE_member_t* elm = &td->elements[i];
            if (key != elm->name) {
                continue;
            }
            const asn_CHOICE_specifics_t* sp =
                static_cast<const asn_CHOICE_specifics_t*>(td->specifics);
            if (!sp || !setChoicePresent(ptr, sp, i + 1)) {
                return fail(path, "bad CHOICE descriptor");
            }
            const std::string childPath = join(path, elm->name);
            void* mp = memberStorage(ptr, elm, childPath);
            return mp && fill(elm->type, elm->encoding_constraints.per_constraints, mp,
                              j.begin().value(), childPath);
        }
        return fail(path, "unknown alternative \"" + key + "\"");
    }

    /** 开放类型：由已填好的前序字段（type_selector）决定实际类型 */
    bool fillOpenType(const asn_TYPE_descriptor_t* parentTd, void* parentPtr,
                      const asn_TYPE_member_t* elm, const Json& j, const std::string& path)
    {
        if (!elm->type_selector) {
            return fail(path, "open type without selector");
        }
        const asn_type_selector_result_t sel = elm->type_selector(parentTd, parentPtr);
        const asn_TYPE_descriptor_t* otd = elm->type;
        if (!sel.type_descriptor || sel.presence_index == 0
            || sel.presence_index > otd->elements_count) {
            return fail(path, "cannot select open type from preceding members");
        }
        void* ot = memberStorage(parentPtr, elm, path);
        const asn_CHOICE_specifics_t* sp =
            static_cast<const asn_CHOICE_specifics_t*>(otd->specifics);
        if (!ot || !sp || !setChoicePresent(ot, sp, sel.presence_index)) {
            return fail(path, "bad open type descriptor");
        }
        const asn_TYPE_member_t* inner = &otd->elements[sel.presence_index - 1];
        void* ip = memberStorage(ot, inner, path);
        return ip && fill(inner->type, inner->encoding_constraints.per_constraints, ip, j, path);
    }

    bool fillList(const asn_TYPE_descriptor_t* td, void* ptr, const Json& j,
                  const std::string& path)
    {
        if (!j.is_array()) {
            return fail(path, "expected array");
        }
        const asn_TYPE_member_t* elm = td->elements;
        const size_t n = structSize(elm->type);
        if (n == 0) {
            return fail(path, std::string("unsupported element type ") + elm->type->name);
        }
        for (size_t i = 0; i < j.size(); ++i) {
            char idx[32];
            snprintf(idx, sizeof(idx), "[%zu]", i);
            const std::string childPath = path + idx;
            void* e = calloc(1, n);
            if (e == 0) {
                return fail(childPath, "out of memory");
            }
            if (asn_set_add(ptr, e) != 0) {
                free(e);
                return fail(childPath, "out of memory");
            }
            if (!fill(elm->type, elm->encoding_constraints.per_constraints, e, j[i], childPath)) {
                return false;
            }
        }
        return true;
    }

    bool fillInteger(const asn_TYPE_descriptor_t* td, void* ptr, const Json& j,
                     const std::string& path)
    {
        if (!j.is_number_integer()) {
            return fail(path, "expected integer");
        }
        const asn_INTEGER_specifics_t* sp =
            static_cast<const asn_INTEGER_specifics_t*>(td->specifics);
        const bool isUnsigned = sp && sp->field_unsigned;
        if (j.is_number_unsigned()) {
            const uint64_t v = j.get<uint64_t>();
            if (v > (isUnsigned ? static_cast<uint64_t>(ULONG_MAX) : static_cast<uint64_t>(LONG_MAX))) {
                return fail(path, "integer out of range for native long");
            }
            *static_cast<unsigned long*>(ptr) = static_cast<unsigned long>(v);
            return true;
        }
        const int64_t v = j.get<int64_t>();
        if (isUnsigned || v < static_cast<int64_t>(LONG_MIN)) {
            return fail(path, "integer out of range for native long");
        }
        *static_cast<long*>(ptr) = static_cast<long>(v);
        return true;
    }

    bool fillEnumerated(const asn_TYPE_descriptor_t* td, void* ptr, const Json& j,
                        const std::string& path)
    {
        if (j.is_number_integer()) {
            return fillInteger(td, ptr, j, path);
        }
        if (!j.is_string()) {
            return fail(path, "expected enumerator name");
        }
        const std::string name = j.get<std::string>();
        const asn_INTEGER_specifics_t* sp =
            static_cast<const asn_INTEGER_specifics_t*>(td->specifics);
        for (int i = 0; sp && i < sp->map_count; ++i) {
            if (name.size() == sp->value2enum[i].enum_len
                && name.compare(0, name.size(), sp->value2enum[i].enum_name) == 0) {
                *static_cast<long*>(ptr) = sp->value2enum[i].nat_value;
                return true;
            }
        }
        return fail(path, "unknown enumerator \"" + name + "\"");
    }

    /** OCTET_STRING_t / BIT_STRING_t 均有 buf+size；按 asn1c 习惯多分配 1 字节 NUL */
    template <typename S>
    bool setOctets(S* os, const std::string& bytes, const std::string& path)
    {
        uint8_t* buf = static_cast<uint8_t*>(calloc(bytes.size() + 1, 1));
        if (buf == 0) {
            return fail(path, "out of memory");
        }
        if (!bytes.empty()) {
            memcpy(buf, bytes.data(), bytes.size());
        }
        free(os->buf);
        os->buf = buf;
        os->size = bytes.size();
        return true;
    }

    bool fillBitString(BIT_STRING_t* bs, const asn_per_constraints_t* pc, const Json& j,
                       const std::string& path)
    {
        std::string bytes;
        long bits = 0;
        long fixed = 0;
        if (j.is_string()) {
            if (!afl::str::parseHexBytes(j.get<std::string>(), bytes)) {
                return fail(path, "expected hex string");
            }
            bits = fixedSizeBits(pc, fixed) ? fixed : static_cast<long>(bytes.size() * 8);
        } else if (j.is_object() && j.size() == 2 && j.count("value") && j.count("length")
                   && j["value"].is_string() && j["length"].is_number_integer()) {
            if (!afl::str::parseHexBytes(j["value"].get<std::string>(), bytes)) {
                return fail(path + ".value", "expected hex string");
            }
            bits = j["length"].get<long>();
        } else {
            return fail(path, "expected hex string or {\"value\": hex, \"length\": bits}");
        }
        const size_t need = static_cast<size_t>((bits + 7) / 8);
        if (bits < 0 || need > bytes.size()) {
            return fail(path, "length exceeds value bytes");
        }
        bytes.resize(need);
        if (!setOctets(bs, bytes, path)) {
            return false;
        }
        bs->bits_unused = static_cast<int>(need * 8 - static_cast<size_t>(bits));
        if (need > 0 && bs->bits_unused > 0) {
            bs->buf[need - 1] &= static_cast<uint8_t>(0xFF << bs->bits_unused);
        }
        return true;
    }
};

} // namespace json_detail

/**
 * 把 asn1c 结构转成 JSON 文本。
 * @param indent 每级缩进空格数；0 输出单行紧凑 JSON
 * @return ptr 为空时返回 "null"
 */
inline std::string toJson(const asn_TYPE_descriptor_t& td, const void* ptr, int indent = 2)
{
    std::string out;
    json_detail::appendValue(out, &td, 0, ptr, indent, 0);
    return out;
}

/**
 * 从 JSON 文本构造 asn1c 结构（calloc 分配，调用方用 ASN_STRUCT_FREE 释放）。
 * 只做结构/类型映射，取值范围等约束请随后用 asn_check_constraints 校验。
 * @param out  成功时输出新结构指针
 * @param err  失败原因，含字段路径，如 "rsiFrame.rtss[0].rtsId: expected integer"
 */
inline bool fromJson(const asn_TYPE_descriptor_t& td, const std::string& text, void** out,
                     std::string* err = 0)
{
    json_detail::Json j;
    try {
        j = json_detail::Json::parse(text);
    } catch (const std::exception& e) {
        if (err) {
            *err = std::string("invalid JSON: ") + e.what();
        }
        return false;
    }
    const size_t n = json_detail::structSize(&td);
    void* p = n ? calloc(1, n) : 0;
    if (p == 0) {
        if (err) {
            *err = n ? "out of memory" : std::string("unsupported top-level type ") + td.name;
        }
        return false;
    }
    json_detail::Reader reader;
    if (!reader.fill(&td, 0, p, j, "")) {
        ASN_STRUCT_FREE(td, p);
        if (err) {
            *err = reader.error;
        }
        return false;
    }
    *out = p;
    return true;
}

} // namespace asn1
} // namespace afl
