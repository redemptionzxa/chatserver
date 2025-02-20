#include "connectionPool.hpp"
#include "public.hpp"

#include <iostream>

ConnectionPool::ConnectionPool()
{
	// if (!loadConfigFile())
	// {
	// 	cerr << "数据库配置信息有误" <<endl;
	// 	return;
	// }
	//创建初始连接
	_initSize = 10;
	_maxSize = 1024;
	_maxIdleTime = 60;
	_connectionTimeout = 100;
	for (int i = 0; i < _initSize; ++i)
	{
		MySQL* p = new MySQL();
		if(!p->connect())
		{
			cerr << "数据库连接有误" << endl;
		}
		_connectionQue.push(p);
		++_connectionCnt;
	}
	//创建生产者线程
	thread produce(std::bind(&ConnectionPool::produceConnectionTask,this));
	produce.detach();
	//启动定时线程
	thread scanner(std::bind(&ConnectionPool::scannerConnectionTask, this));
	scanner.detach();
}

ConnectionPool::~ConnectionPool()
{
	while (!_connectionQue.empty())
	{
		MySQL* p = _connectionQue.front();
		delete p;
		_connectionQue.pop();
	}
}

shared_ptr<MySQL> ConnectionPool::getConnection()
{
	unique_lock<mutex> lock(_queueMutex);
	while (_connectionQue.empty())
	{
		if (cv_status::timeout == cv.wait_for(lock, chrono::milliseconds(_connectionTimeout)))
		{
			cerr << "获取连接超时" << endl;
			return nullptr;
		}
	}
	shared_ptr<MySQL> sp(_connectionQue.front(),
		[&](MySQL* pcon) {
			unique_lock<mutex> lock(_queueMutex);
			pcon->setAliveTime();
			_connectionQue.push(pcon);
		});
	_connectionQue.pop();
	cv.notify_all(); 
	return sp;
}

ConnectionPool* ConnectionPool::getInstance()
{
	static ConnectionPool pool;
	return &pool;
}

void ConnectionPool::scannerConnectionTask()
{
	while (1)
	{
		this_thread::sleep_for(chrono::seconds(_maxIdleTime));
		unique_lock<mutex> lock(_queueMutex);
		while (_connectionCnt>_initSize)
		{
			MySQL* p = _connectionQue.front();
			if (p->getAliceTime() >= _maxIdleTime * 1000)
			{
				_connectionQue.pop();
				delete p;
			}
			else
			{
				break;
			}
		}
	}
}

void ConnectionPool::produceConnectionTask()
{
	while (1)
	{
		{
			unique_lock<mutex> lock(_queueMutex);
			while (!_connectionQue.empty() || _connectionCnt >= _maxSize)
			{
				cv.wait(lock);
			}
			MySQL* p = new MySQL();
			p->connect();
			p->setAliveTime();
			_connectionQue.push(p);
			++_connectionCnt;
			cv.notify_all();
		}
	}
}

bool ConnectionPool::loadConfigFile()
{
	FILE* pf = fopen("mysql.ini", "r");
	if (pf == nullptr)
	{
		cerr << "mysql.ini file is not exist!" << endl;
		return false;
	}
	while (!feof(pf))
	{
		char line[1024] = { 0 };
		fgets(line, 1024, pf);
		string str = line;
		int idx = str.find('=', 0);
		if (idx == -1)
		{
			continue;
		}
		int endidx = str.find('\n', idx);
		string key = str.substr(0, idx);
		string value = str.substr(idx + 1, endidx - idx - 1);
		if (key == "ip")
		{
			_ip = value;
		}
		else if (key == "port")
		{
			_port = atoi(value.c_str());
		}
		else if (key == "username")
		{
			_username = value;
		}
		else if (key == "password")
		{
			_password = value;
		}
		else if (key == "dbname")
		{
			_dbname = value;
		}
		else if (key == "initSize")
		{
			_initSize = atoi(value.c_str());
		}
		else if (key == "maxSize")
		{
			_maxSize = atoi(value.c_str());
		}
		else if (key == "maxIdleTime")
		{
			_maxIdleTime = atoi(value.c_str());
		}
		else if (key == "connectionTimeout")
		{
			_connectionTimeout = atoi(value.c_str());
		}
	}
	return true;
}
