#include "Server.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "HAL/Runnable.h"

class FConnectionListener : public FRunnable
{
public:
	FConnectionListener(FSocket* ServerSocket, TArray<FSocket*>& ClientSockets)
		: ServerSocket(ServerSocket), ClientSockets(ClientSockets), bRun(true), Pending(true)
	{
	}

	virtual bool Init() override
	{
		// 스레드 초기화 코드
		return true;
	}

	virtual uint32 Run() override
	{
		Pending;
		while (bRun)
		{
			if (Pending)
			{

				if (ServerSocket->HasPendingConnection(Pending))
				{
					UE_LOG(LogTemp, Warning, TEXT("Client Connecting"));
					FSocket* ClientSocket = ServerSocket->Accept(TEXT("TCPClient"));
					if (ClientSocket != nullptr)
					{
						UE_LOG(LogTemp, Warning, TEXT("Client Connected"));
						ClientSockets.Add(ClientSocket);
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("Failed connect to client"));
						FPlatformProcess::Sleep(0.01); // 잠시 대기
					}
				}
			}

		}
		return 0;
	}

	virtual void Stop() override
	{
		bRun = false;
		Pending = false;
	}

private:
	FSocket* ServerSocket;
	TArray<FSocket*>& ClientSockets;
	bool bRun;
	bool Pending;
};

// AServer 클래스에 멤버 변수 추가
FRunnableThread* ListenerThread;
FConnectionListener* ConnectionListener;


AServer::AServer()
{
	PrimaryActorTick.bCanEverTick = true;

	// TCP 소켓 생성 및 설정
	ServerSocket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("default"), false);
	FIPv4Address::Parse(TEXT("127.0.0.1"), ServerAddress);
	ServerEndpoint = FIPv4Endpoint(ServerAddress, 8888);

	// 소켓 바인딩 및 리스닝
	//ServerSocket->SetReuseAddr(true);

	if (ServerSocket->Bind(*ServerEndpoint.ToInternetAddr()))
	{

		if (ServerSocket->Listen(8))
		{
			UE_LOG(LogTemp, Warning, TEXT("Server is listening"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to listen socket. Error: %d"), ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLastErrorCode());
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to bind socket. Error: %d"), ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLastErrorCode());
	}

	// 비블로킹 모드 설정 (선택적)
	//ServerSocket->SetNonBlocking(true);
	//ServerSocket->SetRecvErr(true);
}
AServer::~AServer()
{

	if (ListenerThread != nullptr)
	{
		ConnectionListener->Stop();
		ConnectionListener->Exit();
		ListenerThread->WaitForCompletion();
		delete ListenerThread;
		ListenerThread = nullptr;
	}
	if (ServerSocket)
	{
		ServerSocket->Close();
		ServerSocket->Shutdown(ESocketShutdownMode::ReadWrite);
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ServerSocket);
	}
	if (ClientSockets.Num() > 0)
	{
		for (FSocket* ClientSocket : ClientSockets)
		{
			ClientSocket->Close();
			ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ClientSocket);
		}
	}
}

void AServer::BeginPlay()
{
	Super::BeginPlay();

	//thread
	ConnectionListener = new FConnectionListener(ServerSocket, ClientSockets);
	ListenerThread = FRunnableThread::Create(ConnectionListener, TEXT("ConnectionListenerThread"));
}

void AServer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ReceiveMessage();
}


void AServer::ReceiveMessage()
{
	TArray<FSocket*> ClientsToRemove;
	if (ClientSockets.Num() > 0)
	{
		for (FSocket* ClientSocket : ClientSockets)
		{
			uint32 Size;
			if (ClientSocket->HasPendingData(Size))
			{
				TArray<uint8> ReceivedData;
				ReceivedData.SetNumUninitialized(Size);

				int32 BytesRead = 1024;
				if (ClientSocket->Recv(ReceivedData.GetData(), ReceivedData.Num(), BytesRead))
				{
					// 데이터의 끝에 Null 문자가 있을 수 있으므로 제거
					int32 StringLength = BytesRead;
					for (int32 i = 0; i < BytesRead; ++i)
					{
						if (ReceivedData[i] == 0)
						{
							StringLength = i;
							break;
						}
					}

					// UTF-8 문자열로 가정하고 변환
					FString ReceivedMessage = FString(UTF8_TO_TCHAR(ReceivedData.GetData()), StringLength);
					UE_LOG(LogTemp, Warning, TEXT("Server Received: %s"), *ReceivedMessage);
					if (ClientSocket->Send(ReceivedData.GetData(), BytesRead, BytesRead))
					{
						UE_LOG(LogTemp, Warning, TEXT("Server Message Sent: %s"), *ReceivedMessage);
					}
					else
					{
						int32 ErrorCode = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLastErrorCode();
						UE_LOG(LogTemp, Error, TEXT("Failed to send message. Error: %d"), ErrorCode);
					}
				}

				else if (BytesRead <= 0)
				{
					// 연결이 끊겼다고 판단되는 경우, 해당 클라이언트를 배열에서 제거
					ClientsToRemove.Add(ClientSocket);
				}
			}
		}

	}

	// 연결이 끊긴 클라이언트 소켓 제거
	for (FSocket* SocketToRemove : ClientsToRemove)
	{
		ClientSockets.Remove(SocketToRemove);
		SocketToRemove->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(SocketToRemove);
	}
}
