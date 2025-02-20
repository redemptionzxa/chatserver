#include "usermodel.hpp"
#include "db.h"
#include <iostream>

UserModel::UserModel()
{
    _dbConnPool = ConnectionPool::getInstance();
}

bool UserModel::insert(User &user)
{
    char sql[1024] = {0};
    std::sprintf(sql, "insert into User(name,password,state) values('%s','%s','%s')",
                 user.getName().c_str(),
                 user.getPwd().c_str(),
                 user.getState().c_str());
    std::shared_ptr<MySQL> mysql(_dbConnPool->getConnection());
    if (mysql->update(sql))
    {
        // 获取用户主键id
        user.setId(mysql_insert_id(mysql->getConnection()));
        return true;
    }

    return false;
}

User UserModel::query(int id)
{
    char sql[1024] = {0};
    std::sprintf(sql, "select * from User where id = %d", id);
    std::shared_ptr<MySQL> mysql(_dbConnPool->getConnection());
    MYSQL_RES *res = mysql->query(sql);
    if (res != nullptr)
    {
        MYSQL_ROW row = mysql_fetch_row(res);
        if (row != nullptr)
        {
            User user;
            user.setId(atoi(row[0]));
            user.setName(row[1]);
            user.setPwd(row[2]);
            user.setState(row[3]);
            mysql_free_result(res);
            return user;
        }
    }
    return User();
}

bool UserModel::updateState(User user)
{
    char sql[1024] = {0};
    std::sprintf(sql, "update User set state = '%s' where id = %d",
                 user.getState().c_str(),
                 user.getId());
    std::shared_ptr<MySQL> mysql ( _dbConnPool->getConnection());
    if (mysql->update(sql))
    {
            return true;
    }
    return false;
}

void UserModel::resetState()
{
    char sql[1024] = "update User set state = 'offline' where state = 'online'";
    std::shared_ptr<MySQL> mysql ( _dbConnPool->getConnection());
    mysql->update(sql);
}