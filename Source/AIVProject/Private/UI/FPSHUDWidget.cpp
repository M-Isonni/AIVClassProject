// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/FPSHUDWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "FPSCharacter.h"
#include "Blueprint/WidgetTree.h"

void UFPSHUDWidget::UpdateHealth(int32 CurrentHealth)
{
	if (HealthText)
	{
		HealthText->SetText(FText::FromString(FString::Printf(TEXT("Health: %d"),CurrentHealth)));
	}
}

void UFPSHUDWidget::UpdateAmmo(int32 CurrentAmmo, int32 MaxAmmo)
{
	if (AmmoText)
	{
		AmmoText->SetText(FText::FromString(FString::Printf(TEXT("Ammo: %d / %d"), CurrentAmmo, MaxAmmo)));
	}
}

void UFPSHUDWidget::ShowReloadBar(float Duration)
{
	if (ReloadBar)
	{
		AmmoText->SetVisibility(ESlateVisibility::Hidden);
		ReloadDuration = Duration;
		ReloadElapsedTime = 0.0f;
		ReloadBar->SetPercent(0.0f);

		ReloadBar->SetVisibility(ESlateVisibility::Visible);

		GetWorld()->GetTimerManager().SetTimer(
			ReloadTimerHandle,
			this,
			&UFPSHUDWidget::UpdateReloadBar,
			0.01f,
			true
		);
	}
}

void UFPSHUDWidget::PlayDamageFlash(float Duration)
{
	if (DamageFlashImage)
	{
		DamageFlashImage->SetVisibility(ESlateVisibility::Visible);
		FTimerHandle DamageTimerHandle;
		GetWorld()->GetTimerManager().SetTimer(DamageTimerHandle, FTimerDelegate::CreateLambda([this]()
			{
				DamageFlashImage->SetVisibility(ESlateVisibility::Hidden);
			}), Duration, false);
	}
}

void UFPSHUDWidget::UpdateMinimapIcons(const TArray<AActor*>& TrackedActors, UTexture2D* IconTexture)
{
	if (!MinimapActor || !MinimapCanvas || !IconTexture)
	{
		return;
	}

	MinimapCanvas->ClearChildren();

	FVector2D PlayerIconPosition(150, 150); // top be set with the algorithm

	UImage* PlayerIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	PlayerIcon->SetBrushFromTexture(IconTexture);

	constexpr float PlayerIconSize = 16.0f;

	UCanvasPanelSlot* PlayerIconSlot = MinimapCanvas->AddChildToCanvas(PlayerIcon);
	if (PlayerIconSlot)
	{
		PlayerIconSlot->SetPosition(PlayerIconPosition);
		PlayerIconSlot->SetSize(FVector2D(PlayerIconSize, PlayerIconSize));
		PlayerIconSlot->SetAutoSize(false);
	}


	FVector2D MinimapSize = MinimapCanvas->GetCachedGeometry().GetLocalSize();

	float OrthoWidth = MinimapActor->MinimapCaptureComponent->OrthoWidth;
	float Scale = MinimapSize.X / OrthoWidth;
	FVector CaptureCenter3D = MinimapActor->GetActorLocation();

	constexpr float IconSize = 16.0f;

	for (AActor* Actor : TrackedActors)
	{
		if (!Actor)
		{
			continue;
		}

		//Calculate 2d position from 3d actor position 
		//depending on image size and position (canvas size and position)

		FVector2D IconPosition(50,30); // top be set with the algorithm

		UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Icon->SetBrushFromTexture(IconTexture);

		UCanvasPanelSlot* IconSlot = MinimapCanvas->AddChildToCanvas(Icon);
		if (IconSlot)
		{
			IconSlot->SetPosition(IconPosition);
			IconSlot->SetSize(FVector2D(IconSize, IconSize));
			IconSlot->SetAutoSize(false);
		}
	}
}

void UFPSHUDWidget::UpdateReloadBar()
{
	ReloadElapsedTime += 0.01f;

	//Calculate percentage to fill
	float Percent = FMath::Clamp(ReloadElapsedTime / ReloadDuration, 0.0f, 1.0f);
	ReloadBar->SetPercent(Percent);

	if(ReloadElapsedTime>= ReloadDuration)
	{
		GetWorld()->GetTimerManager().ClearTimer(ReloadTimerHandle);
		ReloadBar->SetVisibility(ESlateVisibility::Hidden);
		AmmoText->SetVisibility(ESlateVisibility::Visible);
	}
}
