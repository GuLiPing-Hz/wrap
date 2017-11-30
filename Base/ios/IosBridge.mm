//
//  IosBridge.cpp
//  fishjs
//
//  Created by glp on 2017/8/15.
//
//

#include "IosBridge.h"
#include "JSIosBridge.h"
#include <string.h>

IosBridge::IosBridge(){
    
}
IosBridge::~IosBridge(){
    
}

IosBridge* IosBridge::getInstance(){
    static IosBridge ins;
    return &ins;
}

void IosBridge::callNative(const char* method, const char* param, char* ret){
    NSString* nsMethod = [[NSString alloc] initWithUTF8String:method];
    NSString* nsParam = [[NSString alloc] initWithUTF8String:param];
    NSString* temp = [JSIosBridge callNativeFromJs:nsMethod withParam:nsParam];
    if(ret)
        strcpy(ret,temp?temp.UTF8String:"");
}
