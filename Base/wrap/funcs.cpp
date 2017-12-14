#include "funcs.h"
#include <stdio.h>
#include "config.h"
#include "pool.h"

Wrap::PoolMgr* Wrap::PoolMgr::sIns = NULL;

#ifdef _WIN32
#include <windows.h>
#else
// For nanosleep()
#include <time.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <sstream>

static std::string sPath;

void SetLogToFile(const char* path)
{
	sPath = path;
}

void Printf(const char* type, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	char buf[1024];
	vsprintf(buf, format, args);
	std::stringstream os;
	os << "[" << type << "] : " << buf << std::endl;

	bool writed = false;
	if (!sPath.empty()){//是否已经指定写入文件
		FILE* fp = fopen(sPath.c_str(), "wb");
		if (fp != NULL){
			SYSTEMTIME st;
			GetLocalTime(&st);
			static char timeBuf[260];
			sprintf(timeBuf, "%04d-%02d-%02d,%02d:%02d:%02d.%03d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

			fprintf(fp, "[%s]%s", timeBuf,os.str().c_str());
			fflush(fp);
			fclose(fp);
			writed = true;
		}
	}

	if (!writed){
#ifdef _WIN32
		OutputDebugStringA(os.str().c_str());
#else
		printf(os.str().c_str());
#endif // _WIN32
	}

	va_end(args);
}

bool doendian(int c)
{	// 0x12345678
	// 12 34 56 78 大端字节序
	// 78 56 34 12 小端字节序
	int x = 1;//0x00000001
	int e = *(char*)&x;
	if (c == OP_LITTLEENDIAN) return !e;//当前使用小端格式，本机字节码是大端的 1
	if (c == OP_BIGENDIAN) return e == 1;//当前使用大端格式，本机字节码是小端的 1
	if (c == OP_NATIVE) return 0;
	return 0;
}

void doswap(bool swap, void *p, size_t n)
{
	if (swap)
	{
		char *a = (char*)p;
		int i, j;
		for (i = 0, j = n - 1, n = n / 2; n--; i++, j--)
		{
			char t = a[i]; a[i] = a[j]; a[j] = t;
		}
	}
}

const char* ByteString(const char *msg, const unsigned int len)
{
	if (len > 30000)
		return "";

	static char buf[65535] = { 0 };
	buf[0] = 0;
	for (unsigned int i = 0; i < len; i++){
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
	delete[] pBuf;
	return ret;
}

