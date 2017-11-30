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

const char* ByteString(const char *msg, const unsigned int len);


// This function sleeps for the specified number of milliseconds.
// It may return early if the thread is woken by some other event,
// such as the delivery of a signal on Unix.
void SleepMs(int msecs);


//异或简单加密算
std::string XorString(const char *data, int datalen, const char *key, int len);

#endif//FUNCS_H___
