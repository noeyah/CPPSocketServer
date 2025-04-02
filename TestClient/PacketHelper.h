#pragma once
#include "NetworkPch.h"

namespace PacketHelper
{
	std::shared_ptr<std::vector<byte>> MakeStringPacket(const std::string& msg);

	bool UnpackStringPacket(std::span<const byte> packetData, std::string& recvStr);
};

