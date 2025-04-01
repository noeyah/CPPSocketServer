#include "pch.h"
#include "IocpEvent.h"

void IocpEvent::Init()
{
	memset(static_cast<OVERLAPPED*>(this), 0, sizeof(OVERLAPPED));
}
