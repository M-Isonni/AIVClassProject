// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BossAIController.generated.h"

/**
 * 
 */
UCLASS()
class AIVPROJECT_API ABossAIController : public AAIController
{
	GENERATED_BODY()
public:
	ABossAIController();

	virtual void OnPossess(APawn* InPawn) override;

protected:
	UPROPERTY()
	class UBehaviorTreeComponent* BTC;

	UPROPERTY()
	class UBlackboardComponent* BBC;
	
};
