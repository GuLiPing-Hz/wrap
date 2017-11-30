#ifndef THREADMGR__H__
#define THREADMGR__H__
/*
	注释添加以及修改于 2014-4-2

	封装一个消息循环队列，对发送的信息进行处理。
	消息的的发送有两种方式Post 跟 Send（阻塞等待处理完成）
	提供ProcessMessage纯虚接口，子类实现自己的业务
	*/
//#define _STLP_OUTERMOST_HEADER_ID 0x40
#include <list>
#include "ext/thread.h"

namespace Wrap{
	class Mutex;

	enum
	{
		MSG_INVALID = 0,
	};

	struct _tMsg//定义消息结构
	{
		bool	isSend;//是否立即发送
		int		id;
		void*	data;
	};

	class ThreadMgr
	{
	public:
		ThreadMgr(void);
		virtual ~ThreadMgr(void);

	public:
		//创建线程，指定优先级
		bool createThread(const char* name = 0, eThreadPriority iPriority = kNormalPriority);
		//资源回收
		bool releaseThread();
		//投递消息
		void postMessageOS(int id, void* pData);
		//发送消息
		bool sendMessageOS(int id, void* pData);
		//消息处理
		virtual void processMessage(int id, void* pData) = 0;
		//空闲处理
		virtual void processIdle();
		//线程run函数
		bool run();
	protected:
		//线程函数
		static bool ThreadProc(ThreadObj pData);

	protected:
		volatile bool						mIsStop;
	private:
		ThreadWrapper*						mThread;
		EventWrapper*						mEvent;
		Mutex								mCS;

		std::list<_tMsg>					mLstMsg;
	};

}

#endif//THREADMGR__H__
