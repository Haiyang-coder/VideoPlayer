#include <cstdio>
#include<sys/types.h>
#include<unistd.h>
#include<memory.h>
#include<sys/socket.h>
#include<fcntl.h>
#include<sys/stat.h>
#include <csignal>
#include"Process.h"
#include"Loggere.h"
#include"ThreadPool.h"
#include"EdoyunPlayerServer.h"
#include"HttpParser.h"

int LogTest()
{
	//整个流程，先让服务端建立连接，5s后连接  再过5s发送数据
	sleep(1);
	char buffer[] = "helldddddddd !d大大啊爱看\n";
	TRACEI(buffer);
	//DUMPD((void*)buffer,sizeof(buffer));
	//LOGE << buffer  << "dsdfasfdasdfasfad==========中文测试\n";
	return 0;
	
}



int CreateLogServer(CProcess* proc)
{
	printf("%s(%d):<%s>  pid = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
	CLoggerServer server;
	int ret = server.Start();
	if(ret != 0)
	{
		printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
		return ret;
	}
	int fd = 0;
	while (true)
	{
		ret = proc->RecvFd(fd);
		if (fd <= 0 )
		{
			break;
		}
	}
	ret = server.Close();
	printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
	return 0;
}


int CreateClientServer(CProcess* proc)
{
	//printf("%s(%d):<%s>  pid = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
	int fd = -1;
	int ret = proc->RecvFd(fd);
	if (ret < 0)
	{
		//printf("%s(%d):<%s>  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, ret);
		return 0;
	}
	sleep(1);
	char buffer[20];
	lseek(fd, 0, SEEK_SET);
	read(fd, buffer, 20);
	//printf("%s(%d):<%s>  buffer = %s\n", __FILE__, __LINE__, __FUNCTION__, buffer);
	//printf("%s(%d):<%s>  fd = %d\n", __FILE__, __LINE__, __FUNCTION__, fd);
	
	return 0;
}

int oldtest()
{
	//开启守护进程
	CProcess::SwitchDeamon();
	CProcess procLog, procClient;
	procLog.SetEntryFunction(CreateLogServer, &procLog);
	int ret = procLog.CreateSubProcess();
	if (ret < 0)
	{
		printf("%s(%d):<%s>  pid = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
		return ret;
	}
	LogTest();
	//procClient.SetEntryFunction(CreateClientServer, &procClient);
	//ret = procClient.CreateSubProcess();
	//if (ret < 0)
	//{
	//	printf("%s(%d):<%s>  pid = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
	//	return ret;
	//}
	//int fd = open("./text.txt", O_RDWR | O_CREAT | O_APPEND);
	//printf("%s(%d):<%s>  fd = %d\n", __FILE__, __LINE__, __FUNCTION__, fd);
	//if (fd < 0) return -3;
	//ret  = write(fd, "hello", 5);
	//if (ret < 0)
	//{
	//	return ret;
	//}
	//close(fd);
	//ret = procLog.SendFD(-1);
	//printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);

	CThreadPool pool;
	ret = pool.Start(4);
	printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
	ret = pool.AddTask(LogTest);
	printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
	ret = pool.AddTask(LogTest);
	printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
	ret = pool.AddTask(LogTest);
	printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
	ret = pool.AddTask(LogTest);
	printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
	ret = pool.AddTask(LogTest);
	printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
	printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
	ret = pool.AddTask(LogTest);
	printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
	ret = pool.AddTask(LogTest);
	printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
	ret = pool.AddTask(LogTest);
	printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
	ret = pool.AddTask(LogTest);
	printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
	ret = pool.AddTask(LogTest);
	printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
	ret = pool.AddTask(LogTest);
	printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
	ret = pool.AddTask(LogTest);
	printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
	ret = pool.AddTask(LogTest);
	printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
	ret = pool.AddTask(LogTest);
	printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
	ret = pool.AddTask(LogTest);
	printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
	printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
	ret = pool.AddTask(LogTest);
	printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
	ret = pool.AddTask(LogTest);
	printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
	ret = pool.AddTask(LogTest);
	printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
	ret = pool.AddTask(LogTest);
	printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
	ret = pool.AddTask(LogTest);
	printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
	printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);



	getchar();
	pool.Close();
	getchar();
	procLog.SendFD(-1);
	return 0;
}

int server_test()
{
	int ret = 0;
	CProcess::SwitchDeamon();
	CProcess procLog;
	procLog.SetEntryFunction(CreateLogServer, &procLog);
	ret = procLog.CreateSubProcess();
	ERR_RETURN(ret, -1);
	CEdoyunPlayerServer business(2);
	CServer server;
	ret = server.Init(&business);
	ERR_RETURN(ret, -2);
	ret = server.Run();
	ERR_RETURN(ret, -3);

	return 0;
}

int http_test()
{



	return 0;
}
int main()
{
	const char* str = "";
	CHttpParser parser;
	size_t size = parser.Parser(str);
	if (parser.Errno() != 0)
	{
		printf("error = %d", parser.Errno());
		return -1;
	}
	if (size != 365)
	{
		printf("size = %lld", size);
		return -2;
	}
	http_test();
}

