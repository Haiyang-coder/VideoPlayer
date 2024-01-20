#pragma once
#include"Thread.h"
#include"Epoll.h"
#include"Socket.h"
#include<map>
#include<sys/timeb.h>
#include <sys/stat.h>
#include<iostream>
#include<cstdarg>
#include<sstream>


enum LogLevel {
	LOG_INFO,
	LOG_DEBUG,
	LOG_WARNING,
	LOG_ERROR,
	LOG_FATAL
};

class LogInfo
{
public:
	LogInfo(const char* file, int line, const char* func, pid_t pid, pthread_t tid, int level, const char* fmt, ...);
	
	LogInfo(const char* file, int line, const char* func, pid_t pid, pthread_t tid, int level);
	
	LogInfo(const char* file, int line, const char* func, pid_t pid, pthread_t tid, int level, void* pData, size_t nSize);
	
	~LogInfo();
	
	operator Buffer() const
	{
		return m_buf;
	}

	template<typename T>
	LogInfo& operator<< (const T& data);

private:
	bool bAuto = false;
	Buffer m_buf;
};



class CLoggerServer
{
public:
	CLoggerServer(); 
	~CLoggerServer();

	CLoggerServer(const CLoggerServer&) = delete;
	CLoggerServer& operator=(const CLoggerServer&) = delete;

public:
	int Start()
	{
		if (m_pServer != NULL) return -1;
		if (access("log", W_OK | R_OK) != 0)
		{
			mkdir("log", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
		}
		m_file = fopen(m_path, "w+");
		if (m_file == NULL) return -2;
		int ret = m_epoll.Create(1);
		if (ret != 0)
		{
			printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
			return -3;
		}
		m_pServer = new CLocalSocket();
		if (m_pServer == NULL)
		{
			Close();
			printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
			return -4;
		}
		CSockParam param("./log/server.socket", (int)SOCK_ISSERVER);
		ret = m_pServer->Init(param);
		if (ret != 0)
		{
			Close();
			printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
			return-5;
		}
		ret = m_epoll.Add(*m_pServer, EpollData((void*)m_pServer), EPOLLIN | EPOLLERR);
		if (ret != 0)
		{
			Close();
			printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
			return-6;
		}
		ret = m_thread.Start();
		if (ret != 0)
		{
			Close();
			printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
			return-7;
		}
		return 0;
	}
	

	int Close()
	{
		if (m_pServer != NULL)
		{
			CSocketBase* server = m_pServer;
			m_pServer = NULL;
			delete server;
		}
		m_epoll.Close();
		m_thread.Stop();
		return  0;
	}

	//给其他非日志进程和线程使用的
	static void Trace(const LogInfo& info)
	{
		static thread_local CLocalSocket client;
		int ret = 0;
		if (client == -1)
		{
			
			ret = client.Init(CSockParam("./log/server.socket", 0));
			if (ret != 0)
			{
				printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
				return;
			}
			ret  = client.Link();
			if(ret < 0)
			{
				printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
				return;
			}
		}
		ret = client.Send(info);
		printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno),ret);
	}
	static Buffer GetTimeStr()
	{
		Buffer result(128);
		timeb tmb;
		ftime(&tmb);
		tm* pTm = localtime(&tmb.time);
		int nSize = snprintf(result, result.size(), 
			"%04d_%02d_%02d_%02d-%02d-%02d_%03d",
			pTm->tm_year + 1900,
			pTm->tm_mon + 1, 
			pTm->tm_mday, 
			pTm->tm_hour, 
			pTm->tm_min, 
			pTm->tm_sec, 
			tmb.millitm);
		result.resize(nSize);
		return result;
	}
private:
	int ThreadFunc()
	{
		printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s \n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno));
		EPEvents events;
		std::map<int, CSocketBase*> mapClients;
		while (m_thread.isValid() &&
			m_epoll != -1 &&
			m_pServer != NULL)
		{
			ssize_t ret = m_epoll.WaitEvents(events, 1);
			if (ret < 0)
			{
				printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
				break;
			}
			if (ret > 0)
			{
				printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s \n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno));
				size_t i = 0;
				for (; i < ret; i++)
				{
					if (events[i].events & EPOLLERR)
					{
						printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
						break;
					}
					if (events[i].events & EPOLLIN)
					{
						if (events[i].data.ptr == m_pServer)
						{
							CSocketBase* pClient = NULL;
							int r = m_pServer->Link(&pClient);
							printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), r);
							printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d socket = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), r, (int)*pClient);
							if (r < 0) continue;
							r = m_epoll.Add(*pClient, EpollData((void*)pClient), EPOLLIN | EPOLLERR);
							printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), r);
							if (r < 0)
							{
								printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), r);
								delete pClient;
								continue;
							}
							auto it = mapClients.find(*pClient);
							if (it->second != NULL)
							{
								delete it->second;
							}
							mapClients[*pClient] = pClient;

						}
						else
						{
							printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
							CSocketBase* pClient = (CSocketBase*)events[i].data.ptr;
							if (pClient != NULL)
							{
								Buffer data(1024 * 1024);
								int r = pClient->Recv(data);
								printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s \n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno));
								if (r <= 0)
								{
									printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
									delete pClient;
									mapClients[*pClient] = NULL;
								}
								else
								{
									printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
									WriteLog(data);
								}
							}
						}
					}
				}
				if (i != ret)
				{
					printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s \n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno));
					break;
				}
			}
		}
		printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno));
		for (auto it = mapClients.begin(); it != mapClients.end(); it++)
		{
			if (it->second)
			{
				delete it->second;
			}
		}
		mapClients.clear();
		return 0;
	}
	

	void WriteLog(const Buffer& data)
	{
		if (m_file == NULL)
		{
			return;
		}
		FILE* pfile = m_file;
		fwrite((char*)data, 1, data.size(), pfile);
		fflush(pfile);
		printf("%s", (char*)data);
	}
		

private:
	CThread m_thread;
	CEpoll m_epoll;
	CSocketBase* m_pServer;
	Buffer m_path;
	FILE* m_file;

};


#ifndef TRACE
#define TRACEI(...) CLoggerServer::Trace(LogInfo(__FILE__, __LINE__,__FUNCTION__, getpid(), pthread_self(),LOG_INFO, __VA_ARGS__))
#define TRACED(...) CLoggerServer::Trace(LogInfo(__FILE__, __LINE__,__FUNCTION__, getpid(), pthread_self(),LOG_DEBUG, __VA_ARGS__))
#define TRACEW(...) CLoggerServer::Trace(LogInfo(__FILE__, __LINE__,__FUNCTION__, getpid(), pthread_self(), LOG_WARNING, __VA_ARGS__))
#define TRACEE(...) CLoggerServer::Trace(LogInfo(__FILE__, __LINE__,__FUNCTION__, getpid(), pthread_self(),LOG_ERROR, __VA_ARGS__))
#define TRACEF(...) CLoggerServer::Trace(LogInfo(__FILE__, __LINE__,__FUNCTION__, getpid(), pthread_self(),LOG_FATAL, __VA_ARGS__))

//LOGI << "hello" << "good"
#define LOGI LogInfo(__FILE__, __LINE__,__FUNCTION__, getpid(), pthread_self(),LOG_INFO)
#define LOGD LogInfo(__FILE__, __LINE__,__FUNCTION__, getpid(), pthread_self(),LOG_DEBUG)
#define LOGW LogInfo(__FILE__, __LINE__,__FUNCTION__, getpid(), pthread_self(),LOG_WARNING)
#define LOGE LogInfo(__FILE__, __LINE__,__FUNCTION__, getpid(), pthread_self(),LOG_ERROR)
#define LOGF LogInfo(__FILE__, __LINE__,__FUNCTION__, getpid(), pthread_self(),LOG_FATAL)

//01 02 03 A1 ... ...
#define DUMPI(data, size) LogInfo(__FILE__,__LINE__, __FUNCTION__, getpid(),pthread_self(), LOG_INFO, data, size)
#define DUMPD(data, size) LogInfo(__FILE__,__LINE__, __FUNCTION__, getpid(),pthread_self(), LOG_DEBUG, data, size)
#define DUMPW(data, size) LogInfo(__FILE__,__LINE__, __FUNCTION__, getpid(),pthread_self(), LOG_WARNING, data, size)
#define DUMPE(data, size) LogInfo(__FILE__,__LINE__, __FUNCTION__, getpid(),pthread_self(), LOG_ERROR, data, size)
#define DUMPF(data, size) LogInfo(__FILE__,__LINE__, __FUNCTION__, getpid(),pthread_self(), LOG_FATAL, data, size)
#endif

template<typename T>
inline LogInfo& LogInfo::operator<<(const T& data)
{
	std::stringstream stream;
	stream << data;
	m_buf += stream.str();
	return *this;
}
