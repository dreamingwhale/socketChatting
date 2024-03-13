#include "Server.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "HAL/Runnable.h"

class FConnectionListener : public FRunnable
{
public:
	FConnectionListener(FSocket* ServerSocket, TArray<FSocket*>& ClientSockets)
		: ServerSocket(ServerSocket), ClientSockets(ClientSockets), bRun(true)
	{
	}

	virtual bool Init() override
	{
		// 스레드 초기화 코드
		return true;
	}

	virtual uint32 Run() override
	{
		bool Pending;
		while (bRun)
		{
			if (ServerSocket->HasPendingConnection(Pending) && Pending)
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
					UE_LOG(LogTemp, Warning, TEXT("Failed to connect to client"));
					FPlatformProcess::Sleep(0.01); // 잠시 대기
				}
			}
			
		}
		return 0;
	}

	virtual void Stop() override
	{
		bRun = false;
	}

private:
	FSocket* ServerSocket;
	TArray<FSocket*>& ClientSockets;
	bool bRun;
};

// AServer 클래스에 멤버 변수 추가
FRunnableThread* ListenerThread;
FConnectionListener* ConnectionListener;


AServer::AServer()
{
	PrimaryActorTick.bCanEverTick = true;

	// TCP 소켓 생성 및 설정
	ServerSocket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("default"), false);
	FIPv4Address::Parse(TEXT("192.168.0.133"), ServerAddress);
	ServerEndpoint = FIPv4Endpoint(ServerAddress, 8888);

	// 소켓 바인딩 및 리스닝
	//ServerSocket->SetReuseAddr(true);
	ServerSocket->Bind(*ServerEndpoint.ToInternetAddr());
	ServerSocket->Listen(8);
}
AServer::~AServer()
{
	
	if (ListenerThread != nullptr)
	{
		ConnectionListener->Stop();
		ListenerThread->WaitForCompletion();
		delete ListenerThread;
		ListenerThread = nullptr;
	}
	if (ServerSocket)
	{
		ServerSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ServerSocket);
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
				FArrayReader Reader;
				Reader.SetNumUninitialized(Size);

				int32 BytesRead = 0;
				if (ClientSocket->Recv(Reader.GetData(), Reader.Num(), BytesRead))
				{
					FString ReceivedMessage;
					Reader << ReceivedMessage;
					UE_LOG(LogTemp, Warning, TEXT("Received: %s"), *ReceivedMessage);
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
