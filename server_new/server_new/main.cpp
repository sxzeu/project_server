
#include "Server.h"

int main()
{

	Server server;
	server.Start();

	printf("서버가 정상적으로 실행되고 있습니다. 키보드를 누르면 서버가 종료됩니다.\n");
	_getch();


	server.End();


	return 0;
}

