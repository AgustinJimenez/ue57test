#include "StairsStrategy.h"

#include "../RoomUnit/BaseRoom.h"

FRoomData FStairsStrategy::GenerateRoom(const FBackroomGenerationConfig& Config, int32 RoomIndex)
{
	FRoomData Room;
	InitializeBaseRoomData(Room, ERoomCategory::Stairs, RoomIndex, Config);

	// Generate random dimensions within stairs constraints
	FRandomStream Random = CreateRandomStream(RoomIndex);
	GenerateStairsDimensions(Config, Random, Room.Width, Room.Length);

	// Determine stair direction randomly for standalone generation
	TArray<EWallSide> PossibleDirections = {EWallSide::North, EWallSide::South, EWallSide::East, EWallSide::West};
	// Fix: Add bounds checking for array access
	if (PossibleDirections.Num() > 0)
	{
		Room.StairDirection = PossibleDirections[FMath::RandRange(0, PossibleDirections.Num() - 1)];
	}

	// Calculate elevation change
	bool bGoingUp = Random.RandRange(0, 1) == 0; // 50% chance up or down
	Room.Elevation = CalculateStairElevation(Config, Random, bGoingUp);

	// Create stairs connections (3 connections - stairs occupy one wall)
	CreateStairsConnections(Room, Room.StairDirection);

	return Room;
}

FRoomData FStairsStrategy::GenerateConnectedRoom(const FBackroomGenerationConfig& Config,
                                                 int32 RoomIndex,
                                                 const FRoomData& SourceRoom,
                                                 int32 ConnectionIndex)
{
	// Generate basic room structure
	FRoomData Room = GenerateRoom(Config, RoomIndex);

	// For stairs, connection-aware generation considers source room elevation
	FRandomStream Random = CreateRandomStream(RoomIndex);

	// Determine stair direction based on connection context
	Room.StairDirection = DetermineStairDirection(SourceRoom, ConnectionIndex, Random);

	// Calculate elevation relative to source room
	// Consider whether we want to create elevation variety or specific patterns
	bool bGoingUp = Random.RandRange(0, 1) == 0;

	// If source room is already elevated, bias towards going down to create variety
	if (SourceRoom.Elevation > 0.0f)
	{
		bGoingUp = Random.RandRange(0.0f, 1.0f) < 0.3f; // 30% chance up, 70% down
	}
	// If source room is below ground, bias towards going up
	else if (SourceRoom.Elevation < 0.0f)
	{
		bGoingUp = Random.RandRange(0.0f, 1.0f) < 0.7f; // 70% chance up, 30% down
	}

	// Set elevation relative to source room
	float ElevationChange = CalculateStairElevation(Config, Random, bGoingUp);
	Room.Elevation = SourceRoom.Elevation + ElevationChange;

	// Update room height if it's a tall stair room
	if (FMath::Abs(ElevationChange) > Config.StandardRoomHeight * 1.5f)
	{
		Room.Height = Config.StandardRoomHeight + (FMath::Abs(ElevationChange) * 0.5f);
	}

	// Recreate connections with the determined stair direction
	CreateStairsConnections(Room, Room.StairDirection);

	return Room;
}

bool FStairsStrategy::CanGenerateRoom(const FBackroomGenerationConfig& Config, const FRoomData* SourceRoom) const
{
	// Stairs can be generated as long as size ranges are valid for square-ish rooms
	bool bValidSizeRange = (Config.MinRoomSize > 0.0f && Config.MaxRoomSize > Config.MinRoomSize &&
	                        Config.MaxRoomSize <= 50.0f); // Reasonable upper limit for stairs

	// Stairs rooms need minimum size for stair placement (at least 3x3m)
	bool bValidMinimumSize = (Config.MinRoomSize >= 2.5f);

	// Check elevation limits to prevent extreme vertical generation
	bool bValidElevationRange = (Config.MinStairHeight > 0.0f && Config.MaxStairHeight > Config.MinStairHeight &&
	                             Config.MaxStairHeight <= 20.0f); // Max 20m elevation change

	// If connecting to existing stairs, ensure we don't create excessive elevation chains
	if (SourceRoom && SourceRoom->Category == ERoomCategory::Stairs)
	{
		// Limit consecutive stair rooms to prevent extreme elevation changes
		float MaxElevationFromGround = 30.0f; // Max 30m above or below starting level
		if (FMath::Abs(SourceRoom->Elevation) > MaxElevationFromGround)
		{
			return false;
		}
	}

	return bValidSizeRange && bValidMinimumSize && bValidElevationRange;
}

void FStairsStrategy::GenerateStairsDimensions(const FBackroomGenerationConfig& Config,
                                               FRandomStream& Random,
                                               float& OutWidth,
                                               float& OutLength) const
{
	// Stairs rooms use similar size ranges to standard rooms but tend towards square
	const float MinSize = FMath::Max(2.5f, Config.MinRoomSize); // Minimum 2.5m for stair space
	const float MaxSize = FMath::Max(MinSize + 1.0f, FMath::Min(Config.MaxRoomSize, 12.0f)); // Max 12m for stairs

	// Generate base size within configured range
	const float BaseSize = Random.FRandRange(MinSize, MaxSize);

	// Stairs rooms are more square than standard rooms (less variation)
	// Variation is 0-10% of base size to keep rooms square-ish for stair placement
	const float VariationPercent = Random.FRandRange(0.0f, 0.1f);
	const float Variation = BaseSize * VariationPercent;

	// Randomly decide which dimension gets the variation
	if (Random.RandRange(0, 1) == 0)
	{
		OutWidth = BaseSize;
		OutLength = BaseSize + (Random.RandRange(0, 1) == 0 ? Variation : -Variation);
	}
	else
	{
		OutWidth = BaseSize + (Random.RandRange(0, 1) == 0 ? Variation : -Variation);
		OutLength = BaseSize;
	}

	// Ensure both dimensions stay within bounds and minimum requirements
	OutWidth = FMath::Clamp(OutWidth, MinSize, MaxSize);
	OutLength = FMath::Clamp(OutLength, MinSize, MaxSize);

	// Final validation
	if (!ValidateStairsDimensions(Config, OutWidth, OutLength))
	{
		// Fallback to safe dimensions - perfect square for stair placement
		OutWidth = FMath::Clamp(BaseSize, MinSize, MaxSize);
		OutLength = OutWidth;
	}
}

void FStairsStrategy::CreateStairsConnections(FRoomData& Room, EWallSide StairDirection) const
{
	// Stairs rooms have 3 connections (stairs occupy one wall)
	Room.Connections.SetNum(BackroomConstants::CONNECTIONS_PER_ROOM);

	// Initialize each connection
	for (int32 i = 0; i < BackroomConstants::CONNECTIONS_PER_ROOM; i++)
	{
		FRoomConnection& Connection = Room.Connections[i];
		EWallSide WallSide = static_cast<EWallSide>(i + 1);

		// Assign wall side based on index
		Connection.WallSide = WallSide;
		Connection.ConnectionPoint = FVector::ZeroVector;     // Set during placement
		Connection.ConnectionWidth = 0.8f;                    // Default doorway width
		Connection.ConnectionType = EConnectionType::Doorway; // Default type
		Connection.ConnectedRoomIndex = -1;                   // No connection yet

		// Check if this wall has stairs
		if (WallSide == StairDirection)
		{
			// Stairs wall cannot have connections
			Connection.bIsUsed = true;         // Mark as used to prevent connections
			Connection.ConnectionWidth = 0.0f; // No connection possible
		}
		else
		{
			// Regular connection available
			Connection.bIsUsed = false;
		}
	}
}

float FStairsStrategy::CalculateStairElevation(const FBackroomGenerationConfig& Config,
                                               FRandomStream& Random,
                                               bool bGoingUp) const
{
	// Generate elevation change within configured range
	const float MinHeight = FMath::Max(1.0f, Config.MinStairHeight);
	const float MaxHeight = FMath::Max(MinHeight + 0.5f, Config.MaxStairHeight);

	// Calculate base elevation change
	float ElevationChange = Random.FRandRange(MinHeight, MaxHeight);

	// Convert to Unreal units (centimeters)
	ElevationChange = ElevationChange * 100.0f;

	// Apply direction
	if (!bGoingUp)
	{
		ElevationChange = -ElevationChange;
	}

	return ElevationChange;
}

EWallSide FStairsStrategy::DetermineStairDirection(const FRoomData& SourceRoom,
                                                   int32 ConnectionIndex,
                                                   FRandomStream& Random) const
{
	// Default to random direction if connection index is invalid
	if (ConnectionIndex < 0 || ConnectionIndex >= SourceRoom.Connections.Num())
	{
		TArray<EWallSide> PossibleDirections = {EWallSide::North, EWallSide::South, EWallSide::East, EWallSide::West};
		// Fix: Add bounds checking for array access
		if (PossibleDirections.Num() > 0)
		{
			return PossibleDirections[FMath::RandRange(0, PossibleDirections.Num() - 1)];
		}
		return EWallSide::North; // Safe fallback
	}

	// Get the connection direction from source room
	EWallSide SourceConnection = SourceRoom.Connections[ConnectionIndex].WallSide;

	// For better stair flow, place stairs on the opposite wall from the connection
	// This creates a natural flow: enter room, traverse room, climb/descend stairs
	switch (SourceConnection)
	{
	case EWallSide::North:
		return EWallSide::South; // Enter from North, stairs on South
	case EWallSide::South:
		return EWallSide::North; // Enter from South, stairs on North
	case EWallSide::East:
		return EWallSide::West; // Enter from East, stairs on West
	case EWallSide::West:
		return EWallSide::East; // Enter from West, stairs on East
	default:
		// Fallback to random if connection direction is None
		TArray<EWallSide> PossibleDirections = {EWallSide::North, EWallSide::South, EWallSide::East, EWallSide::West};
		// Fix: Add bounds checking for array access
		if (PossibleDirections.Num() > 0)
		{
			return PossibleDirections[FMath::RandRange(0, PossibleDirections.Num() - 1)];
		}
		return EWallSide::North; // Safe fallback
	}
}

bool FStairsStrategy::ValidateStairsDimensions(const FBackroomGenerationConfig& Config, float Width, float Length) const
{
	// Basic range validation
	if (Width < 2.0f || Length < 2.0f || Width > 100.0f || Length > 100.0f)
	{
		return false;
	}

	// Check against configuration bounds
	const float MinSize = FMath::Max(2.5f, Config.MinRoomSize);
	const float MaxSize = FMath::Min(100.0f, Config.MaxRoomSize);

	if (Width < MinSize || Width > MaxSize || Length < MinSize || Length > MaxSize)
	{
		return false;
	}

	// Stairs rooms should be reasonably square for proper stair placement
	// Allow up to 2:1 aspect ratio (less variation than standard rooms)
	const float AspectRatio = FMath::Max(Width, Length) / FMath::Max(0.1f, FMath::Min(Width, Length));
	if (AspectRatio > 2.0f)
	{
		return false;
	}

	// Ensure minimum size for practical stair placement (at least 2.5m each direction)
	if (Width < 2.5f || Length < 2.5f)
	{
		return false;
	}

	return true;
}