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
/**

std::_Bindres_helper<int, _FUNCTION, _ARGS...>::type �� C++ ��׼���е�һ��ģ�����ͣ�ͨ������ʵ�ְ�(bind)������������͵�Ŀ�����ڰ󶨲����б���󶨵Ĳ����ͺ������Ա����Ժ���á�

�����ǲ��������͵ĸ������֣�

int: ���ʾ�󶨲����ķ������͡����������һ���������ͣ���ʵ���ϣ���������κ��������ķ������ͣ�����ȡ������󶨵ĺ�����

_FUNCTION: ���ǰ󶨵ĺ�����ɵ��ö��󡣿�����һ������ָ�롢�������������һ��lambda���ʽ��

_ARGS...: ���ǰ󶨲����Ĳ���������ʾ�����������������������ȡ������󶨵ĺ�����Ҫ�Ĳ���������

std::_Bindres_helper<...>::type: ����һ���������ͣ����ڱ���͹���󶨲�������Ϣ�������������͡�������ɵ��ö����Լ�������

�ۺ�������std::_Bindres_helper<int, _FUNCTION, _ARGS...>::type ��ʾһ���󶨲��������ͣ����а����˷������͡��󶨵ĺ�����ɵ��ö����Լ���صĲ�����Ϣ����ͨ������ʹ��std::bind�������к�����ʱ�����ڱ�ʾ�󶨵Ľ�����͡�
*/

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