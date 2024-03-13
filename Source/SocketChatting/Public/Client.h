
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
	FSocket* ClientSocket;
	FIPv4Endpoint RemoteEndpoint;

public:
	UFUNCTION(BlueprintCallable, Category = "Socket")
	void SendMessage(FString message);

	UFUNCTION(BlueprintCallable, Category = "Socket")
	FString ReceiveMessage();
};
