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
	LogInfo(const char* file, int line, const char* func, pid_t pid, pthread_t tid, int level, const char* fmt, ...)
	{
		const char sLevel[][8] = { "INFO","DEBUG","WARNING","ERROR","FATAL" };
		char* buf = NULL;
		bAuto = false;
		int count = asprintf(&buf, "%s(%d):[%s][%s]<%d-%d>(%s) ",file, line, sLevel[level], (char*)CLoggerServer::GetTimeStr(), pid, tid, func);
		if (count > 0)
		{
			m_buf = buf;
			free(buf);
		}
		else
		{
			return;
		}
		va_list ap;
		va_start(ap, fmt);
		
		count = vasprintf(&buf, fmt, ap);
		if (count > 0)
		{
			m_buf += buf;
			free(buf);
		}
		va_end(ap);
		
	}
	LogInfo(const char* file, int line, const char* func, pid_t pid, pthread_t tid, int level)
	{
		bAuto = true;
		//自己主动记录日志
		const char sLevel[][8] = { "INFO","DEBUG","WARNING","ERROR","FATAL" };
		char* buf = NULL;
		bAuto = false;
		timeb tmb;
		ftime(&tmb);
		int count = asprintf(&buf, "%s(%d):[%s][%s]<%d-%d>(%s) ", file, line, sLevel[level], (char*)CLoggerServer::GetTimeStr(), pid, tid, func);
		if (count > 0)
		{
			m_buf = buf;
			free(buf);
		}
		
		
	}
	LogInfo(const char* file, int line, const char* func, pid_t pid, pthread_t tid, int level, void* pData, size_t nSize)
	{
		
		bAuto = false;
		const char sLevel[][8] = { "INFO","DEBUG","WARNING","ERROR","FATAL" };
		char* buf = NULL;
		bAuto = false;
		timeb tmb;
		ftime(&tmb);
		int count = asprintf(&buf, "%s(%d):[%s][%s]<%d-%d>(%s) \n", file, line, sLevel[level], (char*)CLoggerServer::GetTimeStr(), pid, tid, func);
		if (count > 0)
		{
			m_buf = buf;
			free(buf);
		}
		else
		{
			return;
		}
		Buffer out;
		size_t i = 0;
		char* Data = (char*)pData;
		for ( ; i < nSize; i++)
		{
			char buf[16] = "";
			snprintf(buf, sizeof(buf), "02X ", Data[i] & 0xFF);
			m_buf += buf;
			if (0 == ((i + 1) % 16))
			{
				m_buf = "\t;";
				for (size_t j = i - 15; j <= i; j++)
				{
					if ((Data[j] & 0xFF) > 31 && (Data[j] & 0xFF) < 0x7F)
					{
						m_buf += Data[i];
					}
					else
					{
						m_buf == ".";
					}
				}
				m_buf == "\n";
			}
		}

		//处理数据尾部
		size_t k = i % 16;
		if (k != 0)
		{
			for (size_t j = 0; j < 16 - k; j++) m_buf += "   ";
			m_buf = "\t;";
			for (size_t j = i - 15; j <= i; j++)
			{
				if ((Data[j] & 0xFF) > 31 && (Data[j] & 0xFF) < 0x7F)
				{
					m_buf += Data[i];
				}
				else
				{
					m_buf == ".";
				}
			}
			m_buf == "\n";
		}
	}
	~LogInfo()
	{
		if (bAuto)
		{
			CLoggerServer::Trace(*this);
		}
	}
	operator Buffer() const{}

	template<typename T>
	LogInfo& operator<< (const T& data)
	{
		std::stringstream stream;
		stream << data;
		m_buf += stream.str();
		return *this;
	}

private:
	bool bAuto = false;
	Buffer m_buf;
};



class CLoggerServer
{
public:
	CLoggerServer():m_thread(&CLoggerServer::ThreadFunc, this)
	{
		m_pServer = NULL;
		m_path = "./log/" + GetTimeStr() + ".log";
		printf("%s(%d):<%s>  path = %s\n", __FILE__, __LINE__, __FUNCTION__,(char*) m_path);
		
	}
	~CLoggerServer(){}

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
		if (ret != 0) return -3;
		m_pServer = new CLocalSocket();
		if (m_pServer == NULL)
		{
			Close();
			return -4;
		}
		CSockParam param("./log/server.socket", (int)SOCK_ISSERVER);
		ret = m_pServer->Init(param);
		if (ret != 0)
		{
			Close();
			return-5;
		}
		ret = m_thread.Start();
		if (ret != 0)
		{
			Close();
			return-6;
		}
		return 0;
	}
	int ThreadFunc()
	{
		EPEvents events;
		std::map<int, CSocketBase*> mapClients;
		while (m_thread.isValid() && 
			m_epoll != -1 &&
			m_pServer != NULL)
		{
			ssize_t ret =  m_epoll.WaitEvents(events,1);
			if (ret < 0) break;
			if (ret > 0)
			{
				size_t i = 0;
				for (; i < ret; i++)
				{
					if (events[i].events & EPOLLERR)
					{
						break;
					}
					if(events[i].events & EPOLLIN)
					{
						if (events[i].data.ptr == m_pServer)
						{
							CSocketBase* pClient = NULL;
							int r = m_pServer->Link(&pClient);
							if (r < 0) continue;
							r = m_epoll.Add(*pClient, EpollData((void*)pClient), EPOLLIN | EPOLLERR);
							if (r < 0)
							{
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
							CSocketBase* pClient = (CSocketBase*)events[i].data.ptr;
							if (pClient != NULL)
							{
								Buffer data(1024 * 1024);
								int r = pClient->Recv(data);
								if (r <= 0)
								{
									delete pClient;
									mapClients[*pClient] = NULL;
								}
								else
								{
									WriteLog(data);
								}
							}
						}
					}
				}
				if (i != ret)
				{
					break;
				}
			}
		}
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
		if (client == -1)
		{
			int ret = 0;
			ret = client.Init(CSockParam("./log/server.socket", 0));
			if (ret != 0)
			{
				std::cout << "Trace error" << std::endl;
				return;
			}
			client.Send(info);
		}
	}
	static Buffer GetTimeStr()
	{
		Buffer result(128);
		timeb tmb;
		ftime(&tmb);
		tm* pTm = localtime(&tmb.time);
		int nSize = snprintf(result, result.size(), "%04d-%02d-%02d %02d-%02d-%02d %03d", pTm->tm_year + 1900, pTm->tm_mon + 1, pTm->tm_mday, pTm->tm_hour, pTm->tm_min, pTm->tm_sec, tmb.millitm);
		result.resize(nSize);
	}
private:
	

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
#define TRACEI(...) CLoggerServer::Trace(LogInfo(__FILE__, __LINE__,__FUNCTION__, getpid(), pthread_self(),LOG_INFO, __VA_AGRS__))
#define TRACED(...) CLoggerServer::Trace(LogInfo(__FILE__, __LINE__,__FUNCTION__, getpid(), pthread_self(),LOG_DEBUG, __VA_AGRS__))
#define TRACEW(...)CLoggerServer::Trace(LogInfo(__FILE__, __LINE__,__FUNCTION__, getpid(), pthread_self(), LOG_WARNING, __VA_AGRS__))
#define TRACEE(...)CLoggerServer::Trace(LogInfo(__FILE__, __LINE__,__FUNCTION__, getpid(), pthread_self(),LOG_ERROR, __VA_AGRS__))
#define TRACEF(...)CLoggerServer::Trace(LogInfo(__FILE__, __LINE__,__FUNCTION__, getpid(), pthread_self(),LOG_FATAL, __VA_AGRS__))

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