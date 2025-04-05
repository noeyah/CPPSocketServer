#pragma once
#include "NetworkService.h"

class Connector;

class ClientNetworkService : public NetworkService
{
public:
	ClientNetworkService(std::string ip, uint16 port, INetworkEventHandler* eventHandler, uint32 connectCount, uint32 workderThreadCount);
	virtual ~ClientNetworkService();

	virtual bool InitSockets() override;
	virtual void CleanupSockets() override;

private:
	std::shared_ptr<Connector> _connector;
	uint32 _connectCount;
};
