#include "chatservice.hpp"
#include "chatserver.hpp"
#include "json.hpp"

#include <functional>
#include <string>

using namespace std::placeholders;
using json = nlohmann::json;

ChatServer::ChatServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const string& nameArg)
            :_server(loop,listenAddr,nameArg),_loop(loop)
{
    //注册回调函数
    _server.setConnectionCallback(
        std::bind(&ChatServer::onConnection,this,_1));
    _server.setMessageCallback(
        std::bind(&ChatServer::onMessage,this,_1,_2,_3));
    //设置网络库参数
    _server.setThreadNum(4);
}

void ChatServer::start()
{
    _server.start();
}

void ChatServer::onConnection(const TcpConnectionPtr& conn)
{
    //用户断开连接，释放资源
    if(!conn->connected())
    {
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

void ChatServer::onMessage(const TcpConnectionPtr& conn,
                        Buffer* buffer,
                        Timestamp time)
{
    //网络流信息
    string buf = buffer->retrieveAllAsString();
    //反序列化
    json js = json::parse(buf);
    //获取处理器，并执行
    ChatService* temp = ChatService::instance();
    auto msgHandler = temp->getHandler(js["msgid"].get<int>());
    msgHandler(conn,js,time);
}