#include"Common_ConnectionPool.h"

// 线程安全的懒汉式单例模式
ConnectionPool* ConnectionPool::getConnectionPool()
{
	static ConnectionPool pool;
	return &pool;
}

// 外部接口，获取一个可用的数据库连接
shared_ptr<Connection> ConnectionPool::getConnection()
{
	unique_lock<mutex> lock(_queueMutex);
	while (_connectionQue.empty())
	{
		// 等待生产者线程生产新连接，等待时间为_connectionTimeout毫秒
		if (!_cv.wait_for(lock, chrono::milliseconds(_connectionTimeout),
			[this] {return !_connectionQue.empty(); }))
		{
			if (_connectionQue.empty())
			{
				LOG("获取数据库连接超时");
				return nullptr;
			}
		}
	}
	// 如果连接池队列不为空，返回一个连接
	if (!_connectionQue.empty())
	{
		// 这里使用lambda表达式自定义了shared_ptr的删除器，目的是让连接归还给连接池，而不是释放掉
		shared_ptr<Connection> p(_connectionQue.front(), 
			[this](Connection* pcon) {
				// 这里是在服务器线程中调用的，一定要用锁保证队列线程安全
				unique_lock<mutex> lock2(_queueMutex);
				pcon->refreshAliveTime();
				_connectionQue.push(pcon);
				_cv.notify_all();	// 通知消费者线程可以消费连接了
			});
		_connectionQue.pop();
		if(_connectionQue.empty())
			_cv.notify_all();	// 谁消费了最后一个连接，通知生产者线程可以生产新连接了
		return p;
	}
	return nullptr;
}

// 私有化构造函数，用于初始化连接池
ConnectionPool::ConnectionPool()
	:_connectionCnt(0)
{
	// 1. 加载配置，失败则终止构造
	if (!loadConfigFile())
	{
		throw std::runtime_error("加载连接池配置失败");
	}
	/*
	// 创建初始的连接
	for (int i = 0; i < _initSize; ++i)
	{
		Connection* p = new Connection();
		p->connect(_ip, _port, _username, _password, _dbname);
		p->refreshAliveTime();
		_connectionQue.push(p);
		++_connectionCnt;
	}
	*/
	// 2. 检查初始连接数量
	if (_initSize <= 0 || _maxSize < _initSize)
	{
		throw std::runtime_error(
			"连接数量配置错误：要求 0 < initSize <= maxSize");
	}

	// 若连续失败达到上限时，放弃初始化
	const int maxConsecutiveFailures = 5;	// 最大连续失败次数
	int consecutiveFailures = 0;			// 当前连续失败次数

	try
	{
		// 创建初始连接，只有成功入队才增加计数
		while (_connectionCnt < _initSize)
		{
			std::unique_ptr<Connection> conn(new Connection());
			// 连接失败
			if (!conn->connect(		
				_ip, _port, _username, _password, _dbname))
			{
				// 释放本次失败的连接，之前成功的连接继续保留
				conn.reset();
				++consecutiveFailures;
				if (consecutiveFailures >= maxConsecutiveFailures)
				{
					throw std::runtime_error(
						"连续连接失败次数达到上限，连接池初始化失败");
				}

				// 等待后重试，避免连续快速建连
				std::this_thread::sleep_for(std::chrono::milliseconds(200));		
				continue;
			}

			// 连接成功，记录进入空闲状态的时间
			conn->refreshAliveTime();

			// 先入队：如果 push 抛异常，unique_ptr 会自动释放连接
			_connectionQue.push(conn.get());
			// 入队成功后，将连接的管理责任交给连接池
			conn.release();

			++_connectionCnt;
			consecutiveFailures = 0;
		}
	}
	catch (...)
	{
		// 最终初始化失败：清理此前已经成功创建的连接
		while (!_connectionQue.empty())
		{
			Connection* conn = _connectionQue.front();
			_connectionQue.pop();
			delete conn;
		}
		_connectionCnt = 0;

		// 将原异常继续传给调用方
		throw;
	}

	/* jthread ：C++20中的线程类型，会自动管理线程的生命周期，析构时自动join */

	// 启动一个新的线程，作为连接的生产者
	thread produce(std::bind(&ConnectionPool::produceConnectionTask, this));
	// 也可以使用lambda表达式
	// thread produce([this]() { this->produceConnectionTask(); });
	produce.detach();	// 让生产者线程与主线程分离，后台运行。jthread不需要

	// 启动一个新的定时线程，作为连接的扫描者，扫描超过最大空闲时间的连接并回收
	thread scanner(std::bind(&ConnectionPool::scannerConnectionTask, this));		// thread scanner([this]() { this->scannerConnectionTask(); });
	scanner.detach();	// 让扫描者线程与主线程分离，后台运行。jthread不需要

}

// 加载配置文件
bool ConnectionPool::loadConfigFile()
{
	FILE* pf = fopen("mysql.ini", "r");
	if (pf == nullptr)
	{
		LOG("mysql.ini配置文件不存在");
		return false;
	}
	
	while (!feof(pf))
	{
		char line[1024] = { 0 };
		fgets(line, 1024, pf);
		string str = line;
		int idx = str.find('=', 0);			// 查找等号
		if (idx == -1) 
			continue;
		int endidx = str.find('\n', idx);	// 查找换行符
		string key = str.substr(0, idx);
		string value = str.substr(idx + 1, endidx - idx - 1);

		cout << "配置文件读取：" << key << "=" << value << endl;
		if (key == "ip")
			_ip = value;
		else if (key == "port")
			_port = atoi(value.c_str());
		else if (key == "user")
			_username = value;
		else if (key == "password")
			_password = value;
		else if (key == "dbname")
			_dbname = value;
		else if (key == "initSize")
			_initSize = atoi(value.c_str());
		else if (key == "maxSize")
			_maxSize = atoi(value.c_str());
		else if (key == "maxIdleTime")
			_maxIdleTime = atoi(value.c_str());
		else if (key == "maxConnectionTimeOut")
			_connectionTimeout = atoi(value.c_str());
	}
	return true;
}

// 生产新连接
/*
void ConnectionPool::produceConnectionTask()
{
	for (;;)
	{
		unique_lock<mutex> lock(_queueMutex);
		while (!_connectionQue.empty())
		{
			// 队列不空，等待消费者消费连接
			_cv.wait(lock);
		}
		// 队列空了，且连接数小于最大连接数，生产新连接
		if (_connectionCnt < _maxSize)
		{
			std::unique_ptr<Connection> p(new Connection());

			if (p->connect(_ip, _port, _username, _password, _dbname))
			{
				// 数据库连接成功，才能入队并计数
				p->refreshAliveTime();
				_connectionQue.push(p.get());
				p.release();
				++_connectionCnt;
			}
			else
			{
				LOG("连接数据库失败");
				continue;
				// 离开作用域时，unique_ptr 自动释放失败的连接
			}
		}
		else
			LOG("连接池已满，无法创建新连接");

		// 通知消费者线程可以消费连接了
		_cv.notify_all();
	}
}
*/
void ConnectionPool::produceConnectionTask()
{
	// 生产连接失败时，等待200毫秒后重试
	const auto retryDelay = std::chrono::milliseconds(200);
	for (;;)
	{
		try
		{
			std::unique_lock<std::mutex> lock(_queueMutex);

			// 只有“空闲队列为空，并且连接数未达到上限”
			// 才继续创建连接；否则释放锁并等待通知
			_cv.wait(lock, [this]() {
				return _connectionQue.empty()
					&& _connectionCnt < _maxSize;
				});

			// 队列为空时批量生产，且不超过连接池最大容量
			const int availableCapacity = _maxSize - _connectionCnt;
			const int batchSize = availableCapacity < 5 ? availableCapacity : 5;
			bool createdConnection = false;
			bool connectionFailed = false;
			for (int i = 0; i < batchSize; ++i)
			{
				std::unique_ptr<Connection> conn(new Connection());
				if (!conn->connect(
					_ip, _port, _username, _password, _dbname))
				{
					// 连接失败时不入队、不增加计数，稍后重试
					connectionFailed = true;
					break;
				}

				conn->refreshAliveTime();
				// push 失败时，conn 仍负责自动释放连接
				_connectionQue.push(conn.get());	// 先入队，避免 push 抛异常时泄漏
				conn.release();		// 入队成功后，连接的管理责任交给连接池
				++_connectionCnt;
				createdConnection = true;
			}

			// 释放锁后通知消费者，避免消费者在锁上等待
			lock.unlock();
			if (createdConnection)
				_cv.notify_all();
			if (connectionFailed)
			{
				LOG("生产连接失败，200毫秒后重试");
				std::this_thread::sleep_for(retryDelay);
			}
		}
		catch (const std::exception& e)
		{
			// 异常离开 try 时，局部智能指针和锁会自动清理
			// 后台线程内部处理异常，避免异常逃出线程入口
			LOG(std::string("生产连接发生异常：") + e.what());
			_cv.notify_all();
			std::this_thread::sleep_for(retryDelay);
		}
	}
}

// 扫描超过最大空闲时间的连接并回收
/*
void ConnectionPool::scannerConnectionTask()
{
	for (;;)
	{
		// 通过sleep模拟定时效果
		this_thread::sleep_for(chrono::seconds(_maxIdleTime));
		// 扫描整个连接池，释放多余的空闲连接
		unique_lock<mutex> lock(_queueMutex);
		while (!_connectionQue.empty() && _connectionCnt > _initSize)
		{
			Connection* p = _connectionQue.front();
			if (p->getAliveTime() >= (_maxIdleTime * 1000))
			{
				_connectionQue.pop();
				delete p;	// 调用~Connection，释放连接资源
				--_connectionCnt;
			}
			else
				break;		// 队头连接没有超过最大空闲时间，后面的连接也不会超过
		}
	}
}
*/
void ConnectionPool::scannerConnectionTask()
{
	for (;;)
	{
		// 定期扫描空闲连接
		std::this_thread::sleep_for(
			std::chrono::seconds(_maxIdleTime));

		for (;;)
		{
			std::unique_lock<std::mutex> lock(_queueMutex);

			// 队列为空，或者连接数已达到保留数量
			if (_connectionQue.empty()
				|| _connectionCnt <= _initSize)
			{
				break; // lock 自动析构并解锁
			}

			Connection* conn = _connectionQue.front();
			// 队头尚未过期，本轮停止扫描
			if (conn->getAliveTime() < _maxIdleTime * 1000)
			{
				break; // lock 自动析构并解锁
			}

			// 接管过期连接，并将其移出空闲队列
			std::unique_ptr<Connection> expired(conn);
			_connectionQue.pop();

			// 释放队列锁，在锁外销毁连接
			lock.unlock();
			expired.reset();

			// 真正关闭后，再加锁更新连接总数
			lock.lock();
			--_connectionCnt;
			lock.unlock();

			// 通知生产线程：连接容量已经释放
			_cv.notify_all();
		}
	}
}