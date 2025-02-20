#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H

#include "connectionPool.hpp"
#include <string>
#include <vector>


class OfflineMsgModel
{
public:
    OfflineMsgModel();
    void insert(int userid,std::string msg);

    void remove(int userid);

    std::vector<std::string> query(int userid);
private:
    ConnectionPool* _dbConnPool;
};


#endif