// Fill out your copyright notice in the Description page of Project Settings.

#include "Client.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"


// Sets default values
AClient::AClient()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ClientSocket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_DGram, TEXT("UDPClient"), true);

	FIPv4Address::Parse(TEXT("192.168.0.133"), RemoteEndpoint.Address);
	RemoteEndpoint.Port = 5001;

	// 비블로킹 모드 설정
	ClientSocket->SetNonBlocking(true);
	ClientSocket->SetRecvErr(true);
}

AClient::~AClient()
{
	if (ClientSocket != nullptr)
	{
		ClientSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ClientSocket);
	}
}


// Called when the game starts or when spawned
void AClient::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void AClient::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	SendMessage("");

}

void AClient::SendMessage(FString message)
{
	int32 BytesSent = 1024;
	TSharedRef<FInternetAddr> Addr = RemoteEndpoint.ToInternetAddr();
	FArrayWriter Writer;
	FTCHARToUTF8 Convert(*message);
	Writer.Serialize((UTF8CHAR*)Convert.Get(), Convert.Length());
	if (message != "")
	{
		UE_LOG(LogTemp, Warning, TEXT("Sending message: %s"), *message);
		UE_LOG(LogTemp, Warning, TEXT("Sent %d bytes to %s"), BytesSent, *Addr->ToString(true));
		UE_LOG(LogTemp, Warning, TEXT("Writer.GetData() :  %s , Writer.Num() : %d"), Writer.GetData(), Writer.Num());
	}
	ClientSocket->SendTo(Writer.GetData(), Writer.Num(), BytesSent, *Addr);
}

FString AClient::ReceiveMessage()
{
	FString ReceivedMessage;
	uint32 Size;
	TSharedRef<FInternetAddr> Sender = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();

	while (ClientSocket->HasPendingData(Size))
	{
		FArrayReader Reader;
		Reader.SetNumUninitialized(FMath::Min(Size, 65507u));

		int32 BytesRead;
		if (ClientSocket->RecvFrom(Reader.GetData(), Reader.Num(), BytesRead, *Sender))
		{
			Reader << ReceivedMessage;
		}
	}

	return ReceivedMessage;
}
