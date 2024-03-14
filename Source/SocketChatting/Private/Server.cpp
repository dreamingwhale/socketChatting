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
					// �������� ���� Null ���ڰ� ���� �� �����Ƿ� ����
					int32 StringLength = BytesRead;
					for (int32 i = 0; i < BytesRead; ++i)
					{
						if (ReceivedData[i] == 0)
						{
							StringLength = i;
							break;
						}
					}

					// UTF-8 ���ڿ��� �����ϰ� ��ȯ
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
