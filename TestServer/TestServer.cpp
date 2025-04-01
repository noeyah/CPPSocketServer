#include "pch.h"
#include <iostream>
#include "NetworkPch.h"
#include "ServerNetworkService.h"
#include "PacketHelper.h"

class EventHandlerEx : public INetworkEventHandler
{
public:
	std::shared_ptr<ServerNetworkService> _server;

	void SetService(std::shared_ptr<ServerNetworkService> s)
	{
		_server = s;
	}

	virtual void OnConnect(SessionID sessionID) override
	{
		std::cout << "Connect " << sessionID << std::endl;
	}
	virtual void OnDisconnect(SessionID sessionID) override
	{
		std::cout << "OnDisconnect " << sessionID << std::endl;
	}
	virtual void OnRecv(SessionID sessionID, byte* buffer, int32 length) override
	{
		std::string msg;
		if (PacketHelper::UnpackStringPacket(buffer, length, msg))
		{
			std::cout << "Client " << sessionID << " ▶ " << msg << std::endl;
		}
		else
		{
			std::cout << "recv unpack failed. sessionID : " << sessionID << ", len : " << length << std::endl;
			return;
		}

		// 받은걸 그대로 전송
		_server->Send(sessionID, buffer, length);
	}
	virtual void OnSendComplete(SessionID sessionID, int32 len) override
	{
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

    std::cout << "Test Server" << std::endl;

	/*Packet::TestReq testReq;
	testReq.set_desc("클라이언트->서버 TestReq");*/

	EventHandlerEx* eventHandler = new EventHandlerEx();
	std::shared_ptr<ServerNetworkService> server = std::make_shared<ServerNetworkService>("127.0.0.1", 7777, eventHandler, 10);

	eventHandler->SetService(server);
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
	delete eventHandler;
}

