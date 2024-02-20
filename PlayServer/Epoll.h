#pragma once
#include<unistd.h>
#include<sys/epoll.h>
#include<vector>
#include<errno.h>
#include<sys/signal.h>
#include<memory.h>

#define SUCCESS 0
#define EVENTS_SIZE 1024
class EpollData
{
public:
	EpollData(){ m_data.fd = 0; }
	EpollData(void* ptr){ m_data.ptr = ptr; }
	explicit EpollData(int fd) { m_data.fd = fd; }
	explicit EpollData(uint64_t u64) { m_data.u64 = u64; }
	explicit EpollData(uint32_t u32) { m_data.u32 = u32; }
	EpollData(const EpollData& epoll) 
	{ 
		m_data.u64 = epoll.m_data.u64;//联合体的赋值，直接给最大的赋值
	}
	~EpollData(){}

public:
	EpollData& operator=(const EpollData& epoll)
	{
		if (this != &epoll)
		{
			m_data = epoll.m_data;
		}
		return *this;
	}
	EpollData& operator=(void* data)
	{
		m_data.ptr = data;
		return *this;
	}
	EpollData& operator=(int data)
	{
		m_data.fd = data;
		return *this;
	}
	EpollData& operator=(uint32_t data)
	{
		m_data.u32 = data;
		return *this;
	}
	EpollData& operator=(uint64_t data)
	{
		m_data.u64 = data;
		return *this;
	}
	operator epoll_data_t() const { return m_data; }//一个函数体被定义为const后，一个const的对象才能调用这个函数
	operator epoll_data_t()  { return m_data; }
	operator epoll_data_t*()  { return &m_data; }
	operator const epoll_data_t*() const { return &m_data; }
private:
	epoll_data_t m_data;

private:

};

using EPEvents = std::vector<epoll_event>;



class CEpoll
{
public:

	CEpoll() { m_epoll = 0; }
	~CEpoll() { Close(); }
	CEpoll(const CEpoll&) = delete;
	CEpoll& operator= (const CEpoll&) = delete;
public:
	operator int() const { return m_epoll; }
public:
	//创建epoll
	int Create(unsigned count)
	{
		if (m_epoll < 0)
		{
			return -1;
		}
		m_epoll = epoll_create(count);
		if (m_epoll < 0)
		{
			return -2;
		}
		return SUCCESS;
	}

	//设置epoll等待
	ssize_t WaitEvents(EPEvents& events, int timeout = 0)
	{
		if (m_epoll < 0) return m_epoll;
		EPEvents evs(EVENTS_SIZE);
		int ret = epoll_wait(m_epoll, evs.data(), static_cast<int>(evs.size()), timeout);
		if (ret < 0)
		{
			if (errno == EINTR || errno == EAGAIN)
			{
				return 0;
			}
			return -2;
		}
		if (ret > static_cast<int>(events.size()) )
		{
			events.resize(ret);
		}
		memcpy(events.data(), evs.data(), sizeof(epoll_event) * ret);
		return ret;

	}

	//添加eopoll事件
	int Add(int fd, const EpollData& data = EpollData((void*)0), uint32_t events = EPOLLIN)
	{
		if (m_epoll < 0) return m_epoll;
		epoll_event ev = { events , data };
		int ret = epoll_ctl(m_epoll, EPOLL_CTL_ADD, fd, &ev);
		if (ret < 0 ) return -2;
		return 0;
	}
	int Modify(int fd, const EpollData& data = EpollData((void*)0), uint32_t events = EPOLLIN)
	{
		if (m_epoll < 0) return m_epoll;
		epoll_event ev = { events , data };
		int ret = epoll_ctl(m_epoll, EPOLL_CTL_MOD, fd, &ev);
		if (ret < 0) return -2;
		return 0;
	}
	int Del(int fd)
	{
		if (m_epoll < 0) return m_epoll;
		int ret = epoll_ctl(m_epoll, EPOLL_CTL_DEL, fd, NULL);
		if (ret < 0) return -2;
		return 0;
	}
	void Close()
	{
		if (m_epoll != -1)
		{
			int fd = m_epoll;
			m_epoll = -1;
			close(fd);
		}
	}


private:
	int m_epoll;
};

