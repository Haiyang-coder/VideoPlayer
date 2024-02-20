#pragma once
#include"Epoll.h"
#include "Thread.h"
#include "Function.h"
#include"Socket.h"
class CThreadPool
{
public:
	CThreadPool();
	~CThreadPool() {
		Close();
	}
	CThreadPool(const CThreadPool&) = delete;
	CThreadPool& operator=(const CThreadPool&) = delete;
public:
	int Start(unsigned count);
	void Close();
	template<typename _FUNCTION_, typename...
		_ARGS_>
	int AddTask(_FUNCTION_ func, _ARGS_... args);
	size_t Size() { return m_threads.size(); }
private:
	int TaskDispatch();
private:
	CEpoll m_epoll;
	std::vector<CThread*> m_threads;
	CSocketBase* m_server;
	Buffer m_path;
};

template<typename _FUNCTION_, typename ..._ARGS_>
inline int CThreadPool::AddTask(_FUNCTION_ func, _ARGS_ ...args)
{
	//每隔线程只有一个
	//不加static，每个线程会有多个
	//不加thread_local,所有的线程只有一个
	//这也就意味着在同一个线程里面多个对象使用同一个客户端
	//这样就可以在多线程的环境下用threadpool了
	static thread_local CSocket client;
	int ret = 0;
	if (client == -1)
	{
		ret = client.Init(CSockParam(m_path, 0));
		if (ret < 0)
		{
			return -1;
		}
		ret = client.Link();
		if (ret < 0)
		{
			return -2;
		}
	}
	//添加任务的逻辑就是
	//把任务函数绑定到base对象里
	//因为是同一个进程，所以将任务对象：也就是线程函数的指针通过本地套接字发送到服务端
	//此时线程池中的线程都在wait，当套接字epollin事件被触发后，被触发的线程就会被唤醒来处理任务
	CFunctionBase* base = new CFunction<_FUNCTION_, _ARGS_ ...>(func, args...);
	if (base == NULL)
	{
		return -3;
	}
	Buffer data(sizeof(base));
	memcpy(data, &base, sizeof(base));
	ret = client.Send(data);
	if (ret < 0)
	{
		delete base;
		return -4;
	}
	return 0;

}
