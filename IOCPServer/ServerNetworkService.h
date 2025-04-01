#pragma once
#include "NetworkService.h"

class Listener;

class ServerNetworkService : public NetworkService
{
public:
	ServerNetworkService(std::string ip, uint16 port, INetworkEventHandler* eventHandler, int32 pendingAcceptCount);
	ServerNetworkService(SOCKADDR_IN address, INetworkEventHandler* eventHandler, int32 pendingAcceptCount);
	virtual ~ServerNetworkService();

	virtual bool Start(uint32 workerThreadCount = 0) override;
	virtual void Stop() override;

private:

	// listener
	std::shared_ptr<Listener> _listener;
	int32 _pendingAcceptCount;

};