#ifndef MUTEX_H___
#define MUTEX_H___

#include <mutex>
#include "config.h"

namespace Wrap{

	/*
	不同于mutex; recursive_mutex在同一线程调用两次不会死锁，而mutex会-_-
	*/
	class Mutex : public std::recursive_mutex 
	{
		DISABLE_COPY_CTOR(Mutex);
	public:
		Mutex() :std::recursive_mutex(){}
		virtual ~Mutex(){}
	};

	//std::recursive_timed_mutex 可以通过try_lock_for()，try_lock_until()。

	/*
	区别于lock_guard；unique_lock给了更好的加锁体验
	*/
	class Guard : public std::unique_lock<Mutex>
	{
		DISABLE_COPY_CTOR(Guard);
	public:
		Guard(Mutex& mutex) :std::unique_lock<Mutex>(mutex){}
		virtual ~Guard(){}
	};
}

#endif//MUTEX_H___

