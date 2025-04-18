// Fill out your copyright notice in the Description page of Project Settings.


#include "FPSCharacter.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "FPSInteractable.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Kismet/GameplayStatics.h"
#include "Data/DataManager.h"
#include "UI/FPSHUDWidget.h"
#include "Enemies/EnemiesManagerSubsystem.h"

// Sets default values
AFPSCharacter::AFPSCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//CreatingCameraComponent
	FPSCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FPSCamera"));
	FPSCamera->SetupAttachment(RootComponent);
	FPSCamera->SetRelativeLocation(FVector(0, 0, 0));
	FPSCamera->bUsePawnControlRotation = true;

	//create stimuli source
	PerceptionStimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("StimuliSourceComponent"));

	//register senses
	PerceptionStimuliSource->RegisterForSense(TSubclassOf<UAISense_Hearing>());
	PerceptionStimuliSource->RegisterForSense(TSubclassOf<UAISense_Sight>());
	PerceptionStimuliSource->RegisterWithPerceptionSystem();

	TeamID = FGenericTeamId(2);


	MinimapCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("MinimapCaptureComponent"));
	MinimapCaptureComponent->SetupAttachment(RootComponent);
	MinimapCaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
	MinimapCaptureComponent->OrthoWidth = 4096.0f;
	MinimapCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	MinimapCaptureComponent->bCaptureEveryFrame = true;

	//MinimapCaptureComponent->bCaptureEveryFrame = false;
	//MinimapCaptureComponent->CaptureScene();

	MinimapCaptureComponent->TextureTarget = MinimapRenderTarget;
}

// Called when the game starts or when spawned
void AFPSCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	LoadGame();

	if (HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UFPSHUDWidget>(GetWorld(), HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport();
			HUDWidget->UpdateHealth(CurrentHealth);
			HUDWidget->UpdateAmmo(CurrentAmmo,MaxAmmo);

			MinimapCaptureComponent->TextureTarget = MinimapRenderTarget;
			HUDWidget->MinimapActor = this;

			UEnemiesManagerSubsystem* EnemiesManager = GetWorld()->GetSubsystem<UEnemiesManagerSubsystem>();			

			MinimapTrackedActors.Append(EnemiesManager->GetActiveEnemies());
			MinimapCaptureComponent->HiddenActors.Append(MinimapTrackedActors);
			MinimapCaptureComponent->HiddenActors.Add(this);

			//TODO: Add new tracked enemies whenever new enemies are spawn and hide them from scene capture component.
			//Add a function that does it bind it to the spawning event

			HUDWidget->UpdateMinimapIcons(MinimapTrackedActors, MinimapIconTexture);
		}
	}
}

void AFPSCharacter::MakeSomeNoise(float Loudness, AActor* NoiseInstigator, FVector Location, USoundBase* Sound)
{
	UAISense_Hearing::ReportNoiseEvent(GetWorld(), Location, Loudness, NoiseInstigator);

	//this play is not perceived by ai but is played for the player
	UGameplayStatics::PlaySoundAtLocation(GetWorld(), Sound, Location);
}

// Called every frame
void AFPSCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if(HUDWidget)
	{
		//To optimize we can update it one every 2 or three frames
		HUDWidget->UpdateMinimapIcons(MinimapTrackedActors, MinimapIconTexture);
	}

}

// Called to bind functionality to input
void AFPSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	//float MyVariable = 0;
	//FInputAxisBinding& MoveForwardBinding = PlayerInputComponent->BindAxis("MoveForward");
	//MoveForwardBinding.AxisDelegate.GetDelegateForManualSet().BindLambda([this, &MyVariable](float Value) {
	//	
	//	AddMovementInput(GetActorForwardVector(), Value);
	//	});


	PlayerInputComponent->BindAxis("MoveForward", this, &AFPSCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AFPSCharacter::MoveRight);
	PlayerInputComponent->BindAxis("LookUp", this, &AFPSCharacter::LookUp);
	PlayerInputComponent->BindAxis("Turn", this, &AFPSCharacter::Turn);

	PlayerInputComponent->BindAction("Interact", EInputEvent::IE_Pressed, this, &AFPSCharacter::Interact);
	PlayerInputComponent->BindAction("Shoot", EInputEvent::IE_Pressed, this, &AFPSCharacter::Shoot);

	PlayerInputComponent->BindAction("SaveGame", EInputEvent::IE_Pressed, this, &AFPSCharacter::SaveGame);
	PlayerInputComponent->BindAction("LoadGame", EInputEvent::IE_Pressed, this, &AFPSCharacter::LoadGame);
}

#pragma region Movement
void AFPSCharacter::MoveForward(float Value)
{
	AddMovementInput(GetActorForwardVector(), Value);
}

void AFPSCharacter::MoveRight(float Value)
{
	AddMovementInput(GetActorRightVector(), Value);
}

void AFPSCharacter::LookUp(float Value)
{
	float PitchValue = Value;
	if (InvertYAxis)
	{
		PitchValue*=-1;
	}
	AddControllerPitchInput(PitchValue);
}

void AFPSCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}
#pragma endregion

#pragma region Actions
void AFPSCharacter::Interact()
{
	TArray<FHitResult> HitResults;

	if (FPSCamera)
	{
		FVector Start = FPSCamera->GetComponentLocation();


		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);

		UWorld* CurrentWorld = GetWorld();


		bool bSuccess = CurrentWorld->SweepMultiByChannel(HitResults, Start, Start, FQuat::Identity, ECollisionChannel::ECC_GameTraceChannel1, FCollisionShape::MakeSphere(InteractRadius), QueryParams);
#if WITH_EDITOR
		DrawDebugSphere(CurrentWorld, Start, InteractRadius, 100, FColor::Red, false, 0.2, 0, 1);
#endif
		if (bSuccess)
		{
			for (FHitResult Result : HitResults)
			{
				AActor* HitActor = Result.GetActor();

				if (HitActor->Implements<UFPSInteractable>())
				{
					FVector Forward = FPSCamera->GetForwardVector();

					FVector ObjDir = HitActor->GetActorLocation() - FPSCamera->GetComponentLocation();
					ObjDir.Normalize();

					float DotProduct = FVector::DotProduct(Forward, ObjDir);

					//DoProduct = 1 when both are parallel and facing the same direction
					//DotProduct = 0 when vectors are perpendicular
					//DotProduct = -1 when vectors are parrallel but in opposite directions

					if (DotProduct >= InteractThreshold)
					{
						FHitResult HitResult;
						FVector End = Start + ObjDir * InteractRadius;
						bool bHasHitSomething = CurrentWorld->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams);
						if (bHasHitSomething && HitResult.GetActor() != HitActor)
						{
							return;
						}

						IFPSInteractable* InterfaceInstance = Cast<IFPSInteractable>(HitActor);
						if (InterfaceInstance)
						{
							InterfaceInstance->TriggerInteraction(this);
							return;
						}

					}
				}
			}
		}	
	}
}

void AFPSCharacter::Shoot()
{
	if (CurrentAmmo > 0)
	{
		CurrentAmmo--;
		if (HUDWidget)
		{
			HUDWidget->UpdateAmmo(CurrentAmmo, MaxAmmo);
		}
		//USoundBase* Sound;
		FSoftObjectPath SoundAssetPath(TEXT("/Game/FPWeapon/Audio/FirstPersonTemplateWeaponFire02.FirstPersonTemplateWeaponFire02"));
		UObject* LoadedObj = SoundAssetPath.TryLoad();
		if (LoadedObj)
		{
			USoundBase* Sound = Cast<USoundBase>(LoadedObj);
			if (Sound)
			{
				MakeSomeNoise(1.0, this, GetActorLocation(), Sound);
			}
		}
		if (bAimAssist)
		{
			TArray<FHitResult> HitResults;
			FVector Start = FPSCamera->GetComponentLocation();
			//FVector End = Start + (FPSCamera->GetForwardVector() * Distance);

			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(this);

			UWorld* CurrentWorld = GetWorld();


			//Instead of using SphereTrace
			//Use list of enemies and cycle it
			//Get Enemies manager subsystem
			//Access the Active enemies list

			bool bSuccess = CurrentWorld->SweepMultiByChannel(HitResults, Start, Start, FQuat::Identity, ECollisionChannel::ECC_GameTraceChannel2, FCollisionShape::MakeSphere(ShootDistance), QueryParams);

#if WITH_EDITOR
			DrawDebugSphere(CurrentWorld, Start, ShootDistance, 100, FColor::Red, false, 0.2, 0, 1);
#endif
			if (bSuccess)
			{
				for (FHitResult Result : HitResults)
				{
					AActor* HitActor = Result.GetActor();

					if (HitActor->Implements<UFPSInteractable>())
					{
						FVector Forward = FPSCamera->GetForwardVector();

						FVector EnemyDir = HitActor->GetActorLocation() - FPSCamera->GetComponentLocation();
						EnemyDir.Normalize();

						float DotProduct = FVector::DotProduct(Forward, EnemyDir);

						//DoProduct = 1 when both are parallel and facing the same direction
						//DotProduct = 0 when vectors are perpendicular
						//DotProduct = -1 when vectors are parrallel but in opposite directions

						if (DotProduct >= InteractThreshold)
						{
							FHitResult HitResult;
							FVector End = Start + EnemyDir * ShootDistance;
							bool bHasHitSomething = CurrentWorld->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams);
							if (bHasHitSomething && HitResult.GetActor() != HitActor)
							{
								return;
							}

							IFPSInteractable* InterfaceInstance = Cast<IFPSInteractable>(HitActor);
							if (InterfaceInstance)
							{
								InterfaceInstance->TriggerHit(this);
								return;
							}

							//Works only for BlueprintNative, BlueprintCallable or BlueprintImplementableEvent
							if (HitActor && HitActor->Implements<UFPSInteractable>())
							{
								IFPSInteractable::Execute_TriggerBlueprintHit(HitActor, this);
							}
						}
					}
				}
			}
		}
		else
		{
			FHitResult Hit;

			//UCameraComponent* FPSCameraComponent = FindComponentByClass<UCameraComponent>();
			if (FPSCamera)
			{
				FVector Start = FPSCamera->GetComponentLocation();

				FVector End = Start + (FPSCamera->GetForwardVector() * ShootDistance);

				FCollisionQueryParams QueryParams;
				QueryParams.AddIgnoredActor(this);

				UWorld* CurrentWorld = GetWorld();
				if (CurrentWorld)
				{
					bool bHasHitSomething = CurrentWorld->LineTraceSingleByChannel(Hit, Start, End, ECC_GameTraceChannel2, QueryParams);

#if WITH_EDITOR
					DrawDebugLine(CurrentWorld, Start, End, FColor::Green, false, 0.2f, 0, 1.0);
#endif

					if (bHasHitSomething)
					{
						AActor* InteractedActor = Hit.GetActor();
						if (InteractedActor)
						{
							UE_LOG(LogTemp, Warning, TEXT("Shot: %s"), *InteractedActor->GetName());

							//check if actor has implemented interface
							//call interface function
							IFPSInteractable* InterfaceInstance = Cast<IFPSInteractable>(InteractedActor);
							if (InterfaceInstance)
							{
								InterfaceInstance->TriggerHit(this);
							}

							//Works only for BlueprintNative, BlueprintCallable or BlueprintImplementableEvent
							if (InteractedActor && InteractedActor->Implements<UFPSInteractable>())
							{
								IFPSInteractable::Execute_TriggerBlueprintHit(InteractedActor, this);
							}
						}
					}
				}
			}
		}
	}
	else
	{
		if (!GetWorld()->GetTimerManager().IsTimerActive(ReloadTimerHandle))
		{
			if (HUDWidget)
			{
				HUDWidget->ShowReloadBar(ReloadDuration);
			}	

			GetWorld()->GetTimerManager().SetTimer(ReloadTimerHandle, FTimerDelegate::CreateLambda([this] ()
				{			
					CurrentAmmo = MaxAmmo;
					if (HUDWidget)
					{
						HUDWidget->UpdateAmmo(CurrentAmmo, MaxAmmo);
					}
				}), ReloadDuration, false);
		}
	}
	
}

//void AFPSCharacter::ReloadAmmo()
//{
//	CurrentAmmo = MaxAmmo;
//	if (HUDWidget)
//	{
//		HUDWidget->UpdateAmmo(CurrentAmmo, MaxAmmo);
//	}
//}

void AFPSCharacter::LaunchPebble(float Force)
{	
}

void AFPSCharacter::TakeDamage(int32 IncomingDamage)
{
	CurrentHealth = FMath::Clamp(CurrentHealth - IncomingDamage, 0, MaxHealth);
	if (HUDWidget)
	{
		HUDWidget->UpdateHealth(CurrentHealth);
		HUDWidget->PlayDamageFlash(0.2);
	}

	if (CurrentHealth <= 0)
	{
		//TODO: invoke OnDeath event (to be implemented)
	}
}

void AFPSCharacter::SaveGame()
{
	UDataManager* DataManager = GetGameInstance()->GetSubsystem<UDataManager>();

	//Bring Up UI
	// through UI selection we can set SlotName and UserID

	DataManager->SaveGame(SaveSlotName, SaveUserID);
}
void AFPSCharacter::LoadGame()
{
	UDataManager* DataManager = GetGameInstance()->GetSubsystem<UDataManager>();

	//Bring Up UI
	// through UI selection we can set SlotName and UserID

	DataManager->LoadGame(SaveSlotName, SaveUserID);
}
#pragma endregion
