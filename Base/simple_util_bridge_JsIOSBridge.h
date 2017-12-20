#ifndef _Included_simple_util_bridge_JsIOSBridge
#define _Included_simple_util_bridge_JsIOSBridge

#include <string>

#ifdef __cplusplus
extern "C" {
#endif

    /**
     IOS  调用回到JS里面
     */
    void ios_callJsFromNative(const char* param);

#ifdef __cplusplus
}
#endif
#endif
