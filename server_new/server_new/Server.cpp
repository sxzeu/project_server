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

// Client의 접속을 받기위한 쓰레드
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
	// Client정보를 저장할 구조체를 생성
	m_client = new OverlappedIOClient[MAX_CLIENT];


	// wsabuf사이즈 체크하기
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
	// 윈속 만들기
	if (!initWinsock()) End();

	// 세션 만들기
	m_Session = new Session;
	if (!m_Session->BeginSession()) End();

	// 사용할 iocp 객체 만들기
	h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	if (h_iocp == NULL) return false;

	// 스레드 풀 만들기
	//if (!CreateThreadPool()) return false;

	// 관리 접속 스레드 만들기
	if (!CreateAccepterThread()) return false;

	// Log.txt 파일 만들기
	if (!CreateLogtxt()) return false;
	
	printf("server start 실행 완료 \n");
	return true;
}

bool Server::End()
{

	// Thread 종료
	DestroyThread();

	// 객체를 초기화
	for (int i = 0; i < MAX_CLIENT; i++)
	{
		if (m_client[i].m_socketClient != INVALID_SOCKET)
		{
			CloseSocket(&m_client[i]);
		}
	}

	// 윈속의 사용을 끝낸다.
	WSACleanup();
	printf("server end 실행 완료 \n");
	return true;
}

void Server::DestroyThread()
{
	/*
	if (h_iocp != NULL)
	{
		for (int i = 0; i < MAX_WORKERTHREAD; i++)
		{
			// WaitingThread Queue에서 대기중인 쓰레드에 사용자 종료 메세지를 보낸다
			PostQueuedCompletionStatus(h_iocp, 0, 0, NULL);
		}

		for (int i = 0; i < MAX_WORKERTHREAD; i++)
		{
			// 쓰레드 핸들을 닫고 쓰레드가 종료될 때까지 기다린다.
			CloseHandle(h_WorkerThread[i]);
			WaitForSingleObject(h_WorkerThread[i], INFINITE);
		}
		}
	*/
	closesocket(m_Session->GetListenSocket());
	CloseHandle(h_AccepterThread);
	// 쓰레드 종료를 기다린다.
	WaitForSingleObject(h_AccepterThread, INFINITE);
	printf("server destroy thread 실행 완료 \n");
}

bool Server::initWinsock()
{
	// 윈속 만들기
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return false;

	printf("server initWinsock 실행 완료 \n");
	return true;
}

bool Server::CreateLogtxt()
{
	TCHAR fileName[] = _T("Log.txt");
	TCHAR fileInitData[] = _T("log.txt 클라이언트 접속 기록 저장");

	hFile = CreateFile(fileName, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf_s("log.txt 생성 실패 \n");
		return -1;
	}

	printf_s("log.txt 생성 성공\n");
	return 0;
}

void Server::WriteAtLogtxt()
{
	DWORD numOfByteWritten = 0;
	WriteFile(hFile, message, sizeof(message), &numOfByteWritten, NULL);
	printf_s("log.txt에 로그 기록 완료\n");
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
	printf("server createThreadPool 실행 완료 \n");
	return true;
}

*/
void Server::AccepterThread()
{
	SOCKADDR_IN stClientAddr;
	int nAddrLen = sizeof(SOCKADDR_IN);
	printf_s("server accepter thread 실행 시작 \n");
	while (1)
	{
		// 접속을 받을 구조체의 인덱스를 얻어온다.
		OverlappedIOClient* p_client = GetEmptyClientInfo();
		if (p_client == NULL)
		{
			return;
		}

		// 클라이언트 접속 요청이 들어올 때까지 기다린다.
		p_client->m_socketClient = accept(m_Session->GetListenSocket(), (SOCKADDR*)&stClientAddr, &nAddrLen);
		if (p_client->m_socketClient == INVALID_SOCKET)
			continue;

		printf_s("[서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
			inet_ntoa(stClientAddr.sin_addr), ntohs(stClientAddr.sin_port));

		if (m_numOfClient > MAX_CLIENT - 1)
		{
			printf("인원 초과! 접속을 해지합니다.\n");

			SOCKADDR_IN clientaddr;
			int addrlen = sizeof(clientaddr);
			getpeername(p_client->m_socketClient, (SOCKADDR*)&clientaddr, &addrlen);
			printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

			CloseSocket(p_client);
			continue;
		}

		// IOCP 객체와 클라이언트를 연결
		HANDLE hIOCP = CreateIoCompletionPort((HANDLE)p_client->m_socketClient, h_iocp, reinterpret_cast<ULONG_PTR>(p_client), 0);
		if (hIOCP == NULL || h_iocp != hIOCP) return;


			// 시간 출력 위한 변수들
		time_t timer;
		struct tm* t;

		timer = time(NULL); // 현재까지의 초
		t = localtime(&timer); // 포맷팅을 위해 구조체에 넣기
		sprintf(message, "%d년 %d월 %d일 %d시 %d분 %d초, %d connected\n", 
			t->tm_year + 1900, t->tm_mon + 1,t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, ntohs(stClientAddr.sin_port));
		//printf("message: %s\n", message);
		

		m_numOfClient++;
		m_clientList.push_back(*p_client);
		ConnectProcess(p_client);

		// 연결된 모든 클라이언트에 데이터 보내주고, log.txt 기록하기
		SendToAllClient();

		// 클라이언트 갯수 증가
	}

	printf("server accepter thread 실행 완료 \n");
}
/*
bool Server ::BindRecv(OverlappedIOClient* p_client)
{
	DWORD dwFlag = 0;
	char* pBuf = NULL;

	if (p_client == NULL || p_client->m_socketClient == INVALID_SOCKET)
		return false;

	// Overlapped I/O를 위해 각정보를 셋팅해준다.
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

	// socket_error 이면 Client Socket이 끊어진걸로 처리한다.
	if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))	return false;
	printf("server bind recv 실행 완료 \n");
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
	printf("server get empty client info 실행 완료 \n");
	return NULL;

}
/*
void Server::WorkerThread()
{
	int retval;
	// I/O 작업을 위해 요청한 Overlapped 구조체를 받을 포인터
	LPOVERLAPPED lpOverlapped = NULL;

	// overlapped I/O 작업에서 전송된 데이터 크기
	DWORD cbTransferred;
	// 클라이언트 소켓
	SOCKET client_sock;
	// 포인터
	OverlappedIOClient* ptr;
	printf("server workerthread 실행 시작 \n");
	while (1) {
		// 비동기 입출력 완료 기다리기
		retval = GetQueuedCompletionStatus(h_iocp,
			&cbTransferred,
			(LPDWORD)&ptr,
			&lpOverlapped,
			INFINITE);

		// 클라이언트 정보 얻기
		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		getpeername(ptr->m_socketClient, (SOCKADDR*)&clientaddr, &addrlen);

		// 비동기 입출력 결과 확인
		if (retval && cbTransferred == 0 && ptr != NULL ) {
	
			DisConnectProcess(ptr);

			SOCKADDR_IN clientaddr;
			int addrlen = sizeof(clientaddr);
			getpeername(ptr->m_socketClient, (SOCKADDR*)&clientaddr, &addrlen);
			CloseSocket(ptr);

			printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n",
				inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

			m_numOfClient--;
			delete ptr;
			continue;
		}

		// 데이터 전송량 갱신
		if (ptr->recvbytes == 0) {
			ptr->recvbytes = cbTransferred;
			ptr->sendbytes = 0;
			// 받은 데이터 출력
			ptr->buf[ptr->recvbytes] = '\0';
			printf("[TCP/%s:%d] %s\n", inet_ntoa(clientaddr.sin_addr),
				ntohs(clientaddr.sin_port), ptr->buf);
		}
		else {
			ptr->sendbytes += cbTransferred;
		}

		else {
			ptr->recvbytes = 0;

			// 데이터 받기
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

	printf("server workerthread 실행 완료 \n");
}
*/

bool Server::CreateAccepterThread()
{
	printf("server createaccepterThread 실행 시작 \n");
	DWORD uiThreadId = 0;
	// Client의 접속요청을 받을 쓰레드 생성
//	m_hAccepterThread = (HANDLE)_beginthreadex( NULL, 0, &CallAccepterThread, this, CREATE_SUSPENDED, &uiThreadId );
	h_AccepterThread = CreateThread(NULL, 0, CallAccepterThread, this, CREATE_SUSPENDED, &uiThreadId);
	if (h_AccepterThread == NULL)
	{
		return false;
	}

	ResumeThread(h_AccepterThread);
	printf("server createaccepterThread 실행 완료 \n");
	return true;
}

void Server::CloseSocket(OverlappedIOClient* pClientInfo, bool bIsForce)
{
	struct linger stLinger = { 0, 0 };		// SO_DONTLINGER 로 설정

	// bIsForce가 true이면 SO_LINGER, timeout = 0으로 설정하여
	// 강제종료 시킨다. 주의:데이터 손실이 있을 수 있음
	if (bIsForce)
		stLinger.l_linger = 1;

	// socketClose소켓의 데이터 송수신을 모두 중단 시킨다.
	shutdown(pClientInfo->m_socketClient, SD_BOTH);
	// 소켓 옵션을 설정한다.
	setsockopt(pClientInfo->m_socketClient, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));
	// 소켓 연결을 종료 시킨다.
	pClientInfo->m_socketClient = INVALID_SOCKET;

	printf("server close socket 실행 완료 \n");
}

bool Server::SendToAllClient()
{
	printf("server send to all client 실행 시작 \n");
	DWORD dwType = 0;
	unsigned char packet_size;
	std::list<OverlappedIOClient> ::iterator iter;

	
	for (iter = m_clientList.begin(); iter != m_clientList.end(); ++iter)
	{
		iter->m_wsaBuf.len = strlen(message);
		iter->m_wsaBuf.buf = message;
		//strcpy(iter->m_wsaBuf.buf, message); // 여기에서 엑세스 위반 발생

		int retval = WSASend(iter->m_socketClient, &iter->m_wsaBuf ,1,
			(LPDWORD)iter->sendbytes, 0, &iter->m_wsaOverlapped, NULL);
		if (retval == SOCKET_ERROR) {
			if (WSAGetLastError() != WSA_IO_PENDING) {
				err_display("WSASend()");
			}
			continue;
		}
		printf("server send to all client  실행중 \n");
	}

	// 로그 파일에 로그 기록하기
	WriteAtLogtxt();

	printf("server send to all client 실행 종료 \n");
	return true;
}