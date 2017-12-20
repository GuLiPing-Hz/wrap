#include "simple_util_bridge_JsIOSBridge.h"
#include "wrap/config.h"

#ifdef COCOS_PROJECT
#include "bridge/SimpleJsBridge.h"
#else
#include "bridge/SimpleBridgeIos.h"
#endif

void ios_callJsFromNative(const char* param)
{
	LOGI("ios_callJsFromNative param=%s\n",param);
#ifdef COCOS_PROJECT
	SimpleJsBridge::getInstance()->callJsFromNative(param);
#else
    SimpleBridgeIos::getInstance()->callByNative(param);
#endif
}

