#include "redis.hpp"
#include <iostream>

Redis::Redis()
    : _publish_context(nullptr),_subscribe_context(nullptr)
{
}
Redis::~Redis()
{
    if(_publish_context != nullptr)
    {
        redisFree(_publish_context);
    }
    if(_subscribe_context != nullptr)
    {
        redisFree(_subscribe_context);
    }
}
// 连接
bool Redis::connect()
{
    _publish_context = redisConnect("127.0.0.1",6379);
    if(nullptr == _publish_context)
    {
        std::cerr << "connect redis failed!" << std::endl;
        return false;
    }
    _subscribe_context = redisConnect("127.0.0.1",6379);
    if(nullptr == _subscribe_context)
    {
        std::cerr << "connect redis failed!" << std::endl;
        return false;
    }
    std::thread t([&](){
        observer_channel_message();
    });
    t.detach();
    std::cout << "connect redis-server success!" << std::endl;
    return true;
}
// 发布
bool Redis::publish(int channel, std::string message)
{
    redisReply* reply = (redisReply*)redisCommand(_publish_context,
            "PUBLISH %d %s",channel,message.c_str());
    if(nullptr == reply)
    {
        std::cerr << "publish failed" << std::endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}
// 订阅
bool Redis::subscribe(int channel)
{
    if(REDIS_ERR == redisAppendCommand(this->_subscribe_context,
            "SUBSCRIBE %d",channel))
    {
        std::cerr << "subscribe fail!" << std::endl;
        return false;
    }
    int done = 0;
    while(!done)
    {
        if(REDIS_ERR == redisBufferWrite(this->_subscribe_context,&done))
        {
            std::cerr << "subscribe fail!" << std::endl;
            return false;
        }
    }
    return true;
}

bool Redis::unsubscribe(int channel)
{
    if(REDIS_ERR == redisAppendCommand(this->_subscribe_context,
            "UNSUBSCRIBE %d",channel))
    {
        std::cerr << "unsubscribe fail!" << std::endl;
        return false;
    }
    int done = 0;
    while(!done)
    {
        if(REDIS_ERR == redisBufferWrite(this->_subscribe_context,&done))
        {
            std::cerr << "unsubscribe fail!" << std::endl;
            return false;
        }
    }
    return true;
}
void Redis::observer_channel_message()
{
    redisReply* reply = nullptr;
    while(REDIS_OK == redisGetReply(this->_subscribe_context,(void**)&reply))
    {
        if(reply != nullptr && reply->element[2] != nullptr 
                && reply->element[2]->str!= nullptr)
        {
            _notify_message_handler(atoi(reply->element[1]->str),
                        reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    std::cerr << "observer_channel_message quit " << std::endl;
}
void Redis::init_notify_handler(std::function<void(int, std::string)> fn)
{
    this->_notify_message_handler = fn;
}