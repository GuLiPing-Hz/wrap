#include "event_handler.h"
#include "reactor.h"
#ifdef WIN32
#include <mstcpip.h>
#else
#endif

namespace Wrap
{
	int IdleEventHandler::registerIdle()
	{
		return getReactor()->registerIdle(this);
	}
	int IdleEventHandler::unRegisterIdle()
	{
		return getReactor()->unRegisterIdle(this);
	}

	int TMEventHandler::registerTimer(time_t to)
	{
		return getReactor()->registerTimer(this, to);
	}
	int TMEventHandler::unRegisterTimer()
	{
		return getReactor()->unRegisterTimer(this);
	}
	
	int FDEventHandler::registerRead()
	{
		return getReactor()->registerReadEvent(this);
	}
	int FDEventHandler::registerWrite()
	{
		return getReactor()->registerWriteEvent(this);
	}
	int FDEventHandler::unRegisterRead()
	{
		return getReactor()->unRegisterReadEvent(this);
	}
	int FDEventHandler::unRegisterWrite()
	{
		return getReactor()->unRegisterWriteEvent(this);
	}
	int FDEventHandler::setSocketParam()
	{
		if (setNonBlocking() == -1)
			return -1;

		//设置发送缓冲大小 尽量别用很多问题的
		int nBuffLen = 128;//子弹数据一个大概70左右,这里最多误差3个子弹
		if (setsockopt(mFD, SOL_SOCKET, SO_SNDBUF, (char *)&nBuffLen, sizeof(int)) == SOCKET_ERROR)
			return -1;

#ifdef WIN32
		BOOL bKeepAlive = TRUE;
		int nRet = setsockopt(mFD, SOL_SOCKET, SO_KEEPALIVE, (char*)&bKeepAlive, sizeof(bKeepAlive));
		// 设置KeepAlive参数
		tcp_keepalive alive_in = { 0 };
		tcp_keepalive alive_out = { 0 };
		alive_in.keepalivetime = 1000;                // //单位为毫秒 开始首次KeepAlive探测前的TCP空闭时间
		alive_in.keepaliveinterval = 1000;            // //单位为毫秒 两次KeepAlive探测间的时间间隔
		alive_in.onoff = TRUE;
		unsigned long ulBytesReturn = 0;
		nRet = WSAIoctl(mFD, SIO_KEEPALIVE_VALS, &alive_in, sizeof(alive_in),
			&alive_out, sizeof(alive_out), &ulBytesReturn, NULL, NULL);
#else
		int keepAlive = 1;   // 开启keepalive属性. 缺省值: 0(关闭)
		int keepIdle = 1;   // 如果在60秒内没有任何数据交互,则进行探测. 缺省值:7200(s)
		int keepInterval = 1;   // 探测时发探测包的时间间隔为5秒. 缺省值:75(s)
		int keepCount = 2;   // 探测重试的次数. 全部超时则认定连接失效..缺省值:9(次)

		setsockopt(mFD, SOL_SOCKET, SO_KEEPALIVE, (void*)&keepAlive, sizeof(keepAlive));
#if defined ANDROID
		//SOL_TCP
		setsockopt(mFD, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle));
		setsockopt(mFD, SOL_TCP, TCP_KEEPINTVL, (void*)&keepInterval, sizeof(keepInterval));
		setsockopt(mFD, SOL_TCP, TCP_KEEPCNT, (void*)&keepCount, sizeof(keepCount));
#else //Apple
		setsockopt(mFD, IPPROTO_TCP, TCP_KEEPALIVE, (void*)&keepIdle, sizeof(keepIdle));
		setsockopt(mFD, IPPROTO_TCP, TCP_KEEPINTVL, (void*)&keepInterval, sizeof(keepInterval));
		setsockopt(mFD, IPPROTO_TCP, TCP_KEEPCNT, (void*)&keepCount, sizeof(keepCount));

		//设置不发送 `SIGPIPE` 信号的 socket 变量
		int value = 1;
		if (setsockopt(mFD, SOL_SOCKET, SO_NOSIGPIPE, &value, sizeof(value)) == SOCKET_ERROR)
			return -1;
#endif
#endif
		return 0;
	}
	//设置地址复用
	int FDEventHandler::setAddrReuse()
	{
		/* REUSEADDR on Unix means, "don't hang on to this address after the
		* listener is closed."  On Windows, though, it means "don't keep other
		* processes from binding to this address while we're using it. */

		//设置地址复用 - 服务器有用
		int on = 1;
		if (setsockopt(mFD, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on)) == SOCKET_ERROR)
			return -1;
		return 0;
	}
	int FDEventHandler::setPortReuse()
	{
#ifndef _WIN32
		int on = 1;
		/* REUSEPORT on Linux 3.9+ means, "Multiple servers (processes or
		* threads) can bind to the same port if they each set the option. */
		if (setsockopt(mFD, SOL_SOCKET, SO_REUSEPORT, (void*)&on, (ev_socklen_t)sizeof(on)) == SOCKET_ERROR)
			return -1;
#endif
		return 0;
	}
	//设置非阻塞 socket
	int FDEventHandler::setNonBlocking()
	{
#ifdef WIN32
		u_long l = 1;//非0：非阻塞；0：阻塞
		if (ioctlsocket(mFD, FIONBIO, &l) == SOCKET_ERROR)
			return -1;
#else 
		int flags = fcntl(mFD, F_GETFL, 0);    
		if(fcntl(mFD, F_SETFL, flags|O_NONBLOCK) == -1)
			return -1;
#endif
		return 0;
	}
	void FDEventHandler::closeSocket()
	{
		if (mFD == INVALID_SOCKET)//如果本身是个非法的网络描述符,那么跳过
			return;

		getReactor()->unRegisterEvent(this);
		mFD = INVALID_SOCKET;
	}
}
