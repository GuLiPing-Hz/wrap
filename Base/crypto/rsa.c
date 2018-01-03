#include "rsa.h"
//!　保存私钥d集合
typedef struct _pKeyset
{
	unsigned int set[MAX_NUM];
	unsigned int size;
}pKeyset;

pKeyset pset;

//! 保存公、私钥对
typedef struct _pPairkey
{
	unsigned int d;
	unsigned int e;
	unsigned int n;
} pPairkey;
pPairkey pairkey;

// 名称：isPrime
// 功能：判断两个数是否互质
//  参数：m: 数a; n: 数b
// 返回：m、n互质返回true; 否则返回false
//
int isPrime(unsigned int m, unsigned int n)
{
	unsigned int i = 0;
	int Flag = 1;
	int temp;

	if (m < 2 || n<2){
		return 0;
	}
	
	temp = (m > n) ? n : m;
	for (i = 2; i <= temp && Flag; i++)
	{
		int mFlag = 1;
		int nFlag = 1;
		if (m % i == 0)
			mFlag = 0;
		if (n % i == 0)
			nFlag = 0;
		if (!mFlag && !nFlag)
			Flag = 0;
	}
	if (Flag)
		return 1;
	else
		return 0;
}

// 名称：MakePrivatedKeyd
// 功能：由素数Q、Q生成私钥d
//  参数：uiP: 素数P; uiQ: 素数Q
// 返回：私钥d

unsigned int MakePrivatedKeyd(unsigned int uiP, unsigned int uiQ)
{
	unsigned int i = 0;

	//! 得到所有与z互质的数( 私钥d的集合 )
	unsigned int z = (uiP - 1) * (uiQ - 1);
	pset.size = 0;
	for (i = 0; i < z; i++)
	{
		if (isPrime(i, z))
		{
			pset.set[pset.size++] = i;
		}
	}

	return pset.size;
}

// 名称：MakePairKey
// 功能：生成RSA公、私钥对
//  参数：uiP: 素数P; uiQ: 素数Q; uiD: 私钥d
// 返回：错误代码

unsigned int MakePairkey(unsigned int uiP, unsigned int uiQ, unsigned int uiD)
{
	int bFlag = 1;
	unsigned int i = 0, e;
	unsigned int z = (uiP - 1) * (uiQ - 1);
	unsigned int d = pset.set[uiD];
	//d=uiD;

	if (!isPrime(z, d))
		return ERROR_NOEACHPRIME;

	for (i = 2; i < z; i++)
	{
		if ((i*d) % z == 1)
		{
			e = i;
			bFlag = 0;
		}
	}
	if (bFlag)
		return ERROR_NOPUBLICKEY;

	if ((d*e) % z != 1)
		ERROR_GENERROR;

	pairkey.d = d;
	pairkey.e = e;
	pairkey.n = uiP * uiQ;
	return OK;
}

// 名称：GetPairKey
// 功能：对外提供接口，获得公、私钥对
//  参数：uiP: 素数P; uiQ: 素数Q; uiD: 私钥d
// 返回：

unsigned int GetPairKey(unsigned int* d, unsigned int* e)
{
	*d = pairkey.d;
	*e = pairkey.e;
	return pairkey.n;
}

// 名称：GetPrivateKeyd
// 功能：对外提供接口，由用户选择ID得以私钥d
//  参数：iWhich: 用户选择私钥d的ID
// 返回：私钥d值

unsigned int GetPrivateKeyd(unsigned int iWhich)
{
	if (pset.size >= iWhich)
		return pset.set[iWhich];
	else
		return 0;
}

// 名称：rsa_encrypt
// 功能：RSA加密运算
//  参数：n: 公钥n; e: 公钥e; mw: 加密明文; iLength: 明文长度; cw: 密文输出
// 返回：无

void rsa_encrypt(int n, int e, char *mw, int mLength, int ** cw)
{
	int i = 0, j = 0;
	__int64 temInt = 0;

	for (i = 0; i < mLength; i++)
	{
		temInt = mw[i];
		if (e != 0)
		{
			for (j = 1; j < e; j++)
			{
				temInt = (temInt * mw[i]) % n;
			}
		}
		else
		{
			temInt = 1;
		}

		(*cw)[i] = (int)temInt;
	}
}

// 名称：rsa_decrypt
// 功能：RSA解密运算
//  参数：n: 私钥n; d: 私钥d; cw: 密文; cLength: 密文长度; mw: 明文输出
// 返回：无

void rsa_decrypt(int n, int d, int **cw, int cLength, char *mw)
{
	int i = 0, j = -1;
	__int64 temInt = 0;

	for (i = 0; i < cLength / 4; ++i)
	{
		mw[i] = 0;
		temInt = (*cw)[i];

		if (d != 0)
		{
			for (j = 1; j < d; j++)
			{
				temInt = (__int64)(temInt * (*cw)[i]) % n;
			}
		}
		else
		{
			temInt = 1;
		}

		mw[i] = (char)temInt;
	}
}
void outputkey()
{
	printf("PublicKey(e,n): (%d,%d)\n", pairkey.e, pairkey.n);
	printf("PrivateKey(d,n): (%d,%d)\n", pairkey.d, pairkey.n);
}

/*
//main.c
// 工程：RSA
// 功能：RSA加、解密文件
//  作者：jlcss|ExpNIS


#include <stdio.h>
#include <afxwin.h>
#include <math.h>
#include "rsa.h"

#define DECRYPT_FILE "RSA加密密文.txt"
#define ENCRYPT_FILE "RSA解密明文.txt"
//! 约束文件最大2M
#define MAX_FILE 1024*1024*2

// 名称：usage
// 功能：帮助信息
//  参数：应用程序名称
// 返回：提示信息

void Usage(const char *appname)
{
	printf("\n\tusage:rsa -k 素数P 素数Q\n");
	printf("\tusage: rsa -e 明文文件 公钥e 公钥n\n");
	printf("\tusage: rsa -d 密文文件 私钥d 私钥n\n");
}

// 名称：IsNumber
// 功能：判断数字字符数组
//  参数：strNumber:字符数组
// 返回：数字字组数组返回true，否则返回false;

bool IsNumber(const char *strNumber)
{
	unsigned int i;

	if (!strNumber)
		return false;

	for (i = 0; i < strlen(strNumber); i++)
	{
		if (strNumber[i] < '0' || strNumber[i] > '9')
			return false;
	}

	return true;
}

// 名称：IsPrimeNumber
// 功能：判断素数
//  参数：num: 输入整数
// 返回：素数返回true，否则返回false;

bool IsPrimeNumber(unsigned int num)
{
	unsigned int i;
	if (num <= 1)
		return false;

	unsigned int sqr = (unsigned int)sqrt((double)num);
	for (i = 2; i <= sqr; i++)
	{
		if (num % i == 0)
			return false;
	}

	return true;
}

// 名称：FileIn
// 功能：读取磁盘文件到内存
//  参数：strFile:文件名称；inBuff:指向文件内容缓冲区
// 返回：实际读取内容大小(字节)

int FileIn(const char *strFile, unsigned char *&inBuff)
{
	int iFileLen = 0, iBuffLen = 0;

	//! 打开密文文件
	CFile file(strFile, CFile::modeRead);
	iFileLen = (int)file.GetLength();
	if (iFileLen > MAX_FILE)
	{
		printf("文件长度不能大于 %dM,!\n", MAX_FILE / (1024 * 1024));
		goto out;
	}
	iBuffLen = iFileLen;

	inBuff = new unsigned char[iBuffLen];
	if (!inBuff)
		goto out;

	ZeroMemory(inBuff, iBuffLen);

	file.Read(inBuff, iFileLen);
	file.Close();

out:
	return iBuffLen;
}

// 名称：FileOut
// 功能：加/解密结果输出到当前目录磁盘文件中
//  参数：strOut指向输出字符缓冲区，输出大小len，strFile为输出文件
// 返回：无

void FileOut(const void *strOut, int len, const char *strFile)
{
	//! 输出到文件
	CFile outfile(strFile, CFile::modeCreate | CFile::modeWrite);
	outfile.Write(strOut, len);
	outfile.Close();
}

// 名称：CheckParse
// 功能：校验应用程序入口参数
//  参数：argc等于main主函数argc参数，argv指向main主函数argv参数
// 返回：若参数合法返回true，否则返回false
//  备注：简单的入口参数校验

bool CheckParse(int argc, char** argv)
{
	bool bRes = false;

	if (argc != 4 && argc != 5)
		goto out;

	if (argc == 4 && argv[1][1] == 'k')
	{
		//! 生成公、私钥对
		if (!IsNumber(argv[2]) ||
			!IsNumber(argv[3]) ||
			atoi(argv[2]) > MAX_PRIME ||
			atoi(argv[3]) > MAX_PRIME)
			goto out;
	}
	else if ((argc == 5) && (argv[1][1] == 'e' || argv[1][1] == 'd'))
	{
		//! 加密、解密操作
		if (!IsNumber(argv[3]) ||
			!IsNumber(argv[4]) ||
			atoi(argv[3]) > MAX_NUM ||
			atoi(argv[4]) > MAX_NUM)
			goto out;
	}
	else
		Usage(*argv);
	bRes = true;

out:
	return bRes;
}

// 名称：kOption1
// 功能：程序k选项操作：由素数P、Q生成私钥d集合
//  参数：uiP: 程序入口参数P; uiQ: 程序入口参数Q
// 返回：执行正确返回生成私钥数目，否则返回0

unsigned int kOption1(unsigned int uiP, unsigned int uiQ)
{
	unsigned int uiRes = 0;

	if (!IsPrimeNumber(uiP))
	{
		printf("P输入错误，P必须为(0, %d]素数", MAX_PRIME);
		return uiRes;
	}
	if (!IsPrimeNumber(uiQ))
	{
		printf("Q输入错误，Q必须为(0, %d]素数", MAX_PRIME);
		return uiRes;
	}
	if (uiP == uiQ)
	{
		printf("素数P与素数Q相同，很容易根据公钥n开平方得出素数P和Q，这种加密不安全，请更换素数!\n");
		return uiRes;
	}
	printf("正在生成私钥d集合......\n");
	uiRes = MakePrivatedKeyd(uiP, uiQ);

	return uiRes;
}


测试RSA

//! 程序主函数
int main(int argc, char **argv)
{
	unsigned int p, q, d, n, e;//two prime p & q, public key(n, e) , private key(n , d)
	CheckParse(argc, argv);

	d = 4828; //uid
	if (argc == 4)
	{
		p = atoi(argv[2]);
		q = atoi(argv[3]);
		MakePrivatedKeyd(p, q);
		MakePairkey(p, q, d);
		outputkey();
	}
	else if (argc == 5)
	{
		char FileName[20];
		strcpy(FileName, argv[2]);
		int len;
		if (argv[1][1] == 'e')
		{
			unsigned char *inBuffer = (unsigned char *)malloc(MAX_FILE); //输入缓冲区
			int *cw = (int *)malloc(MAX_FILE);
			len = FileIn(FileName, inBuffer);
			e = atoi(argv[3]);
			n = atoi(argv[4]);
			rsa_encrypt(n, e, (char *)inBuffer, len, cw);
			FileOut(cw, 4 * len, DECRYPT_FILE);
		}
		else if (argv[1][1] == 'd')
		{
			char *Buffer = (char *)malloc(MAX_FILE); //输入缓冲区
			int *cw = (int *)malloc(MAX_FILE);
			len = FileIn(FileName, (unsigned char *&)cw);
			d = atoi(argv[3]);
			n = atoi(argv[4]);
			rsa_decrypt(n, d, cw, len, Buffer);
			FileOut(Buffer, len / 4, ENCRYPT_FILE);
		}
	}

	return 0;
}
*/
