#pragma once
#include "IocpEvent.h"
#include "RecvBuffer.h"

class NetworkService;

class Session : public std::enable_shared_from_this<Session>
{
public:
	Session(SessionID sessionID, SOCKET socket, std::weak_ptr<NetworkService> service);
	~Session();

	void SetSockAddr(const SOCKADDR_IN& sockAddress) { _sockAddress = sockAddress; }

	// GQCS에서 호출
	void IOEvent(SessionEvent* iocpEvent, int32 bytesTransferred);

	// Connected and PostRecv
	void StartIO();

	void Disconnect();
	void Send(std::shared_ptr<std::vector<byte>> sendBuffer);

private:
	void PostRecv();
	void ProcessRecv(int32 bytesTransferred);
	void ParseRecvData();

	void PostSend();
	void ProcessSend(int32 bytesTransferred);
	
	void PostDisconnect();
	void ProcessDisconnect();

	std::shared_ptr<NetworkService> GetService() { return _service.lock(); }

private:
	SOCKET _socket = INVALID_SOCKET;
	SessionID _sessionID = 0;
	SOCKADDR_IN _sockAddress = {};

	std::weak_ptr<NetworkService> _service;

	// disconnect
	DisconnectEvent _disconnectEvent;
	std::atomic<bool> _connected = false;
	std::atomic<bool> _postDisconnected = false;
	std::mutex _shutdownLock;

	// recv
	RecvEvent _recvEvent;
	RecvBuffer _recvBuffer{ MAX_RECV_BUFFER_SIZE };

	// send
	SendEvent _sendEvent;
	std::atomic<bool> _pendingSend = false;
	std::mutex _sendLock;
	std::deque<std::shared_ptr<std::vector<byte>>> _sendQueue;
	uint64 _bytesToSend = 0;
	uint64 _sendBufferCount = 0;
};
