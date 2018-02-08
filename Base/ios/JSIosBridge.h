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
-(BOOL)sendImgToWX:(NSString*)path withIsPYQ:(BOOL)isPYQ;

@end

@interface JSIosBridge : NSObject{
    
}

+(NSString*)deviceVersion;
+(void)setPhoneModel:(NSString*)phoneModel;
+(void)saveUUIDToKeyChain;
+(NSString *)readUUIDFromKeyChain;
+(NSString *)getUUIDString;
//压缩图片不超过maxLength大小
+ (UIImage *)compressImage:(UIImage *)image toByte:(NSUInteger)maxLength;
//缩小图片的尺寸
+ (UIImage *)scaleImage:(UIImage*)image scaledToSize:(CGSize)newSize;

+(void)registerVisiter:(id<ProtocolVisitUrl>)visiter;
//调用静态方法
+(NSString*)callNativeFromJs:(NSString*)method withParam:(NSString*)param;

+(void)setPayResult:(int)code;
+(void)setPayResultFail:(NSString*)reason;
+(void)setPayResultSuccess:(NSString*)productId withReceipt:(NSString*)receipt;
+(void)setWXShareResult:(int)code;

+(void)saveOrientation:(NSString*)orientation;
+(void)setOrientation:(NSString*)orientation;
@end
