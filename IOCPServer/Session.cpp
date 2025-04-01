#include "pch.h"
#include "Session.h"
#include "NetworkService.h"
#include "IocpEvent.h"
#include "RecvBuffer.h"

Session::Session(SessionID sessionID, SOCKET socket, std::weak_ptr<NetworkService> service)
	: _sessionID(sessionID), _socket(socket), _service(service)
{
}

Session::~Session()
{
}

void Session::IOEvent(SessionEvent* iocpEvent, int32 bytesTransferred)
{
	/*if (_connected == false )
	{
		LOG_ERROR("IOEvent called on already disconnected session : " << _sessionID << ", operation : " << static_cast<int32>(iocpEvent->Operation));
		return;
	}*/

	switch (iocpEvent->Operation)
	{
	case EventOperation::Disconnect:
		ProcessDisconnect();
		break;
	case EventOperation::Recv:
		ProcessRecv(bytesTransferred);
		break;
	case EventOperation::Send:
		ProcessSend(bytesTransferred);
		break;
	default:
		LOG_ERROR("IOEvent called with invalid event operation  : " << static_cast<int32>(iocpEvent->Operation));
		break;
	}
}

void Session::StartIO()
{
	_connected.store(true);

	_recvEvent.SessionPtr = shared_from_this();
	_sendEvent.SessionPtr = shared_from_this();
	_disconnectEvent.SessionPtr = shared_from_this();

	GetService()->GetEventHandler()->OnConnect(_sessionID);

	PostRecv();
}

void Session::Disconnect()
{
	_connected.store(false);

	LOG_INFO("reserve disconnect : " << _sessionID);

	{
		std::lock_guard<std::mutex> lock(_shutdownLock);
		if (_pendingSend)
			return;

		_postDisconnected.store(true);
	}

	LOG_INFO("Disconnect : " << _sessionID);
	PostDisconnect();
}

void Session::Send(std::shared_ptr<std::vector<byte>> sendBuffer)
{
	if (_connected == false)
		return;

	if (sendBuffer == nullptr || sendBuffer->empty())
		return;

	bool postSend = false;

	{
		std::lock_guard<std::mutex> lock(_sendLock);
		_sendQueue.push_back(sendBuffer);

		if (_pendingSend.exchange(true) == false)
			postSend = true;
	}

	if (postSend)
		PostSend();
}

void Session::PostRecv()
{
	if (_connected == false)
		return;

	_recvEvent.Init();

	WSABUF wsaBuf;
	wsaBuf.buf = reinterpret_cast<char*>(_recvBuffer.WriteBuffer());
	wsaBuf.len = static_cast<ULONG>(_recvBuffer.FreeSize());

	DWORD numOfBytes = 0;
	DWORD flag = 0;

	auto ret = WSARecv(
		_socket, 
		&wsaBuf, 
		1, 
		&numOfBytes, 
		&flag, 
		&_recvEvent, 
		nullptr);

	if (ret == SOCKET_ERROR)
	{
		auto errorCode = WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			LOG_ERROR("post recv failed : " << errorCode);
			Disconnect();
			return;
		}
	}
}

void Session::ProcessRecv(int32 bytesTransferred)
{
	if (bytesTransferred == 0)
	{
		LOG_INFO("Disconnected from session, recv 0 bytes : " << _sessionID);
		Disconnect();
		return;
	}

	if (!_recvBuffer.MoveWritePos(bytesTransferred))
	{
		LOG_ERROR("not enough free size in RecvBuffer");
		return;
	}

	ParseRecvData();
	_recvBuffer.Clean();
	
	PostRecv();
}

void Session::ParseRecvData()
{
	auto dataSize = _recvBuffer.DataSize();

	while (true)
	{
		auto dataSize = _recvBuffer.DataSize();
		if (dataSize < HEADER_DATA_SIZE)
			break;

		auto readBuffer = _recvBuffer.ReadBuffer();
		auto packetSize = 0;
		memcpy(&packetSize, readBuffer, HEADER_DATA_SIZE);

		if (dataSize < packetSize)
			break;

		GetService()->GetEventHandler()->OnRecv(_sessionID, readBuffer, packetSize);
		_recvBuffer.MoveReadPos(packetSize);
	}
}

// 호출하기 전에 _pendingSend : true
void Session::PostSend()
{
	if (!_connected)
		return;

	std::vector<WSABUF> wsaBufs;

	{
		std::lock_guard<std::mutex> lock(_sendLock);

		if (_sendQueue.empty())
		{
			_pendingSend.store(false);
			return;
		}

		_sendBufferCount = 0;
		_bytesToSend = 0;

		auto reserveCount = min(_sendQueue.size(), MAX_SEND_BUFFER_COUNT);
		wsaBufs.reserve(reserveCount);

		for (const auto& buffer : _sendQueue)
		{
			if (_sendBufferCount >= MAX_SEND_BUFFER_COUNT || _bytesToSend + buffer->size() > MAX_SEND_BUFFER_SIZE)
				break;

			WSABUF wsaBuf;
			wsaBuf.buf = reinterpret_cast<char*>(buffer->data());
			wsaBuf.len = static_cast<ULONG>(buffer->size());
			wsaBufs.push_back(wsaBuf);

			_bytesToSend += wsaBuf.len;
			_sendBufferCount++;
		}

		if (wsaBufs.empty())
		{
			_pendingSend.store(false);
			LOG_ERROR("send queue to wsabuf failed. sendQueue[0] size : " << _sendQueue[0]->size());
			return;
		}
	}
	
	_sendEvent.Init();

	DWORD numOfBytes = 0;
	auto ret = WSASend(
		_socket,
		wsaBufs.data(),
		static_cast<DWORD>(wsaBufs.size()),
		&numOfBytes,
		0,
		&_sendEvent,
		nullptr
	);

	if (ret == SOCKET_ERROR)
	{
		auto errorCode = WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			LOG_ERROR("post send failed : " << errorCode);
			_pendingSend.store(false);
			_bytesToSend = 0;
			Disconnect();
			return;
		}
	}
}

void Session::ProcessSend(int32 bytesTransferred)
{
	if (bytesTransferred <= 0)
	{
		LOG_ERROR("ProcessSend called with bytesTransferred : " << bytesTransferred);
		_pendingSend.store(false);
		Disconnect();
		return;
	}

	{
		std::lock_guard<std::mutex> lock(_sendLock);

		if (bytesTransferred != _bytesToSend)
		{
			LOG_ERROR("ProcessSend : bytesTransferred(" << bytesTransferred << ") != byteToSend(" << _bytesToSend << ")");
			// disconnect ?
		}

		for (auto i = 0; i < _sendBufferCount; i++)
		{
			_sendQueue.pop_front();
		}

		_sendBufferCount = 0;
		_bytesToSend = 0;
	}

	GetService()->GetEventHandler()->OnSendComplete(_sessionID, bytesTransferred);

	// 연결이 끊어졌으면 여기서 끝냄
	if (!_connected)
	{
		_pendingSend.store(false);
		bool reserveDisconnect = false;
		{
			std::lock_guard<std::mutex> lock(_shutdownLock);
			if (!_pendingSend && !_postDisconnected)
			{
				_postDisconnected.store(true);
				reserveDisconnect = true;
			}
		}

		if (reserveDisconnect)
		{
			PostDisconnect();
		}

		return;
	}
	
	// 그 사이 쌓인 큐가 있으면 또 보냄
	bool postSend = false;
	{
		std::lock_guard<std::mutex> lock(_sendLock);
		if (!_sendQueue.empty())
			postSend = true;
	}

	if (postSend)
		PostSend();
	else
		_pendingSend.store(false);
}

void Session::PostDisconnect()
{
	auto disconnectEx = GetService()->GetDisConnectEx();
	if (disconnectEx == nullptr)
	{
		LOG_ERROR("disconnectEx is null");
		return;
	}

	_disconnectEvent.Init();

	auto ret = disconnectEx(
		_socket,
		&_disconnectEvent,
		0,
		0
	);

	if (!ret)
	{
		const int32 errorCode = WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			LOG_ERROR("disconnectEx : " << errorCode);
			return;
		}
	}
}

void Session::ProcessDisconnect()
{
	LOG_INFO("ProcessDisconnect : " << _sessionID);

	_recvEvent.SessionPtr.reset();
	_sendEvent.SessionPtr.reset();
	_disconnectEvent.SessionPtr.reset();

	GetService()->GetEventHandler()->OnDisconnect(_sessionID);

	SocketUtils::CloseSocket(_socket);

	GetService()->RemoveSession(_sessionID);
}
