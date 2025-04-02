#include "pch.h"
#include "ClientNetworkService.h"
#include "NetworkService.h"
#include "IocpEvent.h"
#include "Session/Connector.h"
#include "Session/Session.h"

ClientNetworkService::ClientNetworkService(std::string ip, uint16 port, INetworkEventHandler* eventHandler, int32 connectCount)
	: NetworkService(ip, port, eventHandler), _connectCount(connectCount)
{
}

ClientNetworkService::ClientNetworkService(SOCKADDR_IN address, INetworkEventHandler* eventHandler, int32 connectCount)
	: NetworkService(address, eventHandler), _connectCount(connectCount)
{
}

ClientNetworkService::~ClientNetworkService()
{
	Stop();
}

bool ClientNetworkService::Start(uint32 workerThreadCount)
{
	if (_isRunning.exchange(true))
		return false;

	_connector = std::make_shared<Connector>(weak_from_this());
	_connector->Connect(_connectCount);

	// worker thread
	StartThread(workerThreadCount);

	LOG_INFO("Client Start !");
	return true;
}

void ClientNetworkService::Stop()
{
	LOG_INFO("Stopping ClientNetworkService...");

	if (_connector)
	{
		_connector.reset();
	}

	LOG_INFO("ClientNetworkService Stop !");
	NetworkService::Stop();
}
