/**
 * @file   CallBacks.h
 * @brief  网络库通用回调定义
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/time/TimeStamp.h"
#include <memory> // std::shared_ptr

namespace afl
{
namespace net
{
class ByteArray;
class EventLoop;
class TcpConnection;
class InetAddress;
class TcpAcceptor;
class ByteBuffer;
using afl::time::TimeStamp;

typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

void defaultConnectionCallback(const TcpConnectionPtr& conn);
void defaultMessageCallback(const TcpConnectionPtr& conn, ByteBuffer* buffer,
                            TimeStamp receiveTime);

typedef std::function<void(const TcpConnectionPtr&)> ConnectionCallback;
typedef std::function<void(const TcpConnectionPtr&)> CloseCallback;
typedef std::function<void(const TcpConnectionPtr&)> WriteCompleteCallback;
typedef std::function<void(const TcpConnectionPtr&, ByteBuffer*, TimeStamp)> MessageCallback;


typedef int TimerId;
typedef std::function<void()> TimerCallback;

typedef std::function<void(int)> SignalCallback;

} // namespace net
} // namespace afl
