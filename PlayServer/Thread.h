#pragma once
#include<pthread.h>
#include<unistd.h>
#include<fcntl.h>
#include"Function.h"
#include<signal.h>
#include<map>

class CThread
{
public:
	CThread()
	{
		m_pfunction = NULL;
		m_thread = 0;
		m_bpause = false;
	}
	template<typename _FUNCTION_, typename... _ARGS_>
	CThread(_FUNCTION_ func, _ARGS_... args):m_pfunction(new CFunction<_FUNCTION_, _ARGS_...>(func, args...))
	{
		
	}
	~CThread();

	CThread(const CThread&) = delete;
	CThread& operator= (const CThread&) = delete;
public:
	template<typename _FUNCTION_, typename... _ARGS_>
	int SetThreadFunc(_FUNCTION_ func, _ARGS_... args)
	{
		m_pfunction = new CFunction<_FUNCTION_, _ARGS_...>(func, args...);
	}

	int Start()
	{
		/*
		1.stacksize����ջ��С���� ����ͨ������ stacksize ��Ա������ָ���̵߳Ķ�ջ��С������ڿ����̵߳��ڴ�ʹ�÷ǳ����á�
		2.detachstate������״̬���� ͨ������ detachstate ��Ա����������ָ���߳��ǿɽ�ϣ�joinable�����Ƿ��루detached�����ɽ�ϵ��߳̿��Ա������̵߳ȴ���join������������߳����˳�ʱ���Զ��ͷ���Դ��
		3.scheduling parameters�����Ȳ������� ����ͨ������ schedparam ��Ա������ָ���̵߳ĵ��Ȳ������������ȼ���
		4.inherit scheduling attributes���̳е������ԣ��� inheritsched ��Ա������������ָ���߳��Ƿ�̳д��������̵߳ĵ������ԡ�
		5.scope���߳������򣩣� ͨ�� scope ��Ա���������������̵߳������򣬿�����ϵͳ��Χ��PTHREAD_SCOPE_SYSTEM������̷�Χ��PTHREAD_SCOPE_PROCESS����
		*/
		pthread_attr_t attr;
		int ret = pthread_attr_init(&attr);
		if (ret != 0) return -1;
		//detachģʽ n
		ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		if (ret != 0) return -2;
		//ֻ�ͽ����ھ���
		ret = pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);//Ŀǰ��ʵ������һ��
		if (ret != 0) return -3;
		ret = pthread_create(&m_thread, &attr, CThread::ThreadEntry, this);
		if (ret != 0) return -4;
		m_mapThread[m_thread] = this;
		ret = pthread_attr_destroy(&attr);
		if (ret != 0) return -5;
		return 0;
	}

	int Stop()
	{
		if (m_thread != 0)
		{
			pthread_t thread = m_thread;
			m_thread = 0;
			timespec ts;
			ts.tv_sec = 0;
			ts.tv_nsec = 100 * 1000, 000;
			int ret = pthread_timedjoin_np(thread, NULL, &ts);
			if (ret == ETIMEDOUT)
			{
				pthread_detach(thread);
				//��Ҫ���߳����Լ���������ź���
				pthread_kill(thread, SIGUSR2);
			}
		}
		return 0;
	}
private:
	//���Ա����Ĭ��_thiscall(����ʽ���ݶ����ָ��),��������Ǿ�̬�ľ���_stdcall
	static void* ThreadEntry(void* arg)
	{
		CThread* thiz = (CThread*)arg;
		struct sigaction act = { 0 };
		sigemptyset(&act.sa_mask);
		act.sa_flags = SA_SIGINFO;
		act.sa_sigaction = &CThread::Sigaction;
		sigaction(SIGUSR1, &act, NULL);
		sigaction(SIGUSR2, &act, NULL);
		thiz->EnterThread();
		pthread_t thread = pthread_self();
		auto it = m_mapThread.find(thread);
		if (it != m_mapThread.end())
		{
			m_mapThread[thread] = NULL;
		}
		pthread_detach(thread);
		thiz->m_thread = 0;
		pthread_exit(NULL);
	}
	int Pause()
	{
		if (m_thread != 0) return -1;
		if (m_bpause)
		{
			m_bpause = false;
			return 0;
		}
		m_bpause = true;
		int ret = pthread_kill(m_thread, SIGUSR1);
		if (ret != 0)
		{
			m_bpause = false;
			return -2;
		}
	}
	static void Sigaction(int signo, siginfo_t* info, void* context)
	{
		if (signo == SIGUSR1)
		{
			pthread_t thread = pthread_self();
			auto it = m_mapThread.find(thread);
			if (it != m_mapThread.end())
			{
				if (it->second)
				{
					while (it->second->m_bpause)
					{
						if (it->second->m_thread == 0) pthread_exit(NULL);
					}
					usleep(1000);
				}
			}
		}
		if (signo == SIGUSR2)
		{
			//�߳��˳�
			pthread_exit(NULL);
		}
	}
	void EnterThread()
	{
		if (m_pfunction != NULL)
		{
			int ret = (*m_pfunction)();
			if (ret != 0)
			{
				printf("%s(%d):<%s>  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, ret);
			}
		}
			

	}

	bool isValid() const { return m_thread == 0; }
private:
	CFunctionBase* m_pfunction;
	pthread_t m_thread;
	bool m_bpause;//true:��ͣ fasle:������
	static std::map<pthread_t, CThread*> m_mapThread;

};
