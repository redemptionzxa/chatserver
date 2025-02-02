#include "chatserver.hpp"
#include "chatservice.hpp"
#include <iostream>
#include <signal.h>

void resetHandler(int)
{
    ChatService::instance()->reset();
    exit(0);
}


int main(int argc,char** argv)
{
    if(argc < 3)
    {
        std::cerr << "command invalid!" << std::endl;
        exit(-1);
    }
    char* ip = argv[1];
    uint16_t port = atoi(argv[2]);

    signal(SIGINT,resetHandler);
    //初始化
    EventLoop loop;
    InetAddress addr(ip,port);
    ChatServer server(&loop,addr,"ChatServer");
    //启动
    server.start();
    loop.loop();
}