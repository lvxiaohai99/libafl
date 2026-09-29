/**
 * @file   Crc.h
 * @brief  CRC 校验计算
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"

namespace afl
{
namespace crypto
{
namespace CRCDetail
{
uint8_t calc_crc_table_8(uint8_t* pStart, int uSize);
uint16_t calc_crc_table_16(uint8_t* pStart, int uSize);
uint32_t calc_crc_table_32(uint8_t* pStart, int uSize);
} // namespace CRCDetail

template <typename T>
class CRC_CALC
{
private:
    typedef typename std::enable_if<std::is_same<uint8_t, T>::value ||
                                        std::is_same<uint16_t, T>::value ||
                                        std::is_same<uint32_t, T>::value,
                                    T>::type R;

public:
    static T calc(void* data, int len)
    {
        if (std::is_same<uint8_t, T>::value)
        {
            return CRCDetail::calc_crc_table_8((uint8_t*)data, len);
        }
        else if (std::is_same<uint16_t, T>::value)
        {
            return CRCDetail::calc_crc_table_16((uint8_t*)data, len);
        }
        else if (std::is_same<uint32_t, T>::value)
        {
            return CRCDetail::calc_crc_table_32((uint8_t*)data, len);
        }
        return -1;
        T a = 1;
        return a;
    }
};

using CRC8 = CRC_CALC<uint8_t>;
using CRC16 = CRC_CALC<uint16_t>;
using CRC32 = CRC_CALC<uint32_t>;


} // namespace crypto
} // namespace afl
