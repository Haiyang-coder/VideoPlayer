#pragma once
#include"Function.h"

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
			//�ӽ���
			close(pipes[1]);//�ر�д�ܵ�
			pipes[1] = 0;
			ret = (*m_func)();
			exit(0);
		}
		else
		{
			//������
			close(pipes[0]);//�رն��ܵ�
			pipes[0] = 0;
			m_pid = pid;
			return 0;
		}
	}

	int SendFD(int fd)
	{
		//�����������
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
			exit(0);//������ֱ���˳�
		}
		//�ӽ��̵�����
		ret = setsid();
		if (ret == -1) return -2;
		ret = fork();
		if (ret == -1) return-3;
		if (ret > 0) exit(0);//�ӽ��̵�ʹ��Ҳ����ˣ��˳�
		//����ֻʣ����������ˣ��ͽ������ػ�״̬��
		for (size_t i = 0; i < 3; i++)
		{
			close(i);
		}
		umask(0);
		signal(SIGCHLD, SIG_IGN);  // ��ֹ�ӽ��̱�ɽ�ʬ����
		return 0;
	}

private:
	CFunctionBase* m_func;
	pid_t m_pid;
	int pipes[2];

};