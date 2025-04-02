#pragma once

struct AcceptEvent;
class NetworkService;

class Listener : public std::enable_shared_from_this<Listener>
{
public:
	Listener(std::weak_ptr<NetworkService> service);
	~Listener();

	bool Start(int32 pendingAcceptCount, int32 backlog = SOMAXCONN);
	void Stop();
	void IOEvent(AcceptEvent* iocpEvent, int32 bytesTransferred);

private:
	void PostAccept(AcceptEvent* iocpEvent);
	void ProcessAccept(AcceptEvent* iocpEvent);

	std::shared_ptr<NetworkService> GetService() { return _service.lock(); }
	
private:
	SOCKET _socket = INVALID_SOCKET;
	std::vector<std::unique_ptr<AcceptEvent>> _iocpEventList;
	std::weak_ptr<NetworkService> _service;
	std::atomic<bool> _isListening = false;

};

