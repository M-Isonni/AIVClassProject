// Fill out your copyright notice in the Description page of Project Settings.


#include "Boss/Components/BossPhaseComponent.h"

// Sets default values for this component's properties
UBossPhaseComponent::UBossPhaseComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UBossPhaseComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;

	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UBossPhaseComponent::HandleTakeAnyDamage);
	}

	// ...
	
}


// Called every frame
void UBossPhaseComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UBossPhaseComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DmgType, AController* InstigatedBy, AActor* Causer)
{
	CurrentHealth = FMath::Clamp(CurrentHealth - Damage, 0.0f, MaxHealth);
	const float HealthPercentage = CurrentHealth / MaxHealth;

	if (HealthPercentage <= PhaseThreeThreshold && CurrentPhase < 3)
	{
		CurrentPhase = 3;
		OnPhaseThree.Broadcast();
	}
	else if (HealthPercentage <= PhaseTwoThreshold && CurrentPhase < 2)
	{
		CurrentPhase = 2;
		OnPhaseTwo.Broadcast();
	}
}

