#ifndef MUTEX_H___
#define MUTEX_H___

#include <mutex>
#include "config.h"

namespace Wrap{

	/*
	��ͬ��mutex; recursive_mutex��ͬһ�̵߳������β�����������mutex��-_-
	*/
	class Mutex : public std::recursive_mutex 
	{
		DISABLE_COPY_CTOR(Mutex);
	public:
		Mutex() :std::recursive_mutex(){}
		virtual ~Mutex(){}
	};

	//std::recursive_timed_mutex ����ͨ��try_lock_for()��try_lock_until()��

	/*
	������lock_guard��unique_lock���˸��õļ�������
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

