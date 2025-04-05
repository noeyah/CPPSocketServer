#pragma once
#include "NetworkService.h"

class Listener;

class ServerNetworkService : public NetworkService
{
public:
	ServerNetworkService(std::string ip, uint16 port, INetworkEventHandler* eventHandler, uint32 pendingAcceptCount, uint32 workerThreadCount);
	virtual ~ServerNetworkService();

	virtual bool InitSockets() override;
	virtual void CleanupSockets() override;

private:

	// listener
	std::shared_ptr<Listener> _listener;
	uint32 _pendingAcceptCount;

};