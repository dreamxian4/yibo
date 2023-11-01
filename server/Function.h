#pragma once
#include <unistd.h>
#include <sys/types.h>
#include <functional>

class CFunctionBase
{
public:
	virtual ~CFunctionBase() {}
	virtual int operator()() = 0;
};

//template<typename _FUNCTION_, typename... _ARGS_>
class CFunction :public CFunctionBase
{
public:
	template<typename _FUNCTION_, typename... _ARGS_>
	CFunction(_FUNCTION_ func, _ARGS_... args)
		:m_binder(std::bind(std::forward<_FUNCTION_>(func), std::forward<_ARGS_>(args)...))
	{}
	virtual ~CFunction() {}
	virtual int operator()() {
		return m_binder();
	}
	//typename std::_Bindres_helper<int, _FUNCTION_, _ARGS_...>::type m_binder;
	std::function<int()> m_binder;
};