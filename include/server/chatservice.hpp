#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include "json.hpp"
#include <unordered_map>
#include <functional>
#include <memory>
#include <muduo/net/TcpConnection.h>
#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "redis.hpp"
#include "groupmodel.hpp"
#include <mutex>

using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

using MsgHandler = std::function<void(const TcpConnectionPtr& ,json& ,Timestamp)>;
//业务
class ChatService
{
public:
    //获取单例对象
    static ChatService* instance();
    //登陆
    void login(const TcpConnectionPtr& conn,json& js,Timestamp time);
    //注册
    void reg(const TcpConnectionPtr& conn,json& js,Timestamp time);
    //一对一聊天
    void oneChat(const TcpConnectionPtr& conn,json& js,Timestamp time);
    //添加好友
    void addFriend(const TcpConnectionPtr& conn,json& js,Timestamp time);
    //创建群组
    void createGroup(const TcpConnectionPtr& conn,json& js,Timestamp time);
    //加入群组
    void addGroup(const TcpConnectionPtr& conn,json& js,Timestamp time);
    //群组聊天
    void groupChat(const TcpConnectionPtr& conn,json& js,Timestamp time);
    //登出处理
    void loginout(const TcpConnectionPtr& conn,json& js,Timestamp time);
    //异常退出
    void clientCloseException(const TcpConnectionPtr& conn);
    //处理订阅消息
    void handleRedisSubscribeMessage(int ,std::string);
    //服务器异常，重置
    void reset();
    //获取消息对应的处理器
    MsgHandler getHandler(int msgid);

private:
    ChatService();
    std::unordered_map<int,MsgHandler> _msgHandlerMap;
    //保证连接映射线程安全
    std::mutex _connMutex;
    std::unordered_map<int,TcpConnectionPtr> _userConnMap;
    UserModel _userModel;
    OfflineMsgModel _offlineMsgModel;
    FriendModel _friendModel;
    GroupModel _groupModel;
    Redis _redis;
};

#endif