/**
 * @file   EventLoopManagerImpl.cpp
 * @brief  EventLoop 管理器实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/framework/details/EventLoopManagerImpl.h"
#include "afl/concurrency/CountDownLatch.h"
#include "afl/base/Exception.h"
#include "afl/log/Log.h"

#include <sstream>
#include <limits.h>
#include <signal.h>
#include <unistd.h>
#include <sys/resource.h>
#include <cstring>
#include <set>
#include <algorithm>
#include <errno.h>

int pthread_create(pthread_t*, const pthread_attr_t*, void* (*)(void*), void*)
    __attribute__((weak));

int pthread_attr_setstacksize(pthread_attr_t* __attr, size_t __stacksize) __attribute__((weak));

int pthread_detach(pthread_t __th) __attribute__((weak));

namespace afl
{
namespace fw
{
const int PTHREAD_CREATE_RETRY_TIME = 3;

std::shared_ptr<afl::net::EventLoop> EventLoopManagerImpl::s_MainLoop;

EventLoopManagerImpl::EventLoopManagerImpl()
{
    s_MainLoop = std::make_shared<afl::net::EventLoop>();

    std::lock_guard<std::mutex> guard(s_Thread2EventLoopMapLock);
    s_Thread2EventLoopMap[pthread_self()] = s_MainLoop;
    LOG_INFO("EventLoopManager: main loop created tid=%lu", (unsigned long)pthread_self());
}

EventLoopManagerImpl::~EventLoopManagerImpl() {}

void EventLoopManagerImpl::run()
{
    LOG_INFO("EventLoopManager::run enter main loop");
    installTraceBackSignals();
    s_MainLoop->loop();
    LOG_INFO("EventLoopManager::run main loop exited");
}

void EventLoopManagerImpl::quit()
{
    LOG_INFO("EventLoopManager::quit");
    {
        std::lock_guard<std::mutex> guard(s_Thread2EventLoopMapLock);
        for (auto& ev : s_Thread2EventLoopMap)
        {
            auto loop = ev.second.lock();
            if (loop)
            {
                loop->quit();
            }
        }
    }
    s_MainLoop->quit();
}

int EventLoopManagerImpl::createCustomAttributes(const std::string& name, bool unique,
                                                 int stackSize, int policy, int priority)
{
    if (name.empty())
    {
        LOG_ERROR("EventLoopManager::createCustomAttributes empty name");
        return -1;
    }

    stackSize *= 1024; /* stackSize 单位为 KB */
    stackSize = std::max(PTHREAD_STACK_MIN, stackSize);
    stackSize = std::min(stackSize, (int)getUpperLimit(RLIMIT_STACK, 8192 * 1024));

    if (SCHED_FIFO != policy && SCHED_RR != policy && SCHED_OTHER != policy)
    {
        LOG_ERROR("EventLoopManager::createCustomAttributes bad policy=%d name=%s", policy,
                  name.c_str());
        return -2;
    }

    if (SCHED_FIFO == policy || SCHED_RR == policy)
    {
        priority = std::max(priority, sched_get_priority_min(policy));
        priority = std::min(priority, sched_get_priority_max(policy));
    }

    ThreadAttribute attr;
    attr.unique = unique;
    attr.stackSize = stackSize;
    attr.policy = policy;
    attr.priority = priority;

    {
        std::lock_guard<std::mutex> guard(s_ModuleThreadAttrsLock);
        s_ModuleThreadAttrs[name] = attr;
    }

    LOG_INFO("EventLoopManager::createCustomAttributes name=%s unique=%d stack=%d policy=%d prio=%d",
             name.c_str(), (int)attr.unique, attr.stackSize, attr.policy, attr.priority);
    return 0;
}

std::shared_ptr<afl::net::EventLoop> EventLoopManagerImpl::getEventLoop(const std::string& name)
{
    std::shared_ptr<afl::net::EventLoop> retval = nullptr;
    ThreadAttribute attr;

    {
        std::lock_guard<std::mutex> guard(s_ModuleThreadAttrsLock);
        auto attrIter = s_ModuleThreadAttrs.find(name);
        if (attrIter == s_ModuleThreadAttrs.end())
        {
            return s_MainLoop;
        }

        attr = attrIter->second;
    }

    if (!attr.unique)
    {
        retval = createEventLoop(attr);
        if (!retval)
        {
            LOG_WARN("EventLoopManager: create failed, use main loop for %s", name.c_str());
            retval = s_MainLoop;
        }
        LOG_DEBUG("EventLoopManager: non-unique loop ready name=%s", name.c_str());
        return retval;
    }

    std::lock_guard<std::mutex> guard(s_Name2EventLoopMapLock);
    auto loopIter = s_Name2EventLoopMap.find(name);
    if (loopIter != s_Name2EventLoopMap.end())
    {
        retval = loopIter->second.lock();
    }

    if (retval)
    {
        LOG_DEBUG("EventLoopManager: reuse unique loop name=%s", name.c_str());
        return retval;
    }

    retval = createEventLoop(attr);
    if (!retval)
    {
        LOG_WARN("EventLoopManager: unique create failed, use main loop for %s", name.c_str());
        retval = s_MainLoop;
    }
    s_Name2EventLoopMap[name] = retval;
    LOG_INFO("EventLoopManager: unique loop created name=%s ptr=%p", name.c_str(), retval.get());

    return retval;
}

std::shared_ptr<afl::net::EventLoop> EventLoopManagerImpl::getCurrentEventLoop()
{
    std::shared_ptr<afl::net::EventLoop> retval;
    pthread_t tid = pthread_self();

    {
        std::lock_guard<std::mutex> guard(s_Thread2EventLoopMapLock);
        auto iter = s_Thread2EventLoopMap.find(tid);
        retval = (iter == s_Thread2EventLoopMap.end()) ? nullptr : iter->second.lock();
    }

    return retval;
}

std::shared_ptr<afl::net::EventLoop>
EventLoopManagerImpl::createEventLoop(const ThreadAttribute& attr)
{
    if (nullptr == pthread_create)
    {
        LOG_ERROR("EventLoopManager: pthread_create unavailable, cannot spawn worker loop");
        return nullptr;
    }

    std::shared_ptr<afl::net::EventLoop> retval = nullptr;

    afl::concurrency::CountDownLatch cdl(1);
    void* args[] = {&cdl, &retval, this};

    pthread_t tid;
    struct sched_param param;
    pthread_attr_t pAttr;
    memset(&pAttr, 0x00, sizeof(pAttr));

    pthread_attr_init(&pAttr);
    pthread_attr_setscope(&pAttr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setstacksize(&pAttr, attr.stackSize);

    if (SCHED_FIFO == attr.policy || SCHED_RR == attr.policy)
    {
        pthread_attr_setschedpolicy(&pAttr, attr.policy);
        pthread_attr_getschedparam(&pAttr, &param);

        param.sched_priority = (0 == geteuid()) ? (attr.priority) : 0;
        pthread_attr_setschedparam(&pAttr, &param);
    }

    int retry = PTHREAD_CREATE_RETRY_TIME;
    while (1)
    {
        int retcode = pthread_create(&tid, &pAttr, createEventLoopDetail, args);
        if (0 != retcode && (EAGAIN == errno || EWOULDBLOCK == errno) && retry-- > 0)
        {
            LOG_WARN("EventLoopManager: pthread_create EAGAIN retry(%d/%d)",
                     PTHREAD_CREATE_RETRY_TIME - retry, PTHREAD_CREATE_RETRY_TIME);
            usleep(1 * 1000);
            continue;
        }

        if (0 != retcode)
        {
            LOG_ERROR("EventLoopManager: pthread_create failed: %s", strerror(retcode));
            pthread_attr_destroy(&pAttr);
            return nullptr;
        }
        break;
    }

    /* 等待子线程创建 EventLoop 并就绪 */
    cdl.wait();
    if (!retval)
    {
        LOG_ERROR("EventLoopManager: worker thread started but EventLoop is null");
        pthread_attr_destroy(&pAttr);
        return nullptr;
    }

    if (0 != pthread_detach(tid))
    {
        LOG_WARN("EventLoopManager: pthread_detach(tid=%lu) failed: %s", (unsigned long)tid,
                 strerror(errno));
    }
    pthread_attr_destroy(&pAttr);
    LOG_INFO("EventLoopManager: worker thread tid=%lu ready", (unsigned long)tid);

    return retval;
}

void* EventLoopManagerImpl::createEventLoopDetail(void* arg)
{
    pthread_t tid = pthread_self();
    afl::concurrency::CountDownLatch* cdl =
        reinterpret_cast<afl::concurrency::CountDownLatch*>(((char**)arg)[0]);
    std::shared_ptr<afl::net::EventLoop>* retloop =
        reinterpret_cast<std::shared_ptr<afl::net::EventLoop>*>(((char**)arg)[1]);
    EventLoopManagerImpl* evmi = reinterpret_cast<EventLoopManagerImpl*>(((char**)arg)[2]);

    assert(cdl && retloop);

    auto loop = std::make_shared<afl::net::EventLoop>();
    *retloop = loop;

    {
        std::lock_guard<std::mutex> guard(evmi->s_Thread2EventLoopMapLock);
        evmi->s_Thread2EventLoopMap[tid] = loop;
    }

    /* 通知主线程继续 */
    cdl->countDown();

    /* 无外部引用时自动退出线程 */
    int timer = loop->addTimer(
        [&]() {
            if (loop.use_count() <= 1)
                loop->quit();
        },
        1, true);

    installTraceBackSignals();

    /* 进入事件循环 */
    loop->loop();

    loop->cancelTimer(timer);

    {
        std::lock_guard<std::mutex> guard(evmi->s_Thread2EventLoopMapLock);
        auto iter = evmi->s_Thread2EventLoopMap.find(tid);
        if (iter != evmi->s_Thread2EventLoopMap.end())
        {
            evmi->s_Thread2EventLoopMap.erase(iter);
        }
    }

    LOG_INFO("EventLoopManager: worker thread tid=%lu exited", (unsigned long)tid);

    return nullptr;
}

void EventLoopManagerImpl::installTraceBackSignals()
{
    std::set<int> tobeCaptured = {SIGILL, SIGABRT,   SIGBUS,  SIGFPE, SIGSEGV,
                                  SIGSYS, SIGSTKFLT, SIGXCPU, SIGXFSZ};

    for (auto s : tobeCaptured)
    {
        signal(s, traceBack);
    }

    signal(SIGPIPE, SIG_IGN);
}

void EventLoopManagerImpl::traceBack(int signo)
{
    afl::base::Exception e("");
    LOG_CRITICAL("EventLoopManager: fatal signal %d\n%s", signo, e.stackTrace());
    _exit(signo);
}

size_t EventLoopManagerImpl::getUpperLimit(int type, size_t dft)
{
    struct rlimit rlim;
    if (0 != getrlimit(type, &rlim))
    {
        return dft;
    }

    return std::min(rlim.rlim_cur, rlim.rlim_max);
}

} // namespace fw
} // namespace afl
