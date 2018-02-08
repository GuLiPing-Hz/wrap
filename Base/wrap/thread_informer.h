#ifndef THREADINFORMER__H__
#define THREADINFORMER__H__
/*
	注释添加以及修改于 2014-4-4

	前前人留下了Udp的消息循环，前人留下了Windows静默窗口的消息循环，
	被我处理掉了因为这个不能跨平台

	我提供了一个新的消息循环队列ProcessIdle来处理业务。
	具体业务处理在私有函数dealMessageInner里面。

	CAvaratData 封装了一个机器人的相关信息，但是被裁掉了。。先保留着。
	*/
#include "client_socket.h"
#include "thread_mgr.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "pool.h"
#include "seq_map.h"

namespace Wrap{

	enum eMsgInformer
	{
		MSG_CLEAR_DATA = MSG_INVALID + 1,
		MSG_SEND_DATA = MSG_INVALID + 2,
	};

	struct MSGINFO{

		int server;
		ClientSocket* con;
		int		cmd;
		void*	v;
		int		len;
		bool	back;
		int		wseq;

		MSGINFO& operator=(const MSGINFO& other){
			if (this != &other)
			{
				this->server = other.server;
				this->con = other.con;
				this->cmd = other.cmd;
				this->v = other.v;
				this->len = other.len;
				this->wseq = other.wseq;
				this->back = other.back;
			}
			return *this;
		}
	};

	class MessageCenter;

	class ThreadInformer : public UdpSocket, public ThreadMgr
	{
	public:
#ifndef TEST_UDP
		ThreadInformer(MessageCenter* center);
#else
		ThreadInformer(Wrap::Reactor *pReactor, const char *host = "127.0.0.1", short port = 27876);
#endif
		virtual ~ThreadInformer();
		//通知消息队列处理消息,,,这里消息只是充当事件的角色
		void inform();
		//初始化
		virtual int init();
		virtual bool unInit();
		//UdpSocket
		virtual void onFDRead();
		//CThreadMgr
		virtual void processMessage(int id, void* pData){};
		virtual void processIdle();

		//在连接服务器的时候，我们必须要清空之前的已经发送给服务器的等待消息，否则后续会报错
		void clearCurWaitMsg();
	protected:
		//处理 > MSG_SEND_DATA 的消息 msg.v 无需做处理
		virtual void dealCustomMsg(MSGINFO* msg){}
	private:
		void dealMessageInner();
	private:
		static bool sIsInit;
		MessageCenter* mMsgCenter;
		EventWrapper* mtEventInform;
	};

	class ReserveData;

	class MessageCenter : public TMEventHandler, public SeqMap_ThreadSafe<ReserveData*>
	{
	public:
		MessageCenter(Reactor *pReactor) :TMEventHandler(pReactor){}
		virtual ~MessageCenter(){}

		int postMessage(int serverId, ClientSocket *conn, int cmd, void *v, int len, int seq, bool back = true){
			Guard lock(mMutex);
			if (m_requestlist.size() > 1000)//请求队列最多1000
				return -1;

			char* data = (char*)wrap_calloc(len);
			if (!data)
				return -1;
			memcpy(data, v, len);//拷贝数据

			MSGINFO msg = { 0 };
			msg.server = serverId;
			msg.con = conn;
			msg.cmd = cmd;
			msg.v = (void*)data;
			msg.len = len;
			msg.back = back;
			msg.wseq = seq;// m_Counter.Get();
			m_requestlist.push_back(msg);

			//通知消息处理器处理
			getInformer()->inform();
			return seq;
		}
		int getMessage(MSGINFO& msg){
			Guard lock(mMutex);
			if (!m_requestlist.empty()) {
				msg = m_requestlist.front();
				m_requestlist.pop_front();
				return 0;
			}
			return -1;
		}
		
		int sendToSvr(ClientSocket* pSvr, const char* buf, int len){
			if (pSvr && !pSvr->isConnected())
				return -1;

			return pSvr->sendBuf(buf, len) ? 0 : -1;
		}

		//通知中心
		virtual ThreadInformer* getInformer() = 0;
		//对数据进行处理
		virtual void onTimeoutData(ReserveData* data) = 0;

		void clearMessage();

		virtual void addTimeout(bool success, MSGINFO* msg);

		virtual void onTimeOut();

		virtual void delTimeout(int seq);

		void destroyReserveData(ReserveData *pRD);

	protected:
		//待请求服务器消息列表
		std::list<Wrap::MSGINFO> m_requestlist;
		Mutex mMutex;

		//本身保存的是已经向服务器发送的消息列表
	};

	class ReserveData
	{
		DISABLE_COPY_CTOR(ReserveData);
	public:
		enum eType{
			TYPE_TIMEOUT,
			TYPE_REQFAILED
		};

		ReserveData(){}
		virtual ~ReserveData(){}

		void setTimeout(eType value){
			type = value;
			if (type == TYPE_TIMEOUT){
				timeout = 30;//30秒后回调到上层
			}
			else{
				timeout = 3;//3秒后回调到上层
			}
		}

		eType type;
		int serverid;
		int seq;
		time_t t;
		int timeout;
	};

}

#endif//THREADINFORMER__H__
