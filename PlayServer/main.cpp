﻿#include <cstdio>
#include<sys/types.h>
#include<unistd.h>
#include<functional>
#include<memory.h>
#include<sys/socket.h>
#include<fcntl.h>
#include<sys/stat.h>

#include <csignal>



class CFunctionBase
{
public:
	CFunctionBase() {}
	virtual ~CFunctionBase() {}
	virtual int operator()() = 0;

private:

};


template<typename _FUNCTION, typename... _ARGS>
class CFunction : public CFunctionBase
{
public:
	
	CFunction(_FUNCTION func, _ARGS... args)
		:m_binder(std::forward<_FUNCTION>(func), std::forward<_ARGS...>(args)...)
	{

	}
	typename std::_Bindres_helper<int, _FUNCTION, _ARGS...>::type m_binder;
	virtual ~CFunction() {}
	virtual int operator()()
	{
		return m_binder();
	}

	
private:

};


class CProcess
{
public:
	CProcess()
	{
		m_func = NULL;
		memset(pipes, 0, sizeof(pipes));
		
	}
	virtual ~CProcess()
	{
		if (m_func != NULL)
		{
			delete m_func;
			m_func = NULL;
		}
	}
	template<typename _FUNCTION, typename... _ARGS>
	int SetEntryFunction(_FUNCTION func, _ARGS... args)
	{
		m_func = new CFunction<_FUNCTION, _ARGS...>(func, args...);
		return 0;
	}
	int CreateSubProcess()
	{
		if (m_func == NULL) return -1;
		int ret = socketpair(AF_LOCAL, SOCK_STREAM, 0, pipes);
		if (ret < 0) return -2;
		pid_t pid = fork();
		if (pid == -1) return -3;
		if (pid == 0)
		{
			//子进程
			close(pipes[1]);//关闭写管道
			pipes[1] = 0;
			ret = (*m_func)();
			exit(0);
		}
		else
		{
			//主进程
			close(pipes[0]);//关闭读管道
			pipes[0] = 0;
			m_pid = pid;
			return 0;
		}
	}

	int SendFD(int fd)
	{
		//在主进程完成
		msghdr msg;
		iovec iov[2];
		char buf[2][20] = { "send hello","send other" };
		iov[0].iov_base = buf[0];
		iov[0].iov_len = 20;
		iov[1].iov_base = buf[1];
		iov[1].iov_len = 20;
		msg.msg_iov = iov;
		msg.msg_iovlen = 2;

		cmsghdr* cmsg = new cmsghdr;
		memset(cmsg, 0, CMSG_LEN(sizeof(int)) );
		cmsg->cmsg_len = CMSG_LEN(sizeof(int));
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;
		*(int*)CMSG_DATA(cmsg) = fd;
		msg.msg_control = cmsg;
		msg.msg_controllen = cmsg->cmsg_len;
		int ret = sendmsg(pipes[1], & msg, 0);
		if (ret < 0) return ret;
		
		delete cmsg;
		return ret;
	}

	int RecvFd(int& fd)
	{
		msghdr msg;
		iovec iov[2];
		char buf[][20] = { "","" };
		iov[0].iov_base = buf[0];
		iov[0].iov_len = sizeof(buf[0]);
		iov[1].iov_base = buf[1];
		iov[1].iov_len = sizeof(buf[1]);
		msg.msg_iov = iov;
		msg.msg_iovlen = 2;

		cmsghdr* cmsg = new cmsghdr();
		if (cmsg == NULL) return -1;
		memset(cmsg, 0, CMSG_LEN(sizeof(int)));
		cmsg->cmsg_len = CMSG_LEN(sizeof(int));
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;
		msg.msg_control = cmsg;
		msg.msg_controllen = CMSG_LEN(sizeof(int));
		ssize_t ret = recvmsg(pipes[0], &msg, 0);
		if (ret == -1)
		{
			delete cmsg;
			return -2;
		}
		fd = *(int*)CMSG_DATA(cmsg);
	}
	static int SwitchDeamon()
	{
		pid_t ret = fork();
		if (ret < 0)
		{
			return ret;
		}
		if (ret > 0)
		{
			exit(0);//主进程直接退出
		}
		//子进程的内容
		ret = setsid();
		if (ret == -1) return -2;
		ret = fork();
		if (ret == -1) return-3;
		if (ret > 0) exit(0);//子进程的使命也完成了，退出
		//现在只剩下了孙进城了，就进入了守护状态了
		for (size_t i = 0; i < 3; i++)
		{
			close(i);
		}
		umask(0);
		signal(SIGCHLD, SIG_IGN);  // 防止子进程变成僵尸进程
		return 0;
	}

private:
	CFunctionBase* m_func;
	pid_t m_pid;
	int pipes[2];

};

int CreateLogServer(CProcess* proc)
{
	printf("%s(%d):<%s>  pid = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
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
	//CProcess::SwitchDeamon();
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