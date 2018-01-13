#ifndef EVENTHANDLER__H__
#define EVENTHANDLER__H__

/*
	注释添加以及修改于 2014-4-2

	封装一个对事件处理的接口类 EventHandler
	IdleEventHandler 提供空闲处理接口
	TMEventHandler 提供超时处理接口
	FDEventHandler 封装了一个描述符 提供fd的相关读写接口
	*/

#ifdef WIN32
#include <Winsock2.h> //需要在引入windows之前，引入该文件，否则会报错
#include <WS2tcpip.h>
#pragma comment(lib,"Ws2_32.lib")
#else
typedef int SOCKET;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#endif

#include <time.h>
#include "wrap_config.h"

namespace Wrap
{
	class Reactor;
	//提供一个反应器
	class EventHandler
	{
		DISABLE_COPY_CTOR(EventHandler);
	public:
		EventHandler() :mReactor(NULL){}
		virtual ~EventHandler(){}

		void setReactor(Reactor *pReactor){ mReactor = pReactor; }
		Reactor* getReactor(){ return mReactor; }
	protected:
		Reactor *mReactor;
	};
	//对空闲的处理
	class IdleEventHandler : virtual public EventHandler
	{
	public:
		IdleEventHandler(Reactor *pReactor)
		{
			setReactor(pReactor);
		}
		virtual ~IdleEventHandler(){}

		virtual void onRun() = 0;
		int registerIdle();
		int unRegisterIdle();
	};
	//对Timer的处理
	class TMEventHandler : virtual public EventHandler
	{
	public:
		TMEventHandler() :mID(0){}
		TMEventHandler(Reactor *pReactor) :mID(0) { setReactor(pReactor); }
		virtual ~TMEventHandler() {}

		//超时处理函数
		virtual void onTimeOut() = 0;
		int registerTimer(time_t to);
		int unRegisterTimer();
		void setTimerID(int id) { mID = id; }
		int getTimerID() { return mID; }
	private:
		int mID;
	};
	//对socket事件的处理
	class FDEventHandler : virtual public EventHandler
	{
	public:
		virtual ~FDEventHandler() {}
		FDEventHandler() :mFD(INVALID_SOCKET) {}
		FDEventHandler(Reactor *pReactor) :mFD(INVALID_SOCKET)  { setReactor(pReactor); }
		
		//fd 读的时候 可接受
		virtual void onFDRead() = 0;
		//fd 写的时候 可发送
		virtual void onFDWrite() = 0;

		virtual void closeSocket();

		int registerRead();
		int registerWrite();
		int unRegisterRead();
		int unRegisterWrite();

		inline void setFD(SOCKET fd) { mFD = fd; }
		inline SOCKET getFD() const { return mFD; }

		int setAddrReuse();//设置地址复用
		int setPortReuse();//端口复用，linux
		int setNonBlocking();//设置非阻塞

		int setSocketParam();
	protected:
		SOCKET mFD;
	};
}

#endif//EVENTHANDLER__H__
