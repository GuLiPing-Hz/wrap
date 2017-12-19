#include "reactor.h"
#include "mutex.h"
#include "funcs.h"
#include "config.h"

#include "config.h"

namespace Wrap
{
//////////////////////////EventHandlerSet 的实现////////////////////////////////////////////////
	void NetReactor::EventHandlerSet::addTMEventHandler(TMEventHandler *pHandler, time_t to)
	{
		if(!pHandler)
			return ;
		std::map<int,TMEHINFO>::iterator it = mTMEHMap.find(pHandler->getTimerID());
		if (it != mTMEHMap.end())//如果存在这个Timer，那么修改下时间
		{
			it->second.life = to;
			it->second.regtime = time(NULL);
		}
		else
		{
			TMEHINFO info;
			info.handler = pHandler;
			info.life = to;
			info.regtime = time(NULL);

			mTMEHMap.insert(std::make_pair(pHandler->getTimerID(),info));
		}
	}
	void NetReactor::EventHandlerSet::dealClose()
    {
		if(mSetCloseSocket.empty())
			return ;

        SETSOCKET::iterator it=mSetCloseSocket.begin();
        for (; it!=mSetCloseSocket.end(); it++)
        {
            SOCKET socket = *it;
			if(socket == SOCKET_ERROR)
				continue;
#ifdef _WIN32
            ::closesocket(socket);
#else
            close(socket);
#endif
        }
        mSetCloseSocket.clear();
    }
	void NetReactor::EventHandlerSet::idle()
	{
		std::set<IdleEventHandler*>::iterator it;
		for(it = mIdleEHList.begin();it != mIdleEHList.end();it++)
			(*it)->onRun();
	}
	void NetReactor::EventHandlerSet::scan()
	{
		static time_t lasttime = 0;//上次的处理时间
		if(lasttime != 0 && (time(NULL) - lasttime) < 1)
			return;

		MAPINTTMHINFO tempMap = mTMEHMap;//拷贝一份
		MAPINTTMHINFO::iterator it,itReal;
		for (it = tempMap.begin(); it != tempMap.end(); it++)
		{
			itReal = mTMEHMap.find(it->first);
			time_t cur = time(NULL);
			if ((cur - it->second.regtime) > it->second.life)
			{
				//先修改一下数据源
				itReal->second.regtime = cur;
				//处理超时
				it->second.handler->onTimeOut();//这里可能会有删除mTMEHMap元素的操作，所以我们必须拷贝map
			}
		}
		lasttime = time(NULL);
	}

///////////////////////NetReactor 的实现///////////////////////////////////////////////////
    NetReactor::NetReactor():mIsRunning(true)
    {
        FD_ZERO(&mReadSet);
        FD_ZERO(&mWriteSet);
    }
    NetReactor::~NetReactor()
    {
    }
	int NetReactor::registerIdle(IdleEventHandler *pHandler)
	{
		Guard lock(mMutex);
		mSet.addIdleEventHandler(pHandler);
		return 0;
	}
	int NetReactor::unRegisterIdle(IdleEventHandler *pHandler)
	{
		Guard lock(mMutex);
		mSet.delIdleEventHandler(pHandler);
		return 0;
	}
	int NetReactor::registerTimer(TMEventHandler *pHandler,time_t to)
	{
		Guard lock(mMutex);
		mSet.addTMEventHandler(pHandler,to);
		return 0;
	}
	int NetReactor::registerReadEvent(FDEventHandler *pHandler)
	{
		Guard lock(mMutex);
        //还没加入读fd_set
		if(FD_ISSET(pHandler->getFD(),&mReadSet) == 0)
			FD_SET(pHandler->getFD(),&mReadSet);//加入写fd_set
		mSet.addFDEventHandler(pHandler);//加入map
		return 0;
	}
	int NetReactor::registerWriteEvent(FDEventHandler *pHandler)
	{
		Guard lock(mMutex);
		//如果不在写fd_set中
		if(FD_ISSET(pHandler->getFD(),&mWriteSet) == 0)
			FD_SET(pHandler->getFD(),&mWriteSet);//加入写fd_set
		mSet.addFDEventHandler(pHandler);//加入map
		return 0;
	}
	
	int NetReactor::unRegisterTimer(TMEventHandler *pHandler)
	{
		Guard lock(mMutex);
		mSet.delTMEventHandler(pHandler);
		return 0;
	}
	int NetReactor::unRegisterReadEvent(FDEventHandler *pHandler)
	{
		Guard lock(mMutex);
        if(FD_ISSET(pHandler->getFD(),&mReadSet))
           FD_CLR(pHandler->getFD(),&mReadSet);
		if(FD_ISSET(pHandler->getFD(),&mWriteSet) == 0)
			mSet.delFDEventHandler(pHandler->getFD());
		return 0;
	}
	int NetReactor::unRegisterWriteEvent(FDEventHandler *pHandler)
	{
		Guard lock(mMutex);
        if(FD_ISSET(pHandler->getFD(),&mWriteSet))
            FD_CLR(pHandler->getFD(),&mWriteSet);
		if(FD_ISSET(pHandler->getFD(),&mReadSet) == 0)
			mSet.delFDEventHandler(pHandler->getFD());
		return 0;
	}
	int NetReactor::unRegisterEvent(FDEventHandler *pHandler)
	{
		Guard lock(mMutex);
        if (FD_ISSET(pHandler->getFD(),&mWriteSet))
            FD_CLR(pHandler->getFD(),&mWriteSet);
        if (FD_ISSET(pHandler->getFD(),&mReadSet))
            FD_CLR(pHandler->getFD(),&mReadSet);
		mSet.delFDEventHandler(pHandler->getFD());
        mSet.addCloseSocket(pHandler->getFD());
		return 0;
	}

	int NetReactor::stop()
	{
		mIsRunning = false;
		return 0;
	}
	bool NetReactor::run()
	{
		mIsRunning = true;

		while(true)
		{
			//执行可能的心跳包
			Reactor::run();

            int maxfd = -1;
            fd_set readset;
            fd_set writeset;
            MAPSOCKETFDH tmpFDMap;
            FD_ZERO(&readset);
            FD_ZERO(&writeset);
            tmpFDMap.clear();
            
            {
				Guard lock(mMutex);
                
				//复制fd_set
                readset = mReadSet;
                writeset = mWriteSet;
                tmpFDMap = mSet.mFdEHMap;
                //把一些无效的描述符关闭
                mSet.dealClose();

                //遍历超时列表，查看是否有socket连接超时
				mSet.scan();
				//空闲处理
				mSet.idle();
            }

            if(tmpFDMap.empty())
            {
                SleepMs(100);
                if(!mIsRunning) {
                    break;
                }
                continue;
            }
            else
            {
                MAPSOCKETFDH::iterator it = tmpFDMap.begin();
                for(;it!=tmpFDMap.end();it++)
                {
                    if (maxfd < (int)it->first)
                        maxfd =(int) it->first;
                }
            }
			maxfd ++;//最大描述符+1
            
            struct timeval tv = {0,50};//50微秒
			int nfds = select(maxfd,&readset,&writeset,NULL,&tv);
			if(nfds == SOCKET_ERROR)
			{
#ifdef _WIN32 
				LOGE("%s : select error :  %d",__FUNCTION__,WSAGetLastError());
#else
				LOGE("%s : select error : %d",__FUNCTION__,errno);
#endif
			}

			if(!mIsRunning && nfds <= 0)
				break;

            MAPSOCKETFDH::iterator it = tmpFDMap.begin();
            for(; it != tmpFDMap.end(); it++)
            {
                SOCKET sock = it->first;

                if(FD_ISSET(sock,&writeset))
                {
                    if(it->second)
                        it->second->onFDWrite();
                }
                if(FD_ISSET(sock,&readset))
                {
                    if(it->second)
                        it->second->onFDRead();
                }
            }
		}
		return false;//结束线程
	}
}
