
#ifdef WIN32
#include <winsock2.h>
#else
#include "sys/socket.h"
#endif

#include <string.h>
#include <stdlib.h>
#include "data_decoder.h"
#include "client_socket.h"

namespace Wrap
{
	unsigned int DataDecoder::getBuflen(unsigned char *buf)
	{
		if (m_pttype == PROTOCOLTYPE_BINARY)
		{
			if (m_hdlen == HEADER_LEN_2)
			{
				short len;
				memcpy(&len, buf, sizeof(len));
				len = ntohs(len);
				return (unsigned int)len;
			}
			else if (m_hdlen == HEADER_LEN_4)
			{
				int len;
				memcpy(&len, buf, sizeof(len));
				len = ntohl(len);
				return (unsigned int)len;
			}
		}
		else if (m_pttype == PROTOCOLTYPE_TEXT)
		{
			char lenbuf[10] = { 0 };
			memcpy(lenbuf, buf, m_hdlen);
			//字符串转长整数，以16进制返回结果
			long int len = strtol(lenbuf, NULL, 16);
			return (unsigned int)len;
		}
		else if (m_pttype == PROTOCOLTYPE_BINARY_BIG){//大端
			if (m_hdlen == HEADER_LEN_2)
			{
				unsigned int len;
				unsigned int first = buf[0];
				unsigned int second = buf[1];
				len = (first << 8) + second;
				return m_hdlen + len;//增加包长需要过滤的字段
			}
			else if (m_hdlen == HEADER_LEN_4)
			{
				unsigned int len;
				unsigned int first = buf[0];
				unsigned int second = buf[1];
				unsigned int third = buf[2];
				unsigned int fourth = buf[3];
				len = (first << 24) + (second << 16) + (third << 8) + fourth;
				return m_hdlen + len;
			}
		}
		else if (m_pttype == PROTOCOLTYPE_BINARY_SMALL)//小端
		{
			if (m_hdlen == HEADER_LEN_2)
			{
				unsigned int len;
				unsigned int first = buf[0];
				unsigned int second = buf[1];
				len = (second << 8) + first;
				return m_hdlen + len;//增加包长需要过滤的字段
			}
			else if (m_hdlen == HEADER_LEN_4)
			{
				unsigned int len;
				unsigned int first = buf[0];
				unsigned int second = buf[1];
				unsigned int third = buf[2];
				unsigned int fourth = buf[3];
				len = (second << 8) + first + (fourth << 24) + (third << 16);
				return m_hdlen + len;
			}
		}
		return 0;
	}
	int DataDecoder::process(ClientSocketBase *pClient)
	{
		DataBlock *recvdb = pClient->getRB();
		const char *buf = recvdb->getBuf();
		const char *ptr = buf;
		unsigned int pos = recvdb->getPos();

		while (true)
		{
			// 长度不超过包头长度的时候
			if ((unsigned int)(buf + pos - ptr) <= (unsigned int)m_hdlen)
				break;
			//取包头长度
			unsigned int buflen = getBuflen((unsigned char*)ptr);
			if (buflen == 0)//异常数据
			{
				recvdb->initPos();
				return -1;
			}
			//不足一个包长度的时候
			if ((ptr + buflen) > (buf + pos))
				break;

			//获取到一个完整包，解析包
			if (onPackage(pClient, ptr, buflen) != 0)
			{
				//解析失败
				recvdb->initPos();
				return -1;
			}
			//把它close了,而不是返回-1
			if (pClient->isClosed())
			{
				recvdb->initPos();
				return 0;
			}
			//解析下一个包
			ptr += buflen;
		}

		//剩余的不足一个包长度的数据，把它拷贝下来。
		unsigned int  remain = buf + pos - ptr;
		if (remain == 0)
			recvdb->initPos();
		else
			recvdb->copy(0, ptr, remain);

		return 0;
	}
}

