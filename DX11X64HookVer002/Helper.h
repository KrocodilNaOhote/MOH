#ifndef HELPER_H
#define HELPER_H

#include <Windows.h>

template<class T> inline void SAFE_RELEASE(T*& p)
{
	if (p)
	{
		p->Release();
		p = nullptr;
	}
}

template<class T> inline void SAFE_DELETE(T*& p)
{
	if (p)
	{
		delete p;
		p = NULL;
	}
}

template<class T> inline void SAFE_DELETE_ARRAY(T*& p)
{
	if (p)
	{
		delete[] p;
		p = NULL;
	}
}

#endif