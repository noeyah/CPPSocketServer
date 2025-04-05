#include "pch.h"
#include "ClientServer.h"
#include "NetworkPch.h"
#include "Network/IPublicService.h"
#include "PacketHelper.h"
using namespace std::chrono;

void ClientServer::Start(std::string ip, uint16 port, uint32 connectCount, uint32 workerThreadCount)
{
	if (_server)
		return;

	_server = NetworkFactory::CreateClientService(ip, port, this, connectCount, workerThreadCount);

	if (!_server->Start())
	{
		throw std::runtime_error("server connect failed");
	}
}

void ClientServer::Stop()
{
	if (!_server)
		return;

	_server->Stop();
	_server.reset();
}

void ClientServer::OnConnect(SessionID sessionID)
{
	std::cout << "Connect " << sessionID << std::endl;

	std::string str = "hello";
	auto data = PacketHelper::MakeStringPacket(str);

	_server->Send(sessionID, data);
}

void ClientServer::OnDisconnect(SessionID sessionID)
{
	std::cout << "Disconnect " << sessionID << std::endl;
}

void ClientServer::OnRecv(SessionID sessionID, std::span<const byte> packetData)
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
	_server->Send(sessionID, packetData);
}

void ClientServer::OnSendComplete(SessionID sessionID, int32 len)
{
}
