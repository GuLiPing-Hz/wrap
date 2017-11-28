// Base.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "mutex.h"

int _tmain(int argc, _TCHAR* argv[])
{
	Wrap::Mutex mutex;
	Wrap::Guard lock(mutex);

	return 0;
}

