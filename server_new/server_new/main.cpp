
#include "Server.h"

int main()
{

	Server server;
	server.Start();

	printf("������ ���������� ����ǰ� �ֽ��ϴ�. Ű���带 ������ ������ ����˴ϴ�.\n");
	_getch();


	server.End();


	return 0;
}

