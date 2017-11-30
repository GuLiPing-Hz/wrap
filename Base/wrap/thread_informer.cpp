#include "config.h"
#include "thread_informer.h"
#include "ext/event.h"
#include <assert.h>
#include <memory>
#include <memory.h>

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
		if (mtEventInform)
			delete mtEventInform;
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
		//LOGI("ThreadInformer::dealMessageInner\n");
		if (!mMsgCenter)
			return;

		MSGINFO _msg = { 0 };
		while (mMsgCenter->getMessage(_msg) == 0)
		{
#if CFG_TARGET_PLATFORM != CC_PLATFORM_ANDROID
			std::shared_ptr<char> g((char*)_msg.v);
#else
			std::tr1::shared_ptr<char> lock((char*)_msg.v);
#endif // WIN32
			if (_msg.cmd > MSG_SEND_DATA){
				dealCustomMsg(&_msg);
			}
			else if (_msg.cmd == MSG_SEND_DATA){
				bool success = mMsgCenter->sendToSvr(_msg.con, (const char*)_msg.v, _msg.len) == 0;

				//成功发送数据并且需要服务器回调的，或者压根没成功发送数据的
				if ((success && _msg.back) || !success){
					//超时在js 使用native buffer的时候完全不能使用，需要在js端实现超时的逻辑
					void *mem = malloc(sizeof(ReserveData));
					if (!mem)
						return;

					ReserveData *pRD = new(mem)ReserveData(mMsgCenter);
					pRD->setTimeout(success ? ReserveData::TYPE_TIMEOUT : ReserveData::TYPE_REQFAILED);
					pRD->serverid = _msg.server;
					pRD->seq = _msg.wseq;
					pRD->t = time(NULL);

					//添加到消息中心，注册timeout
					mMsgCenter->addTimeout(_msg.wseq, pRD);
				}
			}
		}
	}
}
