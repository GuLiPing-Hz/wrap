//
//  JSIosBridge.m
//  fishjs
//
//  Created by glp on 2017/8/14.
//
//
#include "simple_util_bridge_JsIOSBridge.h"
#import "sys/utsname.h"
#import "JSIosBridge.h"
#import  <Security/Security.h>
#import "KeychainItemWrapper.h"
#import <SMS_SDK/SMSSDK.h>
#import "Reachability.h"
#include "simple_util_bridge_JsIOSBridge.h"
#include <string.h>

#define IOS_CHANNEL @"Official"

#define RESULT_METHOD @"method"
#define RESULT_CODE  @"code"
#define RESULT_ARG0  @"arg0"
#define RESULT_ARG1  @"arg1"
#define RESULT_ARG2  @"arg2"
#define RESULT_ARG3  @"arg3"
#define RESULT_ARG4  @"arg4"
#define RESULT_ARG5  @"arg5"

//Js Call Native
#define JS_2_NATIVE_GET_NET_STATUS  @"GET_NET_STATUS"
#define JS_2_NATIVE_GET_TOKEN  @"GET_TOKEN"
#define JS_2_NATIVE_GET_CODE  @"GET_CODE"
//获取UUID
#define JS_2_NATIVE_GET_UUID  @"GET_UUID"
//获取手机型号
#define JS_2_NATIVE_GET_PHONEMODEL  @"GET_PHONEMODEL"
//获取产品渠道，android获取渠道，ios获取bundle id
#define JS_2_NATIVE_GET_FLAVOR  @"GET_FLAVOR"
#define JS_2_NATIVE_PAY  @"PAY"
#define JS_2_NATIVE_GOH5 @"GOH5"
#define JS_2_NATIVE_LOG @"LOG"

//Native Call Js
#define NATIVE_2_JS_GET_CODE  @"CALLBACK_GET_CODE"
#define NATIVE_2_JS_PAY_RESULT @"CALLBACK_PAY_RESULT"

#define Net_None  0
#define Net_Wifi  1
#define Net_Mobile  2

@implementation CallNativeArg

- (id)initWithDictionary:(NSDictionary *)dictionary {
    self = [self init];
    if (self == nil) return nil;
    
    _code = dictionary[@"code"];
    self.arg0 = [dictionary[@"arg0"] copy];
    self.arg1 = [dictionary[@"arg1"] copy];
    self.arg2 = [dictionary[@"arg2"] copy];
    self.arg3 = [dictionary[@"arg3"] copy];
    self.arg4 = [dictionary[@"arg4"] copy];
    
    return self;
}

- (id)initWithCoder:(NSCoder *)coder {
    self = [self init];
    if (self == nil) return nil;
    
    _code = [coder decodeObjectForKey:@"code"];
    self.arg0 = [coder decodeObjectForKey:@"arg0"];
    self.arg1 = [coder decodeObjectForKey:@"arg1"];
    self.arg2 = [coder decodeObjectForKey:@"arg2"];
    self.arg3 = [coder decodeObjectForKey:@"arg3"];
    self.arg4 = [coder decodeObjectForKey:@"arg4"];
    self.arg5 = [coder decodeObjectForKey:@"arg5"];
    
    return self;
}

- (void)encodeWithCoder:(NSCoder *)coder {
    if (_code != nil) [coder encodeObject:_code forKey:@"code"];
    if (self.arg0 != nil) [coder encodeObject:self.arg0 forKey:@"arg0"];
    if (self.arg1 != nil) [coder encodeObject:self.arg1 forKey:@"arg1"];
    if (self.arg2 != nil) [coder encodeObject:self.arg2 forKey:@"arg2"];
    if (self.arg3 != nil) [coder encodeObject:self.arg3 forKey:@"arg3"];
    if (self.arg4 != nil) [coder encodeObject:self.arg4 forKey:@"arg4"];
    if (self.arg5 != nil) [coder encodeObject:self.arg5 forKey:@"arg5"];
}

- (id)copyWithZone:(NSZone *)zone {
    
    CallNativeArg *ret = [[self.class allocWithZone:zone] init];
    ret->_code = self.code;
    ret.arg0 = self.arg0;
    ret.arg1 = self.arg1;
    ret.arg2 = self.arg2;
    ret.arg3 = self.arg3;
    ret.arg4 = self.arg4;
    ret.arg5 = self.arg5;
    
    return ret;
}

@end

@implementation JSIosBridge

static char sPhoneModel[250] = {0};

static id<ProtocolVisitUrl> sVisiter = nullptr;

+(void)registerVisiter:(id<ProtocolVisitUrl>)visiter
{
    sVisiter = visiter;
}

+(void)setPhoneModel:(NSString*)phoneModel{
    strcpy(sPhoneModel,phoneModel.UTF8String);
}

#define UUID_ACCOUNT @"Identfier"
#define UUID_SERVICE @"AppName"
#define UUID_ACCESSG @"20171314.com.pani.userinfo"
#pragma mark - 保存和读取UUID
+(void)saveUUIDToKeyChain
{
    KeychainItemWrapper *keychainItem = [[KeychainItemWrapper alloc] initWithAccount:UUID_ACCOUNT service:UUID_SERVICE accessGroup:nil];
    NSString *string = [keychainItem objectForKey: (__bridge id)kSecAttrGeneric];
    NSLog(@"saveUUIDToKeyChain uuid =%@",string);
    if([string isEqualToString:@""] || !string){
        [keychainItem setObject:[self getUUIDString] forKey:(__bridge id)kSecAttrGeneric];
    }
}

+(NSString *)readUUIDFromKeyChain
{
    KeychainItemWrapper *keychainItemm = [[KeychainItemWrapper alloc] initWithAccount:UUID_ACCOUNT service:UUID_SERVICE accessGroup:nil];
    NSString *UUID = [keychainItemm objectForKey: (__bridge id)kSecAttrGeneric];
    NSLog(@"readUUIDFromKeyChain uuid =%@",UUID);
    return UUID;
}

+(NSString *)getUUIDString
{
    CFUUIDRef uuidRef = CFUUIDCreate(kCFAllocatorDefault);
    CFStringRef strRef = CFUUIDCreateString(kCFAllocatorDefault , uuidRef);
    NSString *uuidString = [(__bridge NSString*)strRef stringByReplacingOccurrencesOfString:@"-" withString:@""];
    CFRelease(strRef);
    CFRelease(uuidRef);
    return uuidString;
}

+ (NSString*)deviceVersion
{
    // 需要#import "sys/utsname.h"
    struct utsname systemInfo;
    uname(&systemInfo);
    NSString * deviceString = [NSString stringWithCString:systemInfo.machine encoding:NSUTF8StringEncoding];
    //iPhone
    if ([deviceString isEqualToString:@"iPhone1,1"])    return @"iPhone 1G";
    if ([deviceString isEqualToString:@"iPhone1,2"])    return @"iPhone 3G";
    if ([deviceString isEqualToString:@"iPhone2,1"])    return @"iPhone 3GS";
    if ([deviceString isEqualToString:@"iPhone3,1"])    return @"iPhone 4";
    if ([deviceString isEqualToString:@"iPhone3,2"])    return @"Verizon iPhone 4";
    if ([deviceString isEqualToString:@"iPhone4,1"])    return @"iPhone 4S";
    if ([deviceString isEqualToString:@"iPhone5,1"])    return @"iPhone 5";
    if ([deviceString isEqualToString:@"iPhone5,2"])    return @"iPhone 5";
    if ([deviceString isEqualToString:@"iPhone5,3"])    return @"iPhone 5C";
    if ([deviceString isEqualToString:@"iPhone5,4"])    return @"iPhone 5C";
    if ([deviceString isEqualToString:@"iPhone6,1"])    return @"iPhone 5S";
    if ([deviceString isEqualToString:@"iPhone6,2"])    return @"iPhone 5S";
    if ([deviceString isEqualToString:@"iPhone7,1"])    return @"iPhone 6 Plus";
    if ([deviceString isEqualToString:@"iPhone7,2"])    return @"iPhone 6";
    if ([deviceString isEqualToString:@"iPhone8,1"])    return @"iPhone 6s";
    if ([deviceString isEqualToString:@"iPhone8,2"])    return @"iPhone 6s Plus";
    
    return deviceString;
}

// 将NSDictionary或NSArray转化为JSON字符串
+(NSString*) ObjToJSONString:(id)obj
{
    NSData *jsonData = [NSJSONSerialization dataWithJSONObject:obj options:NSJSONWritingPrettyPrinted error:NULL];
    
    if (jsonData){
        return [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];;
    }else{
        return nil;
    }
}

//将JSON字符串转化为NSDictionary或NSArray
+(id) JSONStringToObj:(NSString*)str
{
    //将NSString转化为NSData
    NSData* data = [str dataUsingEncoding:NSASCIIStringEncoding];
    
    if(data){
        id jsonObject = [NSJSONSerialization JSONObjectWithData:data options:NSJSONReadingAllowFragments error:NULL];
        
        return jsonObject;
    }else
        return nil;
}

+(void)registerMob
{
    [SMSSDK registerApp:@"1eb63d197f5c7" withSecret:@"05150de50dfb7f890cf7ce5f038bbf67"];
}

//调用静态方法
+(NSString*)callNativeFromJs:(NSString*)method withParam:(NSString*)param
{
    //CallNativeArg ret = new CallNativeArg();
    NSMutableDictionary* ret = [[NSMutableDictionary alloc] init];
    
    if ([method caseInsensitiveCompare: JS_2_NATIVE_GET_NET_STATUS] == NSOrderedSame) {
        NetworkStatus netStatus = [[Reachability reachabilityWithHostName:@"www.baidu.com"] currentReachabilityStatus];
        [ret setValue:[NSNumber numberWithInt:0] forKey:RESULT_CODE];
        if(netStatus == NotReachable)
            [ret setValue: [NSNumber numberWithInt:Net_None] forKey:RESULT_ARG0];
        else if(netStatus == ReachableViaWiFi)
            [ret setValue: [NSNumber numberWithInt:Net_Wifi] forKey:RESULT_ARG0];
        else if(netStatus == ReachableViaWWAN)
            [ret setValue: [NSNumber numberWithInt:Net_Mobile] forKey:RESULT_ARG0];
    } else if ([method caseInsensitiveCompare: JS_2_NATIVE_GET_TOKEN] == NSOrderedSame) {
        [ret setValue:[NSNumber numberWithInt:0] forKey:RESULT_CODE];
        [ret setValue: @"1ebdde31bd197f9be86a49f568c6c66b" forKey:RESULT_ARG0];
    }else if ([method caseInsensitiveCompare: JS_2_NATIVE_GET_CODE] == NSOrderedSame ) {
        //将字符串写到缓冲区。
        NSData* jsonData = [param dataUsingEncoding:NSUTF8StringEncoding];
        //解析json数据，使用系统方法 JSONObjectWithData:  options: error:
        NSDictionary* dic = [NSJSONSerialization JSONObjectWithData:jsonData options:NSJSONReadingMutableLeaves error:nil];
        
        if(dic != NULL){
            NSString* curPhone = dic[RESULT_ARG0];
            NSString* curArea = dic[RESULT_ARG1];
            if(curPhone && curArea){
                [SMSSDK getVerificationCodeByMethod:SMSGetCodeMethodSMS phoneNumber:curPhone zone:curArea customIdentifier:nil result:^(NSError *error) {
                    NSMutableDictionary* result = [[NSMutableDictionary alloc] init];
                    [result setValue:NATIVE_2_JS_GET_CODE forKey:RESULT_METHOD];
                    if (!error)
                    {// 请求成功
                        [result setValue:[NSNumber numberWithInt:0] forKey:RESULT_CODE];
                        [result setValue:curPhone forKey:RESULT_ARG0];
                    }
                    else
                    {// error 验证码获取错误
                        [result setValue:[NSNumber numberWithInt:2] forKey:RESULT_CODE];
                        [result setValue:curPhone forKey:RESULT_ARG0];
                        [result setValue:[NSNumber numberWithUnsignedLong:error.code] forKey:RESULT_ARG1];
                    }
                    ios_callJsFromNative([self ObjToJSONString:result].UTF8String);
                }];
                [ret setValue:[NSNumber numberWithInt:0] forKey:RESULT_CODE];
            } else{
                [ret setValue:[NSNumber numberWithInt:1] forKey:RESULT_CODE];
            }
        }else{
            [ret setValue:[NSNumber numberWithInt:1] forKey:RESULT_CODE];
        }
    } else if ([method caseInsensitiveCompare: JS_2_NATIVE_GET_UUID] == NSOrderedSame  ) {
        [ret setValue:[NSNumber numberWithInt:0] forKey:RESULT_CODE];
        [ret setValue: [self readUUIDFromKeyChain] forKey:RESULT_ARG0];
    } else if ([method caseInsensitiveCompare: JS_2_NATIVE_GET_PHONEMODEL] == NSOrderedSame ) {
        [ret setValue:[NSNumber numberWithInt:0] forKey:RESULT_CODE];
        [ret setValue: [[NSString alloc] initWithUTF8String:sPhoneModel] forKey:RESULT_ARG0];
    } else if ([method caseInsensitiveCompare: JS_2_NATIVE_GET_FLAVOR] == NSOrderedSame ) {
        [ret setValue:[NSNumber numberWithInt:0] forKey:RESULT_CODE];
        [ret setValue: IOS_CHANNEL forKey:RESULT_ARG0];
    } else if([method caseInsensitiveCompare: JS_2_NATIVE_PAY] == NSOrderedSame){
        //将字符串写到缓冲区。
        NSData* jsonData = [param dataUsingEncoding:NSUTF8StringEncoding];
        //解析json数据，使用系统方法 JSONObjectWithData:  options: error:
        NSDictionary* dic = [NSJSONSerialization JSONObjectWithData:jsonData options:NSJSONReadingMutableLeaves error:nil];
        bool isSuccess = false;
        
        if(dic && sVisiter){
            NSNumber* payWay = dic[RESULT_ARG0];
            NSString* urlOrGoodId = dic[RESULT_ARG1];
            NSString* urlSuccess = dic[RESULT_ARG3];
            NSString* urlFail = dic[RESULT_ARG4];
            
            if(payWay && urlOrGoodId){
                isSuccess = true;
                [ret setValue:[NSNumber numberWithInt:0] forKey:RESULT_CODE];
                [sVisiter doFor:[payWay intValue] withUrlOrGoodId:urlOrGoodId withSuccess:urlSuccess withFail:urlFail];
            }
        }
        
        if(!isSuccess)
            [ret setValue:[NSNumber numberWithInt:1] forKey:RESULT_CODE];
    }
    else if([method caseInsensitiveCompare: JS_2_NATIVE_GOH5] == NSOrderedSame){
        //将字符串写到缓冲区。
        NSData* jsonData = [param dataUsingEncoding:NSUTF8StringEncoding];
        //解析json数据，使用系统方法 JSONObjectWithData:  options: error:
        NSDictionary* dic = [NSJSONSerialization JSONObjectWithData:jsonData options:NSJSONReadingMutableLeaves error:nil];
        
        bool isSuccess = false;
        if(dic && sVisiter){
            NSString* url = dic[RESULT_ARG0];
            
            if(url){
                [ret setValue:[NSNumber numberWithInt:0] forKey:RESULT_CODE];
                [sVisiter goH5:url];
            }
        }
        
        if(!isSuccess)
            [ret setValue:[NSNumber numberWithInt:1] forKey:RESULT_CODE];
    }
    else if([method caseInsensitiveCompare: JS_2_NATIVE_LOG] == NSOrderedSame){
        [ret setValue:[NSNumber numberWithInt:0] forKey:RESULT_CODE];//ios 也不用打印日志
    }
    else{
        [ret setValue:[NSNumber numberWithInt:1] forKey:RESULT_CODE];
    }
    
    return [self ObjToJSONString:ret];
}

+(void)setPayResult:(int)code
{
    NSMutableDictionary* result = [[NSMutableDictionary alloc] init];
    [result setValue:NATIVE_2_JS_PAY_RESULT forKey:RESULT_METHOD];
    if (code == 1)
    {// 请求成功
        [result setValue:[NSNumber numberWithInt:0] forKey:RESULT_CODE];
    }
    else if(code == 2)
    {// 取消
        [result setValue:[NSNumber numberWithInt:2] forKey:RESULT_CODE];
    }else{
        return ;
    }
    ios_callJsFromNative([self ObjToJSONString:result].UTF8String);
}

+(void)setPayResultFail:(NSString*)reason
{
    NSMutableDictionary* result = [[NSMutableDictionary alloc] init];
    [result setValue:NATIVE_2_JS_PAY_RESULT forKey:RESULT_METHOD];
    [result setValue:[NSNumber numberWithInt:3] forKey:RESULT_CODE];
    [result setValue:reason forKey:RESULT_ARG0];
    ios_callJsFromNative([self ObjToJSONString:result].UTF8String);
}

+(void)setPayResultSuccess:(NSString*)productId withReceipt:(NSString*)receipt
{
    NSMutableDictionary* result = [[NSMutableDictionary alloc] init];
    [result setValue:NATIVE_2_JS_PAY_RESULT forKey:RESULT_METHOD];
    [result setValue:[NSNumber numberWithInt:4] forKey:RESULT_CODE];
    [result setValue:productId forKey:RESULT_ARG0];
    [result setValue:receipt forKey:RESULT_ARG1];
    ios_callJsFromNative([self ObjToJSONString:result].UTF8String);
}

@end
