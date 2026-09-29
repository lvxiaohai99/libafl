/**
 * @file   ConfigFile.h
 * @brief  JSON 配置文件读写（基于 nlohmann::json）
 * @author libafl
 * @date   2026-09
 */
#pragma once

#include "afl/base/Common.h"
#include "afl/file/FileUtil.h"
#include "afl/string/StringUtil.h"
#include "nlohmann/json.hpp"

#include <fstream>
#include <memory>
#include <string>
#include <type_traits>

namespace afl
{
namespace config
{
/** 配置块类型（JSON 对象） */
using ConfigBlock = nlohmann::json;

/**
 * @brief 单个 JSON 配置文件的加载 / 保存 / 按路径取块
 */
class ConfigFile final
{
public:
    /**
     * @brief 打开或创建配置文件
     * @param fileName 文件路径；不存在时写入 "{}"
     */
    explicit ConfigFile(std::string fileName)
        : m_fileName(std::move(fileName)), m_blocks(new ConfigBlock())
    {
        ensureFileExists();
        loadFromDisk();
    }

    /**
     * @brief 按键路径取配置块（可写）
     * @tparam Dirs 键类型，需可转为 string
     * @param dirs  多级键，如 "section", "item"
     * @return 对应 JSON 引用
     */
    template <typename... Dirs>
    ConfigBlock& configBlock(Dirs... dirs)
    {
        return getConfigBlock(*m_blocks, dirs...);
    }

    /** @brief 配置文件绝对/相对路径 */
    const std::string& fileName() const { return m_fileName; }

    /** @brief 从磁盘重新加载（失败则重置为 {}） */
    void reload()
    {
        m_blocks->clear();
        ensureFileExists();
        loadFromDisk();
    }

    /** @brief 将当前内存内容写回磁盘（缩进 4） */
    void save()
    {
        std::ofstream out(m_fileName);
        out << m_blocks->dump(4);
    }

private:
    void ensureFileExists()
    {
        if (!afl::file::FileUtil::isFileExist(m_fileName.c_str()))
        {
            std::ofstream out(m_fileName);
            out << "{}";
        }
    }

    void loadFromDisk()
    {
        std::ifstream in(m_fileName);
        try
        {
            in >> (*m_blocks);
        }
        catch (const ConfigBlock::exception& e)
        {
            (void)e;
            std::ofstream out(m_fileName);
            out << "{}";
            m_blocks->clear();
            *m_blocks = ConfigBlock::object();
        }
    }

    template <typename Rest>
    ConfigBlock& getConfigBlock(ConfigBlock& cb, Rest r)
    {
        return cb[toKey(r)];
    }

    template <typename First, typename... Dirs>
    ConfigBlock& getConfigBlock(ConfigBlock& cb, First f, Dirs... dirs)
    {
        return getConfigBlock(cb[toKey(f)], dirs...);
    }

    static std::string toKey(const std::string& s) { return s; }

    static std::string toKey(const char* s) { return std::string(s); }

    std::string m_fileName;
    std::unique_ptr<ConfigBlock> m_blocks;
};

} // namespace config
} // namespace afl
