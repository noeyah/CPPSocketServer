#pragma once

class Session;

class INetworkEventHandler
{
public:
	virtual ~INetworkEventHandler() = default;

	virtual void OnConnect(SessionID sessionID) = 0;
	virtual void OnDisconnect(SessionID sessionID) = 0;
	virtual void OnRecv(SessionID sessionID, std::span<const byte> packetData) = 0;
	virtual void OnSendComplete(SessionID sessionID, int32 len) {};
};

class NetworkService : public std::enable_shared_from_this<NetworkService>
{
public:
	NetworkService(std::string ip, uint16 port, INetworkEventHandler* eventHandler);
	NetworkService(SOCKADDR_IN address, INetworkEventHandler* eventHandler);
	virtual ~NetworkService();

	virtual bool Start(uint32 workerThreadCount = 0) = 0;
	virtual void Stop();

	void RegisterEventHandler(INetworkEventHandler* eventHandler) { _eventHandler = eventHandler; }
	INetworkEventHandler* GetEventHandler() const { return _eventHandler; }
	SOCKADDR_IN GetAddress() const { return _address; }
	HANDLE GetHandle() const { return _iocpHandle; }

	bool RegisterIOCP(SOCKET socket);

	// session 관련 함수
	std::shared_ptr<Session> CreateAndStartSession(SOCKET socket);
	void RemoveSession(SessionID sessionID);
	void Send(SessionID sessionID, const byte* data, uint64 length);
	void Send(SessionID sessionID, std::shared_ptr<std::vector<byte>> sendBuffer);
	void Broadcast(const byte* data, uint64 length);
	void Broadcast(std::shared_ptr<std::vector<byte>> sendBuffer);

	// winsock 확장 함수들
	LPFN_ACCEPTEX GetAcceptEx() const { return _acceptEx; }
	LPFN_GETACCEPTEXSOCKADDRS GetAcceptExSockAddrs() const { return _getAcceptExSockAddrs; }
	LPFN_CONNECTEX GetConnectEx() const { return _connectEx; }
	LPFN_DISCONNECTEX GetDisConnectEx() const { return _disconnectEx; }

protected:
	void Init();

	// worker thread
	void StartThread(uint32 workerThreadCount);
	void RunIocpQueue();

	SessionID GetNextSessionID() { return ++_nextSessionID; }
	std::shared_ptr<Session> GetSession(SessionID sessionID);

protected:
	// iocp 핸들
	HANDLE _iocpHandle = nullptr;

	// 설정값
	SOCKADDR_IN _address = {};
	uint32 _workerThreadCount = 0;

	// session
	INetworkEventHandler* _eventHandler = nullptr;
	std::map<SessionID, std::shared_ptr<Session>> _sessions;
	std::atomic<SessionID> _nextSessionID = 0;
	std::mutex _sessionLock;

	// worker thread
	std::vector<std::thread> _workerThreads;
	std::atomic<bool> _isRunning = false;

	// winsock 확장 함수들
	LPFN_ACCEPTEX _acceptEx = nullptr;
	LPFN_GETACCEPTEXSOCKADDRS _getAcceptExSockAddrs = nullptr;
	LPFN_CONNECTEX _connectEx = nullptr;
	LPFN_DISCONNECTEX _disconnectEx = nullptr;

};

