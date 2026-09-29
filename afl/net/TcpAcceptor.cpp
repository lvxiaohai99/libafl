/**
 * @file   TcpAcceptor.cpp
 * @brief  服务端接受器的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/net/TcpAcceptor.h"
#include "afl/net/Socket.h"
#include "afl/net/Channel.h"
#include "afl/net/EventLoop.h"
#include "afl/net/InetAddress.h"
#include "afl/base/Exception.h"
#include "afl/log/Log.h"

namespace afl
{
namespace net
{
TcpAcceptor::TcpAcceptor(EventLoop* loop, const InetAddress& listenAddr) : m_loop(loop)
{
    m_acceptSocket = new Socket(SocketUtil::createSocket());

    m_acceptSocket->setNoDelay();
    m_acceptSocket->setNonBlocking();

    if (!m_acceptSocket->setReuseAddr(true))
    {
        throw afl::base::Exception("Could not reuse socket address.");
    }
    if (!m_acceptSocket->bind(listenAddr))
    {
        throw afl::base::Exception("Could not bind to port.");
    }

    m_acceptChannel = new Channel(loop, m_acceptSocket->fd());
    m_acceptChannel->setReadCallback(
        std::bind(&TcpAcceptor::onAccept, this, std::placeholders::_1));
}

TcpAcceptor::~TcpAcceptor()
{
    m_acceptChannel->disableAll();
    m_acceptChannel->remove();
    SAFE_DELETE(m_acceptChannel);
    SAFE_DELETE(m_acceptSocket);
}

void TcpAcceptor::listen()
{
    m_loop->assertInLoopThread();
    if (!m_acceptSocket->listen(128)) //may be bigger, see 'cat /proc/sys/net/core/somaxconn'
    {
        throw afl::base::Exception("Could not listen to port.");
    }
    LOG_INFO("TcpAcceptor::listen on [%s]",
             SocketUtil::getLocalIpPort(m_acceptSocket->fd()).c_str());

    m_acceptChannel->enableReading();
}

void TcpAcceptor::onAccept(TimeStamp now)
{
    m_loop->assertInLoopThread();
    int count = 0;
    while (count < 100)
    {
        InetAddress peerAddr;
        AFL_SOCKET newfd = m_acceptSocket->accept(&peerAddr);
        if (newfd > 0)
        {
            if (m_newConnCallBack)
            {
                LOG_INFO("TcpAcceptor::OnAccept accept one client from[%d][%s]", newfd,
                         peerAddr.ipPort().c_str());
                m_newConnCallBack(newfd, peerAddr);
            }
            else
            {
                LOG_ALERT(
                    "TcpAcceptor::OnAccept() no callback , and close the coming connection![%d]",
                    newfd);
                SocketUtil::closeSocket(newfd);
            }
            count++;
        }
        else
        {
            if (AFL_SOCKET_ERROR == SOCK_ERR_EAGAIN || AFL_SOCKET_ERROR == SOCK_ERR_EWOULDBLOCK)
            {
                // 已处理本轮全部可读连接
            }
            else if (AFL_SOCKET_ERROR == SOCK_ERR_EMFILE)
            {
                // TODO：fd 耗尽导致 accept 失败；水平触发下可能反复唤醒
                // 会导致poller持续通知可读事件，因此成acceptor频繁去accept，直至进程中关闭
                // 其他连接而有空余描述符才停止。这样会导致CPU 100% loop
                // 解决方案  http://blog.csdn.net/solstice/article/details/6365666
                // http://pod.tst.eu/http://cvs.schmorp.de/libev/ev.pod#The_special_problem_of_accept_ing_wh
            }
            else
            {
                LOG_ALERT("TcpAcceptor::OnAccept() accept connection error![%d][%d]", newfd, errno);
            }
            break;
        }
    }
}

} // namespace net
} // namespace afl
