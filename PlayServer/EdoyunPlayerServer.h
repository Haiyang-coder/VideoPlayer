#pragma once
#include"Server.h"
#include<map>
#include"Loggere.h"
#include"HttpParser.h"
#include "HttpParser.h"
#include "Crypto.h"
#include "MysqlClient.h"
#include "jsoncpp/json.h"

DECLARE_TABLE_CLASS(edoyunLogin_user_mysql, _mysql_table_)
DECLARE_MYSQL_FIELD(TYPE_INT, user_id, NOT_NULL | PRIMARY_KEY | AUTOINCREMENT, "INTEGER", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_VARCHAR, user_qq, NOT_NULL, "VARCHAR", "(15)", "", "")  //QQ号
DECLARE_MYSQL_FIELD(TYPE_VARCHAR, user_phone, DEFAULT, "VARCHAR", "(11)", "'18888888888'", "")  //手机
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_name, NOT_NULL, "TEXT", "", "", "")    //姓名
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_nick, NOT_NULL, "TEXT", "", "", "")    //昵称
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_wechat, DEFAULT, "TEXT", "", "NULL", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_wechat_id, DEFAULT, "TEXT", "", "NULL", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_address, DEFAULT, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_province, DEFAULT, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_country, DEFAULT, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_INT, user_age, DEFAULT | CHECK, "INTEGER", "", "18", "")
DECLARE_MYSQL_FIELD(TYPE_INT, user_male, DEFAULT, "BOOL", "", "1", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_flags, DEFAULT, "TEXT", "", "0", "")
DECLARE_MYSQL_FIELD(TYPE_REAL, user_experience, DEFAULT, "REAL", "", "0.0", "")
DECLARE_MYSQL_FIELD(TYPE_INT, user_level, DEFAULT | CHECK, "INTEGER", "", "0", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_class_priority, DEFAULT, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_REAL, user_time_per_viewer, DEFAULT, "REAL", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_career, NONE, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_password, NOT_NULL, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_INT, user_birthday, NONE, "DATETIME", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_describe, NONE, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_education, NONE, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_INT, user_register_time, DEFAULT, "DATETIME", "", "LOCALTIME()", "")
DECLARE_TABLE_CLASS_EDN()



#define ERR_RETURN(ret, err) if(ret!= 0){TRACEE("ret = %d errno = %d message = [%s]", ret, errno, strerror(errno)); return err;}

#define WARN_CONTINUE(ret) if(ret!= 0){TRACEW("ret = %d", ret); continue;}
class CEdoyunPlayerServer : public CBusiness
{
public:
	CEdoyunPlayerServer(unsigned count):CBusiness()
	{
		m_count = count;
		
	}
	~CEdoyunPlayerServer()
	{
		if (m_db) {
			CDatabaseClient* db = m_db;
			m_db = NULL;
			db->Close();
			delete db;
		}
		m_epoll.Close();
		m_pool.Close();
		for (auto it : m_mapClients)
		{
			if (it.second)
			{
				delete it.second;
			}
		}
		m_mapClients.clear();
	}


	virtual int CBusinessProcess(CProcess* proc)
	{
		int sock = 0;
		int ret = 0;
		ret = SetConnectedcallback(&CEdoyunPlayerServer::Connected, this, std::placeholders::_1);
		ERR_RETURN(ret, -1);
		ret = SetRecvcallback(&CEdoyunPlayerServer::Recived, this, std::placeholders::_1, std::placeholders::_2);
		ERR_RETURN(ret, -2)
		ret = m_epoll.Create(m_count);
		if (ret < 0)
		{
			ERR_RETURN(ret, -1)
		}
		ret = m_pool.Start(m_count);
		if (ret < 0)
		{
			ERR_RETURN(ret, -2)
		}
		for (size_t i = 0; i < m_count; i++)
		{
			ret = m_pool.AddTask(&CEdoyunPlayerServer::ThreadFunc, this);
			if (ret < 0) return -3;
		}
		sockaddr_in addrin;
		while (m_epoll != -1)
		{
			ret = proc->RecvSocket(sock,&addrin);
			if (ret < 0 || sock == 0)
			{
				break;
			}
			CSocketBase* pClient = new CSocket(sock);
			if (pClient == NULL) continue;
			ret = pClient->Init(CSockParam(&addrin, SOCK_ISIP));
			WARN_CONTINUE(ret);
			ret = m_epoll.Add(sock, EpollData((void*)pClient));
			if (m_connnectedcallback)
			{
				(*m_connnectedcallback)(pClient);
			}
			WARN_CONTINUE(ret);
		}
	}
private:
	int ThreadFunc()
	{
		int ret = 0;
		EPEvents events;
		while (m_epoll != -1)
		{
			auto size = m_epoll.WaitEvents(events);
			if (size > 0)
			{
				for (ssize_t i = 0; i < size; i++)
				{
					if (events[i].events & EPOLLERR)
					{
						break;
					}
					else if (events[i].events & EPOLLIN)
					{
						CSocketBase* pClient = (CSocketBase*)events[i].data.ptr;
						if (pClient)
						{
							Buffer data;
							ret = pClient->Recv(data);
							WARN_CONTINUE(ret);
							if (m_recvcallback)
							{
								(*m_recvcallback)(pClient, data);
							}
						}
						

					}

				}
			}
			else if (size < 0)
			{
				break;
			}


		}
		return 0;
	}


	int Connected(CSocketBase* pClient)
	{
		//简单打印客户端的信息
		sockaddr_in* paddr = *pClient;
		TRACEI("client connnected addr %s port %d", inet_ntoa(paddr->sin_addr), paddr->sin_port);
		return 0;
	}

	int Recived(CSocketBase* pClient, const Buffer& data)
	{
		TRACEI("接收到数据！");
		//TODO:主要业务，在此处理
		//HTTP 解析
		int ret = 0;
		Buffer response = "";
		ret = HttpParser(data);
		TRACEI("HttpParser ret=%d", ret);
		//验证结果的反馈
		if (ret != 0) {//验证失败
			TRACEE("http parser failed!%d", ret);
		}
		response = MakeResponse(ret);
		ret = pClient->Send(response);
		if (ret != 0) {
			TRACEE("http response failed!%d [%s]", ret, (char*)response);
		}
		else {
			TRACEI("http response success!%d", ret);
		}
		return 0;
	}
	int HttpParser(const Buffer& data)
	{
		CHttpParser parser;
		size_t size = parser.Parser(data);
		if (size == 0 || (parser.Errno() != 0)) {
			TRACEE("size %llu errno:%u", size, parser.Errno());
			return -1;
		}
		if (parser.Method() == HTTP_GET) {
			//get 处理
			UrlParser url("https://192.168.1.100" + parser.Url());
			int ret = url.Parser();
			if (ret != 0) {
				TRACEE("ret = %d url[%s]", ret, "https://192.168.1.100" + parser.Url());
				return -2;
			}
			Buffer uri = url.Uri();
			TRACEI("**** uri = %s", (char*)uri);
			if (uri == "login") {
				//处理登录
				Buffer time = url["time"];
				Buffer salt = url["salt"];
				Buffer user = url["user"];
				Buffer sign = url["sign"];
				TRACEI("time %s salt %s user %s sign %s", (char*)time, (char*)salt, (char*)user, (char*)sign);
				//数据库的查询
				edoyunLogin_user_mysql dbuser;
				Result result;
				Buffer sql = dbuser.Query("user_name=\"" + user + "\"");
				ret = m_db->Exec(sql, result, dbuser);
				if (ret != 0) {
					TRACEE("sql=%s ret=%d", (char*)sql, ret);
					return -3;
				}
				if (result.size() == 0) {
					TRACEE("no result sql=%s ret=%d", (char*)sql, ret);
					return -4;
				}
				if (result.size() != 1) {
					TRACEE("more than one sql=%s ret=%d", (char*)sql, ret);
					return -5;
				}
				auto user1 = result.front();
				Buffer pwd = *user1->Fields["user_password"]->Value.String;
				TRACEI("password = %s", (char*)pwd);
				//登录请求的验证
				const char* MD5_KEY = "*&^%$#@b.v+h-b*g/h@n!h#n$d^ssx,.kl<kl";
				Buffer md5str = time + MD5_KEY + pwd + salt;
				Buffer md5 = Crypto::MD5(md5str);
				TRACEI("md5 = %s", (char*)md5);
				if (md5 == sign) {
					return 0;
				}
				return -6;
			}
		}
		else if (parser.Method() == HTTP_POST) {
			//post 处理
		}
		return -7;
	}
	Buffer MakeResponse(int ret) {
		Json::Value root;
		root["status"] = ret;
		if (ret != 0) {
			root["message"] = "登录失败，可能是用户名或者密码错误！";
		}
		else {
			root["message"] = "success";
		}
		Buffer json = root.toStyledString();
		Buffer result = "HTTP/1.1 200 OK\r\n";
		time_t t;
		time(&t);
		tm* ptm = localtime(&t);
		char temp[64] = "";
		strftime(temp, sizeof(temp), "%a, %d %b %G %T GMT\r\n", ptm);
		Buffer Date = Buffer("Date: ") + temp;
		Buffer Server = "Server: Edoyun/1.0\r\nContent-Type: text/html; charset=utf-8\r\nX-Frame-Options: DENY\r\n";
		snprintf(temp, sizeof(temp), "%d", json.size());
		Buffer Length = Buffer("Content-Length: ") + temp + "\r\n";
		Buffer Stub = "X-Content-Type-Options: nosniff\r\nReferrer-Policy: same-origin\r\n\r\n";
		result += Date + Server + Length + Stub + json;
		TRACEI("response: %s", (char*)result);
		return result;
	}
private:
	CEpoll m_epoll;
	CThreadPool m_pool;
	std::map<int, CSocketBase*> m_mapClients;
	unsigned m_count;
	CDatabaseClient* m_db;


};
