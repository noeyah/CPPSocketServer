#include "pch.h"
#include "ClientNetworkService.h"
#include "NetworkService.h"
#include "IocpEvent.h"
#include "Session/Connector.h"
#include "Session/Session.h"

ClientNetworkService::ClientNetworkService(std::string ip, uint16 port, INetworkEventHandler* eventHandler, uint32 connectCount, uint32 workderThreadCount)
	: NetworkService(ip, port, eventHandler, workderThreadCount), _connectCount(connectCount)
{
}

ClientNetworkService::~ClientNetworkService()
{
	Stop();
}

bool ClientNetworkService::InitSockets()
{
	_connector = std::make_shared<Connector>(weak_from_this());
	_connector->Connect(_connectCount);

	LOG_INFO("InitSockets");
	return true;
}

void ClientNetworkService::CleanupSockets()
{
	if (_connector)
	{
		_connector.reset();
	}
	LOG_INFO("CleanupSockets");
}
