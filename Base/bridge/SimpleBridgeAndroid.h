#include "../wrap/wrap_config.h"
#ifndef COCOS_PROJECT

#ifndef SIMPLE_BRIDGE_ANDROID_H__
#define SIMPLE_BRIDGE_ANDROID_H__

#include "SimpleBridge.h"
#include "../wrap/mutex.h"


/*
android callback
*/
class SimpleBridgeAndroid : public SimpleBridge{
	SimpleBridgeAndroid();
	virtual ~SimpleBridgeAndroid();
public:
	static SimpleBridgeAndroid* getInstance();
protected:
	virtual void callNative(const std::string& method,const std::string& param);

private:
};

#endif//SIMPLE_BRIDGE_ANDROID_H__

#endif//COCOS_PROJECT
