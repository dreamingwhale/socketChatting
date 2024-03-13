// Fill out your copyright notice in the Description page of Project Settings.


#include "Server.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"

// Sets default values
AServer::AServer()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ServerSocket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_DGram, TEXT("UDPServer"), true);
	FIPv4Address::Parse(TEXT("192.168.0.133"), ServerEndpoint.Address);
	ServerEndpoint.Port = 5001;

	//SetReuseAddr세팅. 이걸 안하면 소켓을 닫고 다시 열때 bind에러가 발생한다.
	ServerSocket->SetReuseAddr(true);

	// 비블로킹 모드 설정
	ServerSocket->SetNonBlocking(true);
	ServerSocket->SetRecvErr(true);

	// 소켓 바인딩
	bool bBind = ServerSocket->Bind(*ServerEndpoint.ToInternetAddr());

	if (!bBind)
	{
		UE_LOG(LogTemp, Error, TEXT("Server socket bind failed"));
        UE_LOG(LogTemp, Error, TEXT("Error: %s"), *ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetSocketError());
	}
    else
    {
		UE_LOG(LogTemp, Warning, TEXT("Server socket bind success"));
        UE_LOG(LogTemp, Warning, TEXT("Server address: %s"), *ServerEndpoint.ToInternetAddr()->ToString(true));
    }


}

AServer::~AServer()
{
	if (ServerSocket)
	{
		ServerSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ServerSocket);
	}
}

// Called when the game starts or when spawned
void AServer::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void AServer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


	ReceiveMessage();

}

void AServer::ReceiveMessage()
{
    uint32 Size = 0;
    TSharedRef<FInternetAddr> Sender = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();

    while (ServerSocket->HasPendingData(Size))
    {
        FArrayReader Reader;
        Reader.SetNumUninitialized(FMath::Min(Size, 65507u));
        int32 BytesRead = 0;
        FString tmp = ServerSocket->RecvFrom(Reader.GetData(), Reader.Num(), BytesRead, *Sender) ? TEXT("TRUE") : TEXT("FALSE");
        UE_LOG(LogTemp, Warning, TEXT("ServerSocket->RecvFrom : %s"), *tmp);


        if (ServerSocket->RecvFrom(Reader.GetData(), Reader.Num(), BytesRead, *Sender))
        {
            UE_LOG(LogTemp, Warning, TEXT("Received %d bytes from %s"), BytesRead, *Sender->ToString(true));

            // UTF-8로 인코딩된 데이터를 FString으로 변환
            FString ReceivedMessage = FString(UTF8_TO_TCHAR(Reader.GetData()));

            if (!ReceivedMessage.IsEmpty())
            {
                UE_LOG(LogTemp, Warning, TEXT("Received: %s"), *ReceivedMessage);

                // 수신된 메시지를 클라이언트에게 다시 보내기
                FArrayWriter Writer;
                // 메시지 다시 UTF-8로 직렬화하여 전송
                FString ConvertedString = StringCast<ANSICHAR>(*ReceivedMessage).Get();
                Writer.Serialize((UTF8CHAR*)TCHAR_TO_UTF8(*ConvertedString), ConvertedString.Len());

                int32 BytesSent;
                ServerSocket->SendTo(Writer.GetData(), Writer.Num(), BytesSent, *Sender);

                if (BytesSent > 0)
                {
                    UE_LOG(LogTemp, Warning, TEXT("Sent back: %s"), *ReceivedMessage);
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("Failed to send message back."));
                }
            }
        }
        else
        {
            // 데이터 수신 실패한 경우의 처리 로직 (예: 로깅)
            UE_LOG(LogTemp, Error, TEXT("Failed to receive data."));
        }
    }
}
