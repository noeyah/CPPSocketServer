#include "pch.h"
#include "GameServer.h"
#include "NetworkPch.h"
#include "Network/IPublicService.h"
#include "PacketHelper.h"

void GameServer::Start(std::string ip, uint16 port, uint32 pendingAcceptCount, uint32 workerThreadCount)
{
	if (_server)
		return;

	_server = NetworkFactory::CreateServerService(ip, port, this, pendingAcceptCount, workerThreadCount);

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
	_server->Send(sessionID, packetData);
}

void GameServer::OnSendComplete(SessionID sessionID, int32 len)
{
}

