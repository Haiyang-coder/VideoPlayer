#pragma once
#include"Function.h"
#include <sys/stat.h>
#include <netinet/in.h>




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
		memset(cmsg, 0, CMSG_LEN(sizeof(int)));
		cmsg->cmsg_len = CMSG_LEN(sizeof(int));
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;
		*(int*)CMSG_DATA(cmsg) = fd;
		msg.msg_control = cmsg;
		msg.msg_controllen = cmsg->cmsg_len;
		int ret = sendmsg(pipes[1], &msg, 0);
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
		delete cmsg;
		return 0;
	}


	int SendSocket(int fd, const sockaddr_in* addrin)
	{
		//在主进程完成
		msghdr msg;
		iovec iov[2];
		char buf[2][10] = { "sllo","senher" };
		iov[0].iov_base = (void*)addrin;
		iov[0].iov_len = sizeof(sockaddr_in);
		iov[1].iov_base = buf[1];
		iov[1].iov_len = sizeof(buf[1]);
		msg.msg_iov = iov;
		msg.msg_iovlen = 2;

		cmsghdr* cmsg = new cmsghdr;
		memset(cmsg, 0, CMSG_LEN(sizeof(int)));
		cmsg->cmsg_len = CMSG_LEN(sizeof(int));
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;
		*(int*)CMSG_DATA(cmsg) = fd;
		msg.msg_control = cmsg;
		msg.msg_controllen = cmsg->cmsg_len;
		int ret = sendmsg(pipes[1], &msg, 0);
		if (ret < 0)
		{
			delete cmsg;
			return ret;
		}
		delete cmsg;
		return ret;
	}

	int RecvSocket(int& fd, sockaddr_in* addrin)
	{
		msghdr msg;
		iovec iov[2];
		char buf[][10] = { "","" };
		iov[0].iov_base = addrin;
		iov[0].iov_len = sizeof(addrin);
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
		delete cmsg;
		return 0;
	}


	/*
	双重fork()：

目的：避免守护进程成为孤儿进程，并脱离控制终端。
不做的影响：如果不进行双重fork()，守护进程可能仍然会保留一个控制终端，接收控制终端的信号（如SIGHUP），这可能会导致守护进程意外终止。
调用setsid()：

目的：创建一个新的会话，并成为该会话的领头进程。这样，守护进程就不会再有控制终端。
不做的影响：如果守护进程仍然属于旧的会话，它将继续与控制终端相关联，这可能导致它意外收到控制终端的信号。
更改工作目录：

目的：将工作目录更改为根目录/，避免守护进程持有某个挂载点，阻止系统卸载文件系统。
不做的影响：如果守护进程的工作目录保持不变，可能会导致文件系统无法卸载，因为工作目录所在的文件系统会被守护进程锁住。
重设文件权限掩码（umask）：

目的：重设文件权限掩码，确保守护进程创建的文件具有预期的权限。
不做的影响：如果不重设umask，守护进程可能继承父进程的文件权限掩码，导致创建的文件权限不符合预期，可能会产生安全风险。
关闭所有文件描述符：

目的：关闭守护进程从父进程继承的所有打开的文件描述符，避免意外使用这些描述符。
不做的影响：如果守护进程继续持有不必要的文件描述符，可能会无意中干扰其他进程或文件系统操作，还可能导致文件描述符泄漏。
重定向标准输入、输出和错误输出到/dev/null：

目的：将标准输入、输出和错误输出重定向到/dev/null，避免守护进程尝试使用这些标准文件描述符，导致意外行为。
不做的影响：守护进程如果不重定向这些文件描述符，可能会尝试向已关闭的终端输出信息，导致写入错误或者程序异常终止。
SIGCHLD信号处理函数：

目的：处理子进程终止时发送的SIGCHLD信号，避免子进程变成僵尸进程。
不做的影响：如果守护进程不处理SIGCHLD信号，终止的子进程将保持在僵尸状态，占用系统资源，最终可能导致资源耗尽。

为什么子进程不需要这些步骤
子进程是在守护进程内部创建的，继承了守护进程的环境，但子进程并不需要具备守护进程的特性。具体原因如下：

不需要脱离控制终端：

子进程在运行时并不会像守护进程一样长时间运行且独立于终端控制。
不需要重新设置工作目录：

子进程继承了守护进程的工作目录设置，不会影响文件系统的挂载与卸载。
不需要重设文件权限掩码：

子进程在创建特定文件时可以手动指定权限，不需要统一重设umask。
不需要关闭文件描述符：

子进程通常有特定的任务，需要使用打开的文件描述符。
不需要重定向标准输入输出：

子进程可能需要使用标准输入、输出和错误输出进行通信和日志记录。
小结
守护进程的设置步骤是为了确保它能在后台稳定地运行，不受控制终端和外部环境的影响。而普通子进程不需要具备这些特性，只需要完成特定任务，因此不需要进行上述复杂的设置。守护进程的创建步骤可以确保其独立性和稳定性，而子进程在守护进程的管理下，可以专注于具体的工作任务。

	
	
	
	*/
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
		//现在只剩下了孙进城了，就进入了守护状态了//找下epoll的位置
		for (size_t i = 0; i < 3; i++)
		{
			//close(i);
		}
		umask(0);
		signal(SIGCHLD, SIG_IGN);  // 防止子进程变成僵尸进程
		printf("%s(%d):<%s>  pid = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
		return 0;
	}

private:
	CFunctionBase* m_func;
	pid_t m_pid;
	int pipes[2];

};