#include "Connection.h"

// 初始化数据库连接
Connection::Connection()
{
	_conn = mysql_init(nullptr);
}
// 关闭数据库连接
Connection::~Connection()
{
	if (_conn != nullptr)
	{
		mysql_close(_conn);
	}
}

// 连接数据库
bool Connection::connect(string ip, unsigned short port, string username, 
						 string password, string dbname)
{
	MYSQL* p = mysql_real_connect(_conn, ip.c_str(), username.c_str(),
								  password.c_str(), dbname.c_str(), port, nullptr, 0);
	if (p == nullptr)
	{
		LOG("连接失败：" + string(mysql_error(_conn)));
		return false;
	}
	return p != nullptr;
}
// 更新数据库 insert、delete、update
bool Connection::update(string sql)
{
	if (mysql_query(_conn, sql.c_str()) != 0)
	{
		LOG("更新失败：" + sql + "；MySQL错误：" + mysql_error(_conn));
		return false;
	}
	return true;
}
// 查询数据库
MYSQL_RES* Connection::query(string sql)
{
	if (mysql_query(_conn, sql.c_str()) != 0)
	{
		LOG("查询失败：" + sql);
		return nullptr;
	}
	return mysql_store_result(_conn);
}

// 获取空闲时间
int Connection::getAliveTime()const { return clock() - _aliveTime; }

// 刷新空闲时间
void Connection::refreshAliveTime() { _aliveTime = clock(); }