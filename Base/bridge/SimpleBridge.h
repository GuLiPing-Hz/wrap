#ifndef SIMPLEBRIDGE_H__
#define SIMPLEBRIDGE_H__

#include <string>
#include <map>

#include "../rapidjson/document.h"
#include "../rapidjson/writer.h"

#include "../wrap/native_buffer.h"
#include "../wrap/mutex.h"
#include "../app/RequestBase.h"

#define MACRO_METHOD "method"
#define MACRO_SEQ "seq"
#define MACRO_CODE "code"

#define MACRO_ARG0 "arg0"
#define MACRO_ARG1 "arg1"
#define MACRO_ARG2 "arg2"
#define MACRO_ARG3 "arg3"
#define MACRO_ARG4 "arg4"
#define MACRO_ARG5 "arg5"

struct MethodParam{
	std::string method;
	std::string param;
};
typedef std::map<int, MethodParam> MAPSEQREQDATA;


class IM_DLL SimpleBridge : public  ResponseBase
{
protected:
	SimpleBridge();
	virtual ~SimpleBridge();
public:
	virtual int callByNative(const std::string& param);

	//ResponseBase
	virtual void onLobbyTunnelConnectSuccess();		//成功连接大厅服务器

	//连接超时
	virtual void onLobbyTunnelConnectTimeout();		//连接大厅服务器超时

	//连接错误
	virtual void onLobbyTunnelConnectError(const int code);

	//服务器主动断开的连接,客户端recv == 0的时候,回调到以下的接口
	virtual void onLobbyTunnelClose();	//断开大厅服务器

	//客户端recv异常,send异常,网络层buf溢出,select出现问题,都会回调到这个以下接口
	virtual void onLobbyTunnelError(const int code);

	/*
	@param code: 0 正常返回，RESULT_TIMEOUT:超时
	@param cmd: 如果请求超时，那么对应的请求seq会传过来
	*/
	virtual void onLobbyMsg(const int code, const char* msg, const unsigned int len, const int seq = 0);
protected:
	void appendSeq(const int seq, const std::string &method, const std::string &data);
	MethodParam removeSeq(const int seq);

protected:
	virtual void callNative(const std::string& method, const std::string& param) = 0;

private:
	Wrap::Mutex mSection;
	MAPSEQREQDATA mMapSeq;
};

IM_DLL std::string GetStrFromRoot(const rapidjson::Value &root);

#endif//SIMPLEBRIDGE_H__
