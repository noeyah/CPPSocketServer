#include "pch.h"
#include "Connector.h"
#include "Network/NetworkService.h"
#include "Network/IocpEvent.h"
#include "Session.h"
#include "SocketUtils.h"

Connector::Connector(std::weak_ptr<NetworkService> service) : _service(service)
{
}

Connector::~Connector()
{
}

void Connector::Connect(int32 connectCount)
{
	for (int32 i = 0; i < connectCount; i++)
	{
		PostConnect();
	}
}

void Connector::IOEvent(ConnectEvent* iocpEvent, int32 bytesTransferred)
{
	if (iocpEvent->Operation != EventOperation::Connect)
	{
		LOG_ERROR("Connector IOEvent called with invalid event operation : " << static_cast<int32>(iocpEvent->Operation));
		return;
	}
	
	ProcessConnect(iocpEvent);
}

void Connector::PostConnect()
{
	if (_service.expired())
		return;

	SOCKET socket = SocketUtils::CreateSocket();
	if (socket == INVALID_SOCKET)
		return;

	if (!SocketUtils::SetReuseAddress(socket, true))
	{
		LOG_ERROR("setsockopt reuse failed");
		SocketUtils::CloseSocket(socket);
		return;
	}

	if (!SocketUtils::BindAnyAddress(socket, 0))
	{
		LOG_ERROR("bind failed : " << WSAGetLastError());
		SocketUtils::CloseSocket(socket);
		return;
	}

	if (!GetService()->RegisterIOCP(socket))
	{
		LOG_ERROR("register iocp failed : " << WSAGetLastError());
		SocketUtils::CloseSocket(socket);
		return;
	}

	auto serverAddress = GetService()->GetAddress();
	auto connectEx = GetService()->GetConnectEx();
	if (connectEx == nullptr)
	{
		LOG_ERROR("connectEx is null ");
		SocketUtils::CloseSocket(socket);
		return;
	}

	auto iocpEvent = std::make_unique<ConnectEvent>();
	iocpEvent->Init();
	iocpEvent->ConnectorPtr = shared_from_this();
	iocpEvent->Socket = socket;

	ConnectEvent* connectEventPtr = iocpEvent.get();

	DWORD bytesSent = 0;

	auto ret = connectEx(
		socket,
		reinterpret_cast<const SOCKADDR*>(&serverAddress),
		sizeof(serverAddress),
		nullptr,
		0,
		&bytesSent,
		connectEventPtr
	);

	if (!ret)
	{
		const int32 errorCode = WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			LOG_ERROR("connectEx : " << errorCode);
			SocketUtils::CloseSocket(socket);
			return;
		}
	}

	_iocpEventList.push_back(std::move(iocpEvent));
}

void Connector::ProcessConnect(ConnectEvent* iocpEvent)
{
	SOCKET socket = iocpEvent->Socket;

	if (socket == INVALID_SOCKET)
		return;

	GetService()->CreateAndStartSession(socket);
}
