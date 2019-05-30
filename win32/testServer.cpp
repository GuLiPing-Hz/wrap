#ifdef WIN32
#include <Winsock2.h> //��Ҫ������windows֮ǰ��������ļ�������ᱨ��
#include <WS2tcpip.h>
#pragma comment(lib,"Ws2_32.lib")
#else
typedef int SOCKET;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#endif
#include<stdio.h>
#include <time.h>
#include<thread>
#include <sstream>
#include <sys/timeb.h>

void Printf(const char* format, ...) {
	va_list args;
	va_start(args, format);
	char buf[1024];
	vsprintf(buf, format, args);
	va_end(args);

	static char timeLog[1024] = { 0 };
	timeb t;
	ftime(&t);
	struct tm* p = gmtime(&t.time); /* ��ȡ��ǰʱ�� */
	sprintf(timeLog, "%04d-%02d-%02d %02d:%02d:%02d.%d", 1900+p->tm_year,(1 + p->tm_mon), p->tm_mday,
		(p->tm_hour + 8), p->tm_min, p->tm_sec, t.millitm);

	std::stringstream os;//file
	os << timeLog << "��"<< buf;

	printf(os.str().c_str());
}

int setAddrReuse(SOCKET fd)
{
	/* REUSEADDR on Unix means, "don't hang on to this address after the
	* listener is closed."  On Windows, though, it means "don't keep other
	* processes from binding to this address while we're using it. */

	//���õ�ַ���� - ����������
	int on = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)& on, sizeof(on)) == SOCKET_ERROR)
		return -1;
	return 0;
}

const char* GetPeerIp(int fd) {
	sockaddr_in addr;
#ifdef WIN32
	int len = sizeof(sockaddr_in);
#else
//#elif defined(NETUTIL_ANDROID)
	socklen_t len = sizeof(sockaddr_in);
//#elif defined(NETUTIL_IOS)
	//unsigned int len = sizeof(sockaddr_in);
#endif
	getpeername(fd, (struct sockaddr*) & addr, &len);
	static char ip[100];
	//strncpy(ip, inet_ntoa(addr.sin_addr), sizeof(ip));
	inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
	return ip;
}

void handlerClient(SOCKET fd) {
	while (true) {
		char buf[65535] = { 0 };
		int len = (int) ::recv(fd, buf, sizeof(buf), 0);//������������
		if (len == 0) {
			Printf("�ͻ������ӱ������ر�\n");
			return;
		}

		if (len == SOCKET_ERROR) {
			int errorcode;
#ifdef WIN32
			errorcode = GetLastError();//����Ĵ����룬������windows��GetLastError��ȡ��
			if (errorcode != WSAEWOULDBLOCK) {
#else//Linux
			errorcode = errno;
			if (errorcode != EAGAIN) {
#endif
				Printf("client[%d] recv err=%d\n", fd, errorcode);
				return;
			}
			continue;
		}

		Printf("client[%d] recv[%d]=%s\n", fd, len, buf);
		char hello[65535] = { 0 };
		sprintf(hello, "from server back[c++]:%s", buf);
		Printf("send to client[%d]=%s\n", fd, hello);
		::send(fd, hello, strlen(hello), 0);
	}
}

class Env {
public:
	Env() {
#ifdef WIN32
		WORD wVersion;
		WSADATA WSAData;
		wVersion = MAKEWORD(2, 2);
		WSAStartup(wVersion, &WSAData);
#endif
	}

	virtual ~Env() {
#ifdef WIN32
		WSACleanup();
#endif
	}
};

int main() {
	Env env = Env();

	SOCKET listenFd = -1;
	listenFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//socket ������
	if (INVALID_SOCKET == listenFd)
	{
		Printf("create socket error %d\n", errno);
		return -1;
	}
	Printf("�������������������� fd=%d\n", listenFd);

	if (setAddrReuse(listenFd) != 0) {
		Printf("setAddrReuse error %d\n", errno);
		return -1;
	}
	Printf("���������õ�ַ���óɹ�\n");

	struct sockaddr_in ipAddr = { 0 };
	ipAddr.sin_family = AF_INET;
	ipAddr.sin_port = htons(20003);
	ipAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (0 != bind(listenFd, (struct sockaddr*) & ipAddr, sizeof(ipAddr)))
	{
		Printf("socket bind error %d\n", errno);
		return -1;
	}
	Printf("�������󶨵�ַ�ɹ�\n");

	if (::listen(listenFd, SOMAXCONN) != 0) {
		Printf("socket listen error %d\n", errno);
		return -1;
	}
	Printf("�����������ɹ�\n");

	sockaddr addr;
#ifdef WIN32
	int len = sizeof(addr);
#else
	socklen_t len = sizeof(addr);
#endif
	while (true) {
		SOCKET clientSocket = accept(listenFd, &addr, &len);
		if (clientSocket == INVALID_SOCKET) {
			Printf("socket accept error %d\n", errno);
			return -1;
		}

		Printf("�ͻ���������:���� fd = %d,ip = %s\n", clientSocket, GetPeerIp(clientSocket));
		auto clientThread = std::thread(handlerClient, clientSocket);
		clientThread.detach();
	}
}
