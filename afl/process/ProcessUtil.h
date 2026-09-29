/**
 * @file   ProcessUtil.h
 * @brief  进程相关信息获取函数
 * @author libafl
 * @date   2026-09
 */
#pragma once

#include "afl/base/Common.h"
#include "afl/time/TimeStamp.h"

#include <unistd.h>
#include <string>
#include <vector>

namespace afl
{
namespace process
{
namespace ProcessUtil
{
uid_t uid();
uid_t euid();
std::string username();
int clockTicksPerSecond();
int pageSize();
std::string hostname();

/** @brief 当前进程 id */
pid_t pid();
std::string pidString();
std::string procname();
std::string procname(const std::string& stat);

/** @brief 当前进程启动时间（毫秒精度） */
afl::time::TimeStamp startTime();

/** @brief 当前进程已运行时间（毫秒） */
int64_t elapsedTime();

std::string procStatus();
std::string procStat();
std::string threadStat();
std::string exePath();

int openedFiles();
int maxOpenFiles();

struct CpuTime
{
    double userSeconds;
    double systemSeconds;

    CpuTime() : userSeconds(0.0), systemSeconds(0.0) {}
};
CpuTime cpuTime();

int numThreads();
std::vector<pid_t> threads();

/**
 * @brief 设置是否允许当前进程生成 coredump
 * @param enabled true 启用 coredump
 * @param core_file_size 小于 0 表示不限制大小
 * @return 设置成功返回 true
 */
bool enableCoreDump(bool enabled = true, int core_file_size = -1);

/** @brief 根据进程名查找 pid，未找到返回 -1 */
int getPidByName(const char* procname);

/** @brief 根据 pid 获取进程名，未找到返回空字符串 */
std::string getNameByPid(pid_t pid);

std::string procStatus(pid_t pid);
std::string procStat(pid_t pid);
int numThreads(pid_t pid);
std::vector<pid_t> threads(pid_t pid);
std::string exePath(pid_t pid);

} // namespace ProcessUtil
} // namespace process
} // namespace afl
