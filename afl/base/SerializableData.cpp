/**
 * @file   SerializableData.cpp
 * @brief  可序列化数据接口的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/base/SerializableData.h"
namespace afl
{
namespace base
{
void to_json(json& j, const SerializableData& csd)
{
    SerializableData& sd = const_cast<SerializableData&>(csd);
    sd.serialize(j);
}

void from_json(const json& j, SerializableData& sd)
{
    sd.deserialize(j);
}

void to_json(json& j, const SerializableData* csd)
{
    SerializableData* sd = const_cast<SerializableData*>(csd);
    sd->serialize(j);
}

} // namespace base
} // namespace afl
