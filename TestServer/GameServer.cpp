#include "pch.h"
#include "GameServer.h"
#include "NetworkPch.h"
#include "Network/ServerNetworkService.h"
#include "PacketHelper.h"

void GameServer::Start(std::string ip, uint16 port, int32 pendingAcceptCount)
{
	if (_server)
		return;

	_server = std::make_shared<ServerNetworkService>(ip, port, this, pendingAcceptCount);

	if (!_server->Start())
	{
		throw std::runtime_error("server start failed");
	}
}

void GameServer::Stop()
{
	if (!_server)
		return;

	_server->Stop();
	_server.reset();
}

void GameServer::OnConnect(SessionID sessionID)
{
}

void GameServer::OnDisconnect(SessionID sessionID)
{
}

void GameServer::OnRecv(SessionID sessionID, std::span<const byte> packetData)
{
	std::string msg;
	if (PacketHelper::UnpackStringPacket(packetData, msg))
	{
		std::cout << "Client " << sessionID << " ▶ " << msg << std::endl;
	}
	else
	{
		std::cout << "recv unpack failed. sessionID : " << sessionID << ", size : " << packetData.size() << std::endl;
		return;
	}

	// 받은걸 그대로 전송
	_server->Send(sessionID, packetData.data(), packetData.size());
}

void GameServer::OnSendComplete(SessionID sessionID, int32 len)
{
}

