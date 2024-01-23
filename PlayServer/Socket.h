#pragma once
#include<unistd.h>
#include<sys/socket.h>
#include<sys/un.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string>
#include<fcntl.h>
class Buffer : public std::string
{
public:
	Buffer(size_t size) : std::string() { resize(size); }
	Buffer(const std::string& str) : std::string(str) {  }
	Buffer(const char* str) : std::string(str) {  }
	Buffer() : std::string() {}
	~Buffer(){}

public:
	//给非常量对象使用
	operator char* ()
	{
		return const_cast<char*>(c_str());
	}
	//给常量对象使用
	operator char* () const
	{
		return const_cast<char*>(c_str());
	}
	operator const char* ()
	{
		return c_str();
	}
	operator const char* () const
	{
		return c_str();
	}

private:

};

enum SOCKATTR {
	SOCK_ISSERVER = 1,//0是客户端 1是服务器 
	SOCK_ISNOBLOCK = 2,//0是阻塞 1是非阻塞
	SOCK_ISUDP = 4, //0是tcp 1是udp
};

class CSockParam
{
public:
	CSockParam() 
	{
		bzero(&m_addr_in, sizeof(m_addr_in));
		bzero(&m_addr_un, sizeof(m_addr_un));
		port = -1;
		arrt = 0;
	}
	//网络套接字的初始化
	CSockParam(const Buffer& ip, short port, int attr)
	{
		this->ip = ip;
		this->port = port;
		this->arrt = attr;
		m_addr_in.sin_family = AF_INET;
		m_addr_in.sin_port = port;
		m_addr_in.sin_addr.s_addr = inet_addr(ip);
		
	}
	//本地套接字的初始化
	CSockParam(const Buffer& path, int attr)
	{
		ip = path;
		m_addr_un.sun_family = AF_UNIX;
		strcpy(m_addr_un.sun_path, path);
		this->arrt = attr;
	}
	~CSockParam(){}
	CSockParam(const CSockParam& data)
	{
		ip = data.ip;
		port = data.port;
		arrt = data.arrt;
		memcpy(&m_addr_in, &data.m_addr_in, sizeof(m_addr_in));
		memcpy(&m_addr_un, &data.m_addr_un, sizeof(m_addr_un));
	}
public:
	CSockParam& operator=(const CSockParam& data)
	{
		if (this != &data)
		{
			ip = data.ip;
			port = data.port;
			arrt = data.arrt;
			memcpy(&m_addr_in, &data.m_addr_in, sizeof(m_addr_in));
			memcpy(&m_addr_un, &data.m_addr_un, sizeof(m_addr_un));
		}
		return *this;
	}

public:
	sockaddr* addrin()
	{
		return (sockaddr*)&m_addr_in;
	}
	sockaddr* addrun()
	{
		return (sockaddr*)&m_addr_un;
	}
public:
	//地址
	sockaddr_in m_addr_in;
	sockaddr_un m_addr_un;
	//ip
	Buffer ip;
	//端口
	short port;
	//是否阻塞
	int arrt;


};




class CSocketBase
{
public:
	CSocketBase() 
	{
		m_socket = -1;
		m_status = 0;
	}
	virtual ~CSocketBase()
	{
		Close();
		
	}//和构造函数的顺序相反,先析构子类
public:
	//初始化服务器，套接字创建，绑定，监听， 客户端只有创建
	virtual int Init(const CSockParam& param) = 0;
	//连接: 服务器：accept 客户端：connect; udp直接返回成功
	virtual int Link(CSocketBase** ppClient = NULL) = 0;
	//发送数据
	virtual int Send(const Buffer& data) = 0;
	//接收数据
	virtual int Recv(Buffer& data) = 0;
	//关闭连接
	virtual int Close()
	{
		m_status = 3;
		if (m_socket != -1)
		{
			unlink(m_param.ip);
			int fd = m_socket;
			m_socket = -1;
			close(fd);
		}
	}
	virtual operator int() { return m_socket; }
	virtual operator int() const{ return m_socket; }
protected:
	//套接字描述符 -1
	int m_socket;
	//状态0初始化未完成， 1初始化完成 2连接完成 3已经关闭
	int m_status;
	//初始化的参数
	CSockParam m_param;


};


class CLocalSocket:public CSocketBase
{
public:
	CLocalSocket() : CSocketBase(){}
	CLocalSocket(int sock):CSocketBase()
	{
		m_socket = sock;
	
	}
	virtual ~CLocalSocket()
	{
		Close();
	}
public:
	//初始化服务器，套接字创建，绑定，监听， 客户端只有创建
	virtual int Init(const CSockParam& param)
	{
		if (m_status != 0)
		{
			printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  m_status = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), m_status);
			return -1;
		}
		m_param = param;
		int type = (m_param.arrt  & SOCK_ISUDP) ?  SOCK_DGRAM : SOCK_STREAM;
		if (m_socket == -1)
		{
			printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno));
			m_socket = socket(PF_LOCAL, type, 0);
		}
		else
		{
			m_status = 2;
		}
		
		int ret = 0;
		if (m_param.arrt & SOCK_ISSERVER)
		{
			//跳转的不准 往下拉
			printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno));
			ret = bind(m_socket, m_param.addrun(), sizeof(sockaddr_un));
			if (ret == -1)
			{
				return -3;
			}	
			ret = listen(m_socket, 32);
			if (ret == -1) return -4;
		}

		if (m_param.arrt & SOCK_ISNOBLOCK)
		{
			printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno));
			int option = fcntl(m_socket, F_GETFL);
			if (option < 0) return -5;
			option |= O_NONBLOCK;
			ret = fcntl(m_socket, F_SETFL, option);
			if (ret < 0) return -6;
		}
		if (m_status == 0)
		{
			printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno));
			m_status = 1;
		}
		printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  m_socket = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), m_socket);
		return 0;
	}
	//连接: 服务器：accept 客户端：connect; udp直接返回成功
	virtual int Link(CSocketBase** ppClient = NULL)
	{
		if (m_status <= 0 || m_socket == -1) return -1;
		int ret = 0;
		if (m_param.arrt & SOCK_ISSERVER)
		{
			if (ppClient == NULL)
			{
				return -2;
			}
			CSockParam param;
			sockaddr_un addr_un;
			socklen_t len = sizeof(addr_un);
			int fd = accept(m_socket, param.addrun(), &len);
			//稍等 我看一下线程号 是对 就是这里 accpet成功了
			printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  m_socket = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), m_socket);
			if (fd == -1) return -3;
			//这里就是我说的 appcet完了之后 又创建了一个收数据的socket
			*ppClient = new CLocalSocket(fd);
			if (*ppClient == NULL)
			{
				return -4;
			}
			ret = (*ppClient)->Init(param);
			if (ret != 0)
			{
				printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
				delete* ppClient;
				*ppClient = NULL;
				return -5;
			}
		}
		else
		{
			ret = connect(m_socket, m_param.addrun(), sizeof(sockaddr_un));
			if (ret != 0) return -6;
		}
		m_status = 2;

		//这里socket=6是监听套接字  
		printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s  m_socket = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), m_socket);
		return 0;

	}
	//发送数据
	virtual int Send(const Buffer& data)
	{
		printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s len = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), data.size());
		printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s data = %s\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), data.c_str());
		printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s socket = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), m_socket);
		if (m_status < 2 || m_socket == -1) return -1;
		size_t index = 0;//size_t是无符号的，ssize_t是有符号的
		while (index < data.size())
		{
			ssize_t len = write(m_socket, (char*)data + index, data.size() - index);
			if (len == 0)
			{
				//连接已经关闭了
				return -2;
			}
			else if (len < 0) {
				//发送失败了电话打不了，我这不好说话
				return -3;
			}else
			{
				index += len;
			}
			printf("%s(%d):<%s>  pid = %d errno = %d  msg:%s ret = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), len);
		}
		return 0;
		

	}
	//接收数据
	virtual int Recv(Buffer& data)
	{
		if (m_status <= 0 || m_socket == -1) return -1;
		ssize_t len = read(m_socket, data, data.size());
		if (len > 0)
		{
			data.resize(len);
			return (int)len;
		}
		if (len < 0)
		{
			if (errno == EINTR || errno == EAGAIN)
			{
				//这是非阻塞模式下被中断的情况
				data.clear();
				//没有收到数据
				return 0;
			}
			else
			{
				//发生错误
				return -2;
			}
		}
		//套接字被关闭了
		return -3;
		
	}
	//关闭连接
	virtual int Close() { return CSocketBase::Close(); }

};

