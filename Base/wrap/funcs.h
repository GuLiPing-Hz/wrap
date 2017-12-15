#ifndef FUNCS_H___
#define FUNCS_H___

#include <string>

/************************************************************************/
/* �ֽ����жϣ�ʵ�ִ��С������                                                                     */
/************************************************************************/

/* little endian */
#define	OP_LITTLEENDIAN	'<'
/* big endian */
#define	OP_BIGENDIAN	'>'
/* native endian */
#define	OP_NATIVE	'='

bool doendian(int c);

void doswap(bool swap, void *p, size_t n);

/*
* Copy src to string dst of size siz.  At most siz-1 characters
* will be copied.  Always NUL terminates (unless siz == 0).
* Returns strlen(src); if retval >= siz, truncation occurred.
*/
size_t StrLCpy(char *dst, const char *src, size_t siz);

const char* ByteString(const char *msg, const unsigned int len);


// This function sleeps for the specified number of milliseconds.
// It may return early if the thread is woken by some other event,
// such as the delivery of a signal on Unix.
void SleepMs(int msecs);


//���򵥼�����
std::string XorString(const char *data, int datalen, const char *key, int len);

#endif//FUNCS_H___
