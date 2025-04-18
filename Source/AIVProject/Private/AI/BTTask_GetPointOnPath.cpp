// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTTask_GetPointOnPath.h"
#include "AIController.h"
#include "Enemies/FPSBehaviorTreeEnemy.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"

UBTTask_GetPointOnPath::UBTTask_GetPointOnPath()
{
	bCreateNodeInstance = true;
	NodeName = "Get Next Point on Path";
}

EBTNodeResult::Type UBTTask_GetPointOnPath::ExecuteTask(UBehaviorTreeComponent& OwnerComponent, uint8* NodeMemory)
{
	UBlackboardComponent* BBC = OwnerComponent.GetBlackboardComponent();
	AAIController* AIController = OwnerComponent.GetAIOwner();
	if (!BBC || !AIController)
	{
		return EBTNodeResult::Failed;
	}

	AFPSBehaviorTreeEnemy* ControlledEnemy = Cast<AFPSBehaviorTreeEnemy>(AIController->GetPawn());
	if (ControlledEnemy)
	{
		TArray<FVector> PatrolPoints = ControlledEnemy->EnemyPath->GetSplinePoints();
		BBC->SetValue<UBlackboardKeyType_Vector>(FName("TargetPatrolLocation"), PatrolPoints[PointIndex]);
		if (PointIndex < PatrolPoints.Num())
		{
			PointIndex++;
		}
		else
		{
			PointIndex = 0;
		}
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}
