#pragma once

class Session;
class Listener;
class Connector;

enum class EventOperation : uint8
{
	None,
	Accept,
	Connect,
	Disconnect,
	Recv,
	Send,
};

struct IocpEvent : public OVERLAPPED
{
	// 메모리 시작 부분에 OVERLAPPED가 있어야 함.
	IocpEvent(EventOperation op) : Operation(op) { }
	EventOperation Operation;

	void Init();
};

struct AcceptEvent : public IocpEvent
{
	AcceptEvent() : IocpEvent(EventOperation::Accept) { }
	std::shared_ptr<Listener> ListenerPtr = nullptr;
	SOCKET ClientSocket = INVALID_SOCKET;
	char ClientAddressBuffer[(sizeof(SOCKADDR_IN) + SOCKADDR_PADDING) * 2] = {};
};

struct ConnectEvent : public IocpEvent
{
	ConnectEvent() : IocpEvent(EventOperation::Connect) {}
	std::shared_ptr<Connector> ConnectorPtr = nullptr;
	SOCKET Socket = INVALID_SOCKET;
};

struct SessionEvent : public IocpEvent
{
	SessionEvent(EventOperation op) : IocpEvent(op) {}
	std::shared_ptr<Session> SessionPtr = nullptr;
};

struct SendEvent : public SessionEvent
{
	SendEvent() : SessionEvent(EventOperation::Send) {}
};

struct RecvEvent : public SessionEvent
{
	RecvEvent() : SessionEvent(EventOperation::Recv) {}
};

struct DisconnectEvent : public SessionEvent
{
	DisconnectEvent() : SessionEvent(EventOperation::Disconnect) {}
};