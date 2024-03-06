#pragma once
#include<string.h>
#include<string>

class Buffer : public std::string
{
public:
	Buffer(size_t size) : std::string() { resize(size); }
	Buffer(const std::string& str) : std::string(str) {  }
	Buffer(const char* str) : std::string(str) {  }
	Buffer(const char* str, size_t length) : std::string(str) {
		resize(length);
		memcpy((char*)c_str(), str, length);
	}
	Buffer(const char* begin, const char* end) : std::string()
	{
		long int len = end - begin;
		if (len > 0)
		{
			resize(len);
			memcpy((char*)c_str(), begin, len);
		}
	}
	Buffer() : std::string() {}
	~Buffer() {}

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