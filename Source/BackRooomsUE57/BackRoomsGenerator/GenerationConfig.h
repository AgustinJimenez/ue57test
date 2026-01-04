#pragma once

#include "CoreMinimal.h"
#include "GenerationConfig.generated.h"
#include "Types.h"

/**
 * Configuration settings for backrooms generation
 * Centralizes all magic numbers and tunable parameters
 */
USTRUCT(BlueprintType)
struct FBackroomGenerationConfig
{
	GENERATED_BODY()

	// === ROOM DISTRIBUTION SETTINGS ===

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "1", ClampMax = "1000"))
	int32 TotalRooms = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RoomRatio = 0.0f; // 0% standard rooms

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HallwayRatio = 0.0f; // 0% hallways

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StairRatio = 1.0f; // 100% stairs

	// === SAFETY LIMITS ===

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Safety", meta = (ClampMin = "1", ClampMax = "20"))
	int32 MaxAttemptsPerConnection = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Safety", meta = (ClampMin = "1", ClampMax = "50"))
	int32 MaxConnectionRetries = 10;

	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Safety",
	          meta = (ClampMin = "5.0", ClampMax = "120.0", Units = "s"))
	float MaxGenerationTime = 20.0f; // seconds

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Safety", meta = (ClampMin = "100", ClampMax = "10000"))
	int32 MaxSafetyIterations = 2000;

	// === ROOM DIMENSIONS ===

	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Dimensions",
	          meta = (ClampMin = "1.0", ClampMax = "10.0", Units = "m"))
	float StandardRoomHeight = 3.0f;

	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Dimensions",
	          meta = (ClampMin = "0.1", ClampMax = "1.0", Units = "m"))
	float WallThickness = 0.2f; // 20cm walls

	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Dimensions",
	          meta = (ClampMin = "0.01", ClampMax = "0.5", Units = "m"))
	float CollisionBuffer = 0.02f; // 2cm buffer for collision detection

	// === CONNECTION SETTINGS ===

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connections", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DoorwayConnectionRatio = 0.30f; // 30% doorways, 70% openings

	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Connections",
	          meta = (ClampMin = "0.5", ClampMax = "1.2", Units = "m"))
	float StandardDoorwayWidth = 0.8f;

	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Connections",
	          meta = (ClampMin = "1.8", ClampMax = "2.5", Units = "m"))
	float StandardDoorwayHeight = 2.0f;

	// === ROOM SIZE RANGES ===

	// Standard Rooms (square-ish)
	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Room Sizes",
	          meta = (ClampMin = "1.0", ClampMax = "10.0", Units = "m"))
	float MinRoomSize = 2.0f;

	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Room Sizes",
	          meta = (ClampMin = "2.0", ClampMax = "20.0", Units = "m"))
	float MaxRoomSize = 15.0f;

	// Hallways (rectangular)
	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Room Sizes",
	          meta = (ClampMin = "1.0", ClampMax = "5.0", Units = "m"))
	float MinHallwayWidth = 2.5f;

	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Room Sizes",
	          meta = (ClampMin = "2.0", ClampMax = "8.0", Units = "m"))
	float MaxHallwayWidth = 5.0f;

	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Room Sizes",
	          meta = (ClampMin = "3.0", ClampMax = "20.0", Units = "m"))
	float MinHallwayLength = 12.0f;

	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Room Sizes",
	          meta = (ClampMin = "5.0", ClampMax = "150.0", Units = "m"))
	float MaxHallwayLength = 150.0f;

	// === HALLWAY LENGTH DISTRIBUTION ===

	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Hallway Distribution",
	          meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ShortHallwayRatio = 0.2f; // 20% short hallways (12-20m)

	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Hallway Distribution",
	          meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MediumHallwayRatio = 0.3f; // 30% medium hallways (20-35m)

	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Hallway Distribution",
	          meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LongHallwayRatio = 0.5f; // 50% long hallways (35-150m)

	// Length thresholds for hallway categories
	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Hallway Distribution",
	          meta = (ClampMin = "15.0", ClampMax = "25.0", Units = "m"))
	float MediumHallwayThreshold = 20.0f;

	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Hallway Distribution",
	          meta = (ClampMin = "25.0", ClampMax = "50.0", Units = "m"))
	float LongHallwayThreshold = 35.0f;

	// Stairs (elevation changes)
	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Room Sizes",
	          meta = (ClampMin = "1.0", ClampMax = "10.0", Units = "m"))
	float MinStairHeight = 2.0f;

	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Room Sizes",
	          meta = (ClampMin = "2.0", ClampMax = "20.0", Units = "m"))
	float MaxStairHeight = 6.0f;

	// === LOGGING SETTINGS ===

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bShowRoomNumbers = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bVerboseLogging = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (ClampMin = "10", ClampMax = "1000"))
	int32 LoggingInterval = 50; // Log every N iterations

	// === VALIDATION ===

	// Validate that ratios sum to approximately 1.0
	bool IsValidConfiguration() const
	{
		const float TotalRatio = RoomRatio + HallwayRatio + StairRatio;
		return FMath::IsNearlyEqual(TotalRatio, 1.0f, 0.01f);
	}

	// Get normalized ratios (in case they don't sum to 1.0)
	void GetNormalizedRatios(float& OutRoomRatio, float& OutHallwayRatio, float& OutStairRatio) const
	{
		const float TotalRatio = RoomRatio + HallwayRatio + StairRatio;
		if (TotalRatio > 0.0f)
		{
			OutRoomRatio = RoomRatio / TotalRatio;
			OutHallwayRatio = HallwayRatio / TotalRatio;
			OutStairRatio = StairRatio / TotalRatio;
		}
		else
		{
			// Fallback to default distribution
			OutRoomRatio = 0.33f;
			OutHallwayRatio = 0.33f;
			OutStairRatio = 0.34f;
		}
	}

	// Default constructor with sensible defaults
	FBackroomGenerationConfig()
	{
		// All values already set via default member initializers
	}
};

/**
 * Constants that should never change during runtime
 */
namespace BackroomConstants
{
// Unit conversions
constexpr float METERS_TO_UNREAL_UNITS = 100.0f;
constexpr float UNREAL_UNITS_TO_METERS = 0.01f;

// Room connection count
constexpr int32 CONNECTIONS_PER_ROOM = 4; // North, South, East, West

// Safety iteration intervals for logging
constexpr int32 MAIN_LOOP_LOG_INTERVAL = 100;
constexpr int32 CONNECTION_RETRY_LOG_INTERVAL = 50;
constexpr int32 PLACEMENT_ATTEMPT_LOG_INTERVAL = 25;

// Initial room settings
constexpr float INITIAL_ROOM_SIZE = 5.0f;         // 5x5m starting room
constexpr float INITIAL_ROOM_FLOOR_OFFSET = 0.5f; // 0.5m below character
} // namespace BackroomConstants