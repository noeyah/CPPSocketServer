#include "pch.h"
#include "ServerNetworkService.h"
#include "NetworkService.h"
#include "IocpEvent.h"
#include "Session/Listener.h"
#include "Session/Session.h"

ServerNetworkService::ServerNetworkService(std::string ip, uint16 port, INetworkEventHandler* eventHandler, uint32 pendingAcceptCount, uint32 workerThreadCount)
	: NetworkService(ip, port, eventHandler, workerThreadCount), _pendingAcceptCount(pendingAcceptCount)
{
}

ServerNetworkService::~ServerNetworkService()
{
	Stop();
}

bool ServerNetworkService::InitSockets()
{
	_listener = std::make_shared<Listener>(weak_from_this());
	if (!_listener->Start(_pendingAcceptCount))
	{
		LOG_ERROR("Listener start fail");
		return false;
	}

	LOG_INFO("InitSockets !");
	return true;
}

void ServerNetworkService::CleanupSockets()
{
	if (_listener)
	{
		_listener->Stop();
		_listener.reset();
	}
	LOG_INFO("CleanupSockets !");
}
