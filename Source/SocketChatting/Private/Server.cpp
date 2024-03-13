// Fill out your copyright notice in the Description page of Project Settings.


#include "Server.h"
#include <iostream>
#include <WinSock2.h>

#pragma comment(lib, "ws2_32")

// Sets default values
AServer::AServer()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	WSAStartup(MAKEWORD(2, 2), &wsaData);

	ServerSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

	memset(&ServerSockAddr, 0, sizeof(ServerSockAddr));
	ServerSockAddr.sin_family = AF_INET;
	ServerSockAddr.sin_addr.s_addr = INADDR_ANY;
	ServerSockAddr.sin_port = htons(5001);
	bind(ServerSocket, (SOCKADDR*)&ServerSockAddr, sizeof(ServerSockAddr));

	// 소켓을 비블로킹 모드로 설정
	u_long mode = 1; // 0이 아닌 값은 소켓을 비블로킹으로 설정
	ioctlsocket(ServerSocket, FIONBIO, &mode);

	
}

AServer::~AServer()
{
	closesocket(ServerSocket);

	WSACleanup();
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

	
	memset(&ClientSockAddr, 0, sizeof(ClientSockAddr));
	int ClientSockAddrLength = sizeof(ClientSockAddr);

	char Buffer[1024] = { 0, };
	int RecvLength = recvfrom(ServerSocket, Buffer, sizeof(Buffer), 0, (SOCKADDR*)&ClientSockAddr, &ClientSockAddrLength);

	
	if (RecvLength > 0)
		UE_LOG(LogTemp, Warning, TEXT("Received: %s"), *FString(Buffer));
	

	int SendLength = sendto(ServerSocket, Buffer, RecvLength, 0, (SOCKADDR*)&ClientSockAddr, sizeof(ClientSockAddr));
	

}

