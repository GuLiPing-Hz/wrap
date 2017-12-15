#include "funcs.h"
#include <stdio.h>
#include "config.h"
#include "pool.h"

Wrap::PoolMgr* Wrap::PoolMgr::sIns = NULL;

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef NETUTIL_ANDROID
#include <android/log.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <sstream>
#include <time.h>
#include <string.h>

static std::string sPath;
static int sLevel = 0;
const char* gLogStr[] = { "VERB", "DEBU", "INFO", "WARN", "ERRO" };

void SetLogLevel(int level)
{
	sLevel = level;
}

void SetLogToFile(const char* path)
{
	sPath = path;
}

void PrintConsole(const char* log)
{
	//�������û�гɹ���ӡ��־����ô����ֱ���������Ļ��
#ifdef _WIN32
	OutputDebugStringA(log);
#elif defined(ANDROID)
	switch (level){
	case 0:
		__android_log_print(ANDROID_LOG_VERBOSE, "Wrap", log);
		break;
	case 1:
		__android_log_print(ANDROID_LOG_DEBUG, "Wrap ", log);
		break;
	case 2:
		__android_log_print(ANDROID_LOG_INFO, "Wrap ", log);
		break;
	case 3:
		__android_log_print(ANDROID_LOG_WARN, "Wrap ", log);
		break;
	case 4:
		__android_log_print(ANDROID_LOG_ERROR, "Wrap ", log);
		break;
	}
#else// ios or other
	printf(log);
#endif // _WIN32
}

void Printf(int level, const char* file, long line, const char* format, ...)
{
	if (level < sLevel || level > 4)
		return;

	va_list args;
	va_start(args, format);
	char buf[1024];
	vsprintf(buf, format, args);
	va_end(args);

	std::stringstream os;
	os << "[" << gLogStr[level] << "]" << file << "_" << line << " : " << buf;//std::endl;

	if (!sPath.empty()){//�Ƿ��Ѿ�ָ��д���ļ�
		static char timeBuf[260];
		time_t timep;
		struct tm *p;
		time(&timep);
		p = gmtime(&timep); /* ��ȡ��ǰʱ�� */
		sprintf(timeBuf, "%s_%02d-%02d.log", sPath.c_str(), (1 + p->tm_mon), p->tm_mday);

		FILE* fp = fopen(timeBuf, "ab");
		if (fp != NULL){
			sprintf(timeBuf, "%04d-%02d-%02d,%02d:%02d:%02d %lld", (1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday
				, (p->tm_hour + 8), p->tm_min, p->tm_sec, timep);

			fprintf(fp, "[%s]%s", timeBuf, os.str().c_str());
			fflush(fp);
			fclose(fp);
			return;
		}
	}

	PrintConsole(os.str().c_str());
}

bool doendian(int c)
{	// 0x12345678
	// 12 34 56 78 ����ֽ���
	// 78 56 34 12 С���ֽ���
	int x = 1;//0x00000001
	int e = *(char*)&x;
	if (c == OP_LITTLEENDIAN) return !e;//��ǰʹ��С�˸�ʽ�������ֽ����Ǵ�˵� 1
	if (c == OP_BIGENDIAN) return e == 1;//��ǰʹ�ô�˸�ʽ�������ֽ�����С�˵� 1
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

size_t StrLCpy(char *dst, const char *src, size_t siz)
{
	register char *d = dst;//register�ѱ����ŵ�cpu�Ĵ����У���߷����ٶ�
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
			*d = '\0';		/* NUL-terminate dst */
		while (*s++)
			;
	}

	return (s - src - 1);	/* count does not include NUL */
}

void LogCiphertext(const unsigned char* ciphertext, size_t len)
{
	std::stringstream ss;
	int pos = 0;
	while (pos < len){
		char buf[10] = { 0 };
		sprintf(buf, "%x-", ciphertext[pos] & 0xff);
		ss << buf;
		pos++;
	}
	PrintConsole(ss.str().c_str());
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
	char *pBuf = (char*)calloc_(datalen);
	Wrap::VoidGuard guard(pBuf);
	if (!pBuf)
		return "oom";

	for (int i = 0; i < datalen; i++) {
		pBuf[i] = data[i] ^ key[i % len];
	}

	std::string ret(pBuf, datalen);
	return ret;
}

