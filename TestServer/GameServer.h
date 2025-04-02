#pragma once
#include "Network/ServerNetworkService.h"

class GameServer : public INetworkEventHandler
{
public:
	void Start(std::string ip, uint16 port, int32 pendingAcceptCount);
	void Stop();

	virtual void OnConnect(SessionID sessionID) override;
	virtual void OnDisconnect(SessionID sessionID) override;
	virtual void OnRecv(SessionID sessionID, std::span<const byte> packetData) override;
	virtual void OnSendComplete(SessionID sessionID, int32 len) override;

private:
	std::shared_ptr<ServerNetworkService> _server;

};
