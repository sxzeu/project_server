#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 

#include <list>
#include <queue>
#include <algorithm>
#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <time.h>
#include <conio.h>

#include <tchar.h>
#include <stdio.h>

#define MAX_SOCKBUF			50			// Recv를 위한 버퍼 크기
#define MAX_CLIENT			4			// 최대 접속자 수
// #define MAX_WORKERTHREAD	50			// 작업자 쓰레드의 갯수
#define MAX_PACKET_SIZE		255
#define BUFSIZE				50


using namespace std;

class Session;

// 현재 진행 중인 작업의 종류
enum IO_TYPE
{
	SEND_IO,
	RECV_IO,
};

/*
struct stOverlappedEx
{
	WSAOVERLAPPED		m_wsaOverlapped;
	WSABUF				m_wsaBuf;
	char buf[BUFSIZE + 1];
	int recvbytes;
	int sendbytes;
};
*/

// overlapped I/O를 위해 클라이언트가 사용할 구조체
struct OverlappedIOClient
{
	WSAOVERLAPPED		m_wsaOverlapped;
	SOCKET				m_socketClient;
	WSABUF				m_wsaBuf;
	char buf[BUFSIZE + 1];
	int recvbytes;
	int sendbytes;

	// 생성자에서 멤버 변수들을 초기화
	OverlappedIOClient()
	{
		ZeroMemory(&m_wsaOverlapped, sizeof(OVERLAPPED));
		ZeroMemory(&m_wsaBuf, sizeof(WSABUF));
		m_socketClient = INVALID_SOCKET;

		recvbytes = 0;
		sendbytes = 0;
	}

};

struct ClientInfo
{
	OverlappedIOClient* overlappedClient;

	ClientInfo()
	{
		ZeroMemory(this, sizeof(ClientInfo));
	}
};

class Server {

	OverlappedIOClient* m_client;
	std::list<OverlappedIOClient> m_clientList;

	unsigned char			m_Buf[BUFSIZE];
public:
	Server();
	~Server();

	void err_display(const char* msg);
	void err_quit(const char* msg);



	bool Start(); // 만든 함수들을 호출한다
	bool End(); // 작업을 끝낸다
	bool initWinsock(); // 윈속 초기화


	//bool CreateThreadPool();
	bool CreateAccepterThread();

	//void WorkerThread();
	void AccepterThread();

	void CloseSocket(OverlappedIOClient* pClientInfo, bool bIsForce = false);

	virtual bool		ConnectProcess(OverlappedIOClient* pClientInfo) { return true; }
	virtual bool		DisConnectProcess(OverlappedIOClient* pClientInfo) { return true; }
	bool		SendToAllClient();



	OverlappedIOClient* GetEmptyClientInfo(); // 빈 클라이언트 가져오기
	void				DestroyThread();		// 생성되어 있는 쓰레드 파괴

	bool CreateLogtxt(); // log.txt 파일 만들기 
	void WriteAtLogtxt(); // log.txt  파일에 로그 기록하기

private:

	HANDLE h_iocp; // iocp  핸들
	//HANDLE h_WorkerThread[MAX_WORKERTHREAD]; // 워커 스레드 핸들
	HANDLE h_AccepterThread; // 접속 관리 위한 스레드 만들기

	SOCKET				m_socketListen;
	char				m_socketBuf[1024]; // 소켓 버퍼
	int m_numOfClient; // 현재 접속한 클라이언트의 수
	char message[100];
	Session* m_Session; //세션

	HANDLE hFile; //  log.txt 기록할 거임

};