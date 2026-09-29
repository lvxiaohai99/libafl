/**
 * @file   SerializableData.h
 * @brief  可序列化数据接口
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "nlohmann/json.hpp"
#include "afl/base/Common.h"

namespace afl
{
namespace base
{
using json = nlohmann::json;

class SerializableData
{
public:
    SerializableData() {}
    virtual ~SerializableData() {}

    virtual void serialize(json& j) = 0;
    virtual void deserialize(const json& j) = 0;
};

extern void to_json(json& j, const SerializableData& csd);
extern void from_json(const json& j, SerializableData& sd);
extern void to_json(json& j, const SerializableData* csd);

} // namespace base
} // namespace afl
