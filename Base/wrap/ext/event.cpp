#include "../config.h"
#include "event.h"

#if defined(_WIN32)
#include <Windows.h>
#pragma comment(lib,"winmm.lib")
#include "Mmsystem.h"
#else

#if defined(NETUTIL_MAC) && !defined(NETUTIL_IOS)
#include <ApplicationServices/ApplicationServices.h>
#include <pthread.h>
#else
#endif

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#endif

#ifdef WIN32

EventWindows::EventWindows()
	: event_(CreateEvent(NULL,    // security attributes
	FALSE,   // manual reset
	FALSE,   // initial state
	NULL)),  // name of event
	timerID_(NULL) {
}

EventWindows::~EventWindows() {
	CloseHandle(event_);
}

bool EventWindows::Set() {
	// Note: setting an event that is already set has no effect.
	return SetEvent(event_) == 1 ? true : false;
}

bool EventWindows::Reset() {
	return ResetEvent(event_) == 1 ? true : false;
}

EventTypeWrapper EventWindows::Wait(unsigned long max_time) {
	unsigned long res = WaitForSingleObject(event_, max_time);
	switch (res) {
	case WAIT_OBJECT_0:
		return kEventSignaled;
	case WAIT_TIMEOUT:
		return kEventTimeout;
	default:
		return kEventError;
	}
}

bool EventWindows::StartTimer(bool periodic, unsigned long time) {
	if (timerID_ != NULL) {
		timeKillEvent(timerID_);
		timerID_ = NULL;
	}
	if (periodic) {
		timerID_ = timeSetEvent(time, 0, (LPTIMECALLBACK)HANDLE(event_), 0,
			TIME_PERIODIC | TIME_CALLBACK_EVENT_PULSE);
	}
	else {
		timerID_ = timeSetEvent(time, 0, (LPTIMECALLBACK)HANDLE(event_), 0,
			TIME_ONESHOT | TIME_CALLBACK_EVENT_SET);
	}

	if (timerID_ == NULL) {
		return false;
	}
	return true;
}

bool EventWindows::StopTimer() {
	timeKillEvent(timerID_);
	timerID_ = NULL;
	return true;
}

#else

#define WEBRTC_CLOCK_TYPE_REALTIME

const long int E6 = 1000000;
const long int E9 = 1000 * E6;

/*
 兼容IOS 10.0 才能使用clock_gettime
 */
void clock_gettime_common(struct timespec *__tp){

#ifdef NETUTIL_MAC
	timeval value;
	struct timezone time_zone;
	time_zone.tz_minuteswest = 0;
	time_zone.tz_dsttime = 0;
	gettimeofday(&value, &time_zone);
	TIMEVAL_TO_TIMESPEC(&value, __tp);
#else
#if defined(NETUTIL_IOS)
	if (__builtin_available(iOS 10.0, *)) {
#endif
#ifdef WEBRTC_CLOCK_TYPE_REALTIME
		clock_gettime(CLOCK_REALTIME, __tp);
#else
		clock_gettime(CLOCK_MONOTONIC, __tp);
#endif
#if defined(NETUTIL_IOS)
	} else {
		timeval value;
		struct timezone time_zone;
		time_zone.tz_minuteswest = 0;
		time_zone.tz_dsttime = 0;
		gettimeofday(&value, &time_zone);
		TIMEVAL_TO_TIMESPEC(&value, __tp);
	}
#endif
#endif
}

EventWrapper* EventPosix::Create() {
	EventPosix* ptr = new EventPosix;
	if (!ptr) {
		return NULL;
	}

	const int error = ptr->Construct();
	if (error) {
		delete ptr;
		return NULL;
	}
	return ptr;
}

EventPosix::EventPosix()
	: timer_thread_(0),
	timer_event_(0),
	periodic_(false),
	time_(0),
	count_(0),
	state_(kDown),
	isInWait_(false)
{
}

int EventPosix::Construct() {
	// Set start time to zero
	memset(&created_at_, 0, sizeof(created_at_));

	int result = pthread_mutex_init(&mutex_, 0);
	if (result != 0) {
		return -1;
	}
#ifdef WEBRTC_CLOCK_TYPE_REALTIME
	result = pthread_cond_init(&cond_, 0);
	if (result != 0) {
		return -1;
	}
#else
	pthread_condattr_t cond_attr;
	result = pthread_condattr_init(&cond_attr);
	if (result != 0) {
		return -1;
	}
	result = pthread_condattr_setclock(&cond_attr, CLOCK_MONOTONIC);
	if (result != 0) {
		return -1;
	}
	result = pthread_cond_init(&cond_, &cond_attr);
	if (result != 0) {
		return -1;
	}
	result = pthread_condattr_destroy(&cond_attr);
	if (result != 0) {
		return -1;
	}
#endif
	return 0;
}

EventPosix::~EventPosix() {
	StopTimer();
	pthread_cond_destroy(&cond_);
	pthread_mutex_destroy(&mutex_);
}

bool EventPosix::Reset() {
	if (0 != pthread_mutex_lock(&mutex_)) {
		return false;
	}
	state_ = kDown;
	pthread_mutex_unlock(&mutex_);
	return true;
}

bool EventPosix::Set() {
	if (0 != pthread_mutex_lock(&mutex_)) {
		return false;
	}

	// 	while(isInWait_){
	// 		SleepMs(1);
	// 	}

	state_ = kUp;
	// Release all waiting threads
	pthread_cond_broadcast(&cond_);
	pthread_mutex_unlock(&mutex_);
	return true;
}

EventTypeWrapper EventPosix::Wait(unsigned long timeout) {
	int ret_val = 0;
	if (0 != pthread_mutex_lock(&mutex_)) {
		return kEventError;
	}

	isInWait_ = true;

	if (kDown == state_) {
		if (UTIL_EVENT_INFINITE != timeout) {
			timespec end_at;
			clock_gettime_common(&end_at);
			end_at.tv_sec  += timeout / 1000;
			end_at.tv_nsec += (timeout - (timeout / 1000) * 1000) * E6;

			if (end_at.tv_nsec >= E9) {
				end_at.tv_sec++;
				end_at.tv_nsec -= E9;
			}
			ret_val = pthread_cond_timedwait(&cond_, &mutex_, &end_at);
		} else {
			ret_val = pthread_cond_wait(&cond_, &mutex_);
		}
	}

	state_ = kDown;

	isInWait_ = false;
	pthread_mutex_unlock(&mutex_);

	switch (ret_val) {
	case 0:
		return kEventSignaled;
	case ETIMEDOUT:
		return kEventTimeout;
	default:
		return kEventError;
	}
}

EventTypeWrapper EventPosix::Wait(timespec& wake_at) {
	int ret_val = 0;
	if (0 != pthread_mutex_lock(&mutex_)) {
		return kEventError;
	}

	if (kUp != state_) {
		ret_val = pthread_cond_timedwait(&cond_, &mutex_, &wake_at);
	}
	state_ = kDown;

	pthread_mutex_unlock(&mutex_);

	switch (ret_val) {
	case 0:
		return kEventSignaled;
	case ETIMEDOUT:
		return kEventTimeout;
	default:
		return kEventError;
	}
}

bool EventPosix::StartTimer(bool periodic, unsigned long time) {
	if (timer_thread_) {
		if (periodic_) {
			// Timer already started.
			return false;
		} else  {
			// New one shot timer
			time_ = time;
			created_at_.tv_sec = 0;
			timer_event_->Set();
			return true;
		}
	}

	// Start the timer thread
	timer_event_ = static_cast<EventPosix*>(EventWrapper::Create());
	const char* thread_name = "WebRtc_event_timer_thread";
	timer_thread_ = ThreadWrapper::CreateThread(Run, this, kRealtimePriority,
		thread_name);
	periodic_ = periodic;
	time_ = time;
	unsigned int id = 0;
	if (timer_thread_->Start(id)) {
		return true;
	}
	return false;
}

bool EventPosix::Run(ThreadObj obj) {
	return static_cast<EventPosix*>(obj)->Process();
}

bool EventPosix::Process() {
	if (created_at_.tv_sec == 0) {
		clock_gettime_common(&created_at_);
		count_ = 0;
	}

	timespec end_at;
	unsigned long long time = time_ * ++count_;
	end_at.tv_sec  = created_at_.tv_sec + (time_t)(time / 1000);
	end_at.tv_nsec = created_at_.tv_nsec + (time_t)((time - (time / 1000) * 1000) * E6);

	if (end_at.tv_nsec >= E9) {
		end_at.tv_sec++;
		end_at.tv_nsec -= E9;
	}

	switch (timer_event_->Wait(end_at)) {
	case kEventSignaled:
		return true;
	case kEventError:
		return false;
	case kEventTimeout:
		break;
	}
	if (periodic_ || count_ == 1) {
		Set();
	}
	return true;
}

bool EventPosix::StopTimer() {
	if (timer_thread_) {
		timer_thread_->SetNotAlive();
	}
	if (timer_event_) {
		timer_event_->Set();
	}
	if (timer_thread_) {
		if (!timer_thread_->Stop()) {
			return false;
		}

		delete timer_thread_;
		timer_thread_ = 0;
	}
	if (timer_event_) {
		delete timer_event_;
		timer_event_ = 0;
	}

	// Set time to zero to force new reference time for the timer.
	memset(&created_at_, 0, sizeof(created_at_));
	count_ = 0;
	return true;
}

#endif

EventWrapper* EventWrapper::Create() {
#if defined(_WIN32)
	return new EventWindows();
#else
	return EventPosix::Create();
#endif
}


