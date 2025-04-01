#pragma once

using byte		= unsigned char;		// byte

using int8		= __int8;				// sbyte
using int16		= __int16;				// short
using int32		= __int32;				// int
using int64		= __int64;				// long
using uint8		= unsigned __int8;		// byte
using uint16	= unsigned __int16;		// ushort
using uint32	= unsigned __int32;		// uint
using uint64	= unsigned __int64;		// ulong

using SessionID = uint32;

#define LOG_INFO(msg) std::cout << "[INFO] " << msg << std::endl;
#define LOG_ERROR(msg) std::cout << "[ERROR] " << msg << std::endl;

// winsock���� �ּ� ���� ������ ����ü ũ�� + 16����Ʈ �ؾ���.
constexpr DWORD SOCKADDR_PADDING = 16;

constexpr WORD NETWORK_WINSOCK_VERSION = MAKEWORD(2, 2);

// ��� �� 2����Ʈ�� �������� ũ��
constexpr uint64 HEADER_DATA_SIZE = sizeof(uint16);

// ��Ŷ �ִ� ũ��
constexpr uint64 MAX_RECV_BUFFER_SIZE = 4096; 
constexpr uint64 MAX_SEND_BUFFER_SIZE = 4096;
constexpr uint64 MAX_SEND_BUFFER_COUNT = 10;