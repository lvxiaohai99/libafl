/**
 * @file   ByteBuffer.cpp
 * @brief  网络字节缓冲区的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/net/ByteBuffer.h"

namespace afl
{
namespace net
{
const char ByteBuffer::kCRLF[] = "\r\n";
const char ByteBuffer::kDoubleCRLF[] = "\r\n\r\n";

//const size_t ByteBuffer::kCheapPrepend;
//const size_t ByteBuffer::kInitialSize;

} // namespace net
} // namespace afl
