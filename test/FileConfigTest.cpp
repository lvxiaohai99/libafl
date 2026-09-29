/**
 * @file   FileConfigTest.cpp
 * @brief  file/config 模块单元测试：FileUtil / ConfigManager / Configurable
 * @author libafl
 * @date   2026-09
 */
#include <gtest/gtest.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>

#include "afl/file/FileUtil.h"
#include "afl/config/ConfigManager.h"
#include "afl/config/ConfigData.h"
#include "afl/config/Configurable.h"

using namespace afl::file;
using namespace afl::config;

namespace
{
std::string testDir()
{
    return "/tmp/libafl_test_" + std::to_string(::getpid());
}

struct TestConf : public ConfigData<TestConf>
{
    int width = 100;
    int height = 200;

    virtual void writeToFile(ConfigBlock& cb) override
    {
        cb["width"] = width;
        cb["height"] = height;
    }

    virtual void readFromFile(const ConfigBlock& cb) override
    {
        width = cb.value("width", 0);
        height = cb.value("height", 0);
    }
};

} // namespace

TEST(FileUtilTest, FileReadWriteDelete)
{
    const std::string path = testDir() + "/plain.txt";
    ::mkdir(testDir().c_str(), 0755);

    {
        std::ofstream ofs(path);
        ofs << "libafl file test";
    }
    EXPECT_TRUE(FileUtil::isFileExist(path.c_str()));
    EXPECT_EQ(static_cast<long>(strlen("libafl file test")), FileUtil::getFileSize(path.c_str()));

    std::string content;
    EXPECT_EQ(strlen("libafl file test"), FileUtil::readFile(path.c_str(), content));
    EXPECT_EQ("libafl file test", content);

    EXPECT_EQ(0, ::remove(path.c_str()));
    EXPECT_FALSE(FileUtil::isFileExist(path.c_str()));
}

TEST(FileUtilTest, DirectoryOperations)
{
    const std::string dir = testDir() + "/a/b/c";
    EXPECT_TRUE(FileUtil::createRecursionDir(dir.c_str()));
    EXPECT_TRUE(FileUtil::isDirectory(dir.c_str()));

    EXPECT_EQ(dir, FileUtil::dirName((dir + "/x.txt").c_str()));
    EXPECT_EQ("x.txt", FileUtil::baseName((dir + "/x.txt").c_str()));
}

TEST(ConfigTest, SaveLoadRoundTrip)
{
    const std::string dir = testDir() + "/conf";
    const std::string file = dir + "/test_conf.json";
    ::remove(file.c_str());

    {
        ConfigManager mgr(dir + "/");
        TestConf conf;
        conf.width = 640;
        conf.height = 480;
        mgr.registerConfig("test_conf", conf, file);
        mgr.saveConfig();
    }

    EXPECT_TRUE(FileUtil::isFileExist(file.c_str())) << "config file should be created: " << file;

    {
        ConfigManager mgr(dir + "/");
        TestConf conf;
        conf.width = 1;
        conf.height = 1;
        mgr.registerConfig("test_conf", conf, file);

        EXPECT_EQ(640, conf.width);
        EXPECT_EQ(480, conf.height);

        conf.width = 800;
        conf.height = 600;
        mgr.saveConfig();
    }

    {
        ConfigManager mgr(dir + "/");
        TestConf conf;
        mgr.registerConfig("test_conf", conf, file);
        EXPECT_EQ(800, conf.width);
        EXPECT_EQ(600, conf.height);
    }
}

TEST(ConfigTest, ConfigurableSaveAndReload)
{
    const std::string dir = testDir() + "/conf2";
    const std::string fileName = "module.json";
    const std::string fullPath = dir + "/" + fileName;
    ::remove(fullPath.c_str());

    ConfigManager mgr(dir);
    {
        Configurable<TestConf> cfg(mgr, "module", fileName);
        cfg.getConfig().width = 1920;
        cfg.getConfig().height = 1080;
        cfg.saveConfig();
    }

    TestConf loaded;
    mgr.registerConfig("module", loaded, fileName);
    EXPECT_EQ(1920, loaded.width);
    EXPECT_EQ(1080, loaded.height);

    {
        std::ofstream out(fullPath);
        out << R"({
  "module": {
    "width": 1280,
    "height": 720
  }
})";
    }
    mgr.reloadConfig(fullPath);
    EXPECT_EQ(1280, loaded.width);
    EXPECT_EQ(720, loaded.height);
}

TEST(ConfigTest, UpdateCallbackOnReload)
{
    const std::string dir = testDir() + "/conf3";
    const std::string path = dir + "/cb.json";
    ::remove(path.c_str());

    ConfigManager mgr(dir + "/");
    TestConf conf;
    int hits = 0;
    conf.setUpdateCallback([&](TestConf& c) {
        (void)c;
        ++hits;
    });
    conf.width = 10;
    conf.height = 20;
    mgr.registerConfig("cb", conf, path);

    // 外部改文件后 reload
    {
        std::ofstream out(path);
        out << R"({
  "cb": {
    "width": 11,
    "height": 22
  }
})";
    }
    mgr.reloadConfig(path);
    EXPECT_EQ(11, conf.width);
    EXPECT_EQ(22, conf.height);
    EXPECT_GE(hits, 1);
}
