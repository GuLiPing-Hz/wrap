#ifndef REACTOR__H__
#define REACTOR__H__
/*
	注释添加以及修改于 2014-4-4

	Reactor 处理器的接口类

	EventHandlerSet 注册事件的封装类，封装了超时列表，FD列表，以及空闲列表。
	封装了加入，删除的操作。

	NetReactor 实现处理器Reactor的接口,
	里面封装了EventHandlerSet对各种事件的加入删除
	Run里面实现了select网络模型，对可写，可读fd的分发给相应的fdevent handler
	每隔15秒给服务器发送心跳包，保持连接。
	*/
#include <map>
#include <set>
#include <time.h>
#include <functional>

#include "event_handler.h"
#include "mutex.h"

namespace Wrap
{
	//typedef void (* RUNKEEPLIVE)();
	typedef std::function<void()> RUNKEEPLIVE;
	class Reactor
	{
		DISABLE_COPY_CTOR(Reactor);
	public:
		Reactor() :mFuncKeepLive(nullptr){}
		virtual ~Reactor(){}

		virtual int registerTimer(TMEventHandler *pHandler, time_t to) = 0;
		virtual int registerReadEvent(FDEventHandler *pHandler) = 0;
		virtual int registerWriteEvent(FDEventHandler *pHandler) = 0;
		virtual int registerIdle(IdleEventHandler *pHandler) = 0;

		virtual int unRegisterTimer(TMEventHandler *pHandler) = 0;
		virtual int unRegisterEvent(FDEventHandler *pHandler) = 0;
		virtual int unRegisterReadEvent(FDEventHandler *pHandler) = 0;
		virtual int unRegisterWriteEvent(FDEventHandler *pHandler) = 0;
		virtual int unRegisterIdle(IdleEventHandler *pHandler) = 0;

		virtual bool run(){ if (mFuncKeepLive)mFuncKeepLive(); return true; }
		virtual int stop() = 0;

		RUNKEEPLIVE mFuncKeepLive;
	};

	typedef std::map<SOCKET, FDEventHandler* > MAPSOCKETFDH;
	typedef std::set<SOCKET> SETSOCKET;
	typedef std::set<IdleEventHandler*> SETIDLEH;

	class NetReactor : public Reactor
	{
	public:
		class EventHandlerSet
		{
			typedef struct TMEHInfo
			{
				TMEventHandler *handler;
				time_t life;//寿命
				time_t regtime;//注册时间
			}TMEHINFO;
			typedef std::map<int, TMEHINFO > MAPINTTMHINFO;

		public:
			//加入到超时map中，如果发现已经存在则修改
			void addTMEventHandler(TMEventHandler *pHandler, time_t to);
			//从超时map中删除
			void delTMEventHandler(TMEventHandler *pHandler){ if (pHandler)mTMEHMap.erase(pHandler->getTimerID()); }
			//加入到空闲set中
			void addIdleEventHandler(IdleEventHandler *pHandler){ if (pHandler)mIdleEHList.insert(pHandler); }
			//从空闲set中删除
			void delIdleEventHandler(IdleEventHandler *pHandler){ if (pHandler)mIdleEHList.erase(pHandler); }
			//加入到socketmap中
			void addFDEventHandler(FDEventHandler *pHandler){ if (pHandler)mFdEHMap[pHandler->getFD()] = pHandler; }
			//从socketmap中删除
			void delFDEventHandler(SOCKET s){ mFdEHMap.erase(s); }
			//加入关闭set中
			void addCloseSocket(SOCKET s){
				mSetCloseSocket.insert(s);
			}
			//遍历超时列表
			void scan();
			//处理空闲列表
			void idle();
			//处理需要关闭的描述符
			void dealClose();
		public:
			MAPSOCKETFDH mFdEHMap;
		private:
			SETSOCKET    mSetCloseSocket;
			MAPINTTMHINFO mTMEHMap;
			SETIDLEH mIdleEHList;
		};

	public:
		NetReactor();
		virtual ~NetReactor();

		virtual int registerTimer(TMEventHandler *pHandler, time_t to);
		virtual int registerReadEvent(FDEventHandler *pHandler);
		virtual int registerWriteEvent(FDEventHandler *pHandler);
		virtual int registerIdle(IdleEventHandler *pHandler);

		virtual int unRegisterTimer(TMEventHandler *pHandler);
		virtual int unRegisterEvent(FDEventHandler *pHandler);
		virtual int unRegisterReadEvent(FDEventHandler *pHandler);
		virtual int unRegisterWriteEvent(FDEventHandler *pHandler);
		virtual int unRegisterIdle(IdleEventHandler *pHandler);

		//封装
		virtual bool run();
		virtual int stop();
	private:
		fd_set mReadSet;
		fd_set mWriteSet;
		EventHandlerSet mSet;
		bool mIsRunning;
		//简单方便就把所有的事件都用一个临界区
		Mutex mMutex;
	};
}
#endif//REACTOR__H__
