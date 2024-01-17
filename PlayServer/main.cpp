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

}



int CreateLogServer(CProcess* proc)
{
	//printf("%s(%d):<%s>  pid = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
	CLoggerServer server;
	int ret = server.Start();
	if(ret != 0)
	{
		printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno));
	}
	return 0;
}


int CreateClientServer(CProcess* proc)
{
	printf("%s(%d):<%s>  pid = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
	int fd = -1;
	int ret = proc->RecvFd(fd);
	if (ret < 0)
	{
		printf("%s(%d):<%s>  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, ret);
		return 0;
	}
	sleep(1);
	char buffer[20];
	lseek(fd, 0, SEEK_SET);
	read(fd, buffer, 20);
	printf("%s(%d):<%s>  buffer = %s\n", __FILE__, __LINE__, __FUNCTION__, buffer);
	printf("%s(%d):<%s>  fd = %d\n", __FILE__, __LINE__, __FUNCTION__, fd);
	
	return 0;
}




int main()
{
	//开启守护进程
	CProcess::SwitchDeamon();
	CProcess procLog, procClient;
	printf("%s(%d):<%s>  pid = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
	procLog.SetEntryFunction(CreateLogServer, &procLog);
	int ret = procLog.CreateSubProcess();
	if (ret < 0)
	{
		printf("%s(%d):<%s>  pid = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
		return ret;
	}
	printf("%s(%d):<%s>  pid = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
	procClient.SetEntryFunction(CreateClientServer, &procClient);
	ret = procClient.CreateSubProcess();
	if (ret < 0)
	{
		printf("%s(%d):<%s>  pid = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
		return ret;
	}
	printf("%s(%d):<%s>  pid = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid());

	int fd = open("./text.txt", O_RDWR | O_CREAT | O_APPEND);
	printf("%s(%d):<%s>  fd = %d\n", __FILE__, __LINE__, __FUNCTION__, fd);
	if (fd < 0) return -3;
	ret  = write(fd, "hello", 5);
	ret = procClient.SendFD(fd);
	if (ret < 0)
	{
		return ret;
	}
	close(fd);
    return 0;
}