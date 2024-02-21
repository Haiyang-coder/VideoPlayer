#include "Server.h"

CServer::CServer()
{
    m_business = NULL;
    m_server = NULL;
}

int CServer::Init(CBusiness* business, const Buffer& ip, short port)
{
    //子进程，线程池，epoll， 服务器都创建并且初始化
    //epoll开始监听服务器的listenfd
    int ret = 0;
    if (business == NULL) return -1;
    m_business = business;
    ret = m_process.SetEntryFunction(&CBusiness::CBusinessProcess, m_business, &m_process);
    if(ret < 0) return -2;
    ret = m_process.CreateSubProcess();
    if (ret < 0) return -3;
    ret = m_pool.Start(2);
    if (ret < 0) return -4;
    ret = m_epoll.Create(2);
    if (ret < 0) return -5;
    m_server = new CSocket();
    if(m_server == NULL) return -6;
    ret = m_server->Init(CSockParam(ip, port, SOCK_ISSERVER | SOCK_ISIP));
    if (ret < 0) return -7;
    ret = m_epoll.Add(*m_server, EpollData((void*)m_server));
    if (ret < 0) return -8;
    for (size_t i = 0; i < m_pool.Size(); i++)
    {
        ret = m_pool.AddTask(&CServer::ThreadFunc, this);
        if (ret < 0) return -9;
    }
    return 0;
}

int CServer::Run()
{
    while (m_server != NULL)
    {
        usleep(10);
    }
    return 0;
}

int CServer::Close()
{
    if (m_server)
    {
        CSocketBase* sock = m_server;
        m_server = NULL;
        m_epoll.Del(*sock);
        delete sock;
    }
    m_epoll.Close();
    m_process.SendFD(-1);
    m_pool.Close();
 
    
    return 0;
}

int CServer::ThreadFunc()
{
    int ret = 0;
    EPEvents events;
    while (m_epoll != -1 && m_server != NULL)
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
                else if(events[i].events & EPOLLIN)
                {
                   if(m_server)
                   {
                       CSocketBase* pClient = NULL;
                       ret = m_server->Link(&pClient);
                       if (ret != 0) continue;
                       ret = m_process.SendSocket(*pClient, *pClient);
                       if (ret != 0)
                       {
                           TRACEE("send client %d failed", (int)*pClient);
                           delete pClient;
                           continue;
                       }
                       delete pClient;
                   }

                }
               
            }
        }
        else if( size < 0)
        {
            break;
        }


    }
    return 0;
}


