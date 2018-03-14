#include "SimpleBridgeAndroid.h"
#include "../jni/JniHelper.h"
#include "../app/NetApp.h"

SimpleBridgeAndroid *SimpleBridgeAndroid::getInstance() {
    static SimpleBridgeAndroid sIns;
    return &sIns;
}

SimpleBridgeAndroid::SimpleBridgeAndroid()
        : SimpleBridge() {
}

SimpleBridgeAndroid::~SimpleBridgeAndroid() {
}

void SimpleBridgeAndroid::callNative(const std::string &method, const std::string &param) {
	
	//const char* errorOccur = NULL;
	////这里我们先检查一下 我们的参数是否是合法的utf8字符串，不然直接传递到java，在android5.0会崩溃
	//CCharsetCodec::CheckUtfChar(param.c_str(), &errorOccur);
	//if (errorOccur)
	//{
	//	LOGE("param is not valid utf8! please enc base64! param = %s", param.c_str());
	//	return;
	//}

	std::string cls_java = "simple/util/bridge/JsAndroidBridge";

	//Wrap::JniHelper::callStaticStringMethod(NetApp::GetInstance()->m_nThreadId,
	//	cls_java, "callNative",method, param);

	//调用callNative2
	std::string ret;
	Wrap::JniMethodInfo t;
	if (Wrap::JniHelper::getStaticMethodInfo(t, NetApp::GetInstance()->m_nThreadId, cls_java.c_str(),
		"callNative2", "(Ljava/lang/String;[B)Ljava/lang/String;")) {
		//LOGD("callStaticStringMethod 1 env = %x", t.env);

		jstring jsMethod = Wrap::JniHelper::newStringUTFJNI(t.env, method, NULL);
		jbyteArray jbArray = t.env->NewByteArray(param.length());
		t.env->SetByteArrayRegion(jbArray, 0, param.length(), (jbyte*)param.c_str());

		jstring jret = (jstring)t.env->CallStaticObjectMethod(t.classID, t.methodID,jsMethod,jbArray);

		//LOGD("callStaticStringMethod 2 env = %x", t.env);
		ret = Wrap::JniHelper::jstring2string(t.env, jret);
		t.env->DeleteLocalRef(t.classID);
		t.env->DeleteLocalRef(jret);

		t.env->DeleteLocalRef(jbArray);
		t.env->DeleteLocalRef(jsMethod);
	}
	else {
		LOGE("Failed to find static java method. Class name: %s, method name: %s, signature: %s ",
			cls_java.c_str(), "callNative2", "(Ljava/lang/String;[B)Ljava/lang/String;");
	}
}



