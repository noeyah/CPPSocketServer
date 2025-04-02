#include "pch.h"
#include "RecvBuffer.h"

RecvBuffer::RecvBuffer(size_t bufferSize) : _buffer(bufferSize * BUFFER_SPARE), _totalSize(bufferSize * BUFFER_SPARE), _bufferSize(bufferSize)
{
}

void RecvBuffer::Clean()
{
	auto dataSise = DataSize();
	if (dataSise == 0)
	{
		_writePos = 0;
		_readPos = 0;
		return;
	}

	if (FreeSize() < _bufferSize)
	{
		memmove(_buffer.data(), _buffer.data() + _readPos, dataSise);
		_readPos = 0;
		_writePos = dataSise;
	}
}

bool RecvBuffer::MoveWritePos(size_t writeBytes)
{
	if (FreeSize() < writeBytes)
		return false;

	_writePos += writeBytes;
	return true;
}

bool RecvBuffer::MoveReadPos(size_t readBytes)
{
	if (DataSize() < readBytes)
		return false;

	_readPos += readBytes;
	return true;
}
