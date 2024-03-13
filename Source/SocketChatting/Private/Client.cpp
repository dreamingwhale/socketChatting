#include "Client.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"



AClient::AClient()
{
    PrimaryActorTick.bCanEverTick = true;

    // TCP 소켓 생성
    ClientSocket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("TCPClient"), false);

    FIPv4Address::Parse(TEXT("192.168.0.133"), RemoteAddress);
    RemoteEndpoint = FIPv4Endpoint(RemoteAddress, 8888);

    // 비블로킹 모드 설정 (선택적)
    //ClientSocket->SetNonBlocking(true);
    //ClientSocket->SetRecvErr(true);
}

AClient::~AClient()
{
    if (ClientSocket)
    {
        ClientSocket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ClientSocket);
    }
}

void AClient::BeginPlay()
{
    Super::BeginPlay();

    ConnectToServer();
}

void AClient::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

bool AClient::ConnectToServer()
{
    TSharedRef<FInternetAddr> Addr = RemoteEndpoint.ToInternetAddr();

    if (ClientSocket->Connect(*Addr))
    {
        UE_LOG(LogTemp, Warning, TEXT("Connected to server!"));
        return true;
    }

    UE_LOG(LogTemp, Error, TEXT("Failed to connect to server."));
    return false;
}

void AClient::SendMessage(const FString& Message)
{
    if (Message.IsEmpty()) return;

    int32 BytesSent = 0;
    FArrayWriter Writer;
    FTCHARToUTF8 Convert(*Message);
    Writer.Serialize((UTF8CHAR*)Convert.Get(), Convert.Length());
    UE_LOG(LogTemp, Warning, TEXT("Message Sent: %s"), *Message);
    ClientSocket->Send(Writer.GetData(), Writer.Num(), BytesSent);
}

FString AClient::ReceiveMessage()
{
    FString ReceivedMessage;
    uint32 Size = 0;
    if (ClientSocket->HasPendingData(Size))
    {
        FArrayReader Reader;
        Reader.SetNumUninitialized(Size);

        int32 BytesRead = 0;
        if (ClientSocket->Recv(Reader.GetData(), Reader.Num(), BytesRead))
        {
            Reader << ReceivedMessage;
        }
    }

    return ReceivedMessage;
}