#include "serversocket.h"
#include "pool.h"
#include <stdlib.h>

namespace Wrap{

	int ListenSocketBase::listen()
	{
		mFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//socket ������
		if (INVALID_SOCKET == mFD)
		{
			LOGE("create socket error %d", errno);
			return -1;
		}
		LOGI("�������������������� fd=%d", mFD);

		if (setAddrReuse() != 0){
			LOGE("setAddrReuse error %d", errno);
			closeSocket();
			return -1;
		}
		LOGI("���������õ�ַ���óɹ�");

		struct sockaddr_in ipAddr = { 0 };
		ipAddr.sin_family = AF_INET;
		ipAddr.sin_port = htons(m_port);
		ipAddr.sin_addr.s_addr = htonl(INADDR_ANY);

		if (0 != bind(mFD, (struct sockaddr*)&ipAddr, sizeof(ipAddr)))
		{
			LOGE("socket bind error %d", errno);
			closeSocket();
			return -1;
		}
		LOGI("�������󶨵�ַ�ɹ�");

		if (::listen(mFD, SOMAXCONN) != 0){
			LOGE("socket listen error %d\n", errno);
			return -1;
		}
		LOGI("�����������ɹ�");

		registerRead();
		return 0;
	}
	void ListenSocketBase::onFDRead()
	{
		sockaddr addr;
		int len = sizeof(addr);
		SOCKET clientSocket = accept(mFD, &addr, &len);
		if (clientSocket == INVALID_SOCKET){
			onAcceptError(errno);
		}
		else {
			onAccept((int)clientSocket);
		}
	}
	ListenSocketBase2::~ListenSocketBase2()
	{
		std::map<int, ClientSocketBase*>* dict = m_pClientMap.getMap();
		std::map<int, ClientSocketBase*>::iterator it = dict->begin();

		while (it != dict->end())
		{
			wrap_delete(ClientSocketBase, it->second);
			it = dict->erase(it);
		}

		ClientList::iterator it_list = m_gClientList.begin();
		while (it_list != m_gClientList.end())
		{
			ClientSocketBase* p = *it_list;
			wrap_delete(ClientSocketBase, p);
			it_list = m_gClientList.erase(it_list);
		}
	}
	void ListenSocketBase2::onAccept(int fd)
	{
		if (m_pClientMap.get(fd) == NULL){//������

			LOGI("�ͻ���������:���� fd = %d,ip = %s", fd, ClientSocketBase::GetPeerIp(fd));

			ClientSocketBase* pClient = getIdleClient();
			if (pClient){
				pClient->setFD(fd);
				pClient->setDecoder(m_pDecoder);
				pClient->setSocketListener(this);
				m_pClientMap.put(fd, pClient);

				//ע�ᵽ����socket���¼�
				pClient->registerRead();
			}
		}
	}
	void ListenSocketBase2::onAcceptError(int code)
	{
		LOGE("%s error = %d", __FUNCTION__, code);
	}
	//���û��...
	bool ListenSocketBase2::onSocketConnect(ClientSocketBase* client)
	{
		LOGI("%s,fd=%d,ip = %s\n", __FUNCTION__, client->getFD(), client->getPeerIp());
		return true;
	}
	//���û��...
	void ListenSocketBase2::onSocketConnectTimeout(ClientSocketBase* client)
	{
		LOGI("%s,fd=%d,ip = %s\n", __FUNCTION__, client->getFD(), client->getPeerIp());
		dealErrClient(client);
	}
	// �����ر�(�����ر�),recv == 0�����
	void ListenSocketBase2::onSocketClose(ClientSocketBase* client)
	{
		LOGI("%s,fd=%d,ip = %s\n", __FUNCTION__, client->getFD(), client->getPeerIp());
		dealErrClient(client);
	}
	// errcodeΪ������(socket�ṩ)
	void ListenSocketBase2::onSocketConnectError(ClientSocketBase* client, int errCode)
	{
		LOGE("%s,fd=%d,ip = %s\n", __FUNCTION__, client->getFD(), client->getPeerIp());
		dealErrClient(client);
	}
	void ListenSocketBase2::onSocketRecvError(ClientSocketBase* client, int errCode)
	{
		LOGE("%s,error = %d,fd=%d,ip = %s\n", __FUNCTION__, errCode, client->getFD(), client->getPeerIp());
		dealErrClient(client);
	}
	void ListenSocketBase2::onSocketSendError(ClientSocketBase* client, int errCode)
	{
		LOGE("%s,error = %d,fd=%d,ip = %s\n", __FUNCTION__, errCode, client->getFD(), client->getPeerIp());
		dealErrClient(client);
	}
	// ��������(errCode����㶨��)
	void ListenSocketBase2::onNetLevelError(ClientSocketBase* client, int errCode)
	{
		LOGE("%s,error = %d,fd=%d,ip = %s\n", __FUNCTION__, errCode, client->getFD(), client->getPeerIp());
		dealErrClient(client);
	}
	void ListenSocketBase2::dealErrClient(ClientSocketBase* client)
	{
		m_pClientMap.del(client->getFD());
		setIdleClient(client);
	}
	ClientSocketBase* ListenSocketBase2::getIdleClient()
	{
		ClientSocketBase* pRet = NULL;

		if (m_gClientList.empty()){
			wrap_new_begin;
			ClientSocketBase* pClient = wrap_new(ClientSocketBase, mReactor);
			pRet = pClient;
		}
		else {
			pRet = m_gClientList.front();
			m_gClientList.pop_front();
		}

		return pRet;
	}
	void ListenSocketBase2::setIdleClient(ClientSocketBase* p)
	{
		m_gClientList.push_back(p);
	}
}

