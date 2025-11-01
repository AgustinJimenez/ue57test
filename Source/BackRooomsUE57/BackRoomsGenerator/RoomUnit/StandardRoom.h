#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "../Types.h"
#include "BaseRoom.h"
#include "StandardRoom.generated.h"

UCLASS(BlueprintType)
class UStandardRoom : public UBaseRoom
{
	GENERATED_BODY()

public:
	UStandardRoom();
	
	// Track wall actors for efficient updates (StandardRoom-specific)
	UPROPERTY()
	TMap<EWallSide, AActor*> WallActors;

	// StandardRoom-specific methods
	
	// Individual actor creation (same as test mode)
	virtual void CreateRoomUsingIndividualActors(AActor* Owner) override;
	
	// Hole management (StandardRoom-specific implementations)
	virtual void AddHoleToWall(AActor* Owner, EWallSide WallSide, const FDoorConfig& DoorConfig) override;
	virtual void AddHoleToWallWithThickness(AActor* Owner, EWallSide WallSide, const FDoorConfig& DoorConfig, float CustomThickness, float SmallerWallSize = -1.0f, UStandardRoom* TargetRoom = nullptr) override;
	
	// Unified room creation from RoomData with automatic numbering
	bool CreateFromRoomData(const FRoomData& RoomData, AActor* Owner, bool bShowNumbers = true);
	

protected:
	// StandardRoom-specific mesh generation (overrides base class)
	virtual void GenerateMesh() override;

private:
	// WallUnit-based mesh generation methods
	void GenerateWallGeometry(TArray<FVector>& CombinedVertices, TArray<int32>& CombinedTriangles,
		TArray<FVector>& CombinedNormals, TArray<FVector2D>& CombinedUVs, TArray<FLinearColor>& CombinedColors);
	void AddWallToMesh(const FVector& WallPosition, const FRotator& WallRotation, float WallWidth, float WallHeight,
		const FLinearColor& WallColor, const FWallHoleConfig* HoleConfig,
		TArray<FVector>& CombinedVertices, TArray<int32>& CombinedTriangles,
		TArray<FVector>& CombinedNormals, TArray<FVector2D>& CombinedUVs, TArray<FLinearColor>& CombinedColors);

	// Legacy methods (kept for compatibility)
	void GenerateFloor(TArray<FVector>& Vertices, TArray<int32>& Triangles, 
		TArray<FVector>& Normals, TArray<FVector2D>& UVs);
	void GenerateIndividualWalls(TArray<TArray<FVector>>& WallVertices, TArray<TArray<int32>>& WallTriangles, 
		TArray<TArray<FVector>>& WallNormals, TArray<TArray<FVector2D>>& WallUVs);

	// Utility methods
	EWallSide GetOppositeWall(EWallSide WallSide) const;
	void CreateRoomNumberText(int32 RoomIndex, bool bShowNumbers = true);
	void GenerateSimpleWall(EWallSide WallSide, float WidthCm, float LengthCm, float HeightCm,
		TArray<FVector>& Vertices, TArray<int32>& Triangles, 
		TArray<FVector>& Normals, TArray<FVector2D>& UVs);
	void GenerateWallWithHoles(EWallSide WallSide, int32 SectionIndex,
		TArray<FVector>& Vertices, TArray<int32>& Triangles,
		TArray<FVector>& Normals, TArray<FVector2D>& UVs);
	FVector2D CalculateUV(const FVector& Vertex, float ScaleFactor = 1.0f);
};