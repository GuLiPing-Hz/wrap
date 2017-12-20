#ifndef GLP_SERVERSOCKET_H_
#define GLP_SERVERSOCKET_H_

#include "client_socket.h"
#include "seq_map.h"
#include <list>

namespace Wrap{
	class DataDecoderBase;
	class Reactor;
	class Counter;

	typedef SeqMap<ClientSocketBase*> ClientMap;
	typedef std::list<ClientSocketBase*> ClientList;

	class ListenSocketBase : public FDEventHandler
	{
	public:
		ListenSocketBase(int port, Reactor *pReactor, int timeout = 3)
			:FDEventHandler(pReactor), m_port(port)
		{
			m_timeout = timeout;
			if (m_timeout > 2100)
				m_timeout = 2100;
		}
		virtual ~ListenSocketBase(){}

		int listen();
		virtual void onFDRead();
		virtual void onAccept(int fd) = 0;
		virtual void onFDWrite(){}

	public:
		virtual void onAcceptError(int code) = 0;
	protected:
		int m_port;
		int m_timeout;
	};

	class ListenSocketBase2 : public ListenSocketBase, public OnSocketListener
	{
	public:
		ListenSocketBase2(int port, Reactor *pReactor, DataDecoderBase*pDecoder, int timeout = 3)
			: ListenSocketBase(port, pReactor, timeout), m_pDecoder(pDecoder)
		{}
		virtual ~ListenSocketBase2();

		virtual void onAccept(int fd);
		virtual void onAcceptError(int code);

		/*监听来自客户端的socket状态*/
		// 连接成功
		virtual bool onSocketConnect(ClientSocketBase* client);
		// 连接超时
		virtual void onSocketConnectTimeout(ClientSocketBase* client);
		// 正常关闭(被动关闭),recv == 0的情况
		virtual void onSocketClose(ClientSocketBase* client);
		// errcode为错误码(socket提供)
		virtual void onSocketConnectError(ClientSocketBase* client, int errCode);
		virtual void onSocketRecvError(ClientSocketBase* client, int errCode);
		virtual void onSocketSendError(ClientSocketBase* client, int errCode);
		// 网络层错误(errCode网络层定义)
		virtual void onNetLevelError(ClientSocketBase* client, int errCode);

	protected:
		virtual void dealErrClient(ClientSocketBase* client);

	private:
		ClientSocketBase* getIdleClient();
		void setIdleClient(ClientSocketBase* p);
	protected:
		DataDecoderBase* m_pDecoder;
		//活跃在线的客户端对象Map
		ClientMap m_pClientMap;
		//空闲客户端对象内存池
		ClientList m_gClientList;
	};

	/*
	class ListenSocket : public ListenSocketBase2,public ObjectManager<ClientSocket>
	{
	public:
	ListenSocket(int port,Reactor *pReactor,ObjectAllocator<ClientSocket> * pObjectAllocator
	,Counter *pCounter,DataDecoderBase *pDecoder,ClientMap *pClientMap,int timeout = 3) :
	ListenSocketBase2(port,pReactor,pCounter,pDecoder,pClientMap,timeout),
	ObjectManager<ClientSocket>(pObjectAllocator)
	{}
	virtual ClientSocket* CreateClient()
	{
	return Create();
	}
	virtual ~ListenSocket(){}
	};


	class UdpListenSocket : public FDEventHandler
	{
	public:
	UdpListenSocket(Reactor *pReactor,int port)
	:FDEventHandler(pReactor),m_port(port)
	{
	}
	int Listen();
	virtual void OnFDRead();
	virtual void OnFDWrite(){}
	private:
	int m_port;
	};
	*/

}

#endif//GLP_SERVERSOCKET_H_
