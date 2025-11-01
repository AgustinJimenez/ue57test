#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "../Types.h"
#include "BaseRoomUnit.generated.h"

UCLASS(BlueprintType, Abstract)
class UBaseRoomUnit : public UObject
{
	GENERATED_BODY()

public:
	UBaseRoomUnit();

	// Room dimensions in meters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room", meta = (ClampMin = "1.0", Units = "m"))
	float Width = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room", meta = (ClampMin = "1.0", Units = "m"))
	float Length = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room", meta = (ClampMin = "1.0", Units = "m"))
	float Height = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room", meta = (ClampMin = "0.1", Units = "m"))
	float WallThickness = 0.2f;

	// Position in the world
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
	FVector Position = FVector::ZeroVector;

	// Room category (for different rendering)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
	ERoomCategory RoomCategory = ERoomCategory::Room;

	// Elevation (vertical offset from ground level)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room", meta = (Units = "m"))
	float Elevation = 0.0f;

	// Door configurations for this room
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
	TArray<FDoorConfig> DoorConfigs;

	// Main interface methods
	virtual void CreateRoom(AActor* Owner);
	virtual void SetMaterial(UMaterialInterface* Material);

protected:
	UPROPERTY()
	UProceduralMeshComponent* MeshComponent;

	// Pure virtual methods for subclasses
	virtual void GenerateMesh() PURE_VIRTUAL(UBaseRoomUnit::GenerateMesh, );
	
	// Shared utility methods
	void InitializeMeshComponent(AActor* Owner);
	void ApplyMaterialToMesh(UMaterialInterface* Material);
	void SetupCollisionSettings();
};