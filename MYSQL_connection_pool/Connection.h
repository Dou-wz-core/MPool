#pragma once
#include <iostream>
#include <mysql.h>
#include <string>
#include<ctime>
#include "log.h"
using namespace std;

// 实现MYSQL数据库的操作

class Connection
{
public:
	// 初始化数据库连接
	Connection();
	// 关闭数据库连接
	~Connection();

	// 连接数据库
	bool connect(string ip, unsigned short port, string username, string password, string dbname);
	// 更新数据库
	bool update(string sql);
	// 查询数据库
	MYSQL_RES* query(string sql);

	// 获取空闲时间
	int getAliveTime()const;

	// 刷新空闲时间
	void refreshAliveTime();

private:
	// 数据库连接对象
	MYSQL* _conn;
	clock_t _aliveTime;	// 记录连接进入空闲状态后的起始存活时间
};
