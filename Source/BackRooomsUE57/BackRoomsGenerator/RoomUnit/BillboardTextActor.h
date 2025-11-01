#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/TimerHandle.h"
#include "BillboardTextActor.generated.h"

UCLASS()
class ABillboardTextActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ABillboardTextActor();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Text render component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UTextRenderComponent* TextRenderComponent;

	// Timer for updating rotation
	FTimerHandle UpdateTimer;

	// Update interval in seconds
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billboard")
	float UpdateInterval = 2.0f;

	// Text properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
	FString DisplayText = TEXT("0");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
	float TextSize = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
	FColor TextColor = FColor::White;

public:
	// Set the text to display
	UFUNCTION(BlueprintCallable, Category = "Billboard")
	void SetText(const FString& NewText);

	// Set text size
	UFUNCTION(BlueprintCallable, Category = "Billboard")
	void SetTextSize(float NewSize);

	// Set text color
	UFUNCTION(BlueprintCallable, Category = "Billboard")
	void SetTextColor(const FColor& NewColor);

private:
	// Timer function to update rotation toward player
	UFUNCTION()
	void UpdateRotationTowardPlayer();
};