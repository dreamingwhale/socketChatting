#include "Client.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"



AClient::AClient()
{
    PrimaryActorTick.bCanEverTick = true;

    // TCP 소켓 생성
    ClientSocket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("TCPClient"), false);

    FIPv4Address::Parse(TEXT("127.0.0.1"), RemoteAddress);
    RemoteEndpoint = FIPv4Endpoint(RemoteAddress, 8888);

    // 비블로킹 모드 설정 (선택적)
    //ClientSocket->SetNonBlocking(true);
    //ClientSocket->SetRecvErr(true);
}

AClient::~AClient()
{
    if (ClientSocket)
    {
        ClientSocket->Shutdown(ESocketShutdownMode::ReadWrite);
        ClientSocket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ClientSocket);
    }
}

void AClient::BeginPlay()
{
    Super::BeginPlay();
}

void AClient::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

bool AClient::ConnectToServer()
{
    
    TSharedRef<FInternetAddr> Addr = RemoteEndpoint.ToInternetAddr();

    int RetryCount = 0;
    while (!ClientSocket->Connect(*Addr) && RetryCount < 5)
    {
        RetryCount++;
        FPlatformProcess::Sleep(0.1); // 1초 대기 후 재시도
        UE_LOG(LogTemp, Warning, TEXT("Retrying to connect..."));
    }

    if (RetryCount < 5)
    {
        UE_LOG(LogTemp, Warning, TEXT("Connected to server!"));
        return true;
    }
    else
    {
        int32 ErrorCode = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLastErrorCode();
        UE_LOG(LogTemp, Error, TEXT("Failed to connect to server. Error: %d"), ErrorCode);
        return false;
    }

   
}

void AClient::SendMessage(const FString& Message)
{
    if (Message.IsEmpty()) return;

    int32 BytesSent = 0;
    FArrayWriter Writer;
    FTCHARToUTF8 Convert(*Message);
    Writer.Serialize((UTF8CHAR*)Convert.Get(), Convert.Length());
    if (ClientSocket->Send(Writer.GetData(), Writer.Num(), BytesSent))
    {
        UE_LOG(LogTemp, Warning, TEXT("Client Message Sent: %s"), *Message);
    }
    else
    {
		int32 ErrorCode = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLastErrorCode();
		UE_LOG(LogTemp, Error, TEXT("Failed to send message. Error: %d"), ErrorCode);
	}
}

FString AClient::ReceiveMessage()
{
    FString ReceivedMessage;
    uint32 Size = 0;
    if (ClientSocket->HasPendingData(Size))
    {
        TArray<uint8> ReceivedData;
        ReceivedData.SetNumUninitialized(Size);

        int32 BytesRead = 0;
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
            FString UTF8String = FString(UTF8_TO_TCHAR(ReceivedData.GetData()), StringLength);
            // Null 문자 제거
            UTF8String = UTF8String.Replace(TEXT("\0"), TEXT(""), ESearchCase::CaseSensitive);
            ReceivedMessage = UTF8String;
            UE_LOG(LogTemp, Warning, TEXT("Client Received: %s"), *ReceivedMessage);
        }
    }

    return ReceivedMessage;
}