#pragma once

#include "Server.h"

class Session {

public: 
	Session();
	~Session();

	bool BeginSession();
	SOCKET GetListenSocket() { return listenSocket; }
	bool TCPBind(); // 리슨 소켓을 TCP 프로토콜을 이용해 bind한다

private:
	SOCKET listenSocket;
};