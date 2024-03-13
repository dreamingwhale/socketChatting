// Fill out your copyright notice in the Description page of Project Settings.
#define _WINSOCK_DEPRECATED_NO_WARNINGS 
#define _CRT_SECURE_NO_WARNINGS

#include "Client.h"
#include <iostream>
#include <WinSock2.h>

using namespace std;

#pragma comment(lib, "ws2_32")


// Sets default values
AClient::AClient()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	WSAStartup(MAKEWORD(2, 2), &wsaData);

	ServerSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

	memset(&ServerSockAddr, 0, sizeof(ServerSockAddr));
	ServerSockAddr.sin_family = AF_INET;
	ServerSockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	ServerSockAddr.sin_port = htons(5001);
	ServerSockAddrLength = sizeof(ServerSockAddr);

	// 소켓을 비블로킹 모드로 설정
	u_long mode = 1; // 0이 아닌 값은 소켓을 비블로킹으로 설정
	ioctlsocket(ServerSocket, FIONBIO, &mode);

}

AClient::~AClient()
{
	closesocket(ServerSocket);

	WSACleanup();

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
	char Buffer[1024] = { 0, };
	FCStringAnsi::Strcpy(Buffer, 1024, TCHAR_TO_ANSI(*message));
	if(message != "")
	UE_LOG(LogTemp, Warning, TEXT("Buffer : %s"), *FString(Buffer));
	int SendLength = sendto(ServerSocket, Buffer, (int)strlen(Buffer), 0, (SOCKADDR*)&ServerSockAddr, sizeof(ServerSockAddr));
}

FString AClient::ReceiveMessage()
{
	char Message[1024] = { 0, };

	int RecvLength = recvfrom(ServerSocket, Message, sizeof(Message), 0, (SOCKADDR*)&ServerSockAddr, &ServerSockAddrLength);
	if (RecvLength > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("RecvLength : %s"), *FString(Message));
		return FString(Message);
	}
	return FString(Message);
}
