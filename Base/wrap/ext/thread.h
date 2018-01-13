// System independant wrapper for spawning threads
// Note: the spawned thread will loop over the callback function until stopped.
// Note: The callback function is expected to return every 2 seconds or more
// often.

#ifndef THREAD_WRAPPER__H__
#define THREAD_WRAPPER__H__

#ifdef WIN32
#include <Windows.h>
#else
#include <pthread.h>
#endif // WIN32

#include "../wrap_config.h"
#include "../mutex.h"
#include "../typedefs.h"
#include "event.h"

// Object that will be passed by the spawned thread when it enters the callback
// function.

// Callback function that the spawned thread will enter once spawned.
// A return value of false is interpreted as that the function has no
// more work to do and that the thread can be released.
typedef bool(*ThreadRunFunction)(ThreadObj);

typedef void(*ONTHREADSTART)(unsigned int threadid);
typedef void(*ONTHREADEND)(unsigned int threadid);


enum eThreadPriority {
	kLowPriority = 1,
	kNormalPriority = 2,
	kHighPriority = 3,
	kHighestPriority = 4,
	kRealtimePriority = 5
};

class ThreadWrapper {
public:
	enum { kThreadMaxNameLength = 64 };
	ThreadWrapper() :m_funStart(NULL), m_funEnd(NULL){}
	virtual ~ThreadWrapper() {}

	// Factory method. Constructor disabled.
	//
	// func        Pointer to a, by user, specified callback function.
	// obj         Object associated with the thread. Passed in the callback
	//             function.
	// prio        Thread priority. May require root/admin rights.
	// thread_name  NULL terminated thread name, will be visable in the Windows
	//             debugger.
	static ThreadWrapper* CreateThread(ThreadRunFunction func,
		ThreadObj obj,
		eThreadPriority prio = kNormalPriority,
		const char* thread_name = 0);

	// Get the current thread's kernel thread ID.
	static uint32_t GetThreadId();

	// Non blocking termination of the spawned thread. Note that it is not safe
	// to delete1 this class until the spawned thread has been reclaimed.
	virtual void SetNotAlive() = 0;

	// Tries to spawns a thread and returns true if that was successful.
	// Additionally, it tries to set thread priority according to the priority
	// from when CreateThread was called. However, failure to set priority will
	// not result in a false return value.
	// TODO(henrike): add a function for polling whether priority was set or
	//                not.
	virtual bool Start(unsigned int& id) = 0;

	// Sets the threads CPU affinity. CPUs are listed 0 - (number of CPUs - 1).
	// The numbers in processor_numbers specify which CPUs are allowed to run the
	// thread. processor_numbers should not contain any duplicates and elements
	// should be lower than (number of CPUs - 1). amount_of_processors should be
	// equal to the number of processors listed in processor_numbers.
	virtual bool SetAffinity(const int* processor_numbers,
		const unsigned int amount_of_processors) {
		return false;
	}

	// Stops the spawned thread and waits for it to be reclaimed with a timeout
	// of two seconds. Will return false if the thread was not reclaimed.
	// Multiple tries to Stop are allowed (e.g. to wait longer than 2 seconds).
	// It's ok to call Stop() even if the spawned thread has been reclaimed.
	virtual bool Stop() = 0;
	//INFINITE
	virtual bool WaitFor(unsigned int ms = UTIL_EVENT_INFINITE) = 0;

	virtual bool Terminate(unsigned long ecode) = 0;

	void SetCallBack(ONTHREADSTART start, ONTHREADEND end){ m_funStart = start; m_funEnd = end; }
public:
	ONTHREADSTART m_funStart;
	ONTHREADEND    m_funEnd;
};

#ifdef WIN32

class ThreadWindows : public ThreadWrapper {
public:
	ThreadWindows(ThreadRunFunction func, ThreadObj obj, eThreadPriority prio,
		const char* thread_name);
	virtual ~ThreadWindows();

	virtual bool Start(unsigned int& id);
	bool SetAffinity(const int* processor_numbers,
		const unsigned int amount_of_processors);
	virtual bool Stop();
	virtual void SetNotAlive();
	virtual bool WaitFor(unsigned int ms = UTIL_EVENT_INFINITE);
	virtual bool Terminate(unsigned long ecode);

	static unsigned int WINAPI StartThread(LPVOID lp_parameter);

protected:
	virtual void Run();

private:
	ThreadRunFunction    run_function_;
	ThreadObj            obj_;

	bool                    alive_;
	bool                    dead_;

	// TODO(hellner)
	// do_not_close_handle_ member seem pretty redundant. Should be able to remove
	// it. Basically it should be fine to reclaim the handle when calling stop
	// and in the destructor.
	bool                    do_not_close_handle_;
	eThreadPriority         prio_;
	Wrap::Mutex				critsect_stop_;

	HANDLE                  thread_;
	unsigned int            id_;
	char                    name_[kThreadMaxNameLength];
	bool                    set_thread_name_;

};

#else

int ConvertToSystemPriority(eThreadPriority priority, int min_prio,
	int max_prio);

class ThreadPosix : public ThreadWrapper {
public:
	static ThreadWrapper* Create(ThreadRunFunction func, ThreadObj obj,
		eThreadPriority prio, const char* thread_name);

	ThreadPosix(ThreadRunFunction func, ThreadObj obj, eThreadPriority prio,
		const char* thread_name);
	~ThreadPosix();

	// From ThreadWrapper.
	virtual void SetNotAlive();
	virtual bool Start(unsigned int& id);
	// Not implemented on Mac.
	virtual bool SetAffinity(const int* processor_numbers,
		unsigned int amount_of_processors);
	virtual bool Stop();
	virtual bool WaitFor(unsigned int ms = UTIL_EVENT_INFINITE);
	virtual bool Terminate(unsigned long ecode);

	void Run();

private:
	int Construct();

private:
	ThreadRunFunction   run_function_;
	ThreadObj           obj_;

	// Internal state.
	Wrap::Mutex				crit_state_;  // Protects alive_ and dead_
	bool                    alive_;
	bool                    dead_;
	eThreadPriority          prio_;
	EventWrapper*           event_;

	// Zero-terminated thread name string.
	char                    name_[kThreadMaxNameLength];
	bool                    set_thread_name_;

	// Handle to thread.
#if (defined(NETUTIL_LINUX) || defined(NETUTIL_ANDROID))
	pid_t                   pid_;
#endif
	pthread_attr_t          attr_;
	pthread_t               thread_;
};

#endif // WIN32

#endif  // THREAD_WRAPPER__H__
