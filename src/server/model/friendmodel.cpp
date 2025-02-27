#include "friendmodel.hpp"
#include "db.h"

FriendModel::FriendModel()
{
    _dbConnPool = ConnectionPool::getInstance();
}


void FriendModel::insert(int userid,int friendid)
{
    char sql[1024] = {0};
    std::sprintf(sql,"insert into Friend values(%d,%d)",
                userid,friendid);
    std::shared_ptr<MySQL> mysql ( _dbConnPool->getConnection());
    mysql->update(sql); 
}

//返回用户好友列表
std::vector<User> FriendModel::query(int userid)
{
    char sql[1024] = {0};
    std::sprintf(sql,"select User.id,User.name,User.state from User inner join Friend on Friend.friendid = User.id where userid = %d",userid);
    std::shared_ptr<MySQL> mysql ( _dbConnPool->getConnection());

    std::vector<User> vec;
    MYSQL_RES* res = mysql->query(sql);
    if(res != nullptr)
    {
        MYSQL_ROW row;
        while((row = mysql_fetch_row(res)) != nullptr)
        {
            User user;
            user.setId(atoi(row[0]));
            user.setName(row[1]);
            user.setState(row[2]);
            vec.push_back(user);
        }
        mysql_free_result(res);
        return vec;
    }
    return vec;
}