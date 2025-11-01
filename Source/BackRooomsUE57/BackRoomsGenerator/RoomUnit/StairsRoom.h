#pragma once

#include "CoreMinimal.h"
#include "StandardRoom.h"
#include "../Types.h"
#include "StairsRoom.generated.h"

UCLASS(BlueprintType)
class UStairsRoom : public UStandardRoom
{
	GENERATED_BODY()

public:
	UStairsRoom();

	// Stairs-specific properties
	
	// Direction the stairs ascend (North, South, East, West)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stairs")
	EWallSide StairDirection = EWallSide::North;
	
	// Number of steps in the staircase
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stairs", meta = (ClampMin = "3", ClampMax = "20"))
	int32 NumberOfSteps = 10;
	
	// Height of each individual step
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stairs", meta = (ClampMin = "0.1", ClampMax = "0.3", Units = "m"))
	float StepHeight = 0.18f; // Standard 18cm step height
	
	// Depth of each step (tread depth)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stairs", meta = (ClampMin = "0.2", ClampMax = "0.4", Units = "m"))
	float StepDepth = 0.25f; // Standard 25cm step depth
	
	// Width of the staircase (perpendicular to direction)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stairs", meta = (ClampMin = "0.8", ClampMax = "3.0", Units = "m"))
	float StairWidth = 1.2f; // Standard 1.2m stair width
	
	// Whether to include railings on the sides
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stairs")
	bool bIncludeRailings = true;
	
	// Height of railings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stairs", meta = (ClampMin = "0.8", ClampMax = "1.2", Units = "m", EditCondition = "bIncludeRailings"))
	float RailingHeight = 0.9f; // Standard 90cm railing height

	// Stairs-specific methods
	
	// Override room creation to include stair geometry
	virtual void CreateRoomUsingIndividualActors(AActor* Owner) override;
	
	// Create stairs from RoomData with automatic stair configuration
	bool CreateStairsFromRoomData(const FRoomData& RoomData, AActor* Owner, bool bShowNumbers = true);
	
	// Calculate total stair elevation based on step count and height
	float CalculateTotalStairHeight() const;
	
	// Calculate total stair length based on step count and depth
	float CalculateTotalStairLength() const;
	
	// Get the position of the top of the stairs (for connecting upper level rooms)
	FVector GetStairTopPosition() const;
	
	// Get the position of the bottom of the stairs (for connecting lower level rooms)
	FVector GetStairBottomPosition() const;
	
	// Get accurate collision bounds for stairs (accounts for actual stair geometry)
	FBox GetStairCollisionBounds() const;

protected:
	// Override mesh generation to include stair-specific geometry
	virtual void GenerateMesh() override;

private:
	// Stair-specific mesh generation methods
	void GenerateStairSteps(TArray<FVector>& Vertices, TArray<int32>& Triangles, 
		TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FLinearColor>& Colors);
	
	void GenerateStairRailings(TArray<FVector>& Vertices, TArray<int32>& Triangles,
		TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FLinearColor>& Colors);
	
	void GenerateStairFoundation(TArray<FVector>& Vertices, TArray<int32>& Triangles,
		TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FLinearColor>& Colors);
	
	// Helper methods for stair geometry calculation
	FVector CalculateStepPosition(int32 StepIndex) const;
	FRotator GetStairDirectionRotation() const;
	void GetStairBounds(FVector& MinBounds, FVector& MaxBounds) const;
	
	// Utility methods
	FVector2D CalculateStairUV(const FVector& Vertex, float ScaleFactor = 1.0f);
	void AddStepToMesh(int32 StepIndex, const FVector& StepPosition, 
		TArray<FVector>& Vertices, TArray<int32>& Triangles,
		TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FLinearColor>& Colors);
	void UpdateRoomDimensionsForStairs();
	void CreateRoomNumberText(int32 RoomIndex, bool bShowNumbers = true);
	float MetersToUnrealUnits(float Meters) const;
};