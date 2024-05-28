#pragma once
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <fcntl.h>
#include "Public.h"


enum SockAttr {
	SOCK_ISSERVER = 1,//�Ƿ������ 1��ʾ�� 0��ʾ�ͻ���
	SOCK_ISNONBLOCK = 2,//�Ƿ����� 1��ʾ������ 0��ʾ����
	SOCK_ISUDP = 4,//�Ƿ�ΪUDP 1��ʾudp 0��ʾtcp
	SOCK_ISIP = 8,//�Ƿ�ΪIPЭ�� 1��ʾIPЭ�� 0��ʾ�����׽���
	SOCK_ISREUSE = 16//�Ƿ����õ�ַ
};

class CSockParam {
public:
	CSockParam() {
		bzero(&addr_in, sizeof(addr_in));
		bzero(&addr_un, sizeof(addr_un));
		port = -1;
		attr = 0;//Ĭ���ǿͻ��ˡ�������tcp
	}
	CSockParam(const Buffer& ip, short port, int attr) {
		this->ip = ip;
		this->port = port;
		this->attr = attr;
		addr_in.sin_family = AF_INET;
		addr_in.sin_port = htons(port);//h host���� n net���� s����short �����ֽ���תΪ�����ֽ���
		addr_in.sin_addr.s_addr = inet_addr(ip);
	}
	CSockParam(const sockaddr_in* addrin, int attr) {
		this->ip = ip;
		this->port = port;
		this->attr = attr;
		memcpy(&addr_in, addrin, sizeof(addr_in));
	}
	CSockParam(const Buffer& path, int attr) {
		ip = path;
		addr_un.sun_family = AF_UNIX;
		strcpy(addr_un.sun_path, path);
		this->attr = attr;
	}
	~CSockParam() {}
	CSockParam(const CSockParam& param) {
		ip = param.ip;
		port = param.port;
		attr = param.attr;
		memcpy(&addr_in, &param.addr_in, sizeof(addr_in));
		memcpy(&addr_un, &param.addr_un, sizeof(addr_un));
	}
public:
	CSockParam& operator=(const CSockParam& param) {
		if (this != &param) {
			ip = param.ip;
			port = param.port;
			attr = param.attr;
			memcpy(&addr_in, &param.addr_in, sizeof(addr_in));
			memcpy(&addr_un, &param.addr_un, sizeof(addr_un));
		}
		return *this;
	}
	sockaddr* addrin() { return (sockaddr*)&addr_in; }
	sockaddr* addrun() { return (sockaddr*)&addr_un; }
public:
	//��ַ
	sockaddr_in addr_in;
	sockaddr_un addr_un;
	//ip
	Buffer ip;
	//�˿�
	short port;
	//�ο�SockAttr
	int attr;
};

class CSocketBase
{
public:
	CSocketBase() {
		m_socket = -1;
		m_status = 0;//��ʼ��δ���
	}
	//������������
	virtual ~CSocketBase() {
		Close();
	}
public:
	//��ʼ�� ������ �׽��ִ�����bind��listen  �ͻ��� �׽��ִ���
	virtual int Init(const CSockParam& param) = 0;
	//���� ������ accept �ͻ��� connect  ����udp������Ժ���
	virtual int Link(CSocketBase** pClient = NULL) = 0;
	//��������
	virtual int Send(const Buffer& data) = 0;
	//��������
	virtual int Recv(Buffer& data) = 0;
	//�ر�����
	virtual int Close() {
		m_status = 3;
		if (m_socket != -1) {
			if ((m_param.attr & SOCK_ISSERVER) && //������
				((m_param.attr & SOCK_ISIP) == 0))//��IP
				unlink(m_param.ip);
			int fd = m_socket;
			m_socket = -1;
			close(fd);
		}
		return 0;
	};
	virtual operator int() { return m_socket; }
	virtual operator int()const { return m_socket; }
	virtual operator const sockaddr_in* ()const { return &m_param.addr_in; }
	virtual operator sockaddr_in* () { return &m_param.addr_in; }
protected:
	//�׽�����������Ĭ����-1
	int m_socket;
	//״̬ 0��ʼ��δ��� 1��ʼ����� 2������� 3�Ѿ��ر�
	int m_status;
	//��ʼ������
	CSockParam m_param;
};

class CSocket
	:public CSocketBase
{
public:
	CSocket() :CSocketBase() {}
	CSocket(int sock) :CSocketBase() {
		m_socket = sock;
	}
	//������������
	virtual ~CSocket() {
		Close();
	}
public:
	//��ʼ�� ������ �׽��ִ�����bind��listen  �ͻ��� �׽��ִ���
	virtual int Init(const CSockParam& param) {
		if (m_status != 0)return -1;
		m_param = param;
		int type = (m_param.attr & SOCK_ISUDP) ? SOCK_DGRAM : SOCK_STREAM;
		if (m_socket == -1) {
			if (param.attr & SOCK_ISIP)
				m_socket = socket(PF_INET, type, 0);
			else
				m_socket = socket(PF_LOCAL, type, 0);
		}
		else
			m_status = 2;//accept�����׽��֣��Ѿ���������״̬
		if (m_socket == -1)return -2;
		int ret = 0;
		if (m_param.attr & SOCK_ISREUSE) {
			int option = 1;
			ret = setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
			if (ret == -1)return -7;
		}
		if (m_param.attr & SOCK_ISSERVER) {
			if (param.attr & SOCK_ISIP)
				ret = bind(m_socket, m_param.addrin(), sizeof(sockaddr_in));
			else
				ret = bind(m_socket, m_param.addrun(), sizeof(sockaddr_un));
			if (ret == -1) return -3;
			ret = listen(m_socket, 32);
			if (ret == -1)return -4;
		}
		if (m_param.attr & SOCK_ISNONBLOCK) {
			int option = fcntl(m_socket, F_GETFL);
			if (option == -1)return -5;
			option |= O_NONBLOCK;
			ret = fcntl(m_socket, F_SETFL, option);
			if (ret == -1)return -6;
		}
		if (m_status == 0)
			m_status = 1;
		return 0;
	}
	//���� ������ accept �ͻ��� connect  ����udp������Ժ���
	virtual int Link(CSocketBase** pClient = NULL) {
		if (m_status <= 0 || (m_socket == -1))return -1;
		int ret = 0;
		if (m_param.attr & SOCK_ISSERVER) {
			if (pClient == NULL)return -2;
			CSockParam param;
			int fd = -1;
			socklen_t len = 0;
			if (m_param.attr & SOCK_ISIP) {
				param.attr |= SOCK_ISIP;
				len = sizeof(sockaddr_in);
				fd = accept(m_socket, param.addrin(), &len);
			}
			else {
				len = sizeof(sockaddr_un);
				fd = accept(m_socket, param.addrun(), &len);
			}
			if (fd == -1)return -3;
			*pClient = new CSocket(fd);
			if (*pClient == NULL)return -4;
			ret = (*pClient)->Init(param);
			if (ret != 0) {
				delete (*pClient);
				*pClient = NULL;
				return -5;
			}
		}
		else {
			if (m_param.attr & SOCK_ISIP)
				ret = connect(m_socket, m_param.addrin(), sizeof(sockaddr_in));
			else
				ret = connect(m_socket, m_param.addrun(), sizeof(sockaddr_un));
			if (ret != 0)return -6;
		}
		m_status = 2;
		return 0;
	}
	//��������
	virtual int Send(const Buffer& data) {
		if (m_status < 2 || (m_socket == -1))return -1;
		ssize_t index = 0;
		while (index < (ssize_t)data.size()) {
			ssize_t len = write(m_socket, (char*)data + index, data.size() - index);
			if (len == 0)return -2;
			if (len < 0)return -3;
			index += len;
		}
		return 0;
	}
	//�������� �����㣬��ʾ���ճɹ� С�� ��ʾʧ�� ����0 ��ʾû���յ����ݣ���û�д���
	virtual int Recv(Buffer& data) {
		if (m_status < 2 || (m_socket == -1))return -1;
		data.resize(1024 * 1024);
		ssize_t len = read(m_socket, data, data.size());
		if (len > 0) {
			data.resize(len);
			return (int)len;//�յ�����
		}
		data.clear();
		if (len < 0) {
			if (errno == EINTR || (errno == EAGAIN)) {//������
				data.clear();
				return 0;//û�������յ�
			}
			return -2;//���ʹ���
		}
		return -3;//�׽��ֱ��ر�
	}
	//�ر�����
	virtual int Close() {
		return CSocketBase::Close();
	}
};

