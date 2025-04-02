#include "pch.h"
#include "NetworkService.h"
#include "IocpEvent.h"
#include "Session/Listener.h"
#include "Session/Connector.h"
#include "Session/Session.h"
#include "Session/SocketUtils.h"

NetworkService::NetworkService(std::string ip, uint16 port, INetworkEventHandler* eventHandler) 
	: _eventHandler(eventHandler)
{
	_address.sin_family = AF_INET;

	if (inet_pton(AF_INET, ip.c_str(), &_address.sin_addr) <= 0)
	{
		LOG_ERROR("inet_pton failed");
		return;
	}

	_address.sin_port = htons(port);

	Init();
}

NetworkService::NetworkService(SOCKADDR_IN address, INetworkEventHandler* eventHandler) 
	: _address(address), _eventHandler(eventHandler)
{
	Init();
}

NetworkService::~NetworkService()
{
}

bool NetworkService::RegisterIOCP(SOCKET socket)
{
	return CreateIoCompletionPort(reinterpret_cast<HANDLE>(socket), _iocpHandle, 0, 0) != NULL;
}

std::shared_ptr<Session> NetworkService::CreateAndStartSession(SOCKET socket)
{
	SessionID newSessionID = GetNextSessionID();
	auto session = std::make_shared<Session>(newSessionID, socket, weak_from_this());

	{
		std::lock_guard<std::mutex> lock(_sessionLock);
		_sessions[newSessionID] = session;
	}

	session->StartIO();
	return session;
}

void NetworkService::RemoveSession(SessionID sessionID)
{
	std::lock_guard<std::mutex> lock(_sessionLock);
	_sessions.erase(sessionID);
}

void NetworkService::Send(SessionID sessionID, const byte* data, uint64 length)
{
	auto session = GetSession(sessionID);
	if (session == nullptr)
		return;

	auto sendBuffer = std::make_shared<std::vector<byte>>(length);
	memcpy(sendBuffer->data(), data, length);

	session->Send(sendBuffer);
}

void NetworkService::Send(SessionID sessionID, std::shared_ptr<std::vector<byte>> sendBuffer)
{
	auto session = GetSession(sessionID);
	if (session == nullptr)
		return;

	session->Send(sendBuffer);
}

void NetworkService::Broadcast(const byte* data, uint64 length)
{
	if (data == nullptr || length == 0)
		return;

	auto sendBuffer = std::make_shared<std::vector<byte>>(length);
	memcpy(sendBuffer->data(), data, length);

	Broadcast(sendBuffer);
}

void NetworkService::Broadcast(std::shared_ptr<std::vector<byte>> sendBuffer)
{
	if (sendBuffer == nullptr || sendBuffer->empty())
		return;

	std::vector<std::shared_ptr<Session>> targetSession;
	{
		std::lock_guard<std::mutex> lock(_sessionLock);
		targetSession.reserve(_sessions.size());
		for (const auto& pair : _sessions)
			targetSession.push_back(pair.second);
	}

	for (auto& session : targetSession)
	{
		session->Send(sendBuffer);
	}
}

void NetworkService::Init()
{
	if (_eventHandler == nullptr)
	{
		LOG_ERROR("INetworkEventHandler is null");
		throw std::invalid_argument("INetworkEventHandler is null");
	}

	_iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (_iocpHandle == NULL)
	{
		LOG_ERROR("create iocp handle failed : " << WSAGetLastError());
		throw std::invalid_argument("create iocp handle failed");
	}

	SOCKET tempSocket = SocketUtils::CreateSocket();
	if (tempSocket == INVALID_SOCKET)
	{
		LOG_ERROR("create temp socket failed : " << WSAGetLastError());
		throw std::runtime_error("create temp socket fail");
	}

	if (!SocketUtils::LoadExtensionFunction(tempSocket, WSAID_ACCEPTEX, reinterpret_cast<LPVOID*>(&_acceptEx)))
	{
		LOG_ERROR("WSAID_ACCEPTEX failed");
		throw std::runtime_error("WSAID_ACCEPTEX fail");
	}
	if (!SocketUtils::LoadExtensionFunction(tempSocket, WSAID_GETACCEPTEXSOCKADDRS, reinterpret_cast<LPVOID*>(&_getAcceptExSockAddrs)))
	{
		LOG_ERROR("WSAID_GETACCEPTEXSOCKADDRS failed");
		throw std::runtime_error("WSAID_GETACCEPTEXSOCKADDRS fail");
	}
	if (!SocketUtils::LoadExtensionFunction(tempSocket, WSAID_CONNECTEX, reinterpret_cast<LPVOID*>(&_connectEx)))
	{
		LOG_ERROR("WSAID_CONNECTEX faileded");
		throw std::runtime_error("WSAID_CONNECTEX fail");
	}
	if (!SocketUtils::LoadExtensionFunction(tempSocket, WSAID_DISCONNECTEX, reinterpret_cast<LPVOID*>(&_disconnectEx)))
	{
		LOG_ERROR("WSAID_DISCONNECTEX failed");
		throw std::runtime_error("WSAID_DISCONNECTEX fail");
	}

	SocketUtils::CloseSocket(tempSocket);
}

void NetworkService::Stop()
{
	if (!_isRunning.exchange(false))
		return;

	LOG_INFO("Stopping NetworkService...");

	std::vector<std::shared_ptr<Session>> targetSession;
	{
		std::lock_guard<std::mutex> lock(_sessionLock);
		targetSession.reserve(_sessions.size());
		for (const auto& pair : _sessions)
			targetSession.push_back(pair.second);
	}

	for (auto& session : targetSession)
	{
		session->Disconnect();
	}

	if (_iocpHandle)
	{
		for (int32 i = 0; i < _workerThreads.size(); i++)
		{
			PostQueuedCompletionStatus(_iocpHandle, 0, 0, NULL);
		}
	}

	for (std::thread& t : _workerThreads)
	{
		if (t.joinable())
			t.join();
	}


	_workerThreads.clear();
	_sessions.clear();

	if (_iocpHandle)
	{
		CloseHandle(_iocpHandle);
		_iocpHandle = nullptr;
	}

	LOG_INFO("NetworkService Stop !");
}

void NetworkService::StartThread(uint32 workerThreadCount)
{
	_workerThreadCount = workerThreadCount;
	if (workerThreadCount == 0)
	{
		// ���� �˾Ƽ� ��
		auto hardwareThreadCount = std::thread::hardware_concurrency();
		_workerThreadCount = hardwareThreadCount;
		if (_workerThreadCount <= 0)
			_workerThreadCount = 4;
	}

	// worker thread
	_workerThreads.reserve(_workerThreadCount);
	for (uint32 i = 0; i < _workerThreadCount; i++)
	{
		_workerThreads.emplace_back([this]() { this->RunIocpQueue(); });
	}
}

void NetworkService::RunIocpQueue()
{
	if (_iocpHandle == nullptr)
	{
		LOG_ERROR("HANDLE is null");
		return;
	}

	DWORD bytesTransferd = 0;
	ULONG_PTR completionKey = NULL;
	IocpEvent* iocpEvent = nullptr; 

	while (true)
	{
		BOOL ret = GetQueuedCompletionStatus(_iocpHandle, &bytesTransferd, &completionKey, reinterpret_cast<LPOVERLAPPED*>(&iocpEvent), INFINITE);

		if (!ret)
		{
			auto errorCode = WSAGetLastError();
			switch (errorCode)
			{
			case ERROR_OPERATION_ABORTED:
			{
				LOG_INFO("pending operation aborted. errorCode : " << errorCode);
			}
			break;
			case ERROR_NETNAME_DELETED:
			case WSAECONNRESET:
			{
				if (iocpEvent != nullptr &&
					(iocpEvent->Operation == EventOperation::Recv 
					|| iocpEvent->Operation == EventOperation::Send 
					|| iocpEvent->Operation == EventOperation::Disconnect))
				{
					auto sessionEvent = static_cast<SessionEvent*>(iocpEvent);
					LOG_WARN("client disconnected abruptly. errorCode : " << errorCode << ", sessionID : " << sessionEvent->SessionPtr->GetSessionID());
					sessionEvent->SessionPtr->Disconnect();
				}
				else
				{
					LOG_WARN("client disconnected abruptly. errorCode : " << errorCode);
				}
			}
			break;
			default:
				LOG_ERROR("GetQueuedCompletionStatus : " << errorCode);
				break;
			} // switch end

			continue;
		}

		if (bytesTransferd == 0 && completionKey == 0 && iocpEvent == NULL)
		{
			LOG_INFO("worker thread exiting...");
			break;
		}

		if (iocpEvent == nullptr)
		{
			LOG_ERROR("iocpEvent is null");
			continue;
		}

		switch (iocpEvent->Operation)
		{
		case EventOperation::Accept:
			{
				auto acceptEvent = static_cast<AcceptEvent*>(iocpEvent);
				acceptEvent->ListenerPtr->IOEvent(acceptEvent, bytesTransferd);
			}
			break;
		case EventOperation::Connect:
			{
				auto connectEvent = static_cast<ConnectEvent*>(iocpEvent);
				connectEvent->ConnectorPtr->IOEvent(connectEvent, bytesTransferd);
			}
			break;
		case EventOperation::Disconnect:
		case EventOperation::Recv:
		case EventOperation::Send:
			{
				auto sessionEvent = static_cast<SessionEvent*>(iocpEvent);
				sessionEvent->SessionPtr->IOEvent(sessionEvent, bytesTransferd);
			}
			break;
		default:
			{
				LOG_ERROR("GQCS called with invalid event operation : " << static_cast<int32>(iocpEvent->Operation));
			}
			break;
		}
	}
}

std::shared_ptr<Session> NetworkService::GetSession(SessionID sessionID)
{
	std::lock_guard<std::mutex> lock(_sessionLock);
	
	auto it = _sessions.find(sessionID);
	
	if (it != _sessions.end())
		return it->second;
		
	return nullptr;
}
