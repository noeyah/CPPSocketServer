#include "pch.h"
#include "ServerNetworkService.h"
#include "NetworkService.h"
#include "IocpEvent.h"
#include "Session/Listener.h"
#include "Session/Session.h"

ServerNetworkService::ServerNetworkService(std::string ip, uint16 port, INetworkEventHandler* eventHandler, int32 pendingAcceptCount)
	: NetworkService(ip, port, eventHandler), _pendingAcceptCount(pendingAcceptCount)
{
}

ServerNetworkService::ServerNetworkService(SOCKADDR_IN address, INetworkEventHandler* eventHandler, int32 pendingAcceptCount)
	: NetworkService(address, eventHandler), _pendingAcceptCount(pendingAcceptCount)
{
}

ServerNetworkService::~ServerNetworkService()
{
	Stop();
}

bool ServerNetworkService::Start(uint32 workerThreadCount)
{
	if (_isRunning.exchange(true))
		return false;

	// listener
	_listener = std::make_shared<Listener>(weak_from_this());
	if (!_listener->Start(_pendingAcceptCount))
	{
		LOG_ERROR("Listener start fail");
		CloseHandle(_iocpHandle);
		_iocpHandle = nullptr;
		_isRunning = false;
		return false;
	}

	// worker thread
	StartThread(workerThreadCount);

	LOG_INFO("Service Start !");
	return true;
}

void ServerNetworkService::Stop()
{
	LOG_INFO("Stopping ServerNetworkService...");

	if (_listener)
	{
		_listener->Stop();
		_listener.reset();
	}

	LOG_INFO("ServerNetworkService Stop !");

	NetworkService::Stop();
}
