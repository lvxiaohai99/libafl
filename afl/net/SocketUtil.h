/**
 * @file   SocketUtil.h
 * @brief  socket 常用定义、宏与函数
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>

typedef int AFL_SOCKET;
typedef sockaddr_in AFL_SOCKADDR_IN;
typedef socklen_t ZL_SOCKLEN;

#define AFL_INVALID_SOCKET -1
#define RECV_FLAGS 0
#define SEND_FLAGS MSG_NOSIGNAL
#define AFL_SOCKET_ERROR errno
#define SOCK_ERR_EINTR EINTR // 阻塞的操作被取消阻塞的调用打断
#define SOCK_ERR_EAGAIN EAGAIN // 非阻塞下没有 连接请求/数据可读/数据可写, 不是错误
#define SOCK_ERR_EINPROGRESS EINPROGRESS // 操作正在进行中，一个阻塞的操作正在执行
#define SOCK_ERR_EWOULDBLOCK EWOULDBLOCK   // 资源暂时不可用, 通常和EAGAIN一样
#define SOCK_ERR_ECONNABORTED ECONNABORTED // 连接中断
#define SOCK_ERR_ECONNREFUSED ECONNREFUSED // 拒绝连接，一般发生在连接建立时
#define SOCK_ERR_EBADF EBADF               // 非法的文件描述符
#define SOCK_ERR_EADDRINUSE EADDRINUSE     // 地址已被使用
#define SOCK_ERR_NOTSOCK ENOTSOCK          // 文件描述符为文件的文件描述符
#define SOCK_ERR_EINVAL EINVAL             // 提供的参数非法
#define SOCK_ERR_EMFILE EMFILE             // 达到进程打开文件描述符限制

#define AFL_CREATE_SOCKET(a, b, c) ::socket(a, b, c)
#define AFL_BIND(a, b, c) ::bind(a, b, c)
#define AFL_LISTEN(a, b) ::listen(a, b)
#define AFL_ACCEPT(a, b, c) ::accept(a, b, c)
#define AFL_CONNECT(a, b, c) ::connect(a, b, c)
#define AFL_CLOSE(a) ::close(a)
#define AFL_READ(a, b, c) ::read(a, b, c)
#define AFL_RECV(a, b, c, d) ::recv(a, b, c, d)
#define AFL_RECVFROM(a, b, c, d, e, f) ::recvfrom(a, (char*)b, c, d, (sockaddr*)e, f)
#define AFL_SELECT(a, b, c, d, e) ::select(a, b, c, d, e)
#define AFL_SEND(a, b, c, d) ::send(a, (const char*)b, c, d)
#define AFL_SENDTO(a, b, c, d, e, f) ::sendto(a, (const char*)b, c, d, e, f)
#define AFL_SENDFILE(a, b, c, d) ::sendfile(a, b, c, d)
#define AFL_WRITE(a, b, c) ::write(a, b, c)
#define AFL_WRITEV(a, b, c) ::writev(a, b, c)
#define AFL_GETSOCKOPT(a, b, c, d, e) ::getsockopt((int)a, (int)b, (int)c, (void*)d, (socklen_t*)e)
#define AFL_SETSOCKOPT(a, b, c, d, e) ::setsockopt((int)a, (int)b, (int)c, (const void*)d, (int)e)
#define AFL_GETHOSTBYNAME(a) ::gethostbyname((const char*)a)
#define AFL_LSEEK(a, b, c) ::lseek(a, b, c)


// 判断错误码e是否表示一个socket的read/write/connect等操作没有错误，但需要等待
#if SOCK_ERR_EAGAIN == SOCK_ERR_EWOULDBLOCK
#define SOCK_ERR_IS_EAGAIN(e) ((e) == SOCK_ERR_EAGAIN)
#else
#define SOCK_ERR_IS_EAGAIN(e) ((e) == SOCK_ERR_EAGAIN || (e) == SOCK_ERR_EWOULDBLOCK)
#endif

// 判断错误码e是否表示一个socket的read/write操作可以重试
#define SOCK_ERR_RW_RETRY(e) ((e) == SOCK_ERR_EINTR || SOCK_ERR_IS_EAGAIN(e))

// 判断错误码e是否表示一个socket的connect操作被拒绝
#define SOCK_ERR_CONNECT_REFUSED(e) ((e) == SOCK_ERR_ECONNREFUSED)


// 判断错误码e是否表示一个socket的connect操作可以重试
#define SOCK_ERR_CONNECT_RETRY(e) ((e) == SOCK_ERR_EINTR || (e) == SOCK_ERR_EINPROGRESS)

// 判断错误码e是否表示一个socket的accept操作可以重试
#define SOCK_ERR_ACCEPT_RETRY(e)                                                                   \
    ((e) == SOCK_ERR_EINTR || SOCK_ERR_IS_EAGAIN(e) || (e) == ECONNABORTED)


namespace afl
{
namespace net
{
class SocketUtil
{
public:
    static AFL_SOCKET createSocket();
    static AFL_SOCKET createSocketAndListen(const char* ip, int port, int backlog = 5);
    static int closeSocket(AFL_SOCKET fd);
    static void shutDown(AFL_SOCKET fd);
    static void shutdownWrite(AFL_SOCKET sockfd);

    static int bind(AFL_SOCKET sockfd, const char* ip, int port);
    static int bind(AFL_SOCKET sockfd, struct sockaddr_in addr);
    static int connect(AFL_SOCKET sockfd, const char* ip, int port);
    static int connect(AFL_SOCKET sockfd, const struct sockaddr_in& addr);
    static AFL_SOCKET accept(AFL_SOCKET sockfd, struct sockaddr_in* addr);
    static ssize_t read(AFL_SOCKET sockfd, void* buf, size_t count);
    static ssize_t write(AFL_SOCKET sockfd, const void* buf, size_t count);

    static int setNonBlocking(AFL_SOCKET fd, bool nonBlocking = true);
    static int setNoDelay(AFL_SOCKET fd, bool noDelay = true);
    static int setReuseAddr(AFL_SOCKET fd, bool resue = true);
    static int setReusePort(AFL_SOCKET fd, bool resue = true);
    static int setKeepAlive(AFL_SOCKET fd, bool alive = true);

    static int setSendTimeout(AFL_SOCKET fd, long long timeoutMs);
    static int getSendTimeout(AFL_SOCKET fd, long long* timeoutMs);
    static int setRecvTimeout(AFL_SOCKET fd, long long timeoutMs);
    static int getRecvTimeout(AFL_SOCKET fd, long long* timeoutMs);

    static int setSendBuffer(AFL_SOCKET fd, int readSize);
    static int getSendBuffer(AFL_SOCKET fd, int* readSize);
    static int setRecvBuffer(AFL_SOCKET fd, int writeSize);
    static int getRecvBuffer(AFL_SOCKET fd, int* writeSize);

    static int setOpt(AFL_SOCKET fd, int level, int name, char* value, int len);
    static int getOpt(AFL_SOCKET fd, int level, int optname, int& optval);

    static struct sockaddr_in getLocalAddr(AFL_SOCKET sockfd);
    static std::string getLocalIp(AFL_SOCKET sockfd);
    static short getLocalPort(AFL_SOCKET sockfd);
    static std::string getLocalIpPort(AFL_SOCKET sockfd);
    static struct sockaddr_in getPeerAddr(AFL_SOCKET sockfd);
    static std::string getPeerIp(AFL_SOCKET sockfd);
    static short getPeerPort(AFL_SOCKET sockfd);
    static std::string getPeerIpPort(AFL_SOCKET sockfd);

    static bool isSelfConnect(AFL_SOCKET sockfd);
    static int getSocketError(AFL_SOCKET sockfd);
};

} // namespace net
} // namespace afl
