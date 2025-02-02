#ifndef DB_H
#define DB_H

#include <mysql/mysql.h>
#include <string>
#include <muduo/base/Logging.h>

static std::string server = "127.0.0.1";
static std::string user = "root";
static std::string password = "as123456z";
static std::string dbname = "chat";

class MySQL
{
public:
    MySQL();
    ~MySQL();
    bool connect();
    bool update(std::string sql);
    MYSQL_RES* query(std::string sql);
    //获取连接
    MYSQL* getConnection();
private:
    MYSQL* _conn;
};

#endif