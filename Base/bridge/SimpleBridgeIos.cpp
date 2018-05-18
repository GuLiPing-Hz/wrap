#include "SimpleBridgeIos.h"

SimpleBridgeIos* SimpleBridgeIos::getInstance()
{
    static SimpleBridgeIos sIns;
    return &sIns;
}

SimpleBridgeIos::SimpleBridgeIos()
	:SimpleBridge()
	, mIosBridge()
{
    
}

SimpleBridgeIos::~SimpleBridgeIos()
{
    mIosBridge = nullptr;
}

void SimpleBridgeIos::callNative(const std::string& method, const std::string& param)
{
    //LOGI("SimpleBridgeIos::callNative method=%s",method.c_str());
    if(mIosBridge != nullptr)
        mIosBridge(method.c_str(),param.c_str());
}




