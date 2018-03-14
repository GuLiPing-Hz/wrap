#include "SimpleJsBridge.h"
#include "external/json/document.h"
#include "external/json/writer.h"
#include "external/json/stringbuffer.h"
#include "network/HttpClient.h"
#include "../crypto/md5.h"
#include "../app/NetApp.h"
#include "network/Uri.h"
#include "../wrap/native_buffer.h"
#include "../wrap/funcs.h"

using namespace cocos2d::network;

#define DEBUG_NET_HTTP 0

std::string GetStrFromRoot(const rapidjson::Value &root)
{
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	root.Accept(writer);
	std::string result = buffer.GetString();

#if (DEBUG_NET_HTTP) 
	cocos2d::log("GetStrFromRoot result = %s", result.c_str());
#endif
	return result;
}

#if CC_TARGET_PLATFORM == CC_PLATFORM_WIN32 //Win

#include <Windows.h>
#include <Wininet.h>

//加载动态连接库的引入库(LIB)
#pragma comment(lib, "Wininet.lib")

#define NET_NONE "0"//无网络 0
#define NET_WIFI "1"//无线网络  pc 电脑如果联网统一认为是无线 1
#define NET_MOBILE "2"//移动网络 2

std::string GetNetStatus()
{
	//#define INTERNET_CONNECTION_MODEM           1
	//#define INTERNET_CONNECTION_LAN             2
	//#define INTERNET_CONNECTION_PROXY           4
	//#define INTERNET_CONNECTION_MODEM_BUSY      8
	DWORD flags; //上网方式
	BOOL isOnline = InternetGetConnectedState(&flags, 0);
	return isOnline ? NET_WIFI : NET_NONE;
}

std::string GetUUID() {
	//9A25755A - 9404 - 42C8 - BB37 - 888E7900D057

	static char C[] = "0123456789ABCDEF";

	std::random_device rd;
	char ret[50] = { 0 };
	sprintf(ret, "%c%c%c%c%c%c%c%c-%c%c%c%c-%c%c%c%c-%c%c%c%c-%c%c%c%c%c%c%c%c%c%c%c%c"
		, C[rd() % 16], C[rd() % 16], C[rd() % 16], C[rd() % 16], C[rd() % 16], C[rd() % 16], C[rd() % 16], C[rd() % 16]
		, C[rd() % 16], C[rd() % 16], C[rd() % 16], C[rd() % 16]
		, C[rd() % 16], C[rd() % 16], C[rd() % 16], C[rd() % 16]
		, C[rd() % 16], C[rd() % 16], C[rd() % 16], C[rd() % 16]
		, C[rd() % 16], C[rd() % 16], C[rd() % 16], C[rd() % 16], C[rd() % 16], C[rd() % 16]
		, C[rd() % 16], C[rd() % 16], C[rd() % 16], C[rd() % 16], C[rd() % 16], C[rd() % 16]);

	return std::string(ret);
}

std::string SimpleJsBridge::callNativeFromJs(std::string method, std::string param, std::string buffer)
{
	rapidjson::Document doc;
	rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> &allocator = doc.GetAllocator();
	rapidjson::Value root(rapidjson::kObjectType);

	if (method == JS_2_NATIVE_GET_NET_STATUS)
	{
		root.AddMember(MACRO_CODE, 0, allocator);

		rapidjson::Value arg0(rapidjson::kStringType);
		arg0.SetString(GetNetStatus().c_str(), allocator);
		root.AddMember(MACRO_ARG0, arg0, allocator);
		// 		rapidjson::Value obj(rapidjson::kArrayType);
		// 		root.AddMember(RESULT_ARG1, obj, allocator);
	}
	else if (method == JS_2_NATIVE_GET_TOKEN)
	{
		/*
				 6ecc5b4227e5c45198c9ec01ead72c84
				 8e76364b3eeff0a141f48e75ece88f77
				 e94a68a9f2a0843479446181efa2ccbf
				 1ebdde31bd197f9be86a49f568c6c66b
				 a8d66be14d7fcde776a5a112f91b24e3
				 1f809590133cdab04e5cf62f28a6ca15
				 df391b832d20202e055a4891baf09acf
				 02baebc2bf21f7acb0c535494f4caea0
				 1d9ecceac6a9dc2398e85fcec490cf18
				 a8c8c99025c493effaf7364b6b264ace
				 */

		root.AddMember(MACRO_CODE, 0, allocator);
		root.AddMember(MACRO_ARG0, "1ebdde31bd197f9be86a49f568c6c66b", allocator);
	}
	else if (method == JS_2_NATIVE_GET_CODE) {

		doc.Parse(param.c_str());
		if (doc.HasParseError()) {
			root.AddMember(MACRO_CODE, 1, allocator);
			return GetStrFromRoot(root);
		}
		std::string phone = doc[MACRO_ARG0].GetString();

		root.AddMember(MACRO_CODE, 0, allocator);//请求成功

		//windows平台暂无验证码
		rapidjson::Value paramRoot(rapidjson::kObjectType);
		paramRoot.AddMember(MACRO_METHOD, NATIVE_2_JS_GET_CODE, allocator);
		paramRoot.AddMember(MACRO_CODE, 1, allocator);

		rapidjson::Value paramArg0(rapidjson::kStringType);
		paramArg0.SetString(phone.c_str(), allocator);
		paramRoot.AddMember(MACRO_ARG0, paramArg0, allocator);
		callJsFromNative(GetStrFromRoot(paramRoot));//回调到js
	}
	else if (method == JS_2_NATIVE_GET_UUID) {
		root.AddMember(MACRO_CODE, 0, allocator);

		rapidjson::Value arg0(rapidjson::kStringType);
		arg0.SetString(GetUUID().c_str(), allocator);
		root.AddMember(MACRO_ARG0, arg0, allocator);
	}
	else if (method == JS_2_NATIVE_GET_PHONEMODEL) {
		root.AddMember(MACRO_CODE, 0, allocator);
		root.AddMember(MACRO_ARG0, "Windows 7", allocator);
	}
	else if (method == JS_2_NATIVE_GET_FLAVOR) {
		root.AddMember(MACRO_CODE, 0, allocator);
		root.AddMember(MACRO_ARG0, "Official", allocator);
	}
	else if (method == JS_2_NATIVE_GOH5) {
		doc.Parse(param.c_str());
		if (doc.HasParseError()) {
			root.AddMember(MACRO_CODE, 1, allocator);
			return GetStrFromRoot(root);
		}

		ShellExecuteA(NULL, "open", doc[MACRO_ARG0].GetString(), NULL, NULL, SW_SHOW);//关键代码
		root.AddMember(MACRO_CODE, 0, allocator);
	}
	else if (method == JS_2_NATIVE_LOG) {
		doc.Parse(param.c_str());
		if (doc.HasParseError()) {
			root.AddMember(MACRO_CODE, 1, allocator);
			return GetStrFromRoot(root);
		}
		LOGI("log %s", doc[MACRO_ARG0].GetString());
		root.AddMember(MACRO_CODE, 0, allocator);//不打印日志
	}
	else if (method == JS_2_NATIVE_HTTP_REQ) {
		return reqHttp(param);//请求Http
	}
	else if (method == JS_2_NATIVE_SOCKET_REQ) {
		return reqSocket(param, buffer);//请求Socket
	}
	else if (method == JS_2_NATIVE_GET_DEVICE) {
		root.AddMember(MACRO_CODE, 0, allocator);
		root.AddMember(MACRO_ARG0, "PC", allocator);
	}
	else if (method == JS_2_NATIVE_GET_ORIENTATION) {
		root.AddMember(MACRO_CODE, 0, allocator);
		root.AddMember(MACRO_ARG0, "2", allocator);
	}
	else
	{
		root.AddMember(MACRO_CODE, 2, allocator);
	}

	/*
	doc.Parse(param.c_str());
	if (doc.HasParseError()){
	root.AddMember(RESULT_CODE, 1, allocator);
	return GetStrFromRoot(root);
	}*/

	return GetStrFromRoot(root);
}

#elif CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID //Android
#include "platform/android/jni/JniHelper.h"

std::string SimpleJsBridge::callNativeFromJs(std::string method, std::string param, std::string buffer)
{
	if (method == JS_2_NATIVE_HTTP_REQ)
		return reqHttp(param);//请求Http
	else if (method == JS_2_NATIVE_SOCKET_REQ) {
		return reqSocket(param, buffer);
	}
	else//call android
		return cocos2d::JniHelper::callStaticStringMethod("simple.util.bridge.JsAndroidBridge", "callNativeFromJs", method, param);
}

#elif CC_TARGET_PLATFORM == CC_PLATFORM_IOS //IOS
std::string SimpleJsBridge::callNativeFromJs(std::string method, std::string param, std::string buffer)
{
	if (method == JS_2_NATIVE_HTTP_REQ)
		return reqHttp(param);//请求Http
	else if (method == JS_2_NATIVE_SOCKET_REQ) {
		return reqSocket(param, buffer);
	}
	else {
		static char buf[1024] = { 0 };
		buf[0] = 0;
		if (mBridge)
			mBridge(method.c_str(), param.c_str(), buf);
		return std::string(buf);
	}
}

#endif

struct HttpUserData {
	char method[50];
	char* param;
	int seq;
};

SimpleJsBridge* SimpleJsBridge::sInstance = nullptr;

SimpleJsBridge *SimpleJsBridge::getInstance()
{
	if (!sInstance)
		sInstance = new SimpleJsBridge();
	return sInstance;
}

void SimpleJsBridge::resetInstance() {
	if (sInstance) {
		sInstance->release();
		sInstance = nullptr;
	}
}

SimpleJsBridge::SimpleJsBridge()
	:mReq(nullptr)
	, mBridge(nullptr)
{
	mPrefix = "ZhejiangZhangjing";
	mAfterfix = "2018.01.09";


	cocos2d::Director::getInstance()->getScheduler()->schedule(CC_CALLBACK_1(SimpleJsBridge::callJsRealNativePerFrame, this), this, 0.03f, false, "SimpleJsBridge");
}
SimpleJsBridge::~SimpleJsBridge()
{
	mReq = nullptr;
	mBridge = nullptr;
	int glp = 1;
}

#define MAKE_PARAM_ROOT(MACRO_name,MACRO_method,MACRO_code) \
rapidjson::Document doc; \
rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> &allocator = doc.GetAllocator(); \
rapidjson::Value paramRoot(rapidjson::kObjectType); \
paramRoot.AddMember(MACRO_METHOD, (MACRO_name), allocator); \
paramRoot.AddMember(MACRO_ARG0, (MACRO_method), allocator); \
paramRoot.AddMember(MACRO_ARG1, (MACRO_code), allocator); \

#define MAKE_PARAM_SOCKET(MACRO_method,MACRO_code) MAKE_PARAM_ROOT(NATIVE_2_JS_SOCKET_RESP,MACRO_method,MACRO_code)

//ResponseBase
void SimpleJsBridge::onLobbyTunnelConnectSuccess()//成功连接大厅服务器
{
	MAKE_PARAM_SOCKET("onLobbyTunnelConnectSuccess", 0);
	callJsFromNative(GetStrFromRoot(paramRoot));

	//启动心跳包
	NetApp::GetInstance()->setHeartbeatFunc([this]()->void {

		//心跳包时间计数
		static int time_old = 0;
		static int time_new = 0;
		time_new = (int)time(NULL);
		if (15 < (time_new - time_old))//15秒
		{
			time_old = time_new;

			if (NetApp::GetInstance()->getLobbyTunnel()->isConnected()) {
				MAKE_PARAM_ROOT(NATIVE_2_JS_HEARTBEAT, 0, 0);
				callJsFromNative(GetStrFromRoot(paramRoot));
			}
		}
	});
}

//连接超时
void SimpleJsBridge::onLobbyTunnelConnectTimeout()		//连接大厅服务器超时
{
	MAKE_PARAM_SOCKET("onLobbyTunnelConnectTimeout", 0);
	callJsFromNative(GetStrFromRoot(paramRoot));
}

//连接错误
void SimpleJsBridge::onLobbyTunnelConnectError(int code) {
	MAKE_PARAM_SOCKET("onLobbyTunnelConnectError", code);
	callJsFromNative(GetStrFromRoot(paramRoot));
}

//服务器主动断开的连接,客户端recv == 0的时候,回调到以下的接口
void SimpleJsBridge::onLobbyTunnelClose()	//断开大厅服务器
{
	MAKE_PARAM_SOCKET("onLobbyTunnelClose", 0);
	callJsFromNative(GetStrFromRoot(paramRoot));
}

//客户端recv异常,send异常,网络层buf溢出,select出现问题,都会回调到这个以下接口
void SimpleJsBridge::onLobbyTunnelError(int code) {
	MAKE_PARAM_SOCKET("onLobbyTunnelError", code);
	callJsFromNative(GetStrFromRoot(paramRoot));
}

void SimpleJsBridge::onLobbyMsg(const int code, const char* msg, const unsigned int len, const int seq) {
	MAKE_PARAM_SOCKET("onLobbyMsg", code);
	paramRoot.AddMember(MACRO_ARG2, len, allocator);
	paramRoot.AddMember(MACRO_ARG3, seq, allocator);

	std::string buffer;
	if (msg && len > 0)
		buffer = std::move(std::string(msg, len));
	callJsFromNative(GetStrFromRoot(paramRoot), buffer);
}

void SimpleJsBridge::setNativeListener(std::function<void(const std::string&, const std::string&)> listener)
{
	mListener = listener;
}

std::string SimpleJsBridge::reqSocket(const std::string& param, const std::string& buffer) {

	rapidjson::Document doc;
	rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> &allocator = doc.GetAllocator();
	rapidjson::Value root(rapidjson::kObjectType);

	if (mReq == nullptr)
		goto FAILED;

	doc.Parse(param.c_str());
	if (doc.HasParseError()) {
		LOGE("reqSocket param parse failed\n");
		goto FAILED;
	}
	else {
		std::string socketM = doc[MACRO_ARG0].GetString();
		std::string socketP = doc[MACRO_ARG1].GetString();

		int seq = 0;
		if (doc.HasMember(MACRO_SEQ))
			seq = doc[MACRO_SEQ].GetInt();

		doc.Parse(socketP.c_str());//解析具体的参数
		if (doc.HasParseError()) {
			LOGE("reqSocket socketP parse failed\n");
			goto FAILED;
		}

		if (socketM == "connectLobby") {
			std::string ip = doc["ip"].GetString();
			int port = doc["port"].GetInt();
			int to = doc["timeout"].GetInt();

			if (mReq->connectLobby(ip.c_str(), port, to) == -1) {
				LOGE("connectLobby send failed\n");
				goto FAILED;
			}
		}
		else if (socketM == "disConnectLobby") {
			mReq->disConnectLobby();
		}
		else if (socketM == "sendMsgToLobby") {
			if (!doc.HasMember("len")) {
				LOGE("sendMsgToLobby need len\n");
				goto FAILED;
			}
			int len = doc["len"].GetInt();

			if (len != buffer.length()) {//如果长度不匹配
				LOGE("sendMsgToLobby len != buffer.length()\n");
				goto FAILED;
			}

			bool needBack = doc["needBack"].GetBool();

			if (mReq->sendMsgToLobby(buffer.c_str(), len, seq, needBack) == 0) {
				LOGE("sendMsgToLobby send failed\n");
				goto FAILED;
			}
		}

		root.AddMember(MACRO_CODE, 0, allocator);//调用成功
		return GetStrFromRoot(root);//返回结果
	}

FAILED:
	root.AddMember(MACRO_CODE, 3, allocator);
	return GetStrFromRoot(root);
}

std::string SimpleJsBridge::reqHttp(const std::string& param) {
	rapidjson::Document doc;
	rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> &allocator = doc.GetAllocator();
	rapidjson::Value root(rapidjson::kObjectType);

	doc.Parse(param.c_str());
	if (doc.HasParseError()) {
		goto FAILED;
	}
	else {

		std::string httpH = doc[MACRO_ARG0].GetString();
		std::string httpM = doc[MACRO_ARG1].GetString();
		std::string httpP = doc[MACRO_ARG2].GetString();
		int seq = doc[MACRO_SEQ].GetInt();

		doc.Parse(httpP.c_str());//解析具体的参数
		if (doc.HasParseError()) {
			goto FAILED;
		}

		std::map<std::string, std::string> mapParam;
		for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); it++) {
			mapParam.insert(std::make_pair(it->name.GetString(), it->value.GetString()));
		}
		std::string reqParam;
		for (std::map<std::string, std::string>::iterator it = mapParam.begin(); it != mapParam.end();) {

			std::string value = it->second;
			reqParam += it->first + "=" + value;
			it++;
			if (it != mapParam.end()) {
				reqParam += "&";
			}
		}

		auto httpReq = new HttpRequest();
		httpReq->setRequestType(HttpRequest::Type::GET);

		std::string plainTxt = mPrefix + reqParam + mAfterfix;
		MD5 md(plainTxt);
		std::string sign = md.toString();

		//转义地址一下
		httpReq->setUrl(Wrap::UrlEncode(httpH + "/" + httpM + "?" + reqParam + "&sign=" + sign));

#if (DEBUG_NET_HTTP) 
		cocos2d::log("Http : %s ; md5(%s)", httpReq->getUrl(), plainTxt.c_str());
#endif

		HttpUserData* pUserData = new HttpUserData();
		if (!pUserData)
			goto FAILED;
		pUserData->seq = seq;
		strcpy(pUserData->method, httpM.c_str());
		pUserData->param = new char[httpP.length() + 1];
		if (!pUserData->param) {
			delete pUserData;
			goto FAILED;
		}

		strcpy(pUserData->param, httpP.c_str());
		httpReq->setUserData(pUserData);

		ccHttpRequestCallback callBack = [this](HttpClient* client, HttpResponse* response)->void {
			HttpUserData* pData = (HttpUserData*)response->getHttpRequest()->getUserData();

#if (DEBUG_NET_HTTP) 
			cocos2d::log("Http Callback: %d , %s ; param(%s)", response->getResponseCode(), pData->method, pData->param);
#endif

			rapidjson::Document doc;
			rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> &allocator = doc.GetAllocator();
			rapidjson::Value paramRoot(rapidjson::kObjectType);
			paramRoot.AddMember(MACRO_METHOD, NATIVE_2_JS_HTTP_RESP, allocator);

			rapidjson::Value arg0(rapidjson::kStringType);
			arg0.SetString(pData->method, allocator);
			paramRoot.AddMember(MACRO_ARG0, arg0, allocator);//请求方法名

			rapidjson::Value arg1(rapidjson::kStringType);
			arg1.SetString(pData->param, allocator);
			paramRoot.AddMember(MACRO_ARG1, arg1, allocator);//请求参数

			paramRoot.AddMember(MACRO_SEQ, pData->seq, allocator);//请求序列

			auto vect = response->getResponseData();
			if (response->isSucceed() && !vect->empty()) {
				rapidjson::Value arg2(rapidjson::kObjectType);
				// 				std::string arg2Str;
				// 				arg2Str.insert(arg2Str.begin(), vect->begin(), vect->end());
				// 				arg2.SetString(arg2Str.c_str(), allocator);
				arg2.SetString(&(vect->front()), vect->size(), allocator);

				paramRoot.AddMember(MACRO_ARG2, arg2, allocator);//response data

				paramRoot.AddMember(MACRO_CODE, 0, allocator);
				callJsFromNative(GetStrFromRoot(paramRoot));
			}
			else {
				rapidjson::Value arg2(rapidjson::kStringType);
				arg2.SetString(response->getErrorBuffer(), allocator);
				paramRoot.AddMember(MACRO_ARG2, arg2, allocator);//error data

				paramRoot.AddMember(MACRO_CODE, 3, allocator);
				callJsFromNative(GetStrFromRoot(paramRoot));
			}

			delete pData->param;
			delete pData;
		};

		httpReq->setResponseCallback(callBack);
		HttpClient::getInstance()->send(httpReq);

		root.AddMember(MACRO_CODE, 0, allocator);
	}

	return GetStrFromRoot(root);

FAILED:
	root.AddMember(MACRO_CODE, 3, allocator);
	return GetStrFromRoot(root);
}

void SimpleJsBridge::callJsFromNative(const std::string& param, const std::string& buffer)
{

	JsBridgeParam data = {
		param,buffer
	};

	//LOGI("push Data len =%d\n", mParams.size());
	{
		Wrap::Guard lock(mMutex);
		if (mParams.size() > 10000)
			return;
		mParams.push_back(data);
	}

	//这里需要做一个消息队列处理，不能一次性推入GL渲染线程，导致画面卡顿
	//cocos2d::Director::getInstance()->getScheduler()->performFunctionInCocosThread(lambda);
}

void SimpleJsBridge::callJsRealNativePerFrame(float)//一帧只处理一次服务器返回，否则会导致客户端掉帧
{
	JsBridgeParam data;

	{
		Wrap::Guard lock(mMutex);
		if (mParams.empty())
			return;

		data = mParams.front();
		mParams.pop_front();
	}

	auto lambda = [=]() -> void {
		if (mListener)
			mListener(data.param, data.buffer);
	};

	cocos2d::Director::getInstance()->getScheduler()->performFunctionInCocosThread(lambda);
}

bool Def_SimpleJsBridge_CallNativeFromJs(JSContext *cx, unsigned int argc, jsval *vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	bool ok = true;
	JS::RootedObject obj(cx, args.thisv().toObjectOrNull());
	js_proxy_t *proxy = jsb_get_js_proxy(obj);
	SimpleJsBridge *cobj = (SimpleJsBridge *)(proxy ? proxy->ptr : NULL);
	JSB_PRECONDITION2(cobj, cx, false, "Def_SimpleJsBridge_SetNativeListener : Invalid Native Object");
	if (argc == 2 || argc == 3)
	{
		std::string arg0;
		ok &= jsval_to_std_string(cx, args.get(0), &arg0);
		JSB_PRECONDITION2(ok, cx, false, "js_Call_Native : Error processing method arguments 1");

		std::string arg1;
		ok &= jsval_to_std_string(cx, args.get(1), &arg1);
		JSB_PRECONDITION2(ok, cx, false, "js_Call_Native : Error processing param arguments 2");

		std::string arg2;
		if (argc == 3 && args.get(2).isString()) {
			arg2 = jsval_to_std_string_len(cx, args.get(2));
		}

		const std::string &ret = cobj->callNativeFromJs(arg0, arg1, arg2);
		JS::RootedValue jsret(cx);
		jsret = std_string_to_jsval(cx, ret);
		args.rval().set(jsret);

		return true;
	}

	JS_ReportError(cx, "Def_SimpleJsBridge_CallNativeFromJs : wrong number of arguments: %d, was expecting %d", argc, 2);
	return false;
}

bool Def_SimpleJsBridge_SetNativeListener(JSContext *cx, unsigned int argc, jsval *vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	bool ok = true;
	JS::RootedObject obj(cx, args.thisv().toObjectOrNull());
	js_proxy_t *proxy = jsb_get_js_proxy(obj);
	SimpleJsBridge *cobj = (SimpleJsBridge *)(proxy ? proxy->ptr : NULL);
	JSB_PRECONDITION2(cobj, cx, false, "Def_SimpleJsBridge_SetNativeListener : Invalid Native Object");
	if (argc == 1)
	{
		std::function<void(const std::string& param, const std::string& buffer)> arg0;
		do
		{
			if (JS_TypeOfValue(cx, args.get(0)) == JSTYPE_FUNCTION)
			{
				JS::RootedObject jstarget(cx, args.thisv().toObjectOrNull());
				std::shared_ptr<JSFunctionWrapper> func(new JSFunctionWrapper(cx, jstarget, args.get(0), args.thisv()));
				auto lambda = [=](const std::string& param, const std::string& buffer) -> void {
					JSB_AUTOCOMPARTMENT_WITH_GLOBAL_OBJCET
						jsval largv[2];
					largv[0] = std_string_to_jsval(cx, param);
					largv[1] = STRING_TO_JSVAL(JS_NewStringCopyN(cx, buffer.c_str(), buffer.size()));
					JS::RootedValue rval(cx);
					bool succeed = func->invoke(2, &largv[0], &rval);
					if (!succeed && JS_IsExceptionPending(cx))
					{
						JS_ReportPendingException(cx);
					}
				};
				arg0 = lambda;
			}
			else
			{
				arg0 = nullptr;
			}
		} while (0);
		JSB_PRECONDITION2(ok, cx, false, "Def_SimpleJsBridge_SetNativeListener : Error processing arguments");
		cobj->setNativeListener(arg0);
		args.rval().setUndefined();
		return true;
	}

	JS_ReportError(cx, "Def_SimpleJsBridge_SetNativeListener : wrong number of arguments: %d, was expecting %d", argc, 1);
	return false;
}

//静态对象
bool Def_SimpleJsBridge_GetInstance(JSContext *cx, unsigned int argc, jsval *vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	if (argc == 0)
	{
		auto ret = SimpleJsBridge::getInstance();
		js_type_class_t *typeClass = js_get_type_from_native<SimpleJsBridge>(ret);
		JS::RootedObject jsret(cx, jsb_ref_get_or_create_jsobject(cx, ret, typeClass, "simple::SimpleJsBridge"));
		args.rval().set(OBJECT_TO_JSVAL(jsret));
		return true;
	}
	JS_ReportError(cx, "Def_SimpleJsBridge_GetInstance : wrong number of arguments");
	return false;
}

bool Def_Native_Md5(JSContext *cx, unsigned int argc, jsval *vp)
{
	bool ok = true;
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	if (argc == 1)
	{

		std::string arg0 = jsval_to_std_string_len(cx, args.get(0));

		MD5 md5(arg0);
		JS::RootedValue jsret(cx);
		jsret = std_string_to_jsval(cx, md5.toString());
		args.rval().set(jsret);
		return true;
	}
	JS_ReportError(cx, "Def_Native_Md5 : wrong number of arguments");
	return false;
}

bool Def_Native_Md5File(JSContext *cx, unsigned int argc, jsval *vp) {
	bool ok = true;
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	if (argc == 1)
	{

		std::string path = jsval_to_std_string_len(cx, args.get(0));
		std::string md5 = MD5::FileDigest(path);

		JS::RootedValue jsret(cx);
		jsret = std_string_to_jsval(cx, md5);
		args.rval().set(jsret);
		return true;
	}
	JS_ReportError(cx, "Def_Native_Md5 : wrong number of arguments");
	return false;
}

bool Def_Native_Xor_String(JSContext *cx, unsigned int argc, jsval *vp)
{
	bool ok = true;
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	if (argc == 2)
	{
		std::string data = jsval_to_std_string_len(cx, args.get(0));
		std::string key = jsval_to_std_string_len(cx, args.get(1));

		std::string ret = XorString(data.c_str(), data.length(), key.c_str(), key.length());

		JS::RootedValue jsret(cx);
		jsret = STRING_TO_JSVAL(JS_NewStringCopyN(cx, ret.c_str(), ret.size()));
		args.rval().set(jsret);
		return true;
	}
	JS_ReportError(cx, "Def_Native_Oxr_String : wrong number of arguments");
	return false;
}

JSClass *jsb_simple_bridge_class;
JSObject *jsb_simple_bridge_prototype;

static bool empty_constructor(JSContext *cx, uint32_t argc, jsval *vp)
{
	return false;
}

void register_SimpleJSBridge(JSContext *cx, JS::HandleObject ns)
{
	jsb_simple_bridge_class = (JSClass *)calloc(1, sizeof(JSClass));
	jsb_simple_bridge_class->name = "SimpleJsBridge";
	jsb_simple_bridge_class->addProperty = JS_PropertyStub;
	jsb_simple_bridge_class->delProperty = JS_DeletePropertyStub;
	jsb_simple_bridge_class->getProperty = JS_PropertyStub;
	jsb_simple_bridge_class->setProperty = JS_StrictPropertyStub;
	jsb_simple_bridge_class->enumerate = JS_EnumerateStub;
	jsb_simple_bridge_class->resolve = JS_ResolveStub;
	jsb_simple_bridge_class->convert = JS_ConvertStub;
	jsb_simple_bridge_class->flags = JSCLASS_HAS_RESERVED_SLOTS(2);

	static JSPropertySpec properties[] = {
		JS_PS_END };

	static JSFunctionSpec funcs[] = {
		JS_FN("callNativeFromJs", Def_SimpleJsBridge_CallNativeFromJs, 3, JSPROP_PERMANENT | JSPROP_ENUMERATE),
		JS_FN("setNativeListener", Def_SimpleJsBridge_SetNativeListener, 1, JSPROP_PERMANENT | JSPROP_ENUMERATE),
		JS_FS_END };

	static JSFunctionSpec st_funcs[] = {
		JS_FN("getInstance", Def_SimpleJsBridge_GetInstance, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),
		JS_FN("md5", Def_Native_Md5, 1, JSPROP_PERMANENT | JSPROP_ENUMERATE),
		JS_FN("md5File", Def_Native_Md5File, 1, JSPROP_PERMANENT | JSPROP_ENUMERATE),
		JS_FN("xorString", Def_Native_Xor_String, 2, JSPROP_PERMANENT | JSPROP_ENUMERATE),
		JS_FS_END };

	jsb_simple_bridge_prototype = JS_InitClass(
		cx, ns,
		JS::NullPtr(),
		jsb_simple_bridge_class,
		empty_constructor, 0, // constructor
		properties,
		funcs,
		NULL, // no static properties
		st_funcs);

	JS::RootedObject proto(cx, jsb_simple_bridge_prototype);
	JS::RootedValue className(cx, std_string_to_jsval(cx, "SimpleJsBridge"));
	JS_SetProperty(cx, proto, "_className", className);
	JS_SetProperty(cx, proto, "__nativeObj", JS::TrueHandleValue);
	JS_SetProperty(cx, proto, "__is_ref", JS::TrueHandleValue);
	// add the proto and JSClass to the type->js info hash table
	jsb_register_class<SimpleJsBridge>(cx, jsb_simple_bridge_class, proto, JS::NullPtr());
}

void register_custom_js(JSContext *cx, JS::HandleObject global) {
	// Get the simple root
	JS::RootedObject ns(cx);
	get_or_create_js_obj(cx, global, "simple", &ns);

	register_SimpleJSBridge(cx, ns);
	register_NativeBuffer(cx, ns);
}

