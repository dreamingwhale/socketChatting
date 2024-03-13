
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Networking/Public/Networking.h"
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
	bool ConnectToServer();

	// Send message to server
	UFUNCTION(BlueprintCallable)
	void SendMessage(const FString& Message);

	// Receive message from server
	UFUNCTION(BlueprintCallable)
	FString ReceiveMessage();

private:
	FSocket* ClientSocket;
	FIPv4Endpoint RemoteEndpoint;
	FIPv4Address RemoteAddress;
};