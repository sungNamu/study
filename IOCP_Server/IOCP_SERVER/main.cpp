#include <iostream>
#include <string>
#include "ServerManager.h"

int main()
{
	ServerManager server;
	server.init();
	server.start();

	printf("quit 입력하면 종료\n");
	while (true)
	{
		std::string input;
		std::getline(std::cin, input);

		if (input == "quit")
		{
			break;
		}
	}

	server.end();
	return 0;
}
