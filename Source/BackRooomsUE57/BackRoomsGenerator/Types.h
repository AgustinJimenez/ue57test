#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Types.generated.h"

// Forward declarations
class UStandardRoom;

// Wall side enumeration for door configuration
UENUM(BlueprintType)
enum class EWallSide : uint8
{
	None = 0 UMETA(DisplayName = "None"),
	North = 1 UMETA(DisplayName = "North"),
	South = 2 UMETA(DisplayName = "South"),
	East = 3 UMETA(DisplayName = "East"),
	West = 4 UMETA(DisplayName = "West")
};

// Hole shape type enumeration
UENUM(BlueprintType)
enum class EHoleShape : uint8
{
	Rectangle = 0 UMETA(DisplayName = "Rectangle"),
	Circle = 1 UMETA(DisplayName = "Circle"),
	Irregular = 2 UMETA(DisplayName = "Irregular")
};

// Door/hole configuration struct
USTRUCT(BlueprintType)
struct FDoorConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	bool bHasDoor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	EWallSide WallSide = EWallSide::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	EHoleShape HoleShape = EHoleShape::Rectangle;

	// Rectangle hole parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (ClampMin = "0.1", Units = "m", EditCondition = "HoleShape == EHoleShape::Rectangle"))
	float Width = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (ClampMin = "0.1", Units = "m", EditCondition = "HoleShape == EHoleShape::Rectangle"))
	float Height = 2.0f;

	// Circle hole parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (ClampMin = "0.1", Units = "m", EditCondition = "HoleShape == EHoleShape::Circle"))
	float Radius = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (ClampMin = "4", ClampMax = "32", EditCondition = "HoleShape == EHoleShape::Circle"))
	int32 CircleSegments = 16;

	// Irregular hole parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (ClampMin = "0.1", Units = "m", EditCondition = "HoleShape == EHoleShape::Irregular"))
	float IrregularSize = 0.8f; // Base size for irregular shape

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (ClampMin = "0.1", ClampMax = "1.0", EditCondition = "HoleShape == EHoleShape::Irregular"))
	float Irregularity = 0.5f; // How irregular (0=circle, 1=very chaotic)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (ClampMin = "5", ClampMax = "20", EditCondition = "HoleShape == EHoleShape::Irregular"))
	int32 IrregularPoints = 8; // Number of random points

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (ClampMin = "1", EditCondition = "HoleShape == EHoleShape::Irregular"))
	int32 RandomSeed = 12345; // Seed for reproducible randomness

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "HoleShape == EHoleShape::Irregular"))
	float IrregularSmoothness = 0.0f; // How smooth the edges are (0=jagged, 1=very smooth/rounded)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (ClampMin = "0.0", ClampMax = "360.0", Units = "deg", EditCondition = "HoleShape == EHoleShape::Irregular"))
	float IrregularRotation = 0.0f; // Rotation angle in degrees (0-360)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (ClampMin = "0.0", Units = "m"))
	float OffsetFromCenter = 0.0f;
};

// Advanced hole configuration for WallUnit system
USTRUCT(BlueprintType)
struct FWallHoleConfig
{
	GENERATED_BODY()

	// Basic hole properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hole")
	float Width = 0.8f; // Hole width in meters

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hole")
	float Height = 2.0f; // Hole height in meters

	// === POSITIONING SYSTEM ===
	// Type-based positioning system for intuitive usage

	// Positioning type determines behavior
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Position")
	FString PositionType = TEXT("default"); // "default", "custom", "normalized"

	// CARTESIAN COORDINATES (in meters from bottom-left corner)
	// Used when PositionType = "custom"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Position", meta = (EditCondition = "PositionType == \"custom\"", Units = "m"))
	float X = 0.0f; // Distance from left edge in meters (0.0 = left edge)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Position", meta = (EditCondition = "PositionType == \"custom\"", Units = "m"))
	float Y = 0.0f; // Distance from bottom edge in meters (0.0 = bottom edge)

	// NORMALIZED COORDINATES (0.0-1.0) for advanced users
	// Used when PositionType = "normalized"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Position", meta = (EditCondition = "PositionType == \"normalized\"", ClampMin = "0.0", ClampMax = "1.0"))
	float HorizontalPosition = 0.5f; // 0.0=left edge, 0.5=center, 1.0=right edge

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Position", meta = (EditCondition = "PositionType == \"normalized\"", ClampMin = "0.0", ClampMax = "1.0"))
	float VerticalPosition = 0.0f; // 0.0=bottom-aligned, 0.5=center, 1.0=top-aligned

	// Hole shape and style
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape")
	EHoleShape Shape = EHoleShape::Rectangle;

	// Optional rotation for aesthetic variation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape", meta = (ClampMin = "0.0", ClampMax = "360.0", Units = "deg"))
	float Rotation = 0.0f;

	// Optional identifier for debugging
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	FString HoleName = TEXT("Door");

	// === CONSTRUCTORS ===

	// Default constructor - creates standard doorway (0.8m × 2.0m, centered, bottom-aligned)
	FWallHoleConfig()
	{
		Width = 0.8f;
		Height = 2.0f;
		PositionType = TEXT("default");
		HorizontalPosition = 0.5f; // Not used in default mode
		VerticalPosition = 0.0f;   // Not used in default mode
		X = 0.0f;
		Y = 0.0f;
		Shape = EHoleShape::Rectangle;
		Rotation = 0.0f;
		HoleName = TEXT("Door");
	}

	// Size-only constructor - "default" type with custom size
	FWallHoleConfig(float InWidth, float InHeight, const FString& InName = TEXT("CustomDoor"))
	{
		Width = InWidth;
		Height = InHeight;
		PositionType = TEXT("default");
		HorizontalPosition = 0.5f; // Not used in default mode
		VerticalPosition = 0.0f;   // Not used in default mode
		X = 0.0f;
		Y = 0.0f;
		Shape = EHoleShape::Rectangle;
		Rotation = 0.0f;
		HoleName = InName;
	}

	// Custom positioning constructor - explicit "custom" type with cartesian coordinates (0,0 = bottom-left)
	static FWallHoleConfig CreateCustom(float InWidth, float InHeight, float InX, float InY, const FString& InName = TEXT("CustomHole"))
	{
		FWallHoleConfig Config;
		Config.Width = InWidth;
		Config.Height = InHeight;
		Config.PositionType = TEXT("custom");
		Config.HorizontalPosition = 0.5f; // Not used in custom mode
		Config.VerticalPosition = 0.0f;   // Not used in custom mode
		Config.X = InX;
		Config.Y = InY;
		Config.Shape = EHoleShape::Rectangle;
		Config.Rotation = 0.0f;
		Config.HoleName = InName;
		return Config;
	}

	// Normalized positioning constructor - "normalized" type with 0.0-1.0 coordinates
	FWallHoleConfig(float InWidth, float InHeight, float InHorizontalPos, float InVerticalPos, const FString& InType, const FString& InName = TEXT("NormalizedHole"))
	{
		Width = InWidth;
		Height = InHeight;
		PositionType = InType; // Should be "normalized"
		HorizontalPosition = FMath::Clamp(InHorizontalPos, 0.0f, 1.0f);
		VerticalPosition = FMath::Clamp(InVerticalPos, 0.0f, 1.0f);
		X = 0.0f;
		Y = 0.0f;
		Shape = EHoleShape::Rectangle;
		Rotation = 0.0f;
		HoleName = InName;
	}

	// Helper function to get final position in normalized coordinates
	void GetNormalizedPosition(float WallWidth, float WallHeight, float& OutHorizontal, float& OutVertical) const
	{
		if (PositionType == TEXT("default"))
		{
			// Default doorway: centered horizontally, bottom-aligned
			OutHorizontal = 0.5f; // Center
			OutVertical = (WallHeight > 0.0f) ? (Height * 0.5f) / WallHeight : 0.3f; // Bottom-aligned
		}
		else if (PositionType == TEXT("custom"))
		{
			// Convert cartesian to normalized
			// X,Y represent the CENTER of the hole
			OutHorizontal = (WallWidth > 0.0f) ? X / WallWidth : 0.5f;
			OutVertical = (WallHeight > 0.0f) ? Y / WallHeight : 0.5f;
		}
		else // "normalized" or any other type
		{
			// Use normalized coordinates directly
			OutHorizontal = HorizontalPosition;
			OutVertical = VerticalPosition;
		}
	}
};

// Utility functions (moved before structs that use them)
inline float MetersToUnrealUnits(float Meters)
{
	return Meters * 100.0f; // Convert meters to centimeters (Unreal units)
}

inline float UnrealUnitsToMeters(float UnrealUnits)
{
	return UnrealUnits / 100.0f; // Convert centimeters to meters
}

// Room category enumeration
UENUM(BlueprintType)
enum class ERoomCategory : uint8
{
	Room = 0 UMETA(DisplayName = "Room"),        // Square-ish rooms (2x2 to 50x50m)
	Hallway = 1 UMETA(DisplayName = "Hallway"),  // Rectangular hallways (1x3 to 4x100m)
	Stairs = 2 UMETA(DisplayName = "Stairs")     // Stair units (4-10m with vertical elevation)
};

// Connection type enumeration
UENUM(BlueprintType)
enum class EConnectionType : uint8
{
	Doorway = 0 UMETA(DisplayName = "Doorway"),  // Standard door opening (0.8m wide)
	Opening = 1 UMETA(DisplayName = "Opening")   // Larger opening (up to smallest wall width)
};

// Room connection data structure
USTRUCT(BlueprintType)
struct FRoomConnection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection")
	EWallSide WallSide = EWallSide::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection")
	bool bIsUsed = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection")
	FVector ConnectionPoint = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection")
	float ConnectionWidth = 0.8f; // Default door width

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection")
	EConnectionType ConnectionType = EConnectionType::Doorway;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection")
	int32 ConnectedRoomIndex = -1; // Index of connected room (-1 = no connection)
};

// Room generation data structure
USTRUCT(BlueprintType)
struct FRoomData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
	UStandardRoom* RoomUnit = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
	ERoomCategory Category = ERoomCategory::Room;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
	FVector Position = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
	float Width = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
	float Length = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
	float Height = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
	float Elevation = 0.0f; // Vertical offset from ground level (for stairs/multi-level)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
	EWallSide StairDirection = EWallSide::None; // For stairs: which direction they ascend (None for non-stairs)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
	TArray<FRoomConnection> Connections;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
	int32 RoomIndex = -1;

	// Helper function to get bounding box for collision detection (includes wall thickness)
	FBox GetBoundingBox() const
	{
		// Include wall thickness in bounding box calculation
		float WallThickness = 0.2f; // 20cm wall thickness
		float WallThicknessCm = MetersToUnrealUnits(WallThickness);
		
		// Calculate vertical bounds based on room category
		float MinZ, MaxZ;
		
		if (Category == ERoomCategory::Stairs)
		{
			// Stairs: Base starts at Position.Z (ground level), extends up by Height + Elevation
			MinZ = Position.Z; // Base at room floor level (not elevated)
			MaxZ = Position.Z + MetersToUnrealUnits(Height + Elevation); // Stair top height
		}
		else
		{
			// Regular rooms/hallways: Standard bounds at elevation
			FVector ElevatedPosition = Position + FVector(0, 0, MetersToUnrealUnits(Elevation));
			MinZ = ElevatedPosition.Z;
			MaxZ = ElevatedPosition.Z + MetersToUnrealUnits(Height);
		}
		
		// Horizontal bounds (same for all room types)
		// Use absolute Z values for proper vertical separation between levels
		FVector Min = FVector(
			Position.X - WallThicknessCm, 
			Position.Y - WallThicknessCm, 
			MinZ
		);
		FVector Max = FVector(
			Position.X + MetersToUnrealUnits(Width) + WallThicknessCm,
			Position.Y + MetersToUnrealUnits(Length) + WallThicknessCm,
			MaxZ
		);
		return FBox(Min, Max);
	}

	// Helper function to get available connections
	TArray<int32> GetAvailableConnections() const
	{
		TArray<int32> Available;
		for (int32 i = 0; i < Connections.Num(); i++)
		{
			if (!Connections[i].bIsUsed)
			{
				Available.Add(i);
			}
		}
		return Available;
	}

	// Helper function to calculate world position of a connection point
	FVector GetConnectionWorldPosition(EWallSide WallSide) const
	{
		FVector RoomCenter = Position + FVector(
			MetersToUnrealUnits(Width) * 0.5f,
			MetersToUnrealUnits(Length) * 0.5f,
			MetersToUnrealUnits(Elevation) // Floor level at elevation
		);

		switch (WallSide)
		{
			case EWallSide::North:
				return FVector(
					RoomCenter.X, // Center X
					Position.Y + MetersToUnrealUnits(Length), // North wall Y position
					RoomCenter.Z // Floor level
				);
			case EWallSide::South:
				return FVector(
					RoomCenter.X, // Center X
					Position.Y, // South wall Y position
					RoomCenter.Z // Floor level
				);
			case EWallSide::East:
				return FVector(
					Position.X + MetersToUnrealUnits(Width), // East wall X position
					RoomCenter.Y, // Center Y
					RoomCenter.Z // Floor level
				);
			case EWallSide::West:
				return FVector(
					Position.X, // West wall X position
					RoomCenter.Y, // Center Y
					RoomCenter.Z // Floor level
				);
			default:
				return RoomCenter; // Fallback to room center
		}
	}
};

// Backrooms Level 0 module types
UENUM(BlueprintType)
enum class EBackroomModuleType : uint8
{
	Corridor = 0 UMETA(DisplayName = "Corridor"),      // 0-300
	Common = 1 UMETA(DisplayName = "Common"),          // 301-700
	Office = 2 UMETA(DisplayName = "Office"),          // 701-900
	Stairs = 3 UMETA(DisplayName = "Stairs")           // 901-1000
};

