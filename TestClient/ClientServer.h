#pragma once
#include "Network/IPublicService.h"

class ClientServer : public INetworkEventHandler
{
public:
	void Start(std::string ip, uint16 port, uint32 connectCount = 1, uint32 workerThreadCount = 0);
	void Stop();

	virtual void OnConnect(SessionID sessionID) override;
	virtual void OnDisconnect(SessionID sessionID) override;
	virtual void OnRecv(SessionID sessionID, std::span<const byte> packetData) override;
	virtual void OnSendComplete(SessionID sessionID, int32 len) override;

private:
	std::shared_ptr<IPublicService> _server;

};
