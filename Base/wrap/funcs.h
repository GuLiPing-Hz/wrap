#ifndef FUNCS_H___
#define FUNCS_H___

/************************************************************************/
/* 字节序判断，实现大端小端排序                                                                     */
/************************************************************************/

#define	OP_LITTLEENDIAN	'<'		/* little endian */
#define	OP_BIGENDIAN	'>'		/* big endian */
#define	OP_NATIVE	'='		/* native endian */

bool doendian(int c);

void doswap(bool swap, void *p, size_t n);

const char* ByteString(const char *msg, const unsigned int len);


// This function sleeps for the specified number of milliseconds.
// It may return early if the thread is woken by some other event,
// such as the delivery of a signal on Unix.
void SleepMs(int msecs);

#endif//FUNCS_H___
