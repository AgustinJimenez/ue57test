#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "Types.h"
#include "RoomUnit/StandardRoom.h"
#include "WallUnit/WallUnit.h"
#include "TestGenerator.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTestGenerator, Log, All);

UCLASS()
class ATestGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	ATestGenerator();

protected:
	virtual void BeginPlay() override;

	// Test mode generation function
	UFUNCTION(BlueprintCallable, Category = "Test Generation")
	void GenerateBackroomsInTestMode();

	// Materials
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	UMaterialInterface* WallMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	UMaterialInterface* FloorMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	UMaterialInterface* CeilingMaterial;

private:
	UPROPERTY()
	TArray<UStandardRoom*> RoomUnits;

	UPROPERTY()
	TArray<FRoomData> GeneratedRooms;

	void DebugLog(const FString& Message) const;
	
	// Helper function to create row labels
	void CreateRowLabel(const FVector& Position, const FString& LabelText);
	void CreateRowLabel(const FVector& Position, const FString& LabelText, float YawRotation);
	
	// Helper function to calculate consistent label position for any row
	FVector CalculateLabelPosition(float RowX, float RowY) const;
	
	// Helper function to create a single complete room
	void CreateSingleRoom(FVector RoomCenter, float Width, float Depth, float Height, 
		float WallThickness, FLinearColor WallColor, const FString& RoomName, 
		bool bSouthDoor, bool bNorthDoor, bool bEastDoor, bool bWestDoor, 
		float YawRotation = 0.0f, float PitchRotation = 0.0f);
	
	// WallUnit testing functions
	void GenerateWallTestGrid();
	
	// Individual row generation functions
	void CreateRowPitchRotationWalls();     // Row 1: Doorways with pitch rotations
	void CreateRowYawRotationWalls();       // Row 2: Solid walls with yaw rotations
	void CreateRowRollRotationWalls();      // Row 3: Solid walls with roll rotations
	void CreateRowDefaultSizingWalls();     // Row 4: Default positioning, different sizes
	void CreateRowCustomPositioningWalls(); // Row 5: Custom positioning, different positions
	
	// Legacy function (now calls CreateRowCustomPositioningWalls)
	void CreateMultipleHolesTestWalls();
	
	// NEW: Irregular hole demonstration functions
	void CreateRowIrregularHolesWalls();    // Row 6: Irregular holes with random shapes
	void CreateRowSpecificShapesWalls();    // Row 7: Specific geometric and organic shapes
	void CreateRowCompleteRoomAssembly();   // Row 8: Complete room using WallUnit components
	
	// StandardRoom demonstration row
	void CreateRowStandardRoomUnits();      // Row 9: Multiple StandardRoom units with different configurations
	
	
	// Modifiable units row for experimentation
	void CreateRowModifiableUnits();        // Row 11: Modifiable units for custom modifications
	
	// Boundary test function
	void TestSmallToLargeRoomConnection();  // Test boundary constraint for hole positioning
	
	// Stair test function
	void TestRoomToHallwayConnection();     // Test room connected to hallway
};