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
private:
	int TaskDispatch();
private:
	CEpoll m_epoll;
	std::vector<CThread*> m_threads;
	CSocketBase* m_server;
	Buffer m_path;
};

