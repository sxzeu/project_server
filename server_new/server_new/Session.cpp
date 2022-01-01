 
#include "Session.h"

Session::Session(void)
{

}


Session::~Session(void)
{
}


bool Session::BeginSession()
{

	// TCP 家南 积己
	listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == listenSocket)
	{
		return false;
	}

	if (!TCPBind())
		return false;

	if (listen(listenSocket, 5) == SOCKET_ERROR)
		return false;

	printf("session begin session 角青 肯丰 \n");
	return true;
}

bool Session::TCPBind()
{
	SOCKADDR_IN serveraddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9000);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listenSocket, (SOCKADDR*)&serveraddr, sizeof(SOCKADDR_IN)) != 0)
		return false;
	printf("session tcp bind 角青 肯丰 \n");
	return true;
}