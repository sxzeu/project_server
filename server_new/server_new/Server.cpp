#include "Server.h"
#include "Session.h"

/*
DWORD WINAPI CallWorkerThread(LPVOID arg)
{
	Server* h_iocp = (Server*)arg;
	h_iocp->WorkerThread();
	return 0;
}
*/

// Client�� ������ �ޱ����� ������
DWORD WINAPI CallAccepterThread(LPVOID arg)
{
	Server* h_iocp = (Server*)arg;
	h_iocp->AccepterThread();
	return 0;
}


Server::Server()
{
	m_numOfClient = 0;
	if (!m_clientList.empty()) m_clientList.clear();

	h_AccepterThread = NULL;
	h_iocp = NULL;
	m_socketListen = INVALID_SOCKET;
	ZeroMemory(m_socketBuf, 1024);
	//for (int i = 0; i < MAX_WORKERTHREAD; i++)
		//h_WorkerThread[i] = NULL;
	// Client������ ������ ����ü�� ����
	m_client = new OverlappedIOClient[MAX_CLIENT];


	// wsabuf������ üũ�ϱ�
	//printf("size of wsabuf %d \n", sizeof(m_client->m_wsaBuf.buf));
}


Server::~Server()
{
	if (!m_clientList.empty()) m_clientList.clear();
}


void Server::err_display(const char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
}

void Server::err_quit(const char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(-1);
}

bool Server::Start()
{
	// ���� �����
	if (!initWinsock()) End();

	// ���� �����
	m_Session = new Session;
	if (!m_Session->BeginSession()) End();

	// ����� iocp ��ü �����
	h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	if (h_iocp == NULL) return false;

	// ������ Ǯ �����
	//if (!CreateThreadPool()) return false;

	// ���� ���� ������ �����
	if (!CreateAccepterThread()) return false;

	// Log.txt ���� �����
	if (!CreateLogtxt()) return false;
	
	printf("server start ���� �Ϸ� \n");
	return true;
}

bool Server::End()
{

	// Thread ����
	DestroyThread();

	// ��ü�� �ʱ�ȭ
	for (int i = 0; i < MAX_CLIENT; i++)
	{
		if (m_client[i].m_socketClient != INVALID_SOCKET)
		{
			CloseSocket(&m_client[i]);
		}
	}

	// ������ ����� ������.
	WSACleanup();
	printf("server end ���� �Ϸ� \n");
	return true;
}

void Server::DestroyThread()
{
	/*
	if (h_iocp != NULL)
	{
		for (int i = 0; i < MAX_WORKERTHREAD; i++)
		{
			// WaitingThread Queue���� ������� �����忡 ����� ���� �޼����� ������
			PostQueuedCompletionStatus(h_iocp, 0, 0, NULL);
		}

		for (int i = 0; i < MAX_WORKERTHREAD; i++)
		{
			// ������ �ڵ��� �ݰ� �����尡 ����� ������ ��ٸ���.
			CloseHandle(h_WorkerThread[i]);
			WaitForSingleObject(h_WorkerThread[i], INFINITE);
		}
		}
	*/
	closesocket(m_Session->GetListenSocket());
	CloseHandle(h_AccepterThread);
	// ������ ���Ḧ ��ٸ���.
	WaitForSingleObject(h_AccepterThread, INFINITE);
	printf("server destroy thread ���� �Ϸ� \n");
}

bool Server::initWinsock()
{
	// ���� �����
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return false;

	printf("server initWinsock ���� �Ϸ� \n");
	return true;
}

bool Server::CreateLogtxt()
{
	TCHAR fileName[] = _T("Log.txt");
	TCHAR fileInitData[] = _T("log.txt Ŭ���̾�Ʈ ���� ��� ����");

	hFile = CreateFile(fileName, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf_s("log.txt ���� ���� \n");
		return -1;
	}

	printf_s("log.txt ���� ����\n");
	return 0;
}

void Server::WriteAtLogtxt()
{
	DWORD numOfByteWritten = 0;
	WriteFile(hFile, message, sizeof(message), &numOfByteWritten, NULL);
	printf_s("log.txt�� �α� ��� �Ϸ�\n");
}
/*
bool Server::CreateThreadPool()
{
	DWORD uiThreadId = 0;

	for (int i = 0; i < MAX_WORKERTHREAD; i++)
	{
		h_WorkerThread[i] = CreateThread(NULL, 0, CallWorkerThread, this, CREATE_SUSPENDED, &uiThreadId);
		if (h_WorkerThread == NULL)
		{
			return false;
		}
		ResumeThread(h_WorkerThread[i]);
	}
	printf("server createThreadPool ���� �Ϸ� \n");
	return true;
}

*/
void Server::AccepterThread()
{
	SOCKADDR_IN stClientAddr;
	int nAddrLen = sizeof(SOCKADDR_IN);
	printf_s("server accepter thread ���� ���� \n");
	while (1)
	{
		// ������ ���� ����ü�� �ε����� ���´�.
		OverlappedIOClient* p_client = GetEmptyClientInfo();
		if (p_client == NULL)
		{
			return;
		}

		// Ŭ���̾�Ʈ ���� ��û�� ���� ������ ��ٸ���.
		p_client->m_socketClient = accept(m_Session->GetListenSocket(), (SOCKADDR*)&stClientAddr, &nAddrLen);
		if (p_client->m_socketClient == INVALID_SOCKET)
			continue;

		printf_s("[����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
			inet_ntoa(stClientAddr.sin_addr), ntohs(stClientAddr.sin_port));

		if (m_numOfClient > MAX_CLIENT - 1)
		{
			printf("�ο� �ʰ�! ������ �����մϴ�.\n");

			SOCKADDR_IN clientaddr;
			int addrlen = sizeof(clientaddr);
			getpeername(p_client->m_socketClient, (SOCKADDR*)&clientaddr, &addrlen);
			printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

			CloseSocket(p_client);
			continue;
		}

		// IOCP ��ü�� Ŭ���̾�Ʈ�� ����
		HANDLE hIOCP = CreateIoCompletionPort((HANDLE)p_client->m_socketClient, h_iocp, reinterpret_cast<ULONG_PTR>(p_client), 0);
		if (hIOCP == NULL || h_iocp != hIOCP) return;


			// �ð� ��� ���� ������
		time_t timer;
		struct tm* t;

		timer = time(NULL); // ��������� ��
		t = localtime(&timer); // �������� ���� ����ü�� �ֱ�
		sprintf(message, "%d�� %d�� %d�� %d�� %d�� %d��, %d connected\n", 
			t->tm_year + 1900, t->tm_mon + 1,t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, ntohs(stClientAddr.sin_port));
		//printf("message: %s\n", message);
		

		m_numOfClient++;
		m_clientList.push_back(*p_client);
		ConnectProcess(p_client);

		// ����� ��� Ŭ���̾�Ʈ�� ������ �����ְ�, log.txt ����ϱ�
		SendToAllClient();

		// Ŭ���̾�Ʈ ���� ����
	}

	printf("server accepter thread ���� �Ϸ� \n");
}
/*
bool Server ::BindRecv(OverlappedIOClient* p_client)
{
	DWORD dwFlag = 0;
	char* pBuf = NULL;

	if (p_client == NULL || p_client->m_socketClient == INVALID_SOCKET)
		return false;

	// Overlapped I/O�� ���� �������� �������ش�.
	p_client->m_wsaBuf.len = MAX_PACKET_SIZE;
	p_client->m_wsaBuf.buf = (CHAR*)p_client->buf;

	ZeroMemory(&p_client->m_wsaOverlapped, sizeof(OVERLAPPED));

	int nRet = WSARecv(p_client->m_socketClient,
		&(p_client->m_wsaBuf),
		1,
		NULL,
		&dwFlag,
		(LPWSAOVERLAPPED) & (p_client),
		NULL);

	// socket_error �̸� Client Socket�� �������ɷ� ó���Ѵ�.
	if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))	return false;
	printf("server bind recv ���� �Ϸ� \n");
	return true;
}

*/
OverlappedIOClient* Server::GetEmptyClientInfo()
{
	for (int i = 0; i < MAX_CLIENT; i++)
	{
		if (m_client[i].m_socketClient == INVALID_SOCKET)
			return &m_client[i];
	}
	printf("server get empty client info ���� �Ϸ� \n");
	return NULL;

}
/*
void Server::WorkerThread()
{
	int retval;
	// I/O �۾��� ���� ��û�� Overlapped ����ü�� ���� ������
	LPOVERLAPPED lpOverlapped = NULL;

	// overlapped I/O �۾����� ���۵� ������ ũ��
	DWORD cbTransferred;
	// Ŭ���̾�Ʈ ����
	SOCKET client_sock;
	// ������
	OverlappedIOClient* ptr;
	printf("server workerthread ���� ���� \n");
	while (1) {
		// �񵿱� ����� �Ϸ� ��ٸ���
		retval = GetQueuedCompletionStatus(h_iocp,
			&cbTransferred,
			(LPDWORD)&ptr,
			&lpOverlapped,
			INFINITE);

		// Ŭ���̾�Ʈ ���� ���
		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		getpeername(ptr->m_socketClient, (SOCKADDR*)&clientaddr, &addrlen);

		// �񵿱� ����� ��� Ȯ��
		if (retval && cbTransferred == 0 && ptr != NULL ) {
	
			DisConnectProcess(ptr);

			SOCKADDR_IN clientaddr;
			int addrlen = sizeof(clientaddr);
			getpeername(ptr->m_socketClient, (SOCKADDR*)&clientaddr, &addrlen);
			CloseSocket(ptr);

			printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
				inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

			m_numOfClient--;
			delete ptr;
			continue;
		}

		// ������ ���۷� ����
		if (ptr->recvbytes == 0) {
			ptr->recvbytes = cbTransferred;
			ptr->sendbytes = 0;
			// ���� ������ ���
			ptr->buf[ptr->recvbytes] = '\0';
			printf("[TCP/%s:%d] %s\n", inet_ntoa(clientaddr.sin_addr),
				ntohs(clientaddr.sin_port), ptr->buf);
		}
		else {
			ptr->sendbytes += cbTransferred;
		}

		else {
			ptr->recvbytes = 0;

			// ������ �ޱ�
			ZeroMemory(&ptr->m_wsaOverlapped, sizeof(ptr->m_wsaOverlapped));
			ptr->m_wsaBuf.buf = ptr->buf;
			ptr->m_wsaBuf.len = BUFSIZE;

			DWORD recvbytes;
			DWORD flags = 0;
			retval = WSARecv(ptr->m_socketClient, &ptr->m_wsaBuf, 1,
				&recvbytes, &flags, &ptr->m_wsaOverlapped, NULL);
			if (retval == SOCKET_ERROR) {
				if (WSAGetLastError() != WSA_IO_PENDING) {
					err_display("WSARecv()");
				}
				continue;
			}
		}

	}

	printf("server workerthread ���� �Ϸ� \n");
}
*/

bool Server::CreateAccepterThread()
{
	printf("server createaccepterThread ���� ���� \n");
	DWORD uiThreadId = 0;
	// Client�� ���ӿ�û�� ���� ������ ����
//	m_hAccepterThread = (HANDLE)_beginthreadex( NULL, 0, &CallAccepterThread, this, CREATE_SUSPENDED, &uiThreadId );
	h_AccepterThread = CreateThread(NULL, 0, CallAccepterThread, this, CREATE_SUSPENDED, &uiThreadId);
	if (h_AccepterThread == NULL)
	{
		return false;
	}

	ResumeThread(h_AccepterThread);
	printf("server createaccepterThread ���� �Ϸ� \n");
	return true;
}

void Server::CloseSocket(OverlappedIOClient* pClientInfo, bool bIsForce)
{
	struct linger stLinger = { 0, 0 };		// SO_DONTLINGER �� ����

	// bIsForce�� true�̸� SO_LINGER, timeout = 0���� �����Ͽ�
	// �������� ��Ų��. ����:������ �ս��� ���� �� ����
	if (bIsForce)
		stLinger.l_linger = 1;

	// socketClose������ ������ �ۼ����� ��� �ߴ� ��Ų��.
	shutdown(pClientInfo->m_socketClient, SD_BOTH);
	// ���� �ɼ��� �����Ѵ�.
	setsockopt(pClientInfo->m_socketClient, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));
	// ���� ������ ���� ��Ų��.
	pClientInfo->m_socketClient = INVALID_SOCKET;

	printf("server close socket ���� �Ϸ� \n");
}

bool Server::SendToAllClient()
{
	printf("server send to all client ���� ���� \n");
	DWORD dwType = 0;
	unsigned char packet_size;
	std::list<OverlappedIOClient> ::iterator iter;

	
	for (iter = m_clientList.begin(); iter != m_clientList.end(); ++iter)
	{
		iter->m_wsaBuf.len = strlen(message);
		iter->m_wsaBuf.buf = message;
		//strcpy(iter->m_wsaBuf.buf, message); // ���⿡�� ������ ���� �߻�

		int retval = WSASend(iter->m_socketClient, &iter->m_wsaBuf ,1,
			(LPDWORD)iter->sendbytes, 0, &iter->m_wsaOverlapped, NULL);
		if (retval == SOCKET_ERROR) {
			if (WSAGetLastError() != WSA_IO_PENDING) {
				err_display("WSASend()");
			}
			continue;
		}
		printf("server send to all client  ������ \n");
	}

	// �α� ���Ͽ� �α� ����ϱ�
	WriteAtLogtxt();

	printf("server send to all client ���� ���� \n");
	return true;
}