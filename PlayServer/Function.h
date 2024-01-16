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

std::_Bindres_helper<int, _FUNCTION, _ARGS...>::type 是 C++ 标准库中的一个模板类型，通常用于实现绑定(bind)操作。这个类型的目的是在绑定操作中保存绑定的参数和函数，以便在稍后调用。

让我们拆解这个类型的各个部分：

int: 这表示绑定操作的返回类型。在这里，它是一个整数类型，但实际上，这可以是任何你期望的返回类型，具体取决于你绑定的函数。

_FUNCTION: 这是绑定的函数或可调用对象。可以是一个函数指针、函数对象或者是一个lambda表达式。

_ARGS...: 这是绑定操作的参数。它表示可以有零个或多个参数，具体取决于你绑定的函数需要的参数数量。

std::_Bindres_helper<...>::type: 这是一个辅助类型，用于保存和管理绑定操作的信息，包括返回类型、函数或可调用对象以及参数。

综合起来，std::_Bindres_helper<int, _FUNCTION, _ARGS...>::type 表示一个绑定操作的类型，其中包含了返回类型、绑定的函数或可调用对象以及相关的参数信息。这通常是在使用std::bind函数进行函数绑定时，用于表示绑定的结果类型。
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