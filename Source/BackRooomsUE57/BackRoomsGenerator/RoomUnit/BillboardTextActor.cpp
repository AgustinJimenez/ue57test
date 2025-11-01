#include "BillboardTextActor.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "TimerManager.h"

ABillboardTextActor::ABillboardTextActor()
{
	PrimaryActorTick.bCanEverTick = false; // We use timer instead of tick

	// Create text render component
	TextRenderComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TextRenderComponent"));
	RootComponent = TextRenderComponent;

	// Configure default text properties
	TextRenderComponent->SetText(FText::FromString(DisplayText));
	TextRenderComponent->SetWorldSize(TextSize);
	TextRenderComponent->SetTextRenderColor(TextColor);
	TextRenderComponent->SetHorizontalAlignment(EHTA_Center);
	TextRenderComponent->SetVerticalAlignment(EVRTA_TextCenter);
	TextRenderComponent->bAlwaysRenderAsText = true;
}

void ABillboardTextActor::BeginPlay()
{
	Super::BeginPlay();
	
	// Start the timer to update rotation every UpdateInterval seconds
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			UpdateTimer,
			this,
			&ABillboardTextActor::UpdateRotationTowardPlayer,
			UpdateInterval,
			true // Loop
		);
		
		// Do initial rotation update
		UpdateRotationTowardPlayer();
	}
}

void ABillboardTextActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Clean up timer
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(UpdateTimer);
	}
	
	Super::EndPlay(EndPlayReason);
}

void ABillboardTextActor::SetText(const FString& NewText)
{
	DisplayText = NewText;
	if (TextRenderComponent)
	{
		TextRenderComponent->SetText(FText::FromString(DisplayText));
	}
}

void ABillboardTextActor::SetTextSize(float NewSize)
{
	TextSize = NewSize;
	if (TextRenderComponent)
	{
		TextRenderComponent->SetWorldSize(TextSize);
	}
}

void ABillboardTextActor::SetTextColor(const FColor& NewColor)
{
	TextColor = NewColor;
	if (TextRenderComponent)
	{
		TextRenderComponent->SetTextRenderColor(TextColor);
	}
}

void ABillboardTextActor::UpdateRotationTowardPlayer()
{
	UWorld* World = GetWorld();
	if (!World) return;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC || !PC->GetPawn()) return;

	FVector PlayerLocation = PC->GetPawn()->GetActorLocation();
	FVector TextLocation = GetActorLocation();
	
	// Calculate direction from text to player
	FVector DirectionToPlayer = (PlayerLocation - TextLocation).GetSafeNormal();
	
	// Create rotation to face the player
	FRotator LookAtRotation = DirectionToPlayer.Rotation();
	LookAtRotation.Pitch = 0.0f; // Keep text upright
	
	// Apply the rotation
	SetActorRotation(LookAtRotation);
}