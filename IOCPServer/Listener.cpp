#include "pch.h"
#include "Listener.h"
#include "NetworkService.h"
#include "IocpEvent.h"
#include "Session.h"

Listener::Listener(std::weak_ptr<NetworkService> service) : _service(service)
{
}

Listener::~Listener()
{
}

bool Listener::Start(int32 pendingAcceptCount, int32 backlog)
{
	if (_service.expired())
		return false;

	if (_socket != INVALID_SOCKET)
		return false;

	_socket = SocketUtils::CreateSocket();
	if (_socket == INVALID_SOCKET)
		return false;

	if (!GetService()->RegisterIOCP(_socket)) 
	{
		LOG_ERROR("Listener register iocp failed : " << GetLastError());
		SocketUtils::CloseSocket(_socket);
		return false;
	}

	if (!SocketUtils::SetReuseAddress(_socket, true))
	{
		LOG_ERROR("setsockopt reuse failed");
		SocketUtils::CloseSocket(_socket);
		return false;
	}
	
	auto address = GetService()->GetAddress();
	if (!SocketUtils::Bind(_socket, address))
	{
		LOG_ERROR("bind failed : " << WSAGetLastError());
		SocketUtils::CloseSocket(_socket);
		return false;
	}
	
	if (!SocketUtils::Listen(_socket, backlog))
	{
		LOG_ERROR("listen failed : " << WSAGetLastError());
		SocketUtils::CloseSocket(_socket);
		return false;
	}

	_isListening.store(true);

	for (int32 i = 0; i < pendingAcceptCount; i++)
	{
		auto iocpEvent = std::make_unique<AcceptEvent>();
		iocpEvent->ListenerPtr = shared_from_this();
		PostAccept(iocpEvent.get());
		
		_iocpEventList.push_back(std::move(iocpEvent));
	}

	return true;
}

void Listener::Stop()
{
	_isListening.store(false);

	CancelIoEx(reinterpret_cast<HANDLE>(_socket), NULL);

	SocketUtils::CloseSocket(_socket);
}

void Listener::IOEvent(AcceptEvent* iocpEvent, int32 bytesTransferred)
{
	if (iocpEvent->Operation != EventOperation::Accept)
	{
		LOG_ERROR("Listener IOEvent called with invalid event operation : " << static_cast<int32>(iocpEvent->Operation));
		return;
	}

	ProcessAccept(iocpEvent);
}

void Listener::PostAccept(AcceptEvent* iocpEvent)
{
	if (!_isListening)
		return;

	SOCKET newClient = SocketUtils::CreateSocket();
	if (newClient == INVALID_SOCKET)
	{
		LOG_ERROR("create socket failed : " << WSAGetLastError());
		PostAccept(iocpEvent);
		return;
	}

	iocpEvent->Init();
	iocpEvent->ClientSocket = newClient;

	// AcceptEx
	auto acceptEx = GetService()->GetAcceptEx();
	if (acceptEx == nullptr)
	{
		LOG_ERROR("acceptEx is null ");
		SocketUtils::CloseSocket(newClient);
		return;
	}

	DWORD bytesReceived = 0;
	
	auto ret = acceptEx(
		_socket,
		iocpEvent->ClientSocket,
		iocpEvent->ClientAddressBuffer,
		0,
		sizeof(SOCKADDR_IN) + SOCKADDR_PADDING,
		sizeof(SOCKADDR_IN) + SOCKADDR_PADDING,
		&bytesReceived,
		static_cast<LPOVERLAPPED>(iocpEvent)
	);

	if (!ret)
	{
		const int32 errorCode = WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			LOG_ERROR("AcceptEx : " << errorCode);
			SocketUtils::CloseSocket(newClient);
			//PostAccept(iocpEvent);
			return;
		}
	}
}

void Listener::ProcessAccept(AcceptEvent* iocpEvent)
{
	SOCKET newClinet = iocpEvent->ClientSocket;

	if (!SocketUtils::SetUpdateAcceptContext(newClinet, _socket))
	{
		LOG_ERROR("SO_UPDATE_ACCEPT_CONTEXT failed : " << WSAGetLastError());
		SocketUtils::CloseSocket(newClinet);
		PostAccept(iocpEvent);
		return;
	}

	if (!GetService()->RegisterIOCP(newClinet))
	{
		LOG_ERROR("register iocp failed : " << WSAGetLastError());
		SocketUtils::CloseSocket(newClinet);
		PostAccept(iocpEvent);
		return;
	}

	LPSOCKADDR localAddr = nullptr;
	LPSOCKADDR remoteAddr = nullptr;
	int32 localAddrLength = 0;
	int32 remoteAddrLength = 0;

	auto getAcceptExSockAddrs = GetService()->GetAcceptExSockAddrs();
	if (getAcceptExSockAddrs == nullptr)
	{
		LOG_INFO("GetAcceptExSockAddrs is null ");
		// 없으면 없는대로 진행?
	}
	else
	{
		getAcceptExSockAddrs(
			iocpEvent->ClientAddressBuffer,
			0,
			sizeof(SOCKADDR_IN) + SOCKADDR_PADDING,
			sizeof(SOCKADDR_IN) + SOCKADDR_PADDING,
			&localAddr,
			&localAddrLength,
			&remoteAddr,
			&remoteAddrLength
		);
	}

	auto newSession = GetService()->CreateAndStartSession(newClinet);
	if (newSession != nullptr)
	{
		SOCKADDR_IN* clientAddr = reinterpret_cast<SOCKADDR_IN*>(remoteAddr);
		if (clientAddr != nullptr)
		{
			newSession->SetSockAddr(*clientAddr);
		}
	}

	PostAccept(iocpEvent);
}

