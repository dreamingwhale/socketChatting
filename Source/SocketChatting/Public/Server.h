#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Networking/Public/Networking.h"
#include "Server.generated.h"

UCLASS()
class SOCKETCHATTING_API AServer : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AServer();
	~AServer();
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	FSocket* ServerSocket;
	FIPv4Endpoint ServerEndpoint;

public:
	UFUNCTION(BlueprintCallable, Category = "Socket")
	void ReceiveMessage();
};
