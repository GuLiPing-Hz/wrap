#include "wrap_config.h"
#include "thread_informer.h"
#include "ext/event.h"
#include <assert.h>
#include <memory>
#include <memory.h>
#include "pool.h"

#if CFG_TARGET_PLATFORM == CC_PLATFORM_ANDROID
#include <tr1/memory>
#endif

namespace Wrap{

	bool ThreadInformer::sIsInit = false;

#ifndef TEST_UDP
	ThreadInformer::ThreadInformer(MessageCenter* center)
		:mMsgCenter(center)
	{
		assert(mMsgCenter != nullptr);
		mtEventInform = EventWrapper::Create();
	}
#else
	ThreadInformer::ThreadInformer(Wrap::Reactor *pReactor, const char *host, short port)
		: Wrap::UdpSocket(pReactor, host, port)
		, mtEventInform(NULL)
	{
		mtEventInform = EventWrapper::Create();
	}
#endif
	ThreadInformer::~ThreadInformer()
	{
		if (mtEventInform){
			//delete mtEventInform;
			wrap_delete(EventWrapper, mtEventInform);
		}
	}

	int ThreadInformer::init()
	{
#ifndef TEST_UDP
		if (!sIsInit)
			sIsInit = createThread("Informer");
		return sIsInit ? 0 : -1;
#else
		return UdpSocket::init();
#endif//TEST_UDP
	}

	bool ThreadInformer::unInit()
	{
		return releaseThread();
	}

	void ThreadInformer::inform()
	{
#ifndef TEST_UDP
		//通知处理消息
		mtEventInform->Set();
#else
		struct sockaddr_in local;
		local.sin_family = AF_INET;
		local.sin_port = htons(getport()); ///监听端口
		local.sin_addr.s_addr = inet_addr("127.0.0.1"); ///本机
		sendto(mFD,"A",1,0,(struct sockaddr*)&local,sizeof(local));
#endif//TEST_UDP
	}

	void ThreadInformer::clearCurWaitMsg() {
		if (mMsgCenter) {
			mMsgCenter->clearMessage();
		}
	}

	void ThreadInformer::processIdle()
	{
		// 		LOGI("%s : wait for", __FUNCTION__);
		if (mtEventInform->Wait(100) != kEventSignaled)
		{
#if defined(NETUTIL_MAC) || defined(NETUTIL_IOS)
			dealMessageInner();//在IOS底下,由于按了Home,导致没有信号回调,需要我们这里去检查一下
#endif
			return;
		}
		dealMessageInner();
	}

	void ThreadInformer::onFDRead()
	{
#ifdef TEST_UDP
		// add by cx 10-6-3
		struct sockaddr_in from;
		int fromlen =sizeof(from);
		char buf[64];
		recvfrom(mFD,buf,sizeof(buf),0,(struct sockaddr*)&from,&fromlen);
#endif//TEST_UDP
		dealMessageInner();
	}

	void ThreadInformer::dealMessageInner()
	{
		//从队列中读出数据处理
		//LOGI("ThreadInformer::dealMessageInner");
		if (!mMsgCenter)
			return;

		MSGINFO _msg = { 0 };
		while (mMsgCenter->getMessage(_msg) == 0)
		{
			VoidGuard guard(_msg.v);
			if (_msg.cmd > MSG_SEND_DATA){
				dealCustomMsg(&_msg);
			}
			else if (_msg.cmd == MSG_SEND_DATA){
				bool success = mMsgCenter->sendToSvr(_msg.con, (const char*)_msg.v, _msg.len) == 0;

				//成功发送数据并且需要服务器回调的，或者压根没成功发送数据的
				if ((success && _msg.back) || !success){
					//超时在js 使用native buffer的时候完全不能使用，需要在js端实现超时的逻辑
					mMsgCenter->addTimeout(success, &_msg);
				}
			}
		}
	}

	void MessageCenter::clearMessage() {
		Guard lock(mMutex);
		m_requestlist.clear();////清空等待发送请求的消息
		clear();//清空已经发送想消息列表
	}

	void MessageCenter::addTimeout(bool success, MSGINFO* msg){
		wrap_new_begin;
		ReserveData *pRD = wrap_new(ReserveData);
		if (!pRD)
			return;
		pRD->setTimeout(success ? ReserveData::TYPE_TIMEOUT : ReserveData::TYPE_REQFAILED);
		pRD->serverid = msg->server;
		pRD->seq = msg->wseq;
		pRD->t = time(NULL);

		//添加到消息中心，注册timeout
		put(msg->wseq, pRD);
	}

	void MessageCenter::onTimeOut(){
		Wrap::Guard lock(mCS);//安全锁

		//对当前的监听事件检查一遍，看下哪一个超时了
		SeqMap_ThreadSafe<ReserveData*>::iterator it;
		for (it = begin(); it != end();)
		{
			ReserveData* pRD = it->second;
			if (pRD && ((time(NULL) - pRD->t) > pRD->timeout)){//超时，没响应
				onTimeoutData(pRD);
				destroyReserveData(pRD);//释放内存
				it = del(it);//移除
			}
			else{
				it++;
			}
		}
	}

	void MessageCenter::delTimeout(int seq){
		ReserveData **data = get(seq);
		if (data && *data) {
			destroyReserveData(*data);
			del(seq);//移除
		}
	}

	void MessageCenter::destroyReserveData(ReserveData *pRD){
		wrap_free(pRD);
	}
}
