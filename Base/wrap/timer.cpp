#include "timer.h"
#include <stdlib.h>

namespace Wrap{
	///////////////////////////////////////////////////////////////////////////////
	// constructor
	///////////////////////////////////////////////////////////////////////////////
	PreciseTimer::PreciseTimer()
	{
#ifdef WIN32
		QueryPerformanceFrequency(&frequency);
		startCount.QuadPart = 0;
		endCount.QuadPart = 0;

		cur_startCount.QuadPart = 0;
		cur_endCount.QuadPart = 0;
#else
		startCount.tv_sec = startCount.tv_usec = 0;
		endCount.tv_sec = endCount.tv_usec = 0;
		cur_startCount.tv_sec = cur_startCount.tv_usec = 0;
		cur_endCount.tv_sec = cur_endCount.tv_usec = 0;
#endif

		stopped = 0;
		startTimeInMicroSec = 0;
		endTimeInMicroSec = 0;
	}



	///////////////////////////////////////////////////////////////////////////////
	// distructor
	///////////////////////////////////////////////////////////////////////////////
	PreciseTimer::~PreciseTimer()
	{
	}



	///////////////////////////////////////////////////////////////////////////////
	// start timer.
	// startCount will be set at this point.
	///////////////////////////////////////////////////////////////////////////////
	void PreciseTimer::start()
	{
		stopped = 0; // reset stop flag
#ifdef WIN32
		QueryPerformanceCounter(&startCount);
#else
		gettimeofday(&startCount, NULL);
#endif
	}

	void PreciseTimer::start2()
	{
		start();
#ifdef WIN32
		QueryPerformanceCounter(&cur_startCount);
#else
		gettimeofday(&cur_startCount, NULL);
#endif
	}


	///////////////////////////////////////////////////////////////////////////////
	// stop the timer.
	// endCount will be set at this point.
	///////////////////////////////////////////////////////////////////////////////
	void PreciseTimer::stop()
	{
		stopped = 1; // set timer stopped flag

#ifdef WIN32
		QueryPerformanceCounter(&endCount);
#else
		gettimeofday(&endCount, NULL);
#endif
	}

	void PreciseTimer::stop2()
	{
		stop();
#ifdef WIN32
		QueryPerformanceCounter(&cur_endCount);
#else
		gettimeofday(&cur_endCount, NULL);
#endif
	}

	double PreciseTimer::getLastElapsedTimerInMilliSec()
	{
		return this->getLastElapsedTimeInMicroSec() * 0.001;
	}

	double PreciseTimer::getLastElapsedTimeInMicroSec()
	{
#ifdef WIN32
		if (!stopped)
			QueryPerformanceCounter(&cur_endCount);

		startTimeInMicroSec = cur_startCount.QuadPart * (1000000.0 / frequency.QuadPart);
		endTimeInMicroSec = cur_endCount.QuadPart * (1000000.0 / frequency.QuadPart);
#else
		if (!stopped)
			gettimeofday(&endCount, NULL);

		startTimeInMicroSec = (cur_startCount.tv_sec * 1000000.0) + cur_startCount.tv_usec;
		endTimeInMicroSec = (cur_endCount.tv_sec * 1000000.0) + cur_endCount.tv_usec;
#endif
		cur_startCount = cur_endCount;
		return endTimeInMicroSec - startTimeInMicroSec;

	}

	///////////////////////////////////////////////////////////////////////////////
	// compute elapsed time in micro-second resolution.
	// other getElapsedTime will call this first, then convert to correspond resolution.
	///////////////////////////////////////////////////////////////////////////////
	double PreciseTimer::getElapsedTimeInMicroSec()
	{
#ifdef WIN32
		if (!stopped)
			QueryPerformanceCounter(&endCount);

		startTimeInMicroSec = startCount.QuadPart * (1000000.0 / frequency.QuadPart);
		endTimeInMicroSec = endCount.QuadPart * (1000000.0 / frequency.QuadPart);
#else
		if (!stopped)
			gettimeofday(&endCount, NULL);

		startTimeInMicroSec = (startCount.tv_sec * 1000000.0) + startCount.tv_usec;
		endTimeInMicroSec = (endCount.tv_sec * 1000000.0) + endCount.tv_usec;
#endif

		return endTimeInMicroSec - startTimeInMicroSec;
	}



	///////////////////////////////////////////////////////////////////////////////
	// divide elapsedTimeInMicroSec by 1000
	///////////////////////////////////////////////////////////////////////////////
	double PreciseTimer::getElapsedTimeInMilliSec()
	{
		return this->getElapsedTimeInMicroSec() * 0.001;
	}



	///////////////////////////////////////////////////////////////////////////////
	// divide elapsedTimeInMicroSec by 1000000
	///////////////////////////////////////////////////////////////////////////////
	double PreciseTimer::getElapsedTimeInSec()
	{
		return this->getElapsedTimeInMicroSec() * 0.000001;
	}



	///////////////////////////////////////////////////////////////////////////////
	// same as getElapsedTimeInSec()
	///////////////////////////////////////////////////////////////////////////////
	double PreciseTimer::getElapsedTime()
	{
		return this->getElapsedTimeInSec();
	}

}
