#pragma once

struct ConnectEvent;
class NetworkService;

class Connector : public std::enable_shared_from_this<Connector>
{
public:
	Connector(std::weak_ptr<NetworkService> service);
	~Connector();

	void Connect(uint32 connectCount);
	void IOEvent(ConnectEvent* iocpEvent, int32 bytesTransferred);

private:
	void PostConnect();
	void ProcessConnect(ConnectEvent* iocpEvent);

	std::shared_ptr<NetworkService> GetService() { return _service.lock(); }

private:
	std::vector<std::unique_ptr<ConnectEvent>> _iocpEventList;
	std::weak_ptr<NetworkService> _service;
};
