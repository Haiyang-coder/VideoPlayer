#pragma once
#include"Server.h"
#include<map>
#include"Loggere.h"


#define ERR_RETURN(ret, err) if(ret!= 0){TRACEE("ret = %d errno = %d message = [%s]", ret, errno, strerror(errno)); return err;}

#define WARN_CONTINUE(ret) if(ret!= 0){TRACEW("ret = %d", ret); continue;}
class CEdoyunPlayerServer : public CBusiness
{
public:
	CEdoyunPlayerServer(unsigned count):CBusiness()
	{
		m_count = count;
	}
	~CEdoyunPlayerServer()
	{
		m_epoll.Close();
		m_pool.Close();
		for (auto it : m_mapClients)
		{
			if (it.second)
			{
				delete it.second;
			}
		}
		m_mapClients.clear();
	}


	virtual int CBusinessProcess(CProcess* proc)
	{
		int sock = 0;
		int ret = 0;
		ret = m_epoll.Create(m_count);
		if (ret < 0)
		{
			ERR_RETURN(ret, -1)
		}
		ret = m_pool.Start(m_count);
		if (ret < 0)
		{
			ERR_RETURN(ret, -2)
		}
		for (size_t i = 0; i < m_count; i++)
		{
			ret = m_pool.AddTask(&CEdoyunPlayerServer::ThreadFunc, this);
			if (ret < 0) return -3;
		}
		while (m_epoll != -1)
		{
			ret = proc->RecvFd(sock);
			if (ret < 0 || sock == 0)
			{
				break;
			}
			CSocketBase* pClient = new CSocket(sock);
			if (pClient == NULL) continue;
			ret = m_epoll.Add(sock, EpollData((void*)pClient));
			if (m_connnectedcallback)
			{
				(*m_connnectedcallback)();
			}
			WARN_CONTINUE(ret);
		}
	}
private:
	int ThreadFunc()
	{
		int ret = 0;
		EPEvents events;
		while (m_epoll != -1)
		{
			auto size = m_epoll.WaitEvents(events);
			if (size > 0)
			{
				for (ssize_t i = 0; i < size; i++)
				{
					if (events[i].events & EPOLLERR)
					{
						break;
					}
					else if (events[i].events & EPOLLIN)
					{
						CSocketBase* pClient = (CSocketBase*)events[i].data.ptr;
						if (pClient)
						{
							Buffer data;
							ret = pClient->Recv(data);
							WARN_CONTINUE(ret);
							if (m_recvcallback)
							{
								(*m_recvcallback)();
							}
						}
						

					}

				}
			}
			else if (size < 0)
			{
				break;
			}


		}
		return 0;
	}
private:
	CEpoll m_epoll;
	CThreadPool m_pool;
	std::map<int, CSocketBase*> m_mapClients;
	unsigned m_count;

};
