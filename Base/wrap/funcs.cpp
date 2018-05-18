#include "funcs.h"
#include "wrap_config.h"
#include <sstream>
#include <time.h>
#include <stdint.h>
#include "pool.h"

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else

#include <unistd.h>

#endif

Wrap::Allocator *Wrap::Allocator::sIns = NULL;
Wrap::PoolMgr *Wrap::PoolMgr::sIns = NULL;

static std::string sPath;
static int sLevel = 0;
const char *gLogStr[] = {"VERB", "DEBU", "INFO", "WARN", "ERRO"};

void SetLogLevel(int level) {
    sLevel = level;
}

void SetLogToFile(const char *path) {
    sPath = path;
}

void PrintConsole(const char *log, int level) {
    //如果上面没有成功打印日志，那么我们直接输出到屏幕上
#ifdef _WIN32
    OutputDebugStringA(log);
#elif defined(ANDROID)
    switch (level) {
        case 0:
            __android_log_print(ANDROID_LOG_VERBOSE, "Wrap", "%s", log);
            break;
        case 1:
            __android_log_print(ANDROID_LOG_DEBUG, "Wrap ", "%s", log);
            break;
        case 2:
            __android_log_print(ANDROID_LOG_INFO, "Wrap ", "%s", log);
            break;
        case 3:
            __android_log_print(ANDROID_LOG_WARN, "Wrap ", "%s", log);
            break;
        case 4:
            __android_log_print(ANDROID_LOG_ERROR, "Wrap ", "%s", log);
            break;
    }
#else// ios or other
    printf(log);
#endif // _WIN32
}

void Printf(int level, const char *file, long line, const char *format, ...) {
    if (level < sLevel || level > 4)
        return;

    va_list args;
    va_start(args, format);
    char buf[1024];
    vsprintf(buf, format, args);
    va_end(args);

    static char timeLog[1024] = {0};
    time_t timep;
    struct tm *p;
    time(&timep);
    p = gmtime(&timep); /* 获取当前时间 */
    sprintf(timeLog, "%02d:%02d:%02d %lld", (p->tm_hour + 8), p->tm_min, p->tm_sec, timep);
    
    std::stringstream os;//file
    os << "[" << gLogStr[level] << "]" << timeLog << "_" << line << " : " << buf << std::endl;

    if (!sPath.empty()) {//是否已经指定写入文件
        static char timeBuf[260] = {0};/* 获取当前时间 */
        sprintf(timeBuf, "%s_%02d-%02d.log", sPath.c_str(), (1 + p->tm_mon), p->tm_mday);

        FILE *fp = fopen(timeBuf, "ab");
        if (fp != NULL) {
            fprintf(fp, "[%s]%s", timeLog, os.str().c_str());
            fflush(fp);
            fclose(fp);
            return;
        }
    }

    PrintConsole(os.str().c_str(), level);
}

bool doendian(int c) {    // 0x12345678
	// 低地址 - > 高地址
    //		12 34 56 78		大端字节序  -- 内存低地址保存数据的高字节
    //		78 56 34 12		小端字节序 -- 内存低地址保存数据的低字节
    int x = 1;//0x00000001
    int e = *(char *) &x;
    if (c == OP_LITTLEENDIAN) return !e;//当前使用小端格式，本机字节码是大端的 1
    if (c == OP_BIGENDIAN) return e == 1;//当前使用大端格式，本机字节码是小端的 1
    if (c == OP_NATIVE) return 0;
    return 0;
}

void doswap(bool swap, void *p, size_t n) {
    if (swap) {
        char *a = (char *) p;
        int i, j;
        for (i = 0, j = n - 1, n = n / 2; n--; i++, j--) {
            char t = a[i];
            a[i] = a[j];
            a[j] = t;
        }
    }
}


//come from Libevent begin
unsigned int
evutil_weakrand_seed_(unsigned int *state, unsigned int seed) {
    if (seed == 0) {
        seed = (unsigned int) time(NULL);
#ifdef _WIN32
        seed += (unsigned int)_getpid();
#else
        seed += (unsigned int) getpid();
#endif
    }
    *state = seed;
    return seed;
}

int
evutil_weakrand_(unsigned int *state) {
    /* This RNG implementation is a linear congruential generator, with
    * modulus 2^31, multiplier 1103515245, and addend 12345.  It's also
    * used by OpenBSD, and by Glibc's TYPE_0 RNG.
    *
    * The linear congruential generator is not an industrial-strength
    * RNG!  It's fast, but it can have higher-order patterns.  Notably,
    * the low bits tend to have periodicity.
    */
    *state = (*state * 1103515245 + 12345) & 0x7fffffff;
    return (int) (*state);
}

int
evutil_weakrand_range_(unsigned int *state, int top) {
    int divisor, result;

    /* We can't just do weakrand() % top, since the low bits of the LCG
    * are less random than the high ones.  (Specifically, since the LCG
    * modulus is 2^N, every 2^m for m<N will divide the modulus, and so
    * therefore the low m bits of the LCG will have period 2^m.) */
    divisor = INT32_MAX / top;
    do {
        result = evutil_weakrand_(state) / divisor;
    } while (result >= top);
    return result;
}
//come from Libevent end

size_t StrLCpy(char *dst, const char *src, size_t siz) {
    register char *d = dst;//register把变量放到cpu寄存器中，提高访问速度
    register const char *s = src;
    register size_t n = siz;

    /* Copy as many bytes as will fit */
    if (n != 0 && --n != 0) {
        do {
            if ((*d++ = *s++) == 0)
                break;
        } while (--n != 0);
    }

    /* Not enough room in dst, add NUL and traverse rest of src */
    if (n == 0) {
        if (siz != 0)
            *d = '\0';        /* NUL-terminate dst */
        while (*s++);
    }

    return (s - src - 1);    /* count does not include NUL */
}

void LogCiphertext(const unsigned char *ciphertext, size_t len) {
    std::stringstream ss;
    size_t pos = 0;
    while (pos < len) {
        char buf[10] = {0};
        sprintf(buf, "%02x-", ciphertext[pos] & 0xff);
        ss << buf;
        pos++;
    }
	ss << std::endl;
    PrintConsole(ss.str().c_str());
}

const char *ByteString(const char *msg, const unsigned int len) {
    if (len > 30000)
        return "";

    static char buf[65535] = {0};
    buf[0] = 0;
    for (unsigned int i = 0; i < len; i++) {
        sprintf(buf, "%s%02x-", buf, msg[i] & 0xff);
    }
    return buf;
}

void SleepMs(int msecs) {
#ifdef WIN32
    Sleep(msecs);
#else
    struct timespec short_wait;
    struct timespec remainder;
    short_wait.tv_sec = msecs / 1000;
    short_wait.tv_nsec = (msecs % 1000) * 1000 * 1000;
    nanosleep(&short_wait, &remainder);
#endif
}

std::string XorString(const char *data, int datalen, const char *key, int len) {
    char *pBuf = new char[datalen];
    if (!pBuf)
        return "oom";

    for (int i = 0; i < datalen; i++) {
        pBuf[i] = data[i] ^ key[i % len];
    }

    std::string ret(pBuf, datalen);
	delete pBuf;

    return ret;
}

