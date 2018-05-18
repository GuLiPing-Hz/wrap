#include "SimpleBridge.h"
#include "../LongConnection.h"
#include "../protocol.h"
#include <memory>
#include "../app/NetApp.h"
#include "../wrap/buffer_value.h"

std::string GetStrFromRoot(const rapidjson::Value &root) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    root.Accept(writer);
    std::string result = buffer.GetString();
#ifdef WIN32
    LOGI("GetStrFromRoot result = %s\n", result.c_str());
#endif
    return result;
}

#define MAKE_PARAM_ROOT(MACRO_code) \
    rapidjson::Document doc; \
    rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> &allocator = doc.GetAllocator(); \
    rapidjson::Value paramRoot(rapidjson::kObjectType); \
    paramRoot.AddMember(MACRO_CODE, (MACRO_code), allocator);

rapidjson::Value GetValueFromBufferValue(const Wrap::BufferValue *data) {
	if (data && data->list.empty()) {
		rapidjson::Value ret;
		if (data->type == Wrap::BufferValue::type_char || data->type == Wrap::BufferValue::type_short
			|| data->type == Wrap::BufferValue::type_int) {
			ret.SetInt(data->data.base.i);
		}
		else if (data->type == Wrap::BufferValue::type_int64) {
			ret.SetInt64(data->data.base.ll);
		}
		else if (data->type == Wrap::BufferValue::type_float) {
			ret.SetFloat(data->data.base.f);
		}
		else if (data->type == Wrap::BufferValue::type_str) {
			ret.SetString(data->data.str.c_str(), data->data.str.size());
		}

		return ret;
	}
	else {
		return rapidjson::Value(rapidjson::kNullType);
	}
}

void AddParam(rapidjson::Document &doc, rapidjson::Value &root, const std::string &name,
	const Wrap::BufferValue *data, bool isInner) {
	rapidjson::Value nameValue(rapidjson::kStringType);
	nameValue.SetString(name.c_str(), name.size(), doc.GetAllocator());

	if (data) {
		if (data->list.empty()) {
			rapidjson::Value value = GetValueFromBufferValue(data);
			root.AddMember(nameValue, value, doc.GetAllocator());
		}
		else {
			rapidjson::Value arraValue(rapidjson::kArrayType);
			for (unsigned int i=0; i<data->list.size(); i++) {
				const Wrap::BufferValue* item = data->list[i];
				rapidjson::Value value = GetValueFromBufferValue(item);
				arraValue.PushBack(value, doc.GetAllocator());
			}

			if (isInner)
				root.PushBack(arraValue, doc.GetAllocator());
			else
				root.AddMember(nameValue, arraValue, doc.GetAllocator());
		}
	}
	else {
		root.AddMember(nameValue, rapidjson::Value(rapidjson::kNullType), doc.GetAllocator());
	}
}

void AddParamEx(rapidjson::Document &doc, rapidjson::Value &root, const Wrap::BufferValue* data,
	const VECSTRING &name) {
	if (!data || data->list.size() < name.size()) {
		root.SetArray();
		return;
	}

	for (unsigned int i = 0; i < name.size(); i++) {
		const Wrap::BufferValue *unit = data->list[i];
		if (!unit->list.empty()) {
			rapidjson::Value arraValue(rapidjson::kArrayType);
			for (unsigned int j = 0; j < unit->list.size(); j++) {
				const Wrap::BufferValue *arra1 = unit->list[j];
				AddParam(doc, arraValue, "", arra1, true);
			}

			rapidjson::Value nameValue(rapidjson::kStringType);
			nameValue.SetString(name[i].c_str(), name[i].size(), doc.GetAllocator());
			root.AddMember(nameValue, arraValue, doc.GetAllocator());
		}
		else {
			AddParam(doc, root, name[i], unit, false);
		}
	}
}

void AddParamNames(rapidjson::Document &doc, rapidjson::Value &root, const Wrap::BufferValue* data, int count, ...)
{
	VECSTRING names;
	va_list args;
	va_start(args, count);
	for (int i = 0; i < count; ++i) {
		const char *arg = va_arg(args, const char*);
		names.push_back(arg);
	}
	va_end(args);

	AddParamEx(doc, root, data, names);
}

SimpleBridge::SimpleBridge(){
    LCSetResponse(this);
}

SimpleBridge::~SimpleBridge() {
}

//ResponseBase
void SimpleBridge::onLobbyTunnelConnectSuccess()        //成功连接大厅服务器
{
    MAKE_PARAM_ROOT(0);
    callNative("onLobbyTunnelConnectSuccess", GetStrFromRoot(paramRoot));

#ifndef LCTEST
    //启动心跳包
    NetApp::GetInstance()->setHeartbeatFunc([this]() -> void {

        //心跳包时间计数
        static int time_old = 0;
        static int time_new = 0;
        time_new = (int) time(NULL);
        if (15 < (time_new - time_old))//15秒
        {
            time_old = time_new;
            if (NetApp::GetInstance()->getLobbyTunnel()->isConnected()) {
                LCHeartbeat();
            }
        }
    });
#endif
}

//连接超时
void SimpleBridge::onLobbyTunnelConnectTimeout()    //连接大厅服务器超时
{
    MAKE_PARAM_ROOT(RESULT_TIMEOUT);
    callNative("onLobbyTunnelConnectTimeout", GetStrFromRoot(paramRoot));
}

//连接错误
void SimpleBridge::onLobbyTunnelConnectError(const int code) {
    MAKE_PARAM_ROOT(code);
    callNative("onLobbyTunnelConnectError", GetStrFromRoot(paramRoot));
}

//服务器主动断开的连接,客户端recv == 0的时候,回调到以下的接口
void SimpleBridge::onLobbyTunnelClose()    //断开大厅服务器
{
    MAKE_PARAM_ROOT(0);
    callNative("onLobbyTunnelClose", GetStrFromRoot(paramRoot));
}

//客户端recv异常,send异常,网络层buf溢出,select出现问题,都会回调到这个以下接口
void SimpleBridge::onLobbyTunnelError(const int code) {
    MAKE_PARAM_ROOT(code);
    callNative("onLobbyTunnelError", GetStrFromRoot(paramRoot));
}

char NetXorKey[] = "yf88wef24JDFH#$";

/*
@param code: 0 正常返回，-9999超时
@param cmd: 如果请求超时，那么对应的请求seq会传过来
*/
void
SimpleBridge::onLobbyMsg(const int code, const char *msg, const unsigned int len, const int seq) {
    //LOGD("________"
    //"Recv Msg: code = %d msg[%d[=[%s]\n", code, len, ByteString(msg,len));
    short netRet;
    netRet = (short) code;
    if (code == 0) {
		static Wrap::NativeBuffer tempNativeBuf;
		Wrap::NativeBuffer* nativeBuf = &tempNativeBuf;
		nativeBuf->clearBuffer();
		if (!nativeBuf->writeStringNoLen(len, msg)){
			LOGE("单个协议超过最大长度 %d > 65535 !!!", len);
			return;
		}

        unsigned short packageLen;
        nativeBuf->readUShort(packageLen);
        //LOGI("packageLen = " + packageLen + ",len=" + len);
        if ((packageLen + 2) != len) {//解析长度不一致，协议故障，，通常是开发人员导致的错误
            //this.mAppPresenter.onMsgBack(RESULT_ERROR_PARSE, jsonResult.arg3, null);
            LOGE("onLobbyMsg parse package len not the same %d != %d", packageLen, len);
            return;
        }

		char *buffer = (char*)wrap_calloc(len);// new char[len];
        if (!buffer) {
            LOGE("%s:OOM", __FUNCTION__);
            return;
        }
        //std::unique_ptr<char> pAuto(buffer);//自动指针
		Wrap::VoidGuard guard(buffer);

		nativeBuf->readBuffer(buffer, packageLen);//读取字符串
        std::string xorStr = XorString(buffer, packageLen, NetXorKey, strlen(NetXorKey));

        nativeBuf->clearBuffer();
        nativeBuf->writeStringNoLen(xorStr.length(), xorStr.c_str());

        short cmd;
        short seq;

        nativeBuf->readShort(cmd);
        nativeBuf->readShort(seq);
        nativeBuf->readShort(netRet);

		LOGD("%s : cmd = %d,seq = %d,ret = %d", __FUNCTION__, cmd, seq, netRet);

		MethodParam mp;
		if (seq > 0){
			mp = std::move(removeSeq(seq));
			LCSetSeqIsBack(seq);
		}

		//构造我们的回调json数据
		MAKE_PARAM_ROOT(netRet);
		if (netRet != 0){//请求失败，那么带上我们之前的请求数据
			rapidjson::Value value;
			value.SetString(mp.param.c_str(), mp.param.size(), allocator);
			paramRoot.AddMember("request", value, allocator);//加上请求的的东西，告诉应用层，这个请求超时了
		}

        if (cmd == CMD_LOGIN_C2S2C) {
			callNative("login", GetStrFromRoot(paramRoot));
        } else if (cmd == CMD_SAYTO_C2S2C) {
// 			PreciseTimer timer;
// 			printf("测试开始！！！\n");
// 			timer.start();
			Wrap::BufferValue* data = AutoParseNativeBufferEx(nativeBuf);
            AddParamNames(doc, paramRoot, data, 4, "arg0", "arg1", "arg2", "arg3", "arg4");
            callNative("sayTo", GetStrFromRoot(paramRoot));
			ReleaseBufferValue(data);
// 			timer.stop();
// 			printf("CMD_SAYTO_C2S2C 消耗 %lf 微秒\n", timer.getElapsedTimeInMicroSec());
        } else if (cmd == CMD_NOTIFY_S2C) {
			Wrap::BufferValue* data = AutoParseNativeBufferEx(nativeBuf);
            AddParamNames(doc, paramRoot, data, 2, "arg0", "arg1");
            callNative("notify", GetStrFromRoot(paramRoot));
			ReleaseBufferValue(data);
        } else if (cmd == CMD_ENTERROOM_C2S2C) {
            //VECBUNIT data = AutoParseNativeBuffer(nativeBuf);
			Wrap::BufferValue* data = AutoParseNativeBufferEx(nativeBuf);
			AddParamNames(doc, paramRoot, data, 2, "arg0", "arg1");
			callNative("enterRoom", GetStrFromRoot(paramRoot));
			ReleaseBufferValue(data);
		}
		else if (cmd == CMD_EXITROOM_C2S2C) {
			Wrap::BufferValue* data = AutoParseNativeBufferEx(nativeBuf);
            AddParamNames(doc, paramRoot, data, 1, "arg0");
            callNative("exitRoom", GetStrFromRoot(paramRoot));
			ReleaseBufferValue(data);
		} else if (cmd == CMD_DRIVEAWAY_S2C){
			//被服务器踢下线了,我们需要主动断开连接
			LCDisconnect();

			callNative("driveAway", GetStrFromRoot(paramRoot));
		}
    } else {
        MethodParam mp = removeSeq(seq);
		MAKE_PARAM_ROOT(code);

        rapidjson::Value value;
        value.SetString(mp.param.c_str(), mp.param.size(),doc.GetAllocator());
        paramRoot.AddMember("request", value, doc.GetAllocator());//加上请求的的东西，告诉应用层，这个请求超时了
        callNative(mp.method, GetStrFromRoot(paramRoot));
    }
}

void SimpleBridge::appendSeq(const int seq, const std::string &method, const std::string &data) {
    Wrap::Guard lock(mSection);
    MethodParam temp = {method, data};
    mMapSeq[seq] = temp;
}

MethodParam SimpleBridge::removeSeq(const int seq) {
	Wrap::Guard lock(mSection);
    MethodParam ret = mMapSeq[seq];
    mMapSeq.erase(seq);
    return ret;
}

int SimpleBridge::callByNative(const std::string &param) {
    int ret = 0;
    //LOGD("%s param=%s",__FUNCTION__,param.c_str());
    
    rapidjson::Document doc;
    //rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> &allocator = doc.GetAllocator();
    rapidjson::Value root(rapidjson::kObjectType);

    doc.Parse(param.c_str());
    if (doc.HasParseError())
        goto FAILED;
    else {

        std::string method;
        if (doc.HasMember(MACRO_METHOD)) {
            method = doc[MACRO_METHOD].GetString();
        } else {
            goto FAILED;
        }

        if (method == "startClient") {
            ret = LCStartClient();
        } else if (method == "stopClient") {
			LCDisconnect();//关闭可能的客户端连接
            bool fish = strcmp(doc["arg0"].GetString(), "1") == 0;
            LCStopClient(fish);
        } else if (method == "isConnected") {
            ret = LCIsConnected() ? 1 : 0;
        } else if (method == "connectLobby") {
            std::string ip = doc["arg0"].GetString();
            short port = (short) atoi(doc["arg1"].GetString());
            int to = atoi(doc["arg2"].GetString());

            ret = LCConnect(ip.c_str(), port, to);
        } else if (method == "disConnectLobby") {
            ret = LCDisconnect();
        } else if (method == "login") {
            int seq = LCGetSeq();

            std::string uid = doc["arg0"].GetString();
            std::string token = doc["arg1"].GetString();
            std::string appkey = doc["arg2"].GetString();
            ret = LCLogin(seq, uid.c_str(), token.c_str(), appkey.c_str());

            if (ret == seq) {
                appendSeq(seq, method, param);
                ret = 0;
            }
        } else if (method == "logout") {
            ret = LCLogout();
        } else if (method == "sayTo") {
            int seq = LCGetSeq();

            int type = atoi(doc["arg0"].GetString());
            std::string from = doc["arg1"].GetString();
            std::string to = doc["arg2"].GetString();
            std::string content = doc["arg3"].GetString();
            std::string ext = doc["arg4"].GetString();

            ret = LCSayTo(seq, type, from.c_str(), to.c_str(), content.c_str(), ext.c_str());

            if (ret == seq) {
                appendSeq(seq, method, param);
                ret = 0;
            }
        } else if (method == "enterRoom") {
            int seq = LCGetSeq();

            std::string room_id = doc["arg0"].GetString();
            ret = LCEnterRoom(seq, room_id.c_str());

            if (ret == seq) {
                appendSeq(seq, method, param);
                ret = 0;
            }
        } else if (method == "exitRoom") {
            ret = LCExitRoom();
        } else {
            goto FAILED;
        }

        return ret;
    }

FAILED:
    return -1;
}


