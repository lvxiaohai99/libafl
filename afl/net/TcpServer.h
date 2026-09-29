/**
 * @file   TcpServer.h
 * @brief  TCP 服务器
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include "afl/time/TimeStamp.h"
#include "afl/concurrency/Mutex.h"
#include "afl/net/CallBacks.h"
#include "afl/net/InetAddress.h"
#include "afl/base/NonCopy.h"

namespace afl
{
namespace net
{
using afl::time::TimeStamp;
class ByteBuffer;
class EventLoop;
class Tcpconnection;
class InetAddress;
class Acceptor;
class EventLoopThreadPool;

class TcpServer : afl::base::NonCopy
{
public:
    TcpServer(EventLoop* loop, const InetAddress& listenAddr,
              const std::string& server_name = "TcpServer");
    virtual ~TcpServer();

    /// 设置EventLoopThreadPool的threads大小；if numThreads
    /// < 0  : 设置该为当前系统CPU并发数；
    /// == 0 : 不使用EventLoopThreadPool，所有Channel都在同一个EventLoop中运行，默认值；
    /// > 0  : 设置numThreads个线程，也即numThreads个EventLoop，每个连接择其中
    /// 注意 该函数必须在start之前调用
    void setMultiReactorThreads(int numThreads);

    /// 启动TcpServer，设置server socket listen
    /// 注意：必须调用该接口，且只限调用
    void start();

public:
    EventLoop* getLoop() const { return m_loop; }

    void setConnectionCallback(const ConnectionCallback& cb) { m_connectionCallback = cb; }

    void setMessageCallback(const MessageCallback& cb) { m_messageCallback = cb; }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { m_writeCompleteCallback = cb; }

private:
    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

protected:
    typedef std::map<int, TcpConnectionPtr> ConnectionMap;
    typedef std::vector<EventLoopThreadPool*> EventLoopList;
    EventLoop* m_loop; // acceptor eventloop
    TcpAcceptor* m_acceptor;
    InetAddress m_serverAddr;

    ConnectionCallback m_connectionCallback;
    MessageCallback m_messageCallback;
    WriteCompleteCallback m_writeCompleteCallback;

    ConnectionMap m_connections;
    EventLoopThreadPool* m_evloopThreadPool;

    const std::string m_serverName;
};

} // namespace net
} // namespace afl
