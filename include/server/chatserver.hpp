#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>

using namespace muduo;
using namespace muduo::net;

class ChatServer
{
public:
    //服务器初始化
    ChatServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const string& nameArg);
    //入口
    void start();

private:
    //相关回调函数
    void onConnection(const TcpConnectionPtr&);
    void onMessage(const TcpConnectionPtr&,
                        Buffer*,
                        Timestamp);
    TcpServer _server; //组合muduo库的服务器对象
    EventLoop* _loop;  //事件循环
};

#endif