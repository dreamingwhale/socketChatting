#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include <iostream>
#include <WinSock2.h>
#include "Client.generated.h"

UCLASS()
class SOCKETCHATTING_API AClient : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AClient();
	~AClient();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	WSADATA wsaData;
	SOCKET ServerSocket;
	SOCKADDR_IN ServerSockAddr;
	int ServerSockAddrLength;

public:
	UFUNCTION(BlueprintCallable, Category = "Socket")
	void SendMessage(FString message);

	UFUNCTION(BlueprintCallable, Category = "Socket")
	FString ReceiveMessage();
};
