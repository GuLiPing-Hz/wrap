#include "client_socket.h"
#include "data_decoder.h"

namespace Wrap {
    void ClientSocketBase::onFDRead() {
        char buf[65535] = {0};/* 16*1024 */
        int len = (int) ::recv(mFD, buf, sizeof(buf), 0);//接收网络数据
		
        if (len == 0) {
            closeSocket();
            onSocketClose();
            return;
        }

        if (len == SOCKET_ERROR) {
#ifdef WIN32
            DWORD derrno = GetLastError();
            if (derrno != WSAEWOULDBLOCK)
            {
                closeSocket();
                onSocketRecvError(derrno);
            }
#else//Linux
            int errorcode = errno;
            if (errorcode != EAGAIN) {
                closeSocket();
                onSocketRecvError(errorcode);
            }
#endif
            return;
        }

        if (!mDecoder)
            return;

        if (mRecvdata.append(buf, len) != len) {
            closeSocket();
            onNetLevelError(EC_RECV_BUFFER);
            return;
        }
        //解析网络数据，不足一个包的部分会有相应处理
        if (mDecoder->process(this) < 0) {
            closeSocket();
            onNetLevelError(EC_STREAM);
            return;
        }
    }

    void ClientSocketBase::onFDWrite() {
		Guard lock(mMutex);
        unsigned int buflen = mSenddata.getPos();
        int len = (int)::send(mFD, mSenddata.getBuf(), (int) buflen, 0);
        //LOGI("%s len = %d", __FUNCTION__, len);
        if (len == SOCKET_ERROR) {
#ifdef WIN32
            DWORD derrno = GetLastError();
            if( derrno != WSAEWOULDBLOCK )
            {
                closeSocket();
                onSocketSendError(derrno);
            }
#else//Linux
            if (errno != EAGAIN) {
                closeSocket();
                onSocketSendError(errno);
            }
#endif
            return;
        }
        if ((unsigned int) len == buflen) {
            mSenddata.initPos();
            unRegisterWrite();
            return;
        } else if ((unsigned int) len < buflen) {
            mSenddata.copy(0, mSenddata.getBuf() + len, buflen - len);
        }
    }

    int ClientSocketBase::addBuf(const char *buf, unsigned int buflen) {
		Guard lock(mMutex);
        if (mSenddata.append(buf, buflen) != buflen) {
            LOGE("%s : SendData Append Failed", __FUNCTION__);
            return -1;
        }
        if (registerWrite() != 0) {
            LOGE("%s : RegisterWrite Failed", __FUNCTION__);
            return -1;
        }
        return 0;
    }

    const char *ClientSocketBase::getPeerIp() {
		return GetPeerIp(mFD);
    }

	const char* ClientSocketBase::GetPeerIp(int fd){
		sockaddr_in addr;
#ifdef WIN32
		int len = sizeof(sockaddr_in);
#elif defined(NETUTIL_ANDROID)
		socklen_t len = sizeof(sockaddr_in);
#elif defined(NETUTIL_IOS)
		unsigned int len = sizeof(sockaddr_in);
#endif
		getpeername(fd, (struct sockaddr *) &addr, &len);
		static char ip[100];
		//strncpy(ip, inet_ntoa(addr.sin_addr), sizeof(ip));
		inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
		return ip;
	}

	const char* ClientSocketBase::GetIpFromHost(const char* host, bool is_ipv6){
		static char sIp[100] = { 0 };
		memset(sIp, 0, sizeof(sIp));
		if (!host)
			return sIp;

		// 返回地址信息 - 废弃
		//         hostent *host = gethostbyname(host);
		// 		if (!host) {
		// 			LOGE("%s : gethostbyname return null[%s], check the internet permission or internet connect.",
		// 				__FUNCTION__, host);
		// 			return sIp;
		// 		}
		// 		// 解析地址信息
		// 		for (char **p = host->h_addr_list; *p; p++) {
		// 			char *temp;
		// 			temp = inet_ntoa(*(struct in_addr *) *p);
		// 			strcpy(sIp, temp);
		// 			break;//只解析第一个地址
		// 		}

		int net_family = is_ipv6 ? AF_INET6 : AF_INET;
		struct addrinfo *answer, hint, *curr;
		memset(&hint, 0, sizeof(hint));
		hint.ai_family = net_family;
		hint.ai_socktype = SOCK_STREAM;

		int ret = getaddrinfo(host, NULL, &hint, &answer);
		if (ret != 0) {
			LOGE("%s : gethostbyname return null[%s], check the internet permission or internet connect.",
				__FUNCTION__, host);
			return sIp;
		}

		for (curr = answer; curr != NULL; curr = curr->ai_next) {
			inet_ntop(net_family, &(((struct sockaddr_in *)(curr->ai_addr))->sin_addr), sIp, sizeof(sIp));
			break;//只解析第一个地址
		}
		freeaddrinfo(answer);

		return sIp;
	}

    const char *ClientSocketBase::GetIpv4FromHostName(const char *name) {
		return GetIpFromHost(name);
    }

    void ClientSocketBase::closeSocket() {
        mIsClosed = true;
        mRecvdata.initPos();
        mSenddata.initPos();
        FDEventHandler::closeSocket();
    }

/////////////////////////ClientSocket 实现/////////////////////////////////////////////////
    void ClientSocket::onTimeOut() {
        mIsWaitingConnectComplete = false;

        //如果socket超时，从超时列表中注销掉
        unRegisterTimer();//注销timer，从连接超时列表中去除
        unRegisterWrite();//注销写fd_set

        ClientSocketBase::closeSocket();
        onSocketConnectTimeout();
    }

    void ClientSocket::closeSocket() {
        /*
            有可能Connect的时候没有网，网卡禁用的时候Connect居然返回成功，
            这时候需要注销超时事件,但是既没有连接上，也没有在等待连接完成，所以去掉这个if语句
        */
        //if (m_isConnected || m_isWaitingConnectComplete)
        {
            mIsConnected = false;
            mIsWaitingConnectComplete = false;
            ClientSocketBase::closeSocket();
            TMEventHandler::unRegisterTimer();
        }
    }

    void ClientSocket::onFDWrite() {
        if (mIsConnected)//如果已经连接上了
        {
            ClientSocketBase::onFDWrite();
            return;
        }
        //如果是第一次回调，则说明网络描述符可用了
        mIsConnected = true;
        mIsWaitingConnectComplete = false;
        open();
        unRegisterTimer();//注销timer，从连接超时列表中去除
        unRegisterWrite();//注销写fd_set

        if (onSocketConnect())
            registerRead();
    }

    int ClientSocket::connectTo(const char *host, short port, int to) {
        //已经连接，或者正在连接
        if (mIsConnected || mIsWaitingConnectComplete)
            return 0;
        int errorCode;

        StrLCpy(mHost, host, sizeof(mHost));
        mPort = port;

        void *svraddr = nullptr;
        int svraddr_len = 0;

        struct sockaddr_in svraddr_4 = {0};
        struct sockaddr_in6 svraddr_6 = {0};

        //获取当前的网络协议
        struct addrinfo *result;
        errorCode = getaddrinfo(host, NULL, NULL, &result);
        if (errorCode != 0) {
            LOGE("getaddrinfo failed");
            onSocketConnectError(errorCode);
            return -1;
        }

        //创建TCP网络描述符
        bool ret = true;
        const struct sockaddr *sa = result->ai_addr;
        socklen_t maxlen = 128;
        switch (sa->sa_family) {
            case AF_INET://ipv4
            {
                mFD = socket(AF_INET, SOCK_STREAM, 0);//IPPROTO_TCP
                if (mFD == INVALID_SOCKET) {
                    LOGE("socket create failed");
                    ret = false;
                    break;
                }
                if (!inet_ntop(AF_INET, &(((struct sockaddr_in *) sa)->sin_addr), mHost, maxlen)) {
                    LOGE("inet_ntop error ip = %s", mHost);
                    ret = false;
                    break;
                }
                svraddr_4.sin_family = AF_INET;
                //svraddr_4.sin_addr.s_addr = inet_addr(mHost);
				inet_pton(AF_INET, mHost, &svraddr_4.sin_addr);
                svraddr_4.sin_port = htons(mPort);
                svraddr_len = sizeof(svraddr_4);
                svraddr = &svraddr_4;
                break;
            }
            case AF_INET6://ipv6
            {
                if ((mFD = socket(AF_INET6, SOCK_STREAM, 0)) == INVALID_SOCKET) {
                    LOGE("socket create failed ipv6");
                    ret = false;
                    break;
                }

                if (!inet_ntop(AF_INET6, &(((struct sockaddr_in6 *) sa)->sin6_addr), mHost,
                               maxlen)) {
                    LOGE("inet_ntop error ipv6 ip = %s", mHost);
                    ret = false;
                    break;
                }

                memset(&svraddr_6, 0, sizeof(svraddr_6));
                svraddr_6.sin6_family = AF_INET6;
                svraddr_6.sin6_port = htons(mPort);
                if (!inet_pton(AF_INET6, mHost, &svraddr_6.sin6_addr)) {
                    LOGE("inet_pton error ipv6 ip = %s", mHost);
                    ret = false;
                    break;
                }
                svraddr_len = sizeof(svraddr_6);
                svraddr = &svraddr_6;
                break;
            }
            default:
                LOGE("Unknown AF");
                ret = false;
        }

        //释放内存
        freeaddrinfo(result);
        //如果我们创建没有成功
        if (!ret)
            return -1;

        //设置非阻塞
        if (setSocketParam() != 0)
            return -1;

#ifdef WIN32
        if (connect(mFD, (struct sockaddr*)svraddr, svraddr_len) == SOCKET_ERROR
            && !(((errorCode = WSAGetLastError()) == WSAEWOULDBLOCK) || (errorCode==WSAEINPROGRESS)))
        {
            closesocket(mFD);
#else
        if (connect(mFD, (struct sockaddr *) svraddr, svraddr_len) == -1
            && !(((errorCode = errno) == EAGAIN) || (errorCode == EINPROGRESS))) {
            close(mFD);
#endif
            onSocketConnectError(errorCode);
            return -1;
        }

        //LOGI("%s : RegisterWrite wait for connected %d",__FUNCTION__,m_fd);
        //如果为connect 返回-1 并且errorno为 EAGAIN
        registerWrite();//注册到写fd_set中 等待 OnFDWrite回调，第一次的话就说明这个fd可用了。
        registerTimer(to);//注册超时处理
        mIsWaitingConnectComplete = true;

        return 0;
    }

    bool ClientSocket::sendBuf(const char *buf, unsigned int buflen) {
        if (addBuf(buf, buflen) != 0) {
            closeSocket();
            return false;
        }
        return true;
    }
};

