#pragma once

class RecvBuffer
{
public:
	RecvBuffer(size_t bufferSize);

	size_t DataSize() const { return _writePos - _readPos; }
	size_t FreeSize() const { return _totalSize - _writePos; }
	byte* WriteBuffer() { return _buffer.data() + _writePos; }
	byte* ReadBuffer() { return _buffer.data() + _readPos; }

	void Clean();
	
	bool MoveWritePos(size_t writeBytes);
	bool MoveReadPos(size_t readBytes);

private:
	static constexpr size_t BUFFER_SPARE = 3;

	std::vector<byte> _buffer;
	size_t _writePos = 0;
	size_t _readPos = 0;
	size_t _totalSize;
	size_t _bufferSize;
};

