#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include<string>
#include<queue>
#include<mutex>
#include<iostream>
#include<atomic>
#include<memory>
#include<thread>
#include<functional>
#include<condition_variable>
#include<stdexcept>
#include"Connection.h"
#include"log.h"
using namespace std;

// 连接池功能模块
class ConnectionPool {

public:
	// 获取连接池对象
	static ConnectionPool* getConnectionPool();

	//外部接口，获取一个可用的数据库连接
	shared_ptr<Connection> getConnection();	

private:
	// 私有化构造函数：单例
	ConnectionPool();	

	// 加载配置文件
	bool loadConfigFile();	

	// 运行在独立的线程中，专门负责 生产新连接
	void produceConnectionTask();		

	// 运行在独立的线程中，专门负责 扫描超过最大空闲时间的连接并回收
	void scannerConnectionTask();	

	/* 数据库连接池的配置信息 */
	string		   _ip;					// 数据库IP地址
	unsigned short _port;				// 端口号
	string		   _username;			// 登录用户名
	string	       _password;			// 数据库密码
	string		   _dbname;				// 数据库名称
	int			   _initSize;			// 连接池初始连接量
	int			   _maxSize;			// 连接池最大连接量
	int			   _maxIdleTime;		// 连接池最大空闲时间
	int			   _connectionTimeout;	// 连接池获取连接的超时时间

	/* 连接池内部数据结构 */
	queue<Connection*> _connectionQue;	// 连接池队列
	mutex			   _queueMutex;		// 维护连接池队列线程安全的互斥锁
	atomic_int		   _connectionCnt;	// 记录连接池总的连接数量
	condition_variable _cv;				// 线程同步条件变量,连接生产者和消费者的通信
};