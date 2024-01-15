#pragma once
#include<pthread.h>
#include<unistd.h>
#include<fcntl.h>
#include"Function.h"

class CThread
{
public:
	CThread();
	~CThread();

	CThread(const CThread&) = delete;
	CThread& operator= (const CThread&) = delete;

private:

};
