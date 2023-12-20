#pragma once
#include<functional>

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