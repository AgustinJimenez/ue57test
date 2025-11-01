#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "../Types.h"
#include "BaseRoom.generated.h"

// Forward declarations
class UStandardRoom;

UCLASS(BlueprintType)
class UBaseRoom : public UObject
{
	GENERATED_BODY()

public:
	UBaseRoom();

	// Room dimensions in meters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room", meta = (ClampMin = "1.0", Units = "m"))
	float Width = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room", meta = (ClampMin = "1.0", Units = "m"))
	float Length = 5.0f;

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
	
	// Static room size generation methods - Test Scale (reasonable sizes)
	static void GenerateRandomRoomSize(ERoomCategory Category, float& OutWidth, float& OutLength, FRandomStream& Random);
	static void GenerateStandardRoomSize(float& OutWidth, float& OutLength, FRandomStream& Random);
	static void GenerateHallwaySize(float& OutWidth, float& OutLength, FRandomStream& Random);
	
	// Static room size generation methods - Full Scale (massive backrooms)
	static void GenerateRandomRoomSizeFullScale(ERoomCategory Category, float& OutWidth, float& OutLength, FRandomStream& Random);
	static void GenerateStandardRoomSizeFullScale(float& OutWidth, float& OutLength, FRandomStream& Random);
	static void GenerateHallwaySizeFullScale(float& OutWidth, float& OutLength, FRandomStream& Random);
	
	// Comprehensive room generation methods - automatically assign all properties based on category
	static void InitializeRandomRoom(FRoomData& Room, ERoomCategory Category, int32 RoomIndex, FRandomStream& Random, bool bUseFullScale = false);
	static void InitializeRandomRoomWithElevation(FRoomData& Room, ERoomCategory Category, int32 RoomIndex, FRandomStream& Random, bool bUseFullScale = false);
	
	// Hole management
	virtual void AddHoleToWall(AActor* Owner, EWallSide WallSide, const FDoorConfig& DoorConfig);
	virtual void AddHoleToWallWithThickness(AActor* Owner, EWallSide WallSide, const FDoorConfig& DoorConfig, float CustomThickness, float SmallerWallSize = -1.0f, UStandardRoom* TargetRoom = nullptr);

	// Individual actor creation (shared)
	virtual void CreateRoomUsingIndividualActors(AActor* Owner);

protected:
	UPROPERTY()
	UProceduralMeshComponent* MeshComponent;


	// Shared utility methods
	void InitializeMeshComponent(AActor* Owner);
	void ApplyMaterialToMesh(UMaterialInterface* Material);
	void SetupCollisionSettings();
	
	bool ShouldRemoveWall(const FDoorConfig* DoorConfig) const;
	

	// Mesh generation for rooms
	virtual void GenerateMesh();
	
	// Floor generation (used by derived classes)
	virtual void GenerateFloorGeometry(TArray<FVector>& Vertices, TArray<int32>& Triangles, 
		TArray<FVector>& Normals, TArray<FVector2D>& UVs);

private:

	
	// Helper functions
	float MetersToUnrealUnits(float Meters) const { return Meters * 100.0f; }
};