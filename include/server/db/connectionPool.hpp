#ifndef CONNECTIONPOOL_H
#define CONNECTIONPOOL_H

#include <string>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <memory>
#include <functional>
#include <condition_variable>
#include <chrono>
#include "db.h"

using namespace std;
class ConnectionPool
{
public:
	//单例接口
	static ConnectionPool* getInstance();
	//获取空闲连接
	shared_ptr<MySQL> getConnection();
	//加载配置文件
	bool loadConfigFile();
private:
	ConnectionPool();
	~ConnectionPool();
	void produceConnectionTask();
	void scannerConnectionTask();
	string _ip;  //数据库ip
	unsigned short _port; //数据库端口号
	string _username;  //数据库用户名
	string _password; //数据库密码
	string _dbname;
	int _initSize; //初始连接量
	int _maxSize; //最大连接量
	int _maxIdleTime; //最大空闲时间
	int _connectionTimeout; //获取连接超时时间

	queue<MySQL*> _connectionQue; //连接队列(生产消费)
	mutex _queueMutex;
	atomic_int _connectionCnt; //所创建的连接数量
	condition_variable cv; // 生产消费模型

};

#endif