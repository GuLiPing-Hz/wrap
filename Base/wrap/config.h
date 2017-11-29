#ifndef CONFIG__H___
#define CONFIG__H___

/*
命名規則公示：

宏												AAA_BBB_CCC
结构、类名、全局/静态函数、静态成员函数			AaaBbbCcc
枚举												eAaaBbbCcc 或者 _AaaBbbCcc
枚举里面的定义									AAA_BBB_CCC 或者 kAaaBbbCcc
成员变量											mAaaBbbCcc
静态成员变量										sAaaBbbCcc
成员函数											aaaBbbCcc
局部变量											aaa_bbb_ccc 或者 aaaBbbCcc

文件命名											aaa_bbb_ccc


有些不符合以上规则的文件可能来自webrtc,bitcoin项目
*/

//借鉴cocos
#define CC_PLATFORM_UNKNOWN            0
#define CC_PLATFORM_IOS                1
#define CC_PLATFORM_ANDROID            2
#define CC_PLATFORM_WIN32              3
#define CC_PLATFORM_MARMALADE          4
#define CC_PLATFORM_LINUX              5
#define CC_PLATFORM_BADA               6
#define CC_PLATFORM_BLACKBERRY         7
#define CC_PLATFORM_MAC                8
#define CC_PLATFORM_NACL               9
#define CC_PLATFORM_EMSCRIPTEN        10
#define CC_PLATFORM_TIZEN             11
#define CC_PLATFORM_QT5               12
#define CC_PLATFORM_WINRT             13

#define CFG_TARGET_PLATFORM CC_PLATFORM_UNKNOWN

// Apple: Mac and iOS
#if defined(__APPLE__) && !defined(ANDROID) // exclude android for binding generator.
#define NETUTIL_IOS_PHONE
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE // TARGET_OS_IPHONE includes TARGET_OS_IOS TARGET_OS_TV and TARGET_OS_WATCH. see TargetConditionals.h
#undef  CFG_TARGET_PLATFORM
#define CFG_TARGET_PLATFORM         CC_PLATFORM_IOS
#elif TARGET_OS_MAC
#undef  CFG_TARGET_PLATFORM
#define CFG_TARGET_PLATFORM         CC_PLATFORM_MAC
#endif
#endif

// android
#if defined(ANDROID)
#undef  CFG_TARGET_PLATFORM
#define CFG_TARGET_PLATFORM         CC_PLATFORM_ANDROID
#endif

// win32
#if defined(_WIN32) && defined(_WINDOWS)
#undef  CFG_TARGET_PLATFORM
#define CFG_TARGET_PLATFORM         CC_PLATFORM_WIN32
#endif

//GLP 支持64位
//#define __x86_64__

#if CFG_TARGET_PLATFORM == CC_PLATFORM_MAC
//使用MAC xcode编译,如需指定手机需定义下面的宏
#define NETUTIL_MAC
#endif

#if CFG_TARGET_PLATFORM == CC_PLATFORM_IOS
//IOS版本定义
#define NETUTIL_IOS
#endif

#if CFG_TARGET_PLATFORM == CC_PLATFORM_ANDROID
//使用Linux环境
#define NETUTIL_LINUX
//使用NDK编译
#define NETUTIL_ANDROID
#endif

///////////////////////////////////////////////////NETUTIL_ANDROID_PHONE
// #if CFG_TARGET_PLATFORM == CC_PLATFORM_IOS || CFG_TARGET_PLATFORM == CC_PLATFORM_MAC
// 
// //使用MAC xcode编译,如需指定手机需定义下面的宏
// #define NETUTIL_MAC
// //IOS版本定义
// #define NETUTIL_IOS
// 
// #else
// 
// //使用Linux环境
// #define NETUTIL_LINUX
// //使用NDK编译
// #define NETUTIL_ANDROID
// 
// #endif

//定义是否是cocos 项目工程
//#define COCOS_PROJECT

//禁止拷贝构造
// #define DISABLE_COPY_CTOR(cls) \
// private:cls(const cls &);cls &operator=(const cls &)

#define DISABLE_COPY_CTOR(cls) \
cls(const cls&) = delete; \
cls& operator=(const cls&) = delete

#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif//MAX
#ifndef MIN
#define MIN(a,b) (((a)>(b))?(b):(a))
#endif//MIN


#if defined _WIN32
void Printf(const char* format, ...);

// VERBOSE
#define LOGV(...) Printf(__VA_ARGS__)
// DEBUG
#define LOGD(...) Printf(__VA_ARGS__)
// INFO
#define LOGI(...) Printf(__VA_ARGS__)
//WARN
#define LOGW(...) Printf(__VA_ARGS__)
// ERROR
#define LOGE(...) Printf(__VA_ARGS__)

#elif defined(__APPLE__) && !defined(ANDROID)

//IOS
// VERBOSE
#define LOGV(...) printf(__VA_ARGS__)
// DEBUG
#define LOGD(...) printf(__VA_ARGS__)
// INFO
#define LOGI(...) printf(__VA_ARGS__)
//WARN
#define LOGW(...) printf(__VA_ARGS__)
// ERROR
#define LOGE(...) printf(__VA_ARGS__)

#else //defined(ANDROID)	//Android
#include <android/log.h>
// VERBOSE
#define LOGV(...)   __android_log_print(ANDROID_LOG_VERBOSE, "JniNetUtil", __VA_ARGS__)
// DEBUG
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG , "JniNetUtil ", __VA_ARGS__)
// INFO
#define LOGI(...)    __android_log_print(ANDROID_LOG_INFO  , "JniNetUtil ",__VA_ARGS__)
//WARN
#define LOGW(...)  __android_log_print(ANDROID_LOG_WARN  , "JniNetUtil ", __VA_ARGS__)
// ERROR
#define LOGE(...)   __android_log_print(ANDROID_LOG_ERROR  , "JniNetUtil ",__VA_ARGS__)

#define LOGGLP(...)  __android_log_print(ANDROID_LOG_INFO  , " GLP ",__VA_ARGS__)

#endif

#endif//CONFIG__H___
