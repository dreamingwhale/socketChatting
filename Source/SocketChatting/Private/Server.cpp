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
		// ������ �ʱ�ȭ �ڵ�
		return true;
	}

	virtual uint32 Run() override
	{
		Pending;
		while (bRun)
		{
			while (Pending)
			{

				while (ServerSocket->HasPendingConnection(Pending))
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
						FPlatformProcess::Sleep(0.01); // ��� ���
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

// AServer Ŭ������ ��� ���� �߰�
FRunnableThread* ListenerThread;
FConnectionListener* ConnectionListener;


AServer::AServer()
{
	PrimaryActorTick.bCanEverTick = true;

	// TCP ���� ���� �� ����
	ServerSocket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("default"), false);
	FIPv4Address::Parse(TEXT("127.0.0.1"), ServerAddress);
	ServerEndpoint = FIPv4Endpoint(ServerAddress, 8888);

	// ���� ���ε� �� ������
	//ServerSocket->SetReuseAddr(true);


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
		ServerSocket->Shutdown(ESocketShutdownMode::ReadWrite);
		ServerSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ServerSocket);
	}
	if (ClientSockets.Num() > 0)
	{
		for (FSocket* ClientSocket : ClientSockets)
		{
			ClientSocket->Shutdown(ESocketShutdownMode::ReadWrite);
			ClientSocket->Close();
			ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ClientSocket);
		}
	}
}

void AServer::BeginPlay()
{
	Super::BeginPlay();


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

	// ����ŷ ��� ���� (������)
	//ServerSocket->SetNonBlocking(true);
	//ServerSocket->SetRecvErr(true);

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
		TArray<uint8> ReceivedData;
		bool bDataReceived = false;
		int32 TotalBytesRead = 0;

		// ��� Ŭ���̾�Ʈ ���� Ȯ��
		for (FSocket* ClientSocket : ClientSockets)
		{
			uint32 Size = 0;
			if (ClientSocket->HasPendingData(Size))
			{
				ReceivedData.SetNumUninitialized(Size);

				int32 BytesRead = 0;
				if (ClientSocket->Recv(ReceivedData.GetData(), ReceivedData.Num(), BytesRead))
				{
					bDataReceived = true;
					TotalBytesRead = BytesRead;
				}
				else if (BytesRead <= 0)
				{
					// ������ ����ٸ� Ŭ���̾�Ʈ ����
					ClientsToRemove.Add(ClientSocket);
				}
			}
		}

		// ���� �����Ͱ� ������ ��� Ŭ���̾�Ʈ���� ����
		if (bDataReceived)
		{
			for (FSocket* ClientSocket : ClientSockets)
			{
				int32 BytesSent = 0;
				ClientSocket->Send(ReceivedData.GetData(), TotalBytesRead, BytesSent);
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
}