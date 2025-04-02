#include "pch.h"
#include <iostream>
#include "NetworkPch.h"
#include "Network/ServerNetworkService.h"
#include "PacketHelper.h"
#include "GameServer.h"

int main()
{
	WSADATA wsaData;
	auto ret = WSAStartup(NETWORK_WINSOCK_VERSION, &wsaData);
	if (ret != 0)
	{
		throw std::runtime_error("WSAStartup error");
	}

    std::cout << "Test Server" << std::endl;

	auto server = std::make_shared<GameServer>();
	server->Start("127.0.0.1", 7777, 10);
	
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

