#define _CRT_SECURE_NO_WARNINGS
#include<iostream>
#include<stdio.h>
#include<Windows.h>
#include"connection.h"
#include"Common_ConnectionPool.h"
using  namespace std;

// 清空user表
bool clearUserTable() {
	Connection clearConn;
	if (!clearConn.connect("127.0.0.1", 3306, "root", "YOUR_MYSQL_PASSWORD", "chat")
		|| !clearConn.update("DELETE FROM user"))
	{
		return 1;
	}
	return 0;
}

int main() {
	SetConsoleOutputCP(CP_UTF8);
	/*
	Connection conn;
	char sql[1024] = { 0 };
	sprintf(sql, "insert into user(name,age,sex) values('%s',%d ,'%s')",
		"张三", 20, "male");
	if (!conn.connect("127.0.0.1", 3306, "root", "YOUR_MYSQL_PASSWORD", "chat"))
		return 1;
	if (!conn.update(sql))
		return 1;
	*/

	//ConnectionPool* cp = ConnectionPool::getConnectionPool();
	//cp->loadConfigFile();

	/*========压力测试========*/
	// 进程级初始化：必须早于创建工作线程和调用其他 MySQL API
	if (mysql_library_init(0, nullptr, nullptr) != 0)
	{
		// 记录初始化失败并退出
		return 1;
	}
	// 单线程
#if 0
	// 1.未使用连接池：1000：11200毫秒	5000：62000毫秒
	clock_t start = clock();
	for (int i = 0; i < 5000; i++)
	{
		Connection conn;
		char sql[1024] = { 0 };
		sprintf(sql, "insert into user(name,age,sex) values('%s',%d ,'%s')",
			"张三", 20, "male");
		conn.connect("127.0.0.1", 3306, "root", "YOUR_MYSQL_PASSWORD", "chat");
		conn.update(sql);
	}
	clock_t end = clock();
	cout << "单线程未使用连接池1000次插入操作耗时：" << (end - start) << "毫秒" << endl;

	// 清除第一轮测试插入的数据
	clearUserTable();

	// 2.使用连接池：1000：6200毫秒	6000：30000毫秒
	start = clock();  // 重新开始计时
	ConnectionPool* cp = ConnectionPool::getConnectionPool();	// 获取连接池对象
	for (int i = 0; i < 5000; i++)
	{
		shared_ptr<Connection> sp = cp->getConnection();
		char sql[1024] = { 0 };
		sprintf(sql, "insert into user(name,age,sex) values('%s',%d ,'%s')",
			"张三", 20, "male");
		sp->update(sql);
	}
	end = clock();
	cout << "单线程使用连接池1000次插入操作耗时：" << (end - start) << "毫秒" << endl;

	clearUserTable();
#endif

	// 四线程 
	// 不使用连接池：1000：3900		5000：18000		10000：37000毫秒
	{
		clock_t start = clock();
		thread t1([]() {
			for (int i = 0; i < 250; i++)
			{
				Connection conn;
				char sql[1024] = { 0 };
				sprintf(sql, "insert into user(name,age,sex) values('%s',%d ,'%s')",
					"张三", 20, "male");
				conn.connect("127.0.0.1", 3306, "root", "YOUR_MYSQL_PASSWORD", "chat");
				conn.update(sql);
			}
			});
		thread t2([]() {
			for (int i = 0; i < 250; i++)
			{
				Connection conn;
				char sql[1024] = { 0 };
				sprintf(sql, "insert into user(name,age,sex) values('%s',%d ,'%s')",
					"张三", 20, "male");
				conn.connect("127.0.0.1", 3306, "root", "YOUR_MYSQL_PASSWORD", "chat");
				conn.update(sql);
			}
			});
		thread t3([]() {
			for (int i = 0; i < 250; i++)
			{
				Connection conn;
				char sql[1024] = { 0 };
				sprintf(sql, "insert into user(name,age,sex) values('%s',%d ,'%s')",
					"张三", 20, "male");
				conn.connect("127.0.0.1", 3306, "root", "YOUR_MYSQL_PASSWORD", "chat");
				conn.update(sql);
			}
			});
		thread t4([]() {
			for (int i = 0; i < 250; i++)
			{
				Connection conn;
				char sql[1024] = { 0 };
				sprintf(sql, "insert into user(name,age,sex) values('%s',%d ,'%s')",
					"张三", 20, "male");
				conn.connect("127.0.0.1", 3306, "root", "YOUR_MYSQL_PASSWORD", "chat");
				conn.update(sql);
			}
			});

		t1.join();
		t2.join();
		t3.join();
		t4.join();

		clock_t end = clock();
		std::cout << "四线程未使用连接池1000次插入操作耗时：" << (end - start) << "毫秒" << endl;
		clearUserTable();
	}
	// 使用连接池：1000：2800毫秒    5000：13000毫秒	10000：25000毫秒
	{
		clock_t start = clock();
		ConnectionPool* cp = ConnectionPool::getConnectionPool();
		thread t1([&cp]() {
			for (int i = 0; i < 250; i++)
			{
				shared_ptr<Connection> sp = cp->getConnection();
				char sql[1024] = { 0 };
				sprintf(sql, "insert into user(name,age,sex) values('%s',%d ,'%s')",
					"张三", 20, "male");
				sp->update(sql);
			}
			});
		thread t2([&cp]() {
			for (int i = 0; i < 250; i++)
			{
				shared_ptr<Connection> sp = cp->getConnection();
				char sql[1024] = { 0 };
				sprintf(sql, "insert into user(name,age,sex) values('%s',%d ,'%s')",
					"张三", 20, "male");
				sp->update(sql);
			}
			});
		thread t3([&cp]() {
			for (int i = 0; i < 250; i++)
			{
				shared_ptr<Connection> sp = cp->getConnection();
				char sql[1024] = { 0 };
				sprintf(sql, "insert into user(name,age,sex) values('%s',%d ,'%s')",
					"张三", 20, "male");
				sp->update(sql);
			}
			});
		thread t4([&cp]() {
			for (int i = 0; i < 250; i++)
			{
				shared_ptr<Connection> sp = cp->getConnection();
				char sql[1024] = { 0 };
				sprintf(sql, "insert into user(name,age,sex) values('%s',%d ,'%s')",
					"张三", 20, "male");
				sp->update(sql);
			}
			});

		t1.join();
		t2.join();
		t3.join();
		t4.join();

		clock_t end = clock();
		std::cout << "四线程使用连接池1000次插入操作耗时：" << (end - start) << "毫秒" << endl;
		clearUserTable();
	}

	return 0;
}