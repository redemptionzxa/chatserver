#include "offlinemessagemodel.hpp"
#include "db.h"

OfflineMsgModel::OfflineMsgModel()
{
    _dbConnPool = ConnectionPool::getInstance();
}

void OfflineMsgModel::insert(int userid, std::string msg)
{
    char sql[1024] = {0};
    std::sprintf(sql, "insert into OfflineMessage values(%d,'%s')",
                 userid, msg.c_str());
    std::shared_ptr<MySQL> mysql(_dbConnPool->getConnection());
    mysql->update(sql);
}

void OfflineMsgModel::remove(int userid)
{
    char sql[1024] = {0};
    std::sprintf(sql, "delete from OfflineMessage where userid = %d", userid);
    std::shared_ptr<MySQL> mysql(_dbConnPool->getConnection());
    mysql->update(sql);
}

std::vector<std::string> OfflineMsgModel::query(int userid)
{
    char sql[1024] = {0};
    std::sprintf(sql, "select message from OfflineMessage where userid = %d", userid);
    std::shared_ptr<MySQL> mysql(_dbConnPool->getConnection());

    std::vector<std::string> vec;
    MYSQL_RES *res = mysql->query(sql);
    if (res != nullptr)
    {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != nullptr)
        {
            vec.push_back(row[0]);
        }
        mysql_free_result(res);
        return vec;
    }
    return vec;
}