/**
 * @file   Daemonize.h
 * @brief  守护进程（daemon）创建工具
 * @author libafl
 * @date   2026-09
 */
#pragma once

#include "afl/base/Common.h"

#include <string>

namespace afl
{
namespace process
{
/**
 * @brief 创建守护进程（可设置同机是否允许多实例）
 * @param nochdir 为 0 时 chdir("/")，否则保持当前目录
 * @param noclose 为 0 时重定向 stdin/out/err 到 /dev/null
 * @param pidfile 非空时写入 pid 并禁止重复启动；空则允许多实例
 * @return 成功返回当前进程 pid，失败返回 -1
 */
int createDaemonize(int nochdir = 1, int noclose = 0, const char* pidfile = nullptr);

/**
 * @brief 退出守护：删除 pid 文件
 * @param pidfile pid 文件路径
 * @return unlink 返回值
 */
int exitDaemonize(const char* pidfile);

/**
 * @brief 守护进程 RAII 包装（基于 pid 文件）
 */
class Daemon
{
public:
    /**
     * @brief 构造
     * @param pidFile pid 文件路径
     */
    explicit Daemon(std::string pidFile);

    /** @brief 切换为守护进程 */
    void daemonize();
    /** @brief 取消守护并删除 pid 文件 */
    void undaemonize();

    /** @brief 是否已在运行（pid 文件有效且进程存活） */
    bool isRunning();
    /** @brief 当前记录的 pid */
    int getPid() { return m_pid; }

private:
    int getPidFromFile();

    int m_pid;
    bool m_running;
    std::string m_pidFile;
};

} // namespace process
} // namespace afl
