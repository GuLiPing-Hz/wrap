// Base.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "mutex.h"

int _tmain(int argc, _TCHAR* argv[])
{
	Wrap::Mutex mutex;
	Wrap::Guard lock(mutex);

	return 0;
}

