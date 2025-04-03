
## IOCPServer


### 🎀 프로젝트 소개

C++을 사용하여 제작한 **IOCP 라이브러리**로, 포트폴리오용으로 개발되었습니다.   



### 📋 기능 요약
- 서버 리스닝 및 클라이언트 연결 관리
- 비동기 소켓 처리 및 데이터 송수신
- 세션 관리



### ✏ 사용 가이드

사용 예시로 `TestServer` 프로젝트를 참고하세요.

---

#### 1. `IOCPServer` 라이브러리 추가   

---

#### 2. 필수 헤더 포함   
필요한 헤더 파일을 포함합니다.   
```cpp
#include "NetworkPch.h" // 필수 : Winsock 관련 헤더, NetworkDefine 등 공통 정의 포함
```
- `NetworkPch.h`는 라이브러리 사용에 필요한 기본 헤더(WinSock2, WS2tcpip 등)와 라이브러리 전용 정의(NetworkDefine)를 포함하고 있습니다.   

```cpp
#include "Network/ServerNetworkService.h"	// 서버로 사용할 경우
#include "Network/ClientNetworkService.h"	// 클라이언트로 사용할 경우
```
- 사용 목적에 따라 둘 중 하나를 선택하여 포함합니다.   

---

#### 3. 이벤트 핸들러 구현   
네트워크 이벤트(연결, 끊김, 수신 등)를 처리하기 위한 핸들러 클래스를 구현해야 합니다.   
**3-1. `INetworkEventHandler` 인터페이스 상속**   
사용자 코드에서 `INetworkEventHandler` 인터페이스를 상속받는 클래스를 정의합니다.   
```cpp
#include "Network/NetworkService.h"

class MyEventHandler : public INetworkEventHandler
{
	virtual void OnConnect(SessionID sessionID) override
	{
		// TODO: 연결 성공 처리 로직
	}

	virtual void OnDisconnect(SessionID sessionID) override
	{
		// TODO: 연결 종료 처리 로직
	}

	virtual void OnRecv(SessionID sessionID, std::span<const byte> packetData) override
	{
		// TODO: 데이터 수신 처리 로직
		// packetData는 내부 버퍼로, 데이터를 즉시 처리하거나 복사해야 합니다.
	}

	virtual void OnSendComplete(SessionID sessionID, int32 len) override
	{
		// 필요시 구현
		// TODO: 데이터 송신 완료시 호출
	}
}
```

---

**3-2. 핸들러 객체 생성 및 관리**   
- 위에서 정의한 클래스의 인스턴스를 생성합니다.   
- ⚠ 중요 : **생성된 이벤트 핸들러 객체는 이후 생성될 `NetworkService` 객체보다 반드시 오래 유지**되어야 합니다. 
- `NetworkService`는 내부적으로 이 핸들러 객체의 포인터를 저장하고 사용하므로, **서비스가 동작하는 동안 핸들러 객체가 파괴되면 안 됩니다.**

---

#### 4. 초기화   
네트워크 기능을 사용하기 전에 Winsock을 초기화합니다.   
```cpp
int main()
{
	// 1. Winsock 초기화
	WSADATA wsaData;
	// NETWORK_WINSOCK_VERSION은 NetworkPch.h 또는 NetworkDefine.h 포함 시 사용 가능
	if (WSAStartup(NETWORK_WINSOCK_VERSION, &wsaData) != 0)
	{
		// 에러 처리
		throw std::runtime_error("WSAStartup error");
	}

	// 2. 이벤트 핸들러 객체 생성
	MyEventHandler eventHandler; // 스택 또는 스마트 포인터 등으로 관리

	// ... 
}
```

---

#### 5. 네트워크 서비스 생성 및 시작   
서버 또는 클라이언트 네트워크 서비스를 생성하고 시작합니다.
```cpp
// 3. 네트워크 서비스 생성

// 서버 예시
auto server = std::make_shared<ServerNetworkService>("127.0.0.1", 7777, &eventHandler, pendingAcceptCount);

// 클라이언트 예시
auto client = std::make_shared<ClientNetworkService>(ip, port, &eventHandler, connectCount);
```
📌 매개변수 설명
- `std::string ip` : IP 주소
- `uint32 Port` : Port 번호
- **IP + Port** 대신 `SOCKADDR_IN` 구조체 사용 가능
- `INetworkEventHandler* eventHandler` : 위에서 정의한 이벤트 핸들러 객체 주소
- `int32 pendingAcceptCount` : (서버 전용) 최대 동시 Accept 수
- `int32 connectCount` : (클라이언트 전용) 연결 시도할 클라이언트(세션) 수

```cpp
// 4. 네트워크 서비스 시작
 int workerThreadCount = 0; // 스레드 개수 지정 가능 (0이면 자동 결정)
 if (!server->Start(workerThreadCount))
 {
	// 에러 처리
 }

 // network service started ...
```

---

#### 6. 서비스 사용   
- 서비스가 시작되면 내부 스레드에서 네트워크 작업이 비동기적으로 처리됩니다.
- 연결, 종료, 데이터 수신 등의 **이벤트는 구현한 이벤트 핸들러의 콜백 함수**를 통해 전달됩니다.
- 데이터 송신은 `ServerNetworkService` 또는 `ClientNetworkService` 객체의 멤버 함수(`Send`, `Broadcast`)를 호출합니다. 
- `Send` 호출 시 `SessionID`가 필요합니다.
```cpp
server->Send(sessionID, packetData);
server->Broadcast(packetData);
```

---

#### 7. 종료   
네트워크 서비스를 중지하고 Winsock을 정리합니다.
```cpp
server->Stop();
WSACleanup();
```

---
