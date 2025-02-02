#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>

class Redis
{
public:
    Redis();
    ~Redis();
    //连接
    bool connect();
    //发布
    bool publish(int channel,std::string message);
    //订阅
    bool subscribe(int channel);
    bool unsubscribe(int channel);
    void observer_channel_message();
    void init_notify_handler(std::function<void(int,std::string)> fn);
private:
    //保存上下文
    redisContext* _publish_context;
    redisContext* _subscribe_context;
    //回调函数，供Service层使用
    std::function<void(int,std::string)> _notify_message_handler;
};

#endif