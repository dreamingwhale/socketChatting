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
		// ������ �ʱ�ȭ �ڵ�
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
					FPlatformProcess::Sleep(0.01); // ��� ���
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

// AServer Ŭ������ ��� ���� �߰�
FRunnableThread* ListenerThread;
FConnectionListener* ConnectionListener;


AServer::AServer()
{
	PrimaryActorTick.bCanEverTick = true;

	// TCP ���� ���� �� ����
	ServerSocket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("default"), false);
	FIPv4Address::Parse(TEXT("192.168.0.133"), ServerAddress);
	ServerEndpoint = FIPv4Endpoint(ServerAddress, 8888);

	// ���� ���ε� �� ������
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
					// ������ ����ٰ� �ǴܵǴ� ���, �ش� Ŭ���̾�Ʈ�� �迭���� ����
					ClientsToRemove.Add(ClientSocket);
				}
			}
		}

	}

	// ������ ���� Ŭ���̾�Ʈ ���� ����
	for (FSocket* SocketToRemove : ClientsToRemove)
	{
		ClientSockets.Remove(SocketToRemove);
		SocketToRemove->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(SocketToRemove);
	}
}
