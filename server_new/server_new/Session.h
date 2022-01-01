#pragma once

#include "Server.h"

class Session {

public: 
	Session();
	~Session();

	bool BeginSession();
	SOCKET GetListenSocket() { return listenSocket; }
	bool TCPBind(); // ���� ������ TCP ���������� �̿��� bind�Ѵ�

private:
	SOCKET listenSocket;
};