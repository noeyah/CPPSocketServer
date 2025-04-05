#pragma once


class IPublicService
{
public:
	virtual ~IPublicService() = default;

	virtual bool Start() = 0;
	virtual void Stop() = 0;

	// send
	virtual void Send(SessionID sessionID, std::span<const byte> data) = 0;
	virtual void Send(SessionID sessionID, std::shared_ptr<std::vector<byte>> data) = 0;

	// broadcast
	virtual void Broadcast(std::span<const byte> data) = 0;
	virtual void Broadcast(std::shared_ptr<std::vector<byte>> data) = 0;
};

class INetworkEventHandler
{
public:
	virtual ~INetworkEventHandler() = default;

	virtual void OnConnect(SessionID sessionID) = 0;
	virtual void OnDisconnect(SessionID sessionID) = 0;
	virtual void OnRecv(SessionID sessionID, std::span<const byte> packetData) = 0;
	virtual void OnSendComplete(SessionID sessionID, int32 len) {};
};

namespace NetworkFactory
{
	std::shared_ptr<IPublicService> CreateServerService(
		std::string ip, uint16 port, 
		INetworkEventHandler* eventHandler, 
		uint32 pendingAcceptCount = 100, 
		uint32 workerThreadCount = 0
	);

	std::shared_ptr<IPublicService> CreateClientService(
		std::string ip, uint16 port,
		INetworkEventHandler* eventHandler,
		uint32 connectCount = 1,
		uint32 workerThreadCount = 0
	);

}