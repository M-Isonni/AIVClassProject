// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_BlackboardBase.h"
#include "BTService_UpdateAmmo.generated.h"

/**
 * 
 */
UCLASS()
class AIVPROJECT_API UBTService_UpdateAmmo : public UBTService_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTService_UpdateAmmo();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blackboard")
	FBlackboardKeySelector CurrentAmmoKey;
	
};
