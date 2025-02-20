#include "json.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <ctime>

using json = nlohmann::json;

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <atomic>

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

// 读写通信量
sem_t rwsem;
// 记录登陆状态
std::atomic_bool g_isLoginSuccess(false);
// 当前登陆用户
User g_currentUser;
// 好友列表
std::vector<User> g_currentUserFriendList;
// 群组列表
std::vector<Group> g_currentUserGroupList;
// 控制程序结束
bool isMainMenuRunning = false;
// 显示当前用户信息
void showCurrentUserData();
// 接收线程
void readTaskHandler(int clientfd);
// 获取系统时间
std::string getCurrentTime();
// 主聊天界面
void mainMenu(int clientfd);
// 帮助界面
void help(int clientfd = 0, std::string s = "");
void chat(int, std::string);
void addfriend(int, std::string);
void creategroup(int, std::string);
void addgroup(int, std::string);
void groupchat(int, std::string);
void loginout(int clientfd = 0, std::string s = "");

std::unordered_map<std::string, std::string> commandMap = {
    {"help", "显示所有支持的命令,格式help"},
    {"chat", "一对一聊天,格式chat:friendid:message"},
    {"addfriend", "添加好友,格式addfriend:friendid"},
    {"creategroup", "创建群组,格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组,格式addgroup:groupid"},
    {"groupchat", "群聊,格式groupchat:groupid:message"},
    {"loginout", "注销,格式loginout"}};

std::unordered_map<std::string, std::function<void(int, std::string)>> commandHandlerMap =
    {
        {"help", help},
        {"chat", chat},
        {"addfriend", addfriend},
        {"creategroup", creategroup},
        {"addgroup", addgroup},
        {"groupchat", groupchat},
        {"loginout", loginout}};

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "command invalid!" << std::endl;
        exit(-1);
    }
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    int clientfd = socket(PF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        std::cerr << "socket error" << std::endl;
        exit(-1);
    }

    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    if (-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        std::cerr << "connect error" << std::endl;
        close(clientfd);
        exit(-1);
    }
    // 初始化信号量
    sem_init(&rwsem, 0, 0);
    // 启动子线程
    std::thread readTask(readTaskHandler, clientfd);
    readTask.detach();

    while (true)
    {
        std::cout << "====================" << std::endl;
        std::cout << "1. login" << std::endl;
        std::cout << "2. register" << std::endl;
        std::cout << "3. quit" << std::endl;
        std::cout << "====================" << std::endl;
        std::cout << "choice:" << std::endl;
        int choice = 0;
        std::cin >> choice;
        std::cin.get();
        switch (choice)
        {
        case 1:
        {
            int id = 0;
            char pwd[50] = {0};
            std::cout << "userid:";
            std::cin >> id;
            std::cin.get();
            std::cout << "userpassword:";
            std::cin.getline(pwd, 50);

            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            std::string request = js.dump();
            const char* ch = request.c_str();
            g_isLoginSuccess = false;

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                std::cerr << "send error" << request << std::endl;
            }
            // 等待读线程消息
            sem_wait(&rwsem);
            if (g_isLoginSuccess)
            {
                isMainMenuRunning = true;
                mainMenu(clientfd);
            }
        }
        break;
        case 2:
        {
            char name[50] = {0};
            char pwd[50] = {0};
            std::cout << "username:";
            std::cin.getline(name, 50);
            std::cout << "userpassword:";
            std::cin.getline(pwd, 50);

            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            std::string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                std::cerr << "send error" << request << std::endl;
            }
            sem_wait(&rwsem);
        }
        break;
        case 3:
            close(clientfd);
            sem_destroy(&rwsem);
            exit(0);
            break;
        default:
            std::cerr << "invalid input" << std::endl;
            break;
        }
    }
    return 0;
}

void doRegResponse(json &responsejs)
{
    if (0 != responsejs["errno"].get<int>())
    {
        std::cerr  << "name is already exist,register error!" << std::endl;
    }
    else
    {
        std::cout << "name register success,userid is " << responsejs["id"]
                  << ",do not forget it!" << std::endl;
    }
}

void doLoginResponse(json &responsejs)
{
    if (0 != responsejs["errno"].get<int>())
    {
        std::cerr << "login fail!" << std::endl
                  << responsejs["errmsg"] << std::endl;
        g_isLoginSuccess = false;
    }
    else
    {
        g_currentUserFriendList.clear();
        g_currentUserGroupList.clear();
        g_currentUser.setId(responsejs["id"].get<int>());
        g_currentUser.setName(responsejs["name"]);
        // 查询好友
        if (responsejs.contains("friends"))
        {
            std::vector<std::string> vec = responsejs["friends"];
            for (std::string &str : vec)
            {
                json js = json::parse(str);
                User user;
                user.setId(js["id"].get<int>());
                user.setName(js["name"]);
                user.setState(js["state"]);
                g_currentUserFriendList.push_back(user);
            }
        }
        // 查询群组
        if (responsejs.contains("groups"))
        {
            std::vector<std::string> vec1 = responsejs["groups"];
            for (std::string &groupstr : vec1)
            {
                json grpjs = json::parse(groupstr);
                Group group;
                group.setId(grpjs["id"].get<int>());
                group.setName(grpjs["groupname"]);
                group.setDesc(grpjs["groupdesc"]);

                std::vector<std::string> vec2 = grpjs["users"];
                for (std::string &userstr : vec2)
                {
                    GroupUser user;
                    json js = json::parse(userstr);
                    user.setId(js["id"].get<int>());
                    user.setName(js["name"]);
                    user.setState(js["state"]);
                    user.setRole(js["role"]);
                    group.getUsers().push_back(user);
                }
                g_currentUserGroupList.push_back(group);
            }
        }
        // 显示用户信息
        showCurrentUserData();
        // 接收离线消息
        if (responsejs.contains("offlinemsg"))
        {
            std::vector<std::string> vec = responsejs["offlinemsg"];
            for (std::string &str : vec)
            {
                json js = json::parse(str);
                int msgtype = js["msgid"].get<int>();
                if (ONE_CHAT_MSG == msgtype)
                {
                    std::cout << js["time"].get<std::string>() << "[" << js["id"] << "]"
                              << js["name"].get<std::string>() << "said: "
                              << js["msg"].get<std::string>() << std::endl;
                }
                else if (GROUP_CHAT_MSG == msgtype)
                {
                    std::cout << "群消息[" << js["groupid"] << "]"
                              << js["time"].get<std::string>() << "[" << js["id"] << "]"
                              << js["name"].get<std::string>() << "said: "
                              << js["msg"].get<std::string>() << std::endl;
                }
            }
        }
        g_isLoginSuccess = true;
    }
}

void showCurrentUserData()
{
    std::cout << "====================login user ====================" << std::endl;
    std::cout << "current login user => id:" << g_currentUser.getId()
              << " name:" << g_currentUser.getName() << std::endl;
    std::cout << "--------------------friend list --------------------" << std::endl;
    if (!g_currentUserFriendList.empty())
    {
        for (User &user : g_currentUserFriendList)
        {
            std::cout << user.getId() << " " << user.getName() << " "
                      << user.getState() << std::endl;
        }
    }
    std::cout << "--------------------group list --------------------" << std::endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            std::cout << group.getId() << " " << group.getName() << " "
                      << group.getDesc() << std::endl;
            for (GroupUser &user : group.getUsers())
            {
                std::cout << user.getId() << " " << user.getName() << " "
                          << user.getState() << " " << user.getRole() << std::endl;
            }
        }
    }
    std::cout << "====================================" << std::endl;
}

std::string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}

void readTaskHandler(int clientfd)
{
    while (true)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);
        if (-1 == len || 0 == len)
        {
            close(clientfd);
            exit(-1);
        }

        // 接收服务器转发的数据
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        if (ONE_CHAT_MSG == msgtype)
        {
            std::cout << js["time"].get<std::string>() << "[" << js["id"] << "]"
                      << js["name"].get<std::string>() << "said: "
                      << js["msg"].get<std::string>() << std::endl;
            continue;
        }

        if (GROUP_CHAT_MSG == msgtype)
        {
            std::cout << "群消息[" << js["groupid"] << "]"
                      << js["time"].get<std::string>() << "[" << js["id"] << "]"
                      << js["name"].get<std::string>() << "said: "
                      << js["msg"].get<std::string>() << std::endl;
            continue;
        }
        if (LOGIN_MSG_ACK == msgtype)
        {
            doLoginResponse(js); // 处理登陆响应
            sem_post(&rwsem);
            continue;
        }
        if (REG_MSG_ACK == msgtype)
        {
            doRegResponse(js); // 处理登陆响应
            sem_post(&rwsem);
            continue;
        }
    }
}

void mainMenu(int clientfd)
{
    // help();
    char buffer[1024] = {0};
    while (isMainMenuRunning)
    {
        std::cin.getline(buffer, 1024);
        std::string commandbuf(buffer);
        std::string command;
        int idx = commandbuf.find(":");
        if (-1 == idx)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx);
        }
        auto it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end())
        {
            std::cerr << "invalid input command!" << std::endl;
            continue;
        }
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx));
    }
}

void help(int, std::string)
{
    std::cout << "show command list >>>" << std::endl;
    for (auto &p : commandMap)
    {
        std::cout << p.first << ":" << p.second << std::endl;
    }
    std::cout << std::endl;
}

void chat(int clientfd, std::string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        std::cerr << "chat command invalid!" << std::endl;
        return;
    }
    int friendid = atoi(str.substr(0, idx).c_str());
    std::string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    std::string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send chat msg error ->" << buffer << std::endl;
    }
}

void addfriend(int clientfd, std::string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    std::string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send addfriend msg error ->" << buffer << std::endl;
    }
}

void creategroup(int clientfd, std::string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        std::cerr << "creategroup command invalid!" << std::endl;
        return;
    }

    std::string groupname = str.substr(0, idx);
    std::string groupdesc = str.substr(idx + 1, str.size() - 1);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    std::string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send creategroup msg error ->" << buffer << std::endl;
    }
}

void addgroup(int clientfd, std::string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    std::string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send addfriend msg error ->" << buffer << std::endl;
    }
}

void groupchat(int clientfd, std::string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        std::cerr << "chat command invalid!" << std::endl;
        return;
    }
    int groupid = atoi(str.substr(0, idx).c_str());
    std::string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    std::string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send chat msg error ->" << buffer << std::endl;
    }
}

void loginout(int clientfd, std::string)
{
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();
    std::string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send addfriend msg error ->" << buffer << std::endl;
    }
    else
    {
        isMainMenuRunning = false;
    }
}