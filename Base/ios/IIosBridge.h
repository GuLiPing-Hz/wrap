//
//  IIosBridge.h
//  fishjs
//
//  Created by glp on 2017/8/15.
//
//

#ifndef IIosBridge_h
#define IIosBridge_h

class IIosBridge{
public:
	virtual ~IIosBridge(){}

	virtual void callNative(const char* method, const char* param, char* ret) = 0;
};

#endif /* IIosBridge_h */
