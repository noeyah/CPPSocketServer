#include "pch.h"
#include "SocketUtils.h"

namespace SocketUtils
{
	SOCKET CreateSocket()
	{
		return WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	}

	bool Bind(SOCKET socket, SOCKADDR_IN address)
	{
		return bind(socket, reinterpret_cast<const SOCKADDR*>(&address), sizeof(address)) != SOCKET_ERROR;
	}

	bool BindAnyAddress(SOCKET socket, uint16 port)
	{
		SOCKADDR_IN address;
		memset(&address, 0, sizeof(address));
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = ::htonl(INADDR_ANY);
		address.sin_port = ::htons(port);

		return Bind(socket, address);
	}

	bool Listen(SOCKET socket, int32 backlog)
	{
		return listen(socket, backlog) != SOCKET_ERROR;
	}

	void CloseSocket(SOCKET& socket)
	{
		if (socket != INVALID_SOCKET)
		{
			closesocket(socket);
			socket = INVALID_SOCKET;
		}
	}

	bool LoadExtensionFunction(SOCKET socket, GUID guid, LPVOID* func)
	{
		DWORD dwBytes = 0;
		return WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
			&guid, sizeof(guid),
			func, sizeof(*func),
			&dwBytes, NULL, NULL) != SOCKET_ERROR;
	}

	bool SetUpdateAcceptContext(SOCKET newSocket, SOCKET acceptSocket)
	{
		return setsockopt(newSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, reinterpret_cast<char*>(&acceptSocket), sizeof(acceptSocket)) != SOCKET_ERROR;
	}

	bool SetReuseAddress(SOCKET socket, bool reuse)
	{
		return setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&reuse), sizeof(reuse)) != SOCKET_ERROR;
	}

	bool SetLinger(SOCKET socket, uint16 onoff, uint16 lingerTime)
	{
		LINGER option;
		option.l_onoff = onoff;		// 0이면 끔. 디폴트 0
		option.l_linger = lingerTime;	// 옵션이 켜져있을 때 대기할 시간(초)
		return setsockopt(socket, SOL_SOCKET, SO_LINGER, reinterpret_cast<char*>(&option), sizeof(option)) != SOCKET_ERROR;
	}

	bool SetNoDelay(SOCKET socket, bool enable)
	{
		// 1 : nodelay
		// 0 : nagle
		// 디폴트 0
		return setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&enable), sizeof(enable)) != SOCKET_ERROR;
	}

}