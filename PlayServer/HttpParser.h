#pragma once
#include"http_parser.h"
#include"Socket.h"
#include<map>
#include"Public.h"
class CHttpParser
{
public:
	CHttpParser();
	~CHttpParser();
	CHttpParser(const CHttpParser& http);
	CHttpParser& operator=(const CHttpParser& http);

public:
	size_t Parser(const Buffer& data);
	//Get post（参考http  HTTP_METHOD宏）
	unsigned Method() const
	{
		return m_parser.method;
	}
	const std::map<Buffer, Buffer>& Headers()
	{
		return m_HeaderValues;
	}
	const Buffer& Status() const
	{
		return m_status;
	}
	const Buffer& Url() const
	{
		return m_url;
	}
	const Buffer& Body() const
	{
		return m_body;
	}
	unsigned Errno() const
	{
		return m_parser.http_errno;
	}
private:
	http_parser m_parser;
	http_parser_settings m_settings;
	std::map<Buffer, Buffer> m_HeaderValues;
	Buffer m_status;
	Buffer m_url;
	Buffer m_body;
	bool m_complete;
	Buffer m_lastField;

protected:
	static int OnMessageBegin(http_parser* parser);
	static int OnUrl(http_parser* parser, const char* at, size_t length);
	static int OnStatus(http_parser* parser, const char* at, size_t length);
	static int OnHeaderField(http_parser* parser, const char* at, size_t length);
	static int OnHeaderValue(http_parser* parser, const char* at, size_t length);
	static int OnHeadersComplete(http_parser* parser);
	static int OnBody(http_parser* parser, const char* at, size_t length);
	static int OnMessageComplete(http_parser* parser);
	int OnMessageBegin();
	int OnUrl(const char* at, size_t length);
	int OnStatus(const char* at, size_t length);
	int OnHeaderField(const char* at, size_t length);
	int OnHeaderValue(const char* at, size_t length);
	int OnHeadersComplete();
	int OnBody(const char* at, size_t length);
	int OnMessageComplete();
};


class UrlParser
{
public:
	UrlParser(const Buffer& url);
	~UrlParser() {}
	int Parser();
	Buffer operator[](const Buffer& name)const;
	//协议
	Buffer Protocol()const { return m_protocol; }
	//域名
	Buffer Host()const { return m_host; }
	//默认返回80
	int Port()const { return m_port; }
	void SetUrl(const Buffer& url);
private:
	Buffer m_url;
	Buffer m_protocol;
	Buffer m_host;
	Buffer m_uri;
	int m_port;
	std::map<Buffer, Buffer> m_values;
};

