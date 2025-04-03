#include "pch.h"
#include <iostream>
#include "NetworkPch.h"
#include "Network/ClientNetworkService.h"
#include "PacketHelper.h"
#include <thread>
#include <chrono>
using namespace std::chrono;

class EventHandlerEx : public INetworkEventHandler
{
public:
	std::shared_ptr<ClientNetworkService> _server;

	void SetService(std::shared_ptr<ClientNetworkService> s)
	{
		_server = s;
	}

	virtual void OnConnect(SessionID sessionID) override
	{
		std::cout << "Connect " << sessionID << std::endl;

		std::string str = "hello";
		auto data = PacketHelper::MakeStringPacket(str);

		_server->Send(sessionID, data->data(), data->size());
	}
	virtual void OnDisconnect(SessionID sessionID) override
	{

	}
	virtual void OnRecv(SessionID sessionID, std::span<const byte> packetData) override
	{
		std::this_thread::sleep_for(100ms);

		std::string msg;
		if (PacketHelper::UnpackStringPacket(packetData, msg))
		{
			std::cout << "Server ▶ " << msg << std::endl;
		}
		else
		{
			std::cout << "recv unpack failed. sessionID : " << sessionID << ", size : " << packetData.size() << std::endl;
			return;
		}

		// 받은걸 그대로 전송
		_server->Send(sessionID, packetData.data(), packetData.size());
	}
};

int main()
{
	WSADATA wsaData;
	auto ret = WSAStartup(NETWORK_WINSOCK_VERSION, &wsaData);
	if (ret != 0)
	{
		throw std::runtime_error("WSAStartup error");
	}

	std::cout << "Test Client" << std::endl;

	

	EventHandlerEx eventHandler;
	std::shared_ptr<ClientNetworkService> server = std::make_shared<ClientNetworkService>("127.0.0.1", 7777, &eventHandler, 1000);
	eventHandler.SetService(server);
	server->Start();

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
