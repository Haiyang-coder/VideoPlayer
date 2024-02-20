#include "Loggere.h"

LogInfo::LogInfo(const char* file, int line, const char* func, pid_t pid, pthread_t tid, int level, const char* fmt, ...)
{
	const char sLevel[][8] = { "INFO","DEBUG","WARNING","ERROR","FATAL" };
	char* buf = NULL;
	bAuto = false;
	int count = asprintf(&buf, "%s(%d):[%s][%s]<%d-%d>(%s) ", file, line, sLevel[level], (char*)CLoggerServer::GetTimeStr(), pid, tid, func);
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
	m_buf += "\n";
	va_end(ap);

}

LogInfo::LogInfo(const char* file, int line, const char* func, pid_t pid, pthread_t tid, int level)
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

LogInfo::LogInfo(const char* file, int line, const char* func, pid_t pid, pthread_t tid, int level, void* pData, size_t nSize)
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
	for (; i < nSize; i++)
	{
		char buf[16] = "";
		snprintf(buf, sizeof(buf), "%02X ", Data[i] & 0xFF);
		m_buf += buf;
		if (0 == ((i + 1) % 16))
		{
			m_buf = "\t;";
			char buf[17] = "";
			memcpy(buf, Data + i - 15, 16);
			for (int j = 0; j < 16; j++)
			{
				if ((unsigned)buf[j] < 32 && buf[j] >= 0) buf[j] = '.';
			}
			m_buf += buf;
			/*for (size_t j = i - 15; j <= i; j++)
			{
				
				if ((Data[j] & 0xFF) > 31 && (Data[j] & 0xFF) < 0x7F)
				{
					m_buf += Data[i];
				}
				else
				{
					m_buf == ".";
				}
			}*/
			m_buf == "\n";
		}
	}

	//处理数据尾部
	size_t k = i % 16;
	if (k != 0)
	{
		for (size_t j = 0; j < 16 - k; j++)
		{
			m_buf += "   ";
		}
		m_buf = "\t;";
		for (size_t j = i - 15; j <= i; j++)
		{
			if ((Data[j] & 0xFF) > 31 && (Data[j] & 0xFF) < 0x7F)
			{
				m_buf += Data[i];
			}
			else
			{
				m_buf += ".";
			}
		}
		m_buf == "\n";
	}

}

LogInfo::~LogInfo()
{
	if (bAuto)
	{
		m_buf += "\n";
		CLoggerServer::Trace(*this);
	}
}

CLoggerServer::CLoggerServer():m_thread(&CLoggerServer::ThreadFunc, this)
{
	m_pServer = NULL;
	char curpath[256] = "";
	getcwd(curpath, sizeof(curpath));
	m_path = curpath;
	m_path += "/log/" + GetTimeStr() + ".log";
	printf("%s(%d):<%s>  path = %s\n", __FILE__, __LINE__, __FUNCTION__, (char*)m_path);

}

CLoggerServer::~CLoggerServer()
{
}
