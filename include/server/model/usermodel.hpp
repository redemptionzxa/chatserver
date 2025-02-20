#ifndef USERMODEL_H
#define USERMODEL_H

#include "user.hpp"
#include "connectionPool.hpp"

class UserModel{
public:
    UserModel();
    bool insert(User& user);
    User query(int id);
    bool updateState(User user);
    void resetState();
    
private:
    ConnectionPool* _dbConnPool;
};

#endif