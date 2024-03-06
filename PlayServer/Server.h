#pragma once
#include"Socket.h"
#include"ThreadPool.h"
#include"Process.h"
#include"Loggere.h"






template<typename _FUNCTION, typename... _ARGS>
class CConnetFunction : public CFunctionBase
{
public:

	CConnetFunction(_FUNCTION func, _ARGS... args)
		:m_binder(std::forward<_FUNCTION>(func), std::forward<_ARGS>(args)...)
	{

	}
	typename std::_Bindres_helper<int, _FUNCTION, _ARGS...>::type m_binder;
	virtual ~CConnetFunction() {}

	virtual int operator()(CSocketBase* pClient)
	{
		return m_binder(pClient);
	}

};



template<typename _FUNCTION, typename... _ARGS>
class CRecvFunction : public CFunctionBase
{
public:

	CRecvFunction(_FUNCTION func, _ARGS... args)
		:m_binder(std::forward<_FUNCTION>(func), std::forward<_ARGS>(args)...)
	{

	}
	
	virtual ~CRecvFunction() {}
	virtual int operator()(CSocketBase* pClient, const Buffer& data)
	{
		return m_binder(pClient, data);
	}
	typename std::_Bindres_helper<int, _FUNCTION, _ARGS...>::type m_binder;

};

class CBusiness
{
public:
	CBusiness()
		:m_recvcallback(NULL),
		m_connnectedcallback(NULL)
	{
		
	}
	virtual int CBusinessProcess(CProcess* proc) = 0;
	template<typename _FUNCTION_, typename ..._ARGS_>
	int SetConnectedcallback(_FUNCTION_ func, _ARGS_... args)
	{
		m_connnectedcallback = new CConnetFunction<_FUNCTION_, _ARGS_...>(func, args...);
		if (m_connnectedcallback == NULL) return -1;
		return 0;
			
	}

	template<typename _FUNCTION_, typename ..._ARGS_>
	int SetRecvcallback(_FUNCTION_ func, _ARGS_... args)
	{
		m_recvcallback = new CRecvFunction<_FUNCTION_, _ARGS_...>(func, args...);
		if (m_recvcallback == NULL) return -1;
		return 0;

	}


	//template<typename _FUNCTION_, typename ..._ARGS_>
	//int SetRecvcallback(_FUNCTION_ func, _ARGS_... args)
	//{
	//	m_connnectedcallback = new CConnetFunction<_FUNCTION_, _ARGS_...>(func, args...);
	//	//m_connnectedcallback = new CConnetFunction<_FUNCTION_, _ARGS_...>(func, args...);
	//	if (m_recvcallback == NULL) return -1;
	//	return 0;
	//}
protected:

	CFunctionBase* m_connnectedcallback;
	CFunctionBase* m_recvcallback;
	unsigned m_count;
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

