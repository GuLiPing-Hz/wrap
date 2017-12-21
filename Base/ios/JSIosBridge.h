//
//  JSIosBridge.h
//  fishjs
//
//  Created by glp on 2017/8/14.
//
//

#import <Foundation/Foundation.h>

@interface CallNativeArg : NSObject <NSCoding, NSCopying>

@property (nonatomic, copy) NSNumber *code;

@property (nonatomic, copy) NSString *arg0;
@property (nonatomic, copy) NSString *arg1;
@property (nonatomic, copy) NSString *arg2;
@property (nonatomic, copy) NSString *arg3;
@property (nonatomic, copy) NSString *arg4;
@property (nonatomic, copy) NSString *arg5;

- (id)initWithDictionary:(NSDictionary *)dictionary;

@end

@protocol ProtocolVisitUrl <NSObject>

-(void)doFor:(int)payWay withUrlOrGoodId:(NSString*)param withSuccess:(NSString*)success withFail:(NSString*)fail;
-(void)goH5:(NSString*)url;

@end

@interface JSIosBridge : NSObject{
    
}

+(NSString*)deviceVersion;
+(void)setPhoneModel:(NSString*)phoneModel;
+(void)saveUUIDToKeyChain;
+(NSString *)readUUIDFromKeyChain;
+(NSString *)getUUIDString;

+(void)registerVisiter:(id<ProtocolVisitUrl>)visiter;
+(void)registerMob;
//调用静态方法
+(NSString*)callNativeFromJs:(NSString*)method withParam:(NSString*)param;

+(void)setPayResult:(int)code;
+(void)setPayResultFail:(NSString*)reason;
+(void)setPayResultSuccess:(NSString*)productId withReceipt:(NSString*)receipt;

@end
