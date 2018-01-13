﻿#ifndef DATADECODER__H__
#define DATADECODER__H__
/*
	注释添加以及修改于 2014-4-2

	封装一个对网络数据处理的接口类 DataDecoder
	Process 对网络一串数据进行分析，分解出一个完整的包，交给OnPackage进行处理
	OnPackage 提供对完整包的具体处理的接口
	*/


#include <stdio.h>
#include "wrap_config.h"

//定义 包解析的接口
namespace Wrap {
	//流的数据类型，
	typedef enum _StreamType {
		PROTOCOLTYPE_TEXT = 1,// 字符串
		PROTOCOLTYPE_BINARY,// 二进制数据 网络字节序
		PROTOCOLTYPE_BINARY_BIG,//二进制数据 >  大端
		PROTOCOLTYPE_BINARY_SMALL,//二进制数据 < 小端
	} StreamType;
	typedef enum _HeadType {
		//定义包长度的枚举类型
		HEADER_LEN_2 = 2,
		HEADER_LEN_4 = 4,
		HEADER_LEN_6 = 6,
	} HeadType;

	class ClientSocketBase;
	class HttpSocketBase;

	class DataDecoderBase {
	public:
		DataDecoderBase() {}

		virtual ~DataDecoderBase() {}

		virtual int process(ClientSocketBase *pClient) = 0;
	};

	class DataDecoder : public DataDecoderBase {
		DISABLE_COPY_CTOR(DataDecoder);
	public:
		DataDecoder(StreamType pttype, HeadType hdlen) : m_pttype(pttype), m_hdlen(hdlen) {}

		virtual ~DataDecoder() {}

		//解析分发完整包数据,交给OnPackage处理
		virtual int process(ClientSocketBase *pClient);

		//对收到的包进行处理： 返回0成功，返回非0失败
		virtual int onPackage(ClientSocketBase *pClient, const char *buf, unsigned int buflen) = 0;

		//从提供的参数buf中获取包的长度 Net -> Loacal
		virtual unsigned int getBuflen(unsigned char *buf);

	protected:
		StreamType m_pttype;
		HeadType m_hdlen;
	};
}
#endif//DATADECODER__H__
