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

#define MAX_SOCKBUF			50			// Recv�� ���� ���� ũ��
#define MAX_CLIENT			4			// �ִ� ������ ��
// #define MAX_WORKERTHREAD	50			// �۾��� �������� ����
#define MAX_PACKET_SIZE		255
#define BUFSIZE				50


using namespace std;

class Session;

// ���� ���� ���� �۾��� ����
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

// overlapped I/O�� ���� Ŭ���̾�Ʈ�� ����� ����ü
struct OverlappedIOClient
{
	WSAOVERLAPPED		m_wsaOverlapped;
	SOCKET				m_socketClient;
	WSABUF				m_wsaBuf;
	char buf[BUFSIZE + 1];
	int recvbytes;
	int sendbytes;

	// �����ڿ��� ��� �������� �ʱ�ȭ
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



	bool Start(); // ���� �Լ����� ȣ���Ѵ�
	bool End(); // �۾��� ������
	bool initWinsock(); // ���� �ʱ�ȭ


	//bool CreateThreadPool();
	bool CreateAccepterThread();

	//void WorkerThread();
	void AccepterThread();

	void CloseSocket(OverlappedIOClient* pClientInfo, bool bIsForce = false);

	virtual bool		ConnectProcess(OverlappedIOClient* pClientInfo) { return true; }
	virtual bool		DisConnectProcess(OverlappedIOClient* pClientInfo) { return true; }
	bool		SendToAllClient();



	OverlappedIOClient* GetEmptyClientInfo(); // �� Ŭ���̾�Ʈ ��������
	void				DestroyThread();		// �����Ǿ� �ִ� ������ �ı�

	bool CreateLogtxt(); // log.txt ���� ����� 
	void WriteAtLogtxt(); // log.txt  ���Ͽ� �α� ����ϱ�

private:

	HANDLE h_iocp; // iocp  �ڵ�
	//HANDLE h_WorkerThread[MAX_WORKERTHREAD]; // ��Ŀ ������ �ڵ�
	HANDLE h_AccepterThread; // ���� ���� ���� ������ �����

	SOCKET				m_socketListen;
	char				m_socketBuf[1024]; // ���� ����
	int m_numOfClient; // ���� ������ Ŭ���̾�Ʈ�� ��
	char message[100];
	Session* m_Session; //����

	HANDLE hFile; //  log.txt ����� ����

};