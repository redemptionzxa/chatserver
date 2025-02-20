#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H

#include "user.hpp"
#include "connectionPool.hpp"
#include <vector>
#include <memory>

class FriendModel
{
public:
    FriendModel();
    //添加好友关系
    void insert(int userid,int friendid);

    //返回用户好友列表
    std::vector<User> query(int userid);
private:
    ConnectionPool* _dbConnPool;
};
#endif