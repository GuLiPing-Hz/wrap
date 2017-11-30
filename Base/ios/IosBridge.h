//
//  IosBridge.hpp
//  fishjs
//
//  Created by glp on 2017/8/15.
//
//

#ifndef IosBridge_hpp
#define IosBridge_hpp

#include <stdio.h>
#import <Foundation/Foundation.h>
#include "IIosBridge.h"

class IosBridge : public IIosBridge
{
    IosBridge();
    virtual ~IosBridge();
public:
    static IosBridge* getInstance();
    
    virtual void callNative(const char* method, const char* param, char* ret);
};

#endif /* IosBridge_hpp */
