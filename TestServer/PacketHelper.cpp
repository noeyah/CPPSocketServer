#include "pch.h"
#include "PacketHelper.h"
#include "NetworkPch.h"

namespace PacketHelper
{
	std::shared_ptr<std::vector<byte>> MakeStringPacket(const std::string& msg)
	{
		auto packetSize = HEADER_DATA_SIZE + msg.length();

		auto  sendBuffer = std::make_shared<std::vector<byte>>(packetSize);
		byte* bufferPtr = sendBuffer->data();

		memcpy(bufferPtr, &packetSize, HEADER_DATA_SIZE);

		if (msg.length() > 0)
			memcpy(bufferPtr + HEADER_DATA_SIZE, msg.data(), msg.length());

		return sendBuffer;
	}

	bool UnpackStringPacket(std::span<const byte> packetData, std::string& recvStr)
	{
		if (packetData.size() < HEADER_DATA_SIZE)
			return false;

		uint64 dataSize = 0;
		memcpy(&dataSize, packetData.data(), HEADER_DATA_SIZE);

		if (dataSize != packetData.size())
			return false;

		uint64 packetSize = dataSize - HEADER_DATA_SIZE;
		if (packetSize > 0)
		{
			recvStr.assign(reinterpret_cast<const char*>(packetData.data() + HEADER_DATA_SIZE), packetSize);
		}
		else
		{
			recvStr.clear();
		}

		return true;
	}
};