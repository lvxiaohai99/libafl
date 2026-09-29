/**
 * @file   ByteBuffer.h
 * @brief  网络字节缓冲区
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include "afl/net/NetUtil.h"

namespace afl
{
namespace net
{
/**
 * @brief 网络缓冲区（布局参考 Netty ChannelBuffer）
 * @see http://blog.csdn.net/solstice/article/details/6329080
 * @code
 * +-------------------+------------------+------------------+
 * | 可前置区          |  可读区          |  可写区          |
 * |                   |   （有效载荷）   |                  |
 * +-------------------+------------------+------------------+
 * 0      <=      readerIndex   <=   writerIndex    <=     size
 * @endcode
 */
class ByteBuffer
{
public:
    //  static const size_t kCheapPrepend = 8;
    //  static const size_t kInitialSize = 1024;

public:
    ByteBuffer(size_t cheapPrepend = 8, size_t initialSize = 1024)
        : kCheapPrepend(cheapPrepend), kInitialSize(initialSize), m_readerIndex(kCheapPrepend),
          m_writerIndex(kCheapPrepend), m_buffer(kCheapPrepend + kInitialSize)
    {
        assert(readableBytes() == 0);
        assert(writableBytes() == kInitialSize);
        assert(prependableBytes() == kCheapPrepend);
    }

    size_t readableBytes() const { return m_writerIndex - m_readerIndex; }

    size_t writableBytes() const { return m_buffer.size() - m_writerIndex; }

    size_t prependableBytes() const { return m_readerIndex; }

    std::string toString() const { return std::string(peek(), static_cast<int>(readableBytes())); }

public: // 读写接口
    void write(const std::string& str) { write(str.data(), str.size()); }

    void write(const char* data) { write(data, strlen(data)); }

    void write(const char* data, size_t len)
    {
        ensureWritableBytes(len);
        std::copy(data, data + len, beginWrite());
        hasWritten(len);
    }

    void write(const void* data, size_t len) { write(static_cast<const char*>(data), len); }

    /** @brief 以网络字节序写入 Number */
    /** @tparam Number bool/int8_t/int16_t/int32_t/int64_t/float/double 等 */
    template <typename Number>
    void write(Number num)
    {
        Number nnum = NetUtil::host2Net(num);
        write(&nnum, sizeof(nnum));
    }

    /** @brief 读取并消费一个主机字节序 Number */
    /** @pre 可读字节数 >= sizeof(Number) */
    template <typename Number>
    Number read()
    {
        Number nnum = peek<Number>();
        retrieve(sizeof(nnum));
        return nnum;
    }

    /** @brief 窥视主机字节序 Number，不移动读指针 */
    /** @pre 可读字节数 >= sizeof(Number) */
    template <typename Number>
    Number peek() const
    {
        assert(readableBytes() >= sizeof(Number));
        Number nnum = 0;
        ::memcpy(&nnum, peek(), sizeof(nnum));
        return NetUtil::net2Host(nnum);
    }

    /** @brief 在可前置区以网络字节序写入 Number */
    template <typename Number>
    void prepend(Number num)
    {
        Number nnum = NetUtil::host2Net(num);
        prepend(&nnum, sizeof(nnum));
    }

    void prepend(const void* data, size_t len)
    {
        assert(len <= prependableBytes());
        m_readerIndex -= len;
        const char* d = static_cast<const char*>(data);
        std::copy(d, d + len, begin() + m_readerIndex);
    }

    void retrieve(size_t len)
    {
        assert(len <= readableBytes());
        if (len < readableBytes())
        {
            m_readerIndex += len;
        }
        else
        {
            retrieveAll();
        }
    }

    template <typename Number>
    void retrieve()
    {
        retrieve(sizeof(Number));
    }

    void retrieveUntil(const char* end)
    {
        assert(peek() <= end);
        assert(end <= beginWrite());
        retrieve(end - peek());
    }

    void retrieveAll()
    {
        m_readerIndex = kCheapPrepend;
        m_writerIndex = kCheapPrepend;
    }

    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
        ;
    }

    std::string retrieveAsString(size_t len)
    {
        assert(len <= readableBytes());
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

public: // search
    const char* peek() const { return begin() + m_readerIndex; }

    const char* findCRLF() const
    {
        const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF + 2);
        return crlf == beginWrite() ? NULL : crlf;
    }

    const char* findCRLF(const char* start) const
    {
        assert(peek() <= start);
        assert(start <= beginWrite());
        const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF + 2);
        return crlf == beginWrite() ? NULL : crlf;
    }

    const char* findDoubleCRLF() const
    {
        const char* crlf = std::search(peek(), beginWrite(), kDoubleCRLF, kDoubleCRLF + 4);
        return crlf == beginWrite() ? NULL : crlf;
    }

    const char* findEOL() const
    {
        const void* eol = memchr(peek(), '\n', readableBytes());
        return static_cast<const char*>(eol);
    }

    const char* findEOL(const char* start) const
    {
        assert(peek() <= start);
        assert(start <= beginWrite());
        const void* eol = memchr(start, '\n', beginWrite() - start);
        return static_cast<const char*>(eol);
    }

    const char* findFeature(const void* f, size_t l) const
    {
        const char* feature = std::search(peek(), beginWrite(), (char*)f, ((char*)f) + l);
        return feature == beginWrite() ? NULL : feature;
    }

public:
    void ensureWritableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len);
        }
        assert(writableBytes() >= len);
    }

    char* beginWrite() { return begin() + m_writerIndex; }

    const char* beginWrite() const { return begin() + m_writerIndex; }

    void hasWritten(size_t len)
    {
        assert(len <= writableBytes());
        m_writerIndex += len;
    }

    void unwrite(size_t len)
    {
        assert(len <= readableBytes());
        m_writerIndex -= len;
    }

    void shrink(size_t reserve)
    {
        ByteBuffer other;
        other.ensureWritableBytes(readableBytes() + reserve);
        other.write(toString());
        swap(other);
    }

    size_t capacity() const { return m_buffer.capacity(); }

    void swap(ByteBuffer& rhs)
    {
        m_buffer.swap(rhs.m_buffer);
        std::swap(m_readerIndex, rhs.m_readerIndex);
        std::swap(m_writerIndex, rhs.m_writerIndex);
    }

private:
    char* begin() { return &*m_buffer.begin(); }

    const char* begin() const { return &*m_buffer.begin(); }

    void makeSpace(size_t len)
    {
        if (writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            // FIXME: 应优先搬移可读区而非直接扩容
            m_buffer.resize(m_writerIndex + len);
        }
        else
        {
            // 将可读数据前移，在缓冲区内腾挪空间
            assert(kCheapPrepend < m_readerIndex);
            size_t readable = readableBytes();
            std::copy(begin() + m_readerIndex, begin() + m_writerIndex, begin() + kCheapPrepend);
            m_readerIndex = kCheapPrepend;
            m_writerIndex = m_readerIndex + readable;
            assert(readable == readableBytes());
        }
    }

private:
    const size_t kCheapPrepend;
    const size_t kInitialSize;

    size_t m_readerIndex;
    size_t m_writerIndex;
    std::vector<char> m_buffer; // save buffer of network endian

    static const char kCRLF[];
    static const char kDoubleCRLF[];
};

} // namespace net
} // namespace afl
