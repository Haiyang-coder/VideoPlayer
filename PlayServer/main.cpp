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

int LogTest()
{
	sleep(5);
	printf("%s(%d):<%s>  pid = %d %s \n" , __FILE__, __LINE__, __FUNCTION__, getpid(), "trace start");
	char buffer[] = "hell !d大大啊爱看";
	TRACEI("here is log  大大 释放 \n");
	//printf("%s(%d):<%s>  pid = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
	//DUMPD((void*)buffer,sizeof(buffer));
	//printf("%s(%d):<%s>  pid = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
	//LOGE << 100 << "dfsf" << "打法" << 1515.5f << buffer;
	//printf("%s(%d):<%s>  pid = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
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
		if (fd <= -1 )
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




int main()
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
	ret = procLog.SendFD(-1);
	printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
	getchar();
    return 0;
}