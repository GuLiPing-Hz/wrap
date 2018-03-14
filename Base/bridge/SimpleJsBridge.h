#ifndef SIMPLE_JS_NATIVE_BRIDGE_H__
#define SIMPLE_JS_NATIVE_BRIDGE_H__

#include <string>
#include <functional>
#include "platform/CCPlatformConfig.h"
#include "cocos2d.h"
//#include "../LuaAPI.h"
#include "cocos-ext.h"
#include "../app/RequestBase.h"

#include "jsapi.h"
#include "jsfriendapi.h"
#include "scripting/js-bindings/manual/cocos2d_specifics.hpp"
#include "../wrap/mutex.h"

#define MACRO_METHOD "method"
#define MACRO_SEQ "seq"
#define MACRO_CODE "code"

#define MACRO_ARG0 "arg0"
#define MACRO_ARG1 "arg1"
#define MACRO_ARG2 "arg2"
#define MACRO_ARG3 "arg3"
#define MACRO_ARG4 "arg4"
#define MACRO_ARG5 "arg5"


//Js Call Native
//获取手机联网状态
#define JS_2_NATIVE_GET_NET_STATUS "GET_NET_STATUS"
//获取手机TOKEN
#define JS_2_NATIVE_GET_TOKEN "GET_TOKEN"
//获取验证码
#define JS_2_NATIVE_GET_CODE  "GET_CODE"
//获取UUID
#define JS_2_NATIVE_GET_UUID "GET_UUID"
//获取手机型号
#define JS_2_NATIVE_GET_PHONEMODEL "GET_PHONEMODEL"
#define JS_2_NATIVE_GET_DEVICE "GET_DEVICE"
//获取产品渠道，android获取渠道，ios获取bundle id
#define JS_2_NATIVE_GET_FLAVOR "GET_FLAVOR"
//获取当前的屏幕方向
#define JS_2_NATIVE_GET_ORIENTATION "GET_ORIENTATION"
#define JS_2_NATIVE_GOH5 "GOH5"
//打印文件日志
#define JS_2_NATIVE_LOG "LOG"
//分享到微信
#define JS_2_NATIVE_SHARE_WX  "SHARE_WX"

#define JS_2_NATIVE_SETPBACK "SETPBACK"
//统计用户登录登出
#define JS_2_NATIVE_LOG_INOUT "LOG_INOUT"
#define JS_2_NATIVE_LOG_PAY "LOG_PAY"
#define JS_2_NATIVE_IS_IPHONEX "IS_IPHONEX"

#define JS_2_NATIVE_HTTP_REQ "HTTP_REQ"
#define JS_2_NATIVE_SOCKET_REQ "SOCKET_REQ"

//Native Call Js
#define NATIVE_2_JS_GET_CODE "CALLBACK_GET_CODE"
#define NATIVE_2_JS_PAY_RESULT "CALLBACK_PAY_RESULT"
#define NATIVE_2_JS_ORIENTATION "CALLBACK_ORIENTATION"
#define NATIVE_2_JS_SHARERESULT  "CALLBACK_SHARERESULT"

#define NATIVE_2_JS_HEARTBEAT "HEART_BEAT"
#define NATIVE_2_JS_HTTP_RESP "HTTP_RESP"
#define NATIVE_2_JS_SOCKET_RESP "SOCKET_RESP"

typedef std::function<void(const char*, const char*, char*)> FUNCIOSRET;

struct JsBridgeParam {
	std::string param;
	std::string buffer;
};
typedef std::list<JsBridgeParam> LISTJSBPARAM;

class SimpleJsBridge : public cocos2d::Ref, public ResponseBase {

	friend bool Def_SimpleJsBridge_SetNativeListener(JSContext *cx, unsigned int argc, jsval *vp);

	SimpleJsBridge();
	virtual ~SimpleJsBridge();
public:
	static SimpleJsBridge* getInstance();
	static void resetInstance();

	void setIosBridge(FUNCIOSRET& pBridge) { mBridge = pBridge; }
	void setRequest(RequestBase* req) { mReq = req; }

	//ResponseBase
	/*
	成功连接大厅服务器
	*/
	virtual void onLobbyTunnelConnectSuccess();
	/*
	成功连接房间服务器
	*/
	virtual void onRoomTunnelConnectSuccess() {};

	/*
	连接大厅服务器超时
	*/
	virtual void onLobbyTunnelConnectTimeout();
	/*
	连接房间服务器超时
	*/
	virtual void onRoomTunnelConnectTimeout() {};

	/*
	连接大厅服务器错误
	@param code 错误码
	*/
	virtual void onLobbyTunnelConnectError(const int code);
	/*
	连接房间服务器错误
	@param code 错误码
	*/
	virtual void onRoomTunnelConnectError(const int code) {};

	//服务器主动断开的连接,客户端recv == 0的时候,回调到以下的接口
	/*
	大厅服务器断开
	*/
	virtual void onLobbyTunnelClose();	//
	/*
	房间服务器断开
	*/
	virtual void onRoomTunnelClose() {};	//

	//客户端recv异常,send异常,网络层buf溢出,select出现问题,都会回调到这个以下接口
	/*
	大厅服务器异常
	@param code 错误码
	*/
	virtual void onLobbyTunnelError(const int code);
	/*
	房间服务器异常
	@param code 错误码
	*/
	virtual void onRoomTunnelError(const int code) {};

	/*
	收到来自大厅服务器的信息
	@param code 0 正常, OPTION_TIMEOUT 超时
	@param msg 信息内容
	@param len 信息长度
	*/
	virtual void onLobbyMsg(const int code, const char* msg, const unsigned int len, const int seq = 0);

	/*
	@method: 指定方法名
	@param json 数据结构
	{
	arg0 : 第一个参数
	arg1 : 第二个参数
	arg2 : 第三个参数
	arg3 : 第四个参数
	arg4 : 第五个参数
	arg5 : 第六个参数   最多支持6个参数
	}
	@param buffer 二进制数据,在关键的时候需要

	@return json 数据结构
	{
	code: 0表示成功; > 0表示失败
	 1 json 解析出错
	 2 未知的method name
	 3 调用method失败,或者是回调的网络错误
	arg0 : 第一个返回值
	arg1 : 第二个返回值
	arg2 : 第三个返回值
	arg3 : 第四个返回值
	arg4 : 第五个返回值
	arg5 : 第六个返回值   最多支持6个参数
	}
	*/
	std::string callNativeFromJs(std::string method, std::string param, std::string buffer);
private:
	//only call in js
	void setNativeListener(std::function<void(const std::string&, const std::string&)> listener);

	std::string reqSocket(const std::string& param, const std::string& buffer);
	std::string reqHttp(const std::string& param);
public:
	//call for native in cocos Thread
	void callJsFromNative(const std::string& param, const std::string& buffer = "");

private:
	void callJsRealNativePerFrame(float);
private:
	static SimpleJsBridge* sInstance;
	std::function<void(const std::string&, const std::string&)> mListener;

	std::string mPrefix;
	std::string mAfterfix;

	RequestBase* mReq;
	FUNCIOSRET mBridge;

	Wrap::Mutex mMutex;
	LISTJSBPARAM mParams;
};

void register_custom_js(JSContext *cx, JS::HandleObject global);

#endif//SIMPLE_JS_NATIVE_BRIDGE_H__
