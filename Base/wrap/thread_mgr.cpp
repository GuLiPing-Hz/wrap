#include "thread_mgr.h"
#include "ext/event.h"
#include "mutex.h"
#include "funcs.h"

namespace Wrap{

ThreadMgr::ThreadMgr(void)
: mIsStop( false )
,mEvent(NULL)
,mThread(NULL)
{
	mEvent = EventWrapper::Create();
}

ThreadMgr::~ThreadMgr(void)
{
	if(mThread)
		delete mThread;
	if(mEvent)
		delete mEvent;
}

bool ThreadMgr::createThread(const char* name, eThreadPriority iPriority)
{
	if (mThread){
		//ReleaseThread();
		return true;
	}

	mThread = ThreadWrapper::CreateThread(ThreadProc, this, kNormalPriority, name);
	if(!mThread)
		return false;
	unsigned int thread_id;
	if(!mThread->Start(thread_id))
	{
		return false;
	}
	return true;
}

bool ThreadMgr::releaseThread()
{
	mIsStop = true;
	if (mThread){
		mThread->Stop();
		delete mThread;
		mThread = nullptr;
	}
	return true;
}

void ThreadMgr::postMessageOS( int id, void* pData )
{
	_tMsg msg = { false, id, pData };
	
	{
		Guard lock(mCS);
		mLstMsg.push_back( msg );
	}
}

bool ThreadMgr::sendMessageOS( int id, void* pData )
{
	_tMsg msg = { true, id, pData };

	{
		Guard lock(mCS);
		mLstMsg.push_front( msg );
	}
	

	if(mEvent->Wait(UTIL_EVENT_INFINITE) == kEventSignaled)
	{
		return true;
	}

	return false;
}

void ThreadMgr::processIdle()
{
	SleepMs(50);
}

bool ThreadMgr::ThreadProc(ThreadObj pData)
{
	ThreadMgr * pThis = static_cast<ThreadMgr *>(pData);
	return pThis->run();
}

bool ThreadMgr::run()
{
	while ( !mIsStop )
	{
		bool bIsEmpty = false;
		_tMsg msg = { false, MSG_INVALID, 0 };
		
		{
			Guard lock(mCS);
			bIsEmpty = mLstMsg.empty();
			if ( !bIsEmpty )
			{
				msg = mLstMsg.front();
				mLstMsg.pop_front();
			}
		}

		
		if ( !bIsEmpty && msg.id != MSG_INVALID )//如果不为空，并且消息不为MSG_INVALID,
		{
			processMessage(msg.id,msg.data);
			if ( msg.isSend )
				mEvent->Set();
		}
		else//空闲处理
		{
			processIdle();
		}
	}

	return false;//结束线程
}

}

