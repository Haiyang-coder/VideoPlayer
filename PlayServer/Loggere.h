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
	LogInfo& operator<< (const T& data)
	{
		std::stringstream stream;
		stream << data;
		
		m_buf += stream.str().c_str();
		printf(m_buf);
		return *this;
	}


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
		m_pServer = new CSocket();
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
		static thread_local CSocket client;
		printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s client=%d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), (int)client);
		int ret = 0;
		if (client == -1)
		{
			printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s %s\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), "client == -1");
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
		if (ret < 0)
		{
			printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %s\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), "发送失败了");//这里ret零
		}
		printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %s\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), "发送成功了");//这里ret零
		
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
			ssize_t ret = m_epoll.WaitEvents(events, -1);
			if (ret < 0)
			{
				printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
				break;
			}
			if (ret > 0)
			{
				//我在给你跑一边，我把连接和发送设置一个延时你看清楚一些
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
						char buff[] = "============检测到epollin事件=============================\n";
						printf("%s", buff);
						if (events[i].data.ptr == m_pServer)
						{
							//创建的新的接收数据的服务端
							CSocketBase* pClient = NULL;
							int r = m_pServer->Link(&pClient);
							printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), r);
							printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d socket = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), r, (int)*pClient);
							if (r < 0) continue;
							r = m_epoll.Add(*pClient, EpollData((void*)pClient), EPOLLIN | EPOLLERR);
							printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d clientSocket = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), r, (int)*pClient);
							if (r < 0)
							{
								printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), r);
								delete pClient;
								continue;
							}
							printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s \n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno));
							auto it = mapClients.find(*pClient);
							printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s \n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno));
							if (it->second != NULL && it != mapClients.end())
							{
								printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s \n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno));
								delete it->second;

							}
							mapClients[*pClient] = pClient;
							printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s \n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno));
							//我看一下走没走玩这个if语句
							//我好像找到问题了
							//这个if好像卡在那里了，都没有走到这里是的，
							//我加个日志看看哪里死了还是卡住了
							printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s \n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno));
						}
						else
						{
							//没有进入到else中 也就是没有收到send，send的位置在哪
							CSocketBase* pClient = (CSocketBase*)events[i].data.ptr;
							if (pClient != NULL)
							{
								Buffer data(1024 * 1024);
								int r = pClient->Recv(data);							
								if (r <= 0)
								{
									printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), r);
									mapClients[*pClient] = NULL;
									delete pClient;
									
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
		std::string str;
		str.resize(data.size());
		memcpy((char*)str.c_str(), data.c_str(), data.size());
		printf("我要开始写入日志了 ==================================================== msg:%s\n", str.c_str());
		fwrite((char*)str.c_str(), 1, str.size(), pfile);
		fflush(pfile);
		printf("日志写入结束了");
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
#define DUMPI(data, size) CLoggerServer::Trace(LogInfo(__FILE__,__LINE__, __FUNCTION__, getpid(),pthread_self(), LOG_INFO, data, size))
#define DUMPD(data, size) CLoggerServer::Trace(LogInfo(__FILE__,__LINE__, __FUNCTION__, getpid(),pthread_self(), LOG_DEBUG, data, size))
#define DUMPW(data, size) CLoggerServer::Trace(LogInfo(__FILE__,__LINE__, __FUNCTION__, getpid(),pthread_self(), LOG_WARNING, data, size))
#define DUMPE(data, size) CLoggerServer::Trace(LogInfo(__FILE__,__LINE__, __FUNCTION__, getpid(),pthread_self(), LOG_ERROR, data, size))
#define DUMPF(data, size) CLoggerServer::Trace(LogInfo(__FILE__,__LINE__, __FUNCTION__, getpid(),pthread_self(), LOG_FATAL, data, size))
#endif
