#pragma once
#include"Socket.h"
#include"ThreadPool.h"
#include"Process.h"
#include"Loggere.h"


class CBusiness
{
public:
	virtual int CBusinessProcess() = 0;
	template<typename _FUNCTION_, typename ..._ARGS_>
	int SetConnectedcallback(_FUNCTION_ func, _ARGS_... args)
	{
		m_connnectedcallback = new CFunction<_FUNCTION_, _ARGS_...>(func, args...);
		if (m_connnectedcallback == NULL) return -1;
		return 0;
			
	}


	template<typename _FUNCTION_, typename ..._ARGS_>
	int SetRecvcallback(_FUNCTION_ func, _ARGS_... args)
	{
		m_recvcallback = new CFunction<_FUNCTION_, _ARGS_...>(func, args...);
		if (m_recvcallback == NULL) return -1;
		return 0;

	}
private:
	CFunctionBase* m_connnectedcallback;
	CFunctionBase* m_recvcallback;
};



class CServer
{
public:
	CServer();
	virtual ~CServer()
	{
		Close();
	}
	CServer(const CServer& ) = delete;
	CServer& operator=(const CServer&) = delete;

	int Init(CBusiness* business,const Buffer& ip = "127.0.0.1", short port = 9999);
	int Run();
	int Close();
private:
	int ThreadFunc();

private:
	CThreadPool m_pool;
	CSocketBase* m_server;
	CEpoll m_epoll;
	CProcess m_process;
	//业务模块需要我们手动去释放它
	CBusiness* m_business;
};

