#pragma once

namespace SocketUtils
{
	SOCKET CreateSocket();

	bool Bind(SOCKET socket, SOCKADDR_IN sockAddr);
	bool BindAnyAddress(SOCKET socket, uint16 port);
	bool Listen(SOCKET socket, int32 backlog = SOMAXCONN);
	void CloseSocket(SOCKET& socket);

	bool LoadExtensionFunction(SOCKET socket, GUID guid, LPVOID* func);

	bool SetUpdateAcceptContext(SOCKET newSocket, SOCKET acceptSocket);
	bool SetReuseAddress(SOCKET socket, bool reuse);
	bool SetLinger(SOCKET socket, uint16 onoff, uint16 lingerTime = 0);
	bool SetNoDelay(SOCKET socket, bool enable);

}
