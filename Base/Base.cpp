// Base.cpp : 定义控制台应用程序的入口点。
//
#include "stdafx.h"
#include "wrap/client_socket.h"
#include "wrap/mutex.h"
#include "wrap/timer.h"
#include "wrap/pool.h"
#include "wrap/seq_map.h"
#include <memory>

static void print_number(double x) {
	double y = x;
	int c = 0;
	if (y < 0.0) {
		y = -y;
	}
	while (y < 100.0) {
		y *= 10.0;
		c++;
	}
	printf("%.*f", c, x);
}

static void run_benchmark(char *name, void(*benchmark)(void*), void(*setup)(void*), void(*teardown)(void*), void* data, int count, int iter) {

	Wrap::PreciseTimer timer;
	int i;
	double min = HUGE_VAL;
	double sum = 0.0;
	double max = 0.0;
	for (i = 0; i < count; i++) {
		double begin, total;
		if (setup != NULL) {
			setup(data);
		}

		timer.start();
		benchmark(data);
		timer.stop();
		total = timer.getElapsedTimeInSec();
		if (teardown != NULL) {
			teardown(data);
		}
		if (total < min) {
			min = total;
		}
		if (total > max) {
			max = total;
		}
		sum += total;
	}
	printf("%s: min ", name);
	print_number(min * 1000000000.0 / iter);
	printf("ns / avg ");
	print_number((sum / count) * 1000000000.0 / iter);
	printf("ns / max ");
	print_number(max * 1000000000.0 / iter);
	printf("ns\n");
}


class PoolObj2 : public Wrap::PoolObj{
public:
	PoolObj2(){
		LOGI("PoolObj2");
	}
	virtual ~PoolObj2(){
		LOGI("~PoolObj2");
	}
};

int _tmain(int argc, _TCHAR* argv[])
{
	WORD wVersion;
	WSADATA WSAData;
	wVersion = MAKEWORD(2, 2);
	WSAStartup(wVersion, &WSAData);

	Wrap::Mutex mutex;
	Wrap::Guard lock(mutex);

	//SetLogToFile("xx");
	LOGI("xxxx\n");


// 	void* M_temp = NULL;
// 	PoolObj2* pool = (M_temp = Wrap::Allocator::GetInstance()->alloc(sizeof(PoolObj2), __FILE__, __LINE__)) == NULL ? NULL : new (M_temp)PoolObj2();

	//测试父类指针使用new_和delete_
	wrap_new_begin;
	Wrap::PoolObj* pool = wrap_new(PoolObj2);
	
// 	if (pool != NULL){
// 		pool->~PoolObj();
// 		Wrap::Allocator::GetInstance()->dealloc(pool);
// 		pool = NULL;
// 	}
	wrap_delete(PoolObj, pool);
	
	void* p = wrap_calloc(sizeof(short) * 10);
	wrap_free(p);

	{
		LOGI("++++++++ 测试自动内存释放是否能成功\n");
		Wrap::VoidGuard guard(wrap_calloc(sizeof(float) * 10));
		Wrap::ClsGuard<PoolObj2> guard2(wrap_new(PoolObj2));
		LOGI("~~~~~~~\n");
	}

	std::shared_ptr<Wrap::PoolObj> p2(new Wrap::PoolObj());//官方标准智能指针，在linux下忽略，是另一种写法


	/*
	根据域名获取ipv4地址
	*/
	const char* ip = Wrap::ClientSocket::GetIpv4FromHostName("baidu.com");
	ip = Wrap::ClientSocket::GetIpFromHost("qq.com", false);

	WSACleanup();

	return 0;
}

