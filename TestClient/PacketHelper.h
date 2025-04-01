#pragma once
#include "NetworkPch.h"

namespace PacketHelper
{
	std::shared_ptr<std::vector<byte>> MakeStringPacket(const std::string& msg);

	bool UnpackStringPacket(const byte* buffer, int32 len, std::string& recvStr);
};

