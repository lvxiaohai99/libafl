/**
 * @file   MasterWorkerProcess.h
 * @brief  固定数目的 watcher-worker 多进程模型（prefork）
 * @author libafl
 * @date   2026-09
 */
#pragma once

#include "afl/base/Common.h"

#include <unistd.h>
#include <list>

namespace afl
{
namespace process
{
// 常用的 watcher-worker 多进程模型，亦称 prefork 模型。
//
// 参考: https://github.com/lighttpd/lighttpd1.4/blob/master/src/server.c
//
// 主进程创建固定数目的子进程处理任务；子进程退出后主进程再 fork，使 worker 数量保持恒定。
//
// while (主进程 && 程序继续运行)
// {
//     if (还有未创建的子进程)
//     {
//         创建一个新的子进程;
//         if (是子进程) 跳出循环;
//         if (是父进程) 未创建的子进程数量减 1;
//     }
//     else
//     {
//         阻塞等待子进程退出;
//         一旦有子进程退出，未创建的子进程数量加 1;
//     }
// }
//
// if (子进程)
// {
//     // 子进程入口，处理任务
// }
// else
// {
//     // 主进程退出点：终止全部子进程并清理资源
// }

class MasterWorkerProcess;

typedef void (*ProcessCallback)(MasterWorkerProcess* master, int jobId, void* arg);

class MasterWorkerProcess
{
public:
    MasterWorkerProcess();
    ~MasterWorkerProcess();

    static pid_t mainProcessPid() { return m_mainProcessPid; }

    void createWorkProcess(int workProcessNum, const ProcessCallback& callback, void* arg);

    bool isMainProcess() const { return mainProcessPid() == ::getpid(); }

    bool hasChild() const { return !m_childrenPids.empty(); }

    pid_t pid() const { return m_pid; }

    bool shutdown() const;

    void stop();

private:
    pid_t createOneProcess();

    static pid_t m_mainProcessPid;

    volatile bool m_running;
    pid_t m_pid;
    std::list<pid_t> m_childrenPids;
};

} // namespace process
} // namespace afl
