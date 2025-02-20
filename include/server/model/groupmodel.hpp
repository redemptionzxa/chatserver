#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include "group.hpp"
#include "connectionPool.hpp"
#include <string>
#include <vector>

class GroupModel
{
public:
    GroupModel();
    bool createGroup(Group& group);
    void addGroup(int userid,int groupid, std::string role);
    std::vector<Group> queryGroups(int userid);
    std::vector<int> queryGroupUsers(int userid,int groupid);
private:
    ConnectionPool* _dbConnPool;
};

#endif