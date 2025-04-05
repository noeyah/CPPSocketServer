#include "pch.h"
#include <iostream>
#include "NetworkPch.h"
#include "ClientServer.h"
#include "PacketHelper.h"

int main()
{
	WSADATA wsaData;
	auto ret = WSAStartup(NETWORK_WINSOCK_VERSION, &wsaData);
	if (ret != 0)
	{
		throw std::runtime_error("WSAStartup error");
	}

	std::cout << "Test Client" << std::endl;

	std::shared_ptr<ClientServer> server = std::make_shared<ClientServer>();
	server->Start("127.0.0.1", 7777, 100);

	while (true)
	{
		std::string input;
		std::getline(std::cin, input);
		if (input == "q")
			break;
	}

	server->Stop();
	WSACleanup();
}
