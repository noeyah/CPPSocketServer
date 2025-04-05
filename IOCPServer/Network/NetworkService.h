#pragma once
#include "IPublicService.h"

class Session;

class NetworkService : public IPublicService, public std::enable_shared_from_this<NetworkService>
{
public:
	NetworkService(std::string ip, uint16 port, INetworkEventHandler* eventHandler, uint32 workerThreadCount);
	virtual ~NetworkService();

	// IPublicService �Լ� override
	virtual bool Start() final;
	virtual void Stop() final;
	virtual void Send(SessionID sessionID, std::span<const byte> data) override;
	virtual void Send(SessionID sessionID, std::shared_ptr<std::vector<byte>> sendBuffer) override;
	virtual void Broadcast(std::span<const byte> data) override;
	virtual void Broadcast(std::shared_ptr<std::vector<byte>> sendBuffer) override;

	// ���� Ŭ�������� ���� ���� �ʱ�ȭ
	virtual bool InitSockets() = 0;
	virtual void CleanupSockets() = 0;

	//
	INetworkEventHandler* GetEventHandler() const { return _eventHandler; }
	SOCKADDR_IN GetAddress() const { return _address; }
	bool RegisterIOCP(SOCKET socket);
	//HANDLE GetHandle() const { return _iocpHandle; }

	// session ���� �Լ�
	std::shared_ptr<Session> CreateAndStartSession(SOCKET socket);
	void RemoveSession(SessionID sessionID);

	// winsock Ȯ�� �Լ���
	LPFN_ACCEPTEX GetAcceptEx() const { return _acceptEx; }
	LPFN_GETACCEPTEXSOCKADDRS GetAcceptExSockAddrs() const { return _getAcceptExSockAddrs; }
	LPFN_CONNECTEX GetConnectEx() const { return _connectEx; }
	LPFN_DISCONNECTEX GetDisConnectEx() const { return _disconnectEx; }

protected:
	void Init();

	// worker thread
	void StartThread();
	void Dispatcher();

	SessionID GetNextSessionID() { return ++_nextSessionID; }
	std::shared_ptr<Session> GetSession(SessionID sessionID);

protected:
	// iocp �ڵ�
	HANDLE _iocpHandle = nullptr;

	// ������
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

	// winsock Ȯ�� �Լ���
	LPFN_ACCEPTEX _acceptEx = nullptr;
	LPFN_GETACCEPTEXSOCKADDRS _getAcceptExSockAddrs = nullptr;
	LPFN_CONNECTEX _connectEx = nullptr;
	LPFN_DISCONNECTEX _disconnectEx = nullptr;

};

