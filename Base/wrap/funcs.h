#ifndef FUNCS_H___
#define FUNCS_H___

#include <string>

/************************************************************************/
/* 字节序判断，实现大端小端排序                                                                     */
/************************************************************************/

/* little endian */
#define	OP_LITTLEENDIAN	'<'
/* big endian */
#define	OP_BIGENDIAN	'>'
/* native endian */
#define	OP_NATIVE	'='

bool doendian(int c);

void doswap(bool swap, void *p, size_t n);

/* From Libevent begin https://github.com/nmathewson/Libevent */
unsigned int
evutil_weakrand_seed_(unsigned int *state, unsigned int seed);

int
evutil_weakrand_(unsigned int *state);

int
evutil_weakrand_range_(unsigned int *state, int top);
/* From Libevent end*/


/*
* Copy src to string dst of size siz.  At most siz-1 characters
* will be copied.  Always NUL terminates (unless siz == 0).
* Returns strlen(src); if retval >= siz, truncation occurred.
*/
size_t StrLCpy(char *dst, const char *src, size_t siz);
#define STR_CPY(dst,src) StrLCpy(dst,src,sizeof(dst))

//打印密文，通过16进制的方式打印
void LogCiphertext(const unsigned char* ciphertext, size_t len);

const char* ByteString(const char *msg, const unsigned int len);

// This function sleeps for the specified number of milliseconds.
// It may return early if the thread is woken by some other event,
// such as the delivery of a signal on Unix.
void SleepMs(int msecs);

//异或简单加密算
std::string XorString(const char *data, int datalen, const char *key, int len);



#endif//FUNCS_H___
