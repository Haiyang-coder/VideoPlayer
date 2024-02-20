#include "ThreadPool.h"

CThreadPool::CThreadPool()
{
	m_server = NULL;
	timespec tp{ 0,0 };
	clock_gettime(CLOCK_REALTIME, &tp);
	char* buf = NULL;
	asprintf(&buf, "%d.%d.sock", tp.tv_sec % 100000, tp.tv_nsec % 100000);
	
	if (buf != NULL)
	{
		m_path = buf;
		free(buf);
	}
	usleep(1);
	
}

int CThreadPool::Start(unsigned count)
{
	if (m_server != NULL)
	{
		return -1;
	}
	if (m_path.size() == 0)
	{
		return -2;
	}
	m_server = new CSocket();
	if(m_server == NULL)
	{
		return -3;
	}
	int ret = m_server->Init(CSockParam(m_path, SOCK_ISSERVER));
	if (ret != 0)
	{
		return -4;
	}
	ret = m_epoll.Create(count);
	if (ret < 0)
	{
		return -5;
	}
	ret = m_epoll.Add(*m_server, EpollData((void*) m_server));
	if (ret < 0)
	{
		return -6;
	}
	m_threads.resize(count);
	for (size_t i = 0; i < count; i++)
	{
		m_threads[i] = new CThread(&CThreadPool::TaskDispatch, this);
		if (m_threads[i] == NULL)
		{
			return -7;
		}
		ret = m_threads[i]->Start();
		if (ret < 0)
		{
			return -8;
		}
	}

	return 0;
}

void CThreadPool::Close()
{
	m_epoll.Close();
	if (m_server)
	{
		CSocketBase* p = m_server;
		m_server = NULL;
		delete p;
	}
	for (auto thread : m_threads)
	{
		if (thread) {
			delete thread;
		}
	}
	m_threads.clear();
	unlink(m_path);
}

int CThreadPool::TaskDispatch()
{
	while (m_epoll != -1)
	{
		EPEvents events;
		int ret = 0;
		ssize_t esize =  m_epoll.WaitEvents(events);
		if (esize > 0)
		{
			for (ssize_t i = 0; i < esize; i++)
			{
				if (events[i].events & EPOLLIN)
				{
					CSocketBase* pclient = NULL;
					if (events[i].data.ptr == m_server)
					{
						//如果是连接的请求来到
						ret = m_server->Link(&pclient);
						if (ret < 0)
						{
							continue;
						}
						ret = m_epoll.Add(*pclient, EpollData((void*)pclient));
						if (ret < 0)
						{
							delete pclient;
							continue;
						}

					}
					else
					{
						//数据传输的请求来到
						pclient = (CSocketBase*)events[i].data.ptr;
						if (pclient)
						{
							CFunctionBase* func = NULL;
							Buffer data(sizeof(func));
							ret = pclient->Recv(data);
							if (ret <= 0)
							{
								m_epoll.Del(*pclient);
								delete pclient;
								continue;
							}
							else
							{
								memcpy(&func, (char*)data, sizeof(func));
								if (func != NULL)
								{
									(*func)();
									delete func;
									continue;
								}
							}

						}

					}
				}
			}
		}
	}
	return 0;
}
