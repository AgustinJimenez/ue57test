#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "Types.h"
#include "GenerationConfig.h"
#include "Services/ICollisionDetectionService.h"
#include "Services/CollisionDetectionService.h"
#include "Services/IRoomConnectionManager.h"
#include "Services/RoomConnectionManager.h"
#include "Services/IGenerationOrchestrator.h"
#include "Services/GenerationOrchestrator.h"
#include "Main.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBackRoomGenerator, Log, All);

UCLASS()
class ABackRoomGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	ABackRoomGenerator();

protected:
	virtual void BeginPlay() override;

	// Configuration - Centralized settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	FBackroomGenerationConfig Config;

	// Materials
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	UMaterialInterface* WallMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	UMaterialInterface* FloorMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	UMaterialInterface* CeilingMaterial;

	// Generation function
	UFUNCTION(BlueprintCallable, Category = "Generation")
	void GenerateBackrooms();
	
	// Test mode generation function
	UFUNCTION(BlueprintCallable, Category = "Generation")
	void GenerateBackroomsInTestMode();

private:
	UPROPERTY()
	TArray<UStandardRoom*> RoomUnits;

	UPROPERTY()
	TArray<FRoomData> GeneratedRooms;

	// Services (Dependency Injection)
	TUniquePtr<ICollisionDetectionService> CollisionService;
	TUniquePtr<IRoomConnectionManager> ConnectionManager;
	TUniquePtr<IGenerationOrchestrator> GenerationOrchestrator;

	void DebugLog(const FString& Message) const;
	void CreateIdentifierSpheres(UStandardRoom* Room);
	void CreateRoomNumberIdentifier(const FRoomData& Room);
	void CreateConnectionInRoomWall(FRoomData& Room, EWallSide WallSide, EConnectionType ConnectionType, float ConnectionWidth);
	void CreateConnectionInRoomWallWithThickness(FRoomData& Room, EWallSide WallSide, EConnectionType ConnectionType, float ConnectionWidth, float WallThickness, float SmallerWallSize = -1.0f);
	void PrintRoomSizeSummary();
	
	// Helper for WallGenerator testing (moved to TestGenerator)
	
	// Boundary test function
	void TestSmallToLargeRoomConnection();
	
	// Procedural room generation functions
	void GenerateProceduralRooms();
	FRoomData CreateInitialRoom(const FVector& CharacterLocation);
	FRoomData GenerateRandomRoom(ERoomCategory Category, int32 RoomIndex);
	FRoomData GenerateRandomRoom(ERoomCategory Category, int32 RoomIndex, const FRoomData& SourceRoom, int32 ConnectionIndex);
	void RegenerateSpecificWall(FRoomData& Room, EWallSide WallSide, const FDoorConfig& DoorConfig);
};