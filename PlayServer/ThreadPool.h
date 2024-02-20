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
	//ÿ���߳�ֻ��һ��
	//����static��ÿ���̻߳��ж��
	//����thread_local,���е��߳�ֻ��һ��
	//��Ҳ����ζ����ͬһ���߳�����������ʹ��ͬһ���ͻ���
	//�����Ϳ����ڶ��̵߳Ļ�������threadpool��
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
	//����������߼�����
	//���������󶨵�base������
	//��Ϊ��ͬһ�����̣����Խ��������Ҳ�����̺߳�����ָ��ͨ�������׽��ַ��͵������
	//��ʱ�̳߳��е��̶߳���wait�����׽���epollin�¼��������󣬱��������߳̾ͻᱻ��������������
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