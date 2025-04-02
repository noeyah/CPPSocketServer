#pragma once
#include "NetworkService.h"

class Connector;

class ClientNetworkService : public NetworkService
{
public:
	ClientNetworkService(std::string ip, uint16 port, INetworkEventHandler* eventHandler, int32 connectCount);
	ClientNetworkService(SOCKADDR_IN address, INetworkEventHandler* eventHandler, int32 connectCount);
	virtual ~ClientNetworkService();

	virtual bool Start(uint32 workerThreadCount = 0) override;
	virtual void Stop() override;

private:
	std::shared_ptr<Connector> _connector;
	int32 _connectCount;
};
