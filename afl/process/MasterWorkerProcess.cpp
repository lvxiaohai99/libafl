/**
 * @file   MasterWorkerProcess.cpp
 * @brief  prefork 多进程模型的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/process/MasterWorkerProcess.h"

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h> // for strerror
#include <stdlib.h> // for abort
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/signalfd.h>
#include <sys/syscall.h>

namespace afl
{
namespace process
{
volatile sig_atomic_t srv_shutdown = 0;
volatile sig_atomic_t graceful_shutdown = 0;
volatile sig_atomic_t handle_sig_alarm = 1;
volatile sig_atomic_t handle_sig_hup = 0;
volatile sig_atomic_t forwarded_sig_hup = 0;

namespace detail
{
static void signalHandler(int sig)
{
    printf("process [%d] get signal no[%d]\n", ::getpid(), sig);
    switch (sig)
    {
    case SIGTERM: // 15
        srv_shutdown = 1;
        break;
    case SIGINT: // 2
        if (graceful_shutdown)
        {
            srv_shutdown = 1;
        }
        else
        {
            graceful_shutdown = 1;
        }
        printf("process [%d] SIGINT: [%d][%d]\n", ::getpid(), graceful_shutdown, srv_shutdown);
        break;
    case SIGALRM: // 14
        handle_sig_alarm = 1;
        break;
    case SIGHUP: // 1
        handle_sig_hup = 1;
        break;
    case SIGCHLD:
        break;
    }
}

static void setSignalHandler()
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGUSR1, SIG_IGN);
    signal(SIGALRM, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGHUP, signalHandler);
    signal(SIGCHLD, signalHandler);
    signal(SIGINT, signalHandler);
    signal(SIGALRM, signalHandler);
}
} // namespace detail

pid_t gettid()
{
    return static_cast<pid_t>(::syscall(SYS_gettid));
}

pid_t MasterWorkerProcess::m_mainProcessPid = 0;

MasterWorkerProcess::MasterWorkerProcess() : m_running(0), m_pid(::getpid())
{
    detail::setSignalHandler();
}

MasterWorkerProcess::~MasterWorkerProcess()
{
    stop();
}

bool MasterWorkerProcess::shutdown() const
{
    return srv_shutdown;
}

pid_t MasterWorkerProcess::createOneProcess()
{
    pid_t pid = ::fork();
    if (pid < 0)
    {
        printf("fork error %s", strerror(errno));
        abort();
        return -1;
    }
    else if (pid > 0) // parent process
    {
        m_childrenPids.push_back(pid);
        return 0;
    }
    else //if (pid == 0)   // child process
    {
        m_childrenPids.clear();
        //close(m_fd);
        return ::getpid(); // return child process pid
    }
}

void MasterWorkerProcess::createWorkProcess(int workProcessNum, const ProcessCallback& callback,
                                            void* arg)
{
    assert(workProcessNum >= 0);
    assert(m_mainProcessPid <= 0);
    assert(callback);

    m_running = 1;
    m_mainProcessPid = ::getpid();

    if (workProcessNum == 0)
    {
        printf("not create work process,just do work on main process[%d]\n", ::getpid());
        callback(this, 0, arg);
        return;
    }

    int childId = 0;
    bool child = false;
    while (m_running && !child && !srv_shutdown && !graceful_shutdown) //prefork child
    {
        if (workProcessNum > 0) // continue create process
        {
            childId++;
            pid_t pid = ::fork();
            switch (pid)
            {
            case -1: // fork error
                return;
            case 0: // child process, set flag for breaking while-loop
                m_childrenPids.clear();
                child = true;
                break;
            default: // master process
                m_childrenPids.push_back(pid);
                workProcessNum--;
                break;
            }
        }
        else // all children be created, just wait children exit
        {
            int status = 0;
            pid_t pid;
            if ((pid = waitpid(-1, &status, WNOHANG)) > 0) // one child process exit
            {
                printf("master process: get one child[%d] exit\n", pid);
                workProcessNum++;
                m_childrenPids.remove(pid);
            }
            else
            {
                switch (errno)
                {
                case EINTR:
                    if (handle_sig_hup)
                    {
                        handle_sig_hup = 0;
                    }
                    break;
                default:
                    break;
                }
            }
        }
    } // end of while

    if (child) // child process, do some non-trival thing
    {
        printf("child process[%d], jobid[%d]: father process id[%d], current thread[%d]\n",
               ::getpid(), childId, ::getppid(), gettid());
        callback(this, childId, arg);
        printf("child process[%d], jobid[%d]: exited......\n", ::getpid(), childId);
    }
    else //if (!child)    //for the parent this is the exit-point
    {
        assert(!child);
        printf("master process[%d]: exit, kill all children process\n", m_mainProcessPid);
        if (graceful_shutdown) // kill all children
        {
            printf("master process[%d]: graceful_shutdown, kill all child by SIGINT\n",
                   m_mainProcessPid);
            for (size_t i = 0; i < m_childrenPids.size(); ++i)
            {
                kill(0, SIGINT);
            }
        }
        else if (srv_shutdown)
        {
            printf("master process[%d]: srv_shutdown, kill all child by SIGTERM\n",
                   m_mainProcessPid);
            for (size_t i = 0; i < m_childrenPids.size(); ++i)
            {
                kill(0, SIGTERM);
            }
        }

        // do something that need cleanup or close
        // ......
    }
}

void MasterWorkerProcess::stop()
{
    printf("process[%d] : stopped[%zd]\n", ::getpid(), m_childrenPids.size());
    if (!m_running)
        return;

    m_running = 0;
    for (size_t i = 0; i < m_childrenPids.size(); ++i)
    {
        kill(0, SIGTERM);
    }
}

} // namespace process
} // namespace afl
