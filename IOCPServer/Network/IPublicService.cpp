#include "pch.h"
#include "IPublicService.h"
#include "ServerNetworkService.h"
#include "ClientNetworkService.h"

namespace NetworkFactory
{
	std::shared_ptr<IPublicService> CreateServerService(std::string ip, uint16 port, INetworkEventHandler* eventHandler, uint32 pendingAcceptCount, uint32 workerThreadCount)
	{
		auto service = std::make_shared<ServerNetworkService>(ip, port, eventHandler, pendingAcceptCount, workerThreadCount);

		return service;
	}

	std::shared_ptr<IPublicService> CreateClientService(std::string ip, uint16 port, INetworkEventHandler* eventHandler, uint32 connectCount, uint32 workerThreadCount)
	{
		auto service = std::make_shared<ClientNetworkService>(ip, port, eventHandler, connectCount, workerThreadCount);

		return service;
	}
}


