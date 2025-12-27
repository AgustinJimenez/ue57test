#include "HallwayStrategy.h"
#include "../RoomUnit/BaseRoom.h"

FRoomData FHallwayStrategy::GenerateRoom(const FBackroomGenerationConfig& Config, int32 RoomIndex)
{
    FRoomData Room;
    InitializeBaseRoomData(Room, ERoomCategory::Hallway, RoomIndex, Config);
    
    // Generate random dimensions within hallway constraints
    FRandomStream Random = CreateRandomStream(RoomIndex);
    GenerateHallwayDimensions(Config, Random, Room.Width, Room.Length);
    
    // Create hallway connections
    CreateHallwayConnections(Room);
    
    return Room;
}

FRoomData FHallwayStrategy::GenerateConnectedRoom(const FBackroomGenerationConfig& Config, 
                                                 int32 RoomIndex,
                                                 const FRoomData& SourceRoom, 
                                                 int32 ConnectionIndex)
{
    // For hallways, connection-aware generation considers source room orientation
    FRoomData Room = GenerateRoom(Config, RoomIndex);
    
    // Optimization: Try to align hallway direction with connection direction
    // This creates more natural flow between rooms and hallways
    if (ConnectionIndex >= 0 && ConnectionIndex < SourceRoom.Connections.Num())
    {
        const FRoomConnection& Connection = SourceRoom.Connections[ConnectionIndex];
        EWallSide ConnectionSide = Connection.WallSide;
        
        // If connecting to North/South walls, prefer East-West oriented hallways
        // If connecting to East/West walls, prefer North-South oriented hallways
        if (ConnectionSide == EWallSide::North || ConnectionSide == EWallSide::South)
        {
            // Prefer wider East-West hallways for better flow
            if (Room.Width > Room.Length)
            {
                // Swap dimensions to make length > width (East-West orientation)
                float Temp = Room.Width;
                Room.Width = Room.Length;
                Room.Length = Temp;
            }
        }
        else if (ConnectionSide == EWallSide::East || ConnectionSide == EWallSide::West)
        {
            // Prefer longer North-South hallways for better flow
            if (Room.Length > Room.Width)
            {
                // Keep current orientation (North-South)
                // Length is already > Width, which is correct
            }
        }
    }
    
    return Room;
}

bool FHallwayStrategy::CanGenerateRoom(const FBackroomGenerationConfig& Config, 
                                      const FRoomData* SourceRoom) const
{
    // Hallways can be generated as long as size ranges are valid for rectangular rooms
    bool bValidSizeRange = (Config.MinHallwayWidth > 0.0f && 
                           Config.MaxHallwayWidth > Config.MinHallwayWidth &&
                           Config.MinHallwayLength > 0.0f &&
                           Config.MaxHallwayLength > Config.MinHallwayLength &&
                           Config.MaxHallwayWidth <= 20.0f &&  // Reasonable upper limits
                           Config.MaxHallwayLength <= 50.0f);
    
    // Ensure width is generally less than length for proper hallway proportions
    bool bValidProportions = (Config.MinHallwayLength > Config.MaxHallwayWidth * 0.8f);
    
    return bValidSizeRange && bValidProportions;
}

void FHallwayStrategy::GenerateHallwayDimensions(const FBackroomGenerationConfig& Config,
                                                FRandomStream& Random,
                                                float& OutWidth, 
                                                float& OutLength) const
{
    // Generate width (always the shorter dimension) within configured range
    const float MinWidth = FMath::Max(1.0f, Config.MinHallwayWidth);
    const float MaxWidth = FMath::Max(MinWidth + 0.5f, Config.MaxHallwayWidth);
    OutWidth = Random.FRandRange(MinWidth, MaxWidth);
    
    // Use sub-ratio system to determine hallway length category
    float RandomValue = Random.FRand();
    float CumulativeRatio = 0.0f;
    
    // Normalize ratios to ensure they sum to 1.0
    float TotalRatio = Config.ShortHallwayRatio + Config.MediumHallwayRatio + Config.LongHallwayRatio;
    if (TotalRatio <= 0.0f) TotalRatio = 1.0f; // Safety check
    
    float NormalizedShort = Config.ShortHallwayRatio / TotalRatio;
    float NormalizedMedium = Config.MediumHallwayRatio / TotalRatio;
    float NormalizedLong = Config.LongHallwayRatio / TotalRatio;
    
    // Determine hallway category and generate length accordingly
    if (RandomValue < NormalizedShort)
    {
        // Short hallways: MinLength to MediumThreshold
        float MinLength = FMath::Max(2.0f, Config.MinHallwayLength);
        float MaxLength = FMath::Min(Config.MediumHallwayThreshold, Config.MaxHallwayLength);
        OutLength = Random.FRandRange(MinLength, MaxLength);
    }
    else if (RandomValue < (NormalizedShort + NormalizedMedium))
    {
        // Medium hallways: MediumThreshold to LongThreshold
        float MinLength = Config.MediumHallwayThreshold;
        float MaxLength = FMath::Min(Config.LongHallwayThreshold, Config.MaxHallwayLength);
        OutLength = Random.FRandRange(MinLength, MaxLength);
    }
    else
    {
        // Long hallways: LongThreshold to MaxLength
        float MinLength = Config.LongHallwayThreshold;
        float MaxLength = Config.MaxHallwayLength;
        OutLength = Random.FRandRange(MinLength, MaxLength);
    }
    
    // Ensure minimum hallway aspect ratio (length should be at least 2x width)
    float MinRequiredLength = OutWidth * 2.0f;
    if (OutLength < MinRequiredLength)
    {
        OutLength = MinRequiredLength;
        OutLength = FMath::Clamp(OutLength, Config.MinHallwayLength, Config.MaxHallwayLength);
    }
    
    // 25% chance to swap dimensions for variety in orientation (horizontal vs vertical long hallways)
    // But ensure we maintain the hallway aspect ratio
    if (Random.FRand() < 0.25f)
    {
        float Temp = OutWidth;
        OutWidth = OutLength;
        OutLength = Temp;
    }
    
    // Final validation and ensure proper hallway proportions
    if (!ValidateHallwayDimensions(Config, OutWidth, OutLength))
    {
        // Fallback to safe long hallway dimensions
        OutWidth = FMath::Clamp(MinWidth, 2.5f, MaxWidth);
        OutLength = Random.FRandRange(Config.LongHallwayThreshold, Config.MaxHallwayLength);
    }
    
    // Note: Individual size logging removed - will be shown in generation summary
}

void FHallwayStrategy::CreateHallwayConnections(FRoomData& Room) const
{
    // Hallways have 4 connections (one per wall) like standard rooms
    Room.Connections.SetNum(BackroomConstants::CONNECTIONS_PER_ROOM);
    
    // Initialize each connection
    for (int32 i = 0; i < BackroomConstants::CONNECTIONS_PER_ROOM; i++)
    {
        FRoomConnection& Connection = Room.Connections[i];
        
        // Assign wall side based on index
        Connection.WallSide = static_cast<EWallSide>(i + 1); // Enum values: North=1, South=2, East=3, West=4
        Connection.bIsUsed = false;
        Connection.ConnectionPoint = FVector::ZeroVector; // Set during placement
        Connection.ConnectionWidth = 0.8f; // Default doorway width - can be overridden
        Connection.ConnectionType = EConnectionType::Doorway; // Default type - can be overridden  
        Connection.ConnectedRoomIndex = -1; // No connection yet
    }
}

bool FHallwayStrategy::ValidateHallwayDimensions(const FBackroomGenerationConfig& Config,
                                                float Width, 
                                                float Length) const
{
    // Basic range validation
    if (Width < 1.0f || Length < 1.0f || Width > 100.0f || Length > 100.0f)
    {
        return false;
    }
    
    // Check against configuration bounds
    const float MinWidth = FMath::Max(1.0f, Config.MinHallwayWidth);
    const float MaxWidth = FMath::Min(100.0f, Config.MaxHallwayWidth);
    const float MinLength = FMath::Max(1.0f, Config.MinHallwayLength);
    const float MaxLength = FMath::Min(100.0f, Config.MaxHallwayLength);
    
    if (Width < MinWidth || Width > MaxWidth || Length < MinLength || Length > MaxLength)
    {
        return false;
    }
    
    // Ensure proper hallway aspect ratio (rectangular, not square)
    // Hallways should have an aspect ratio of at least 1.2:1 for navigation
    const float AspectRatio = FMath::Max(Width, Length) / FMath::Max(0.1f, FMath::Min(Width, Length));
    if (AspectRatio < 1.2f) // Minimum aspect ratio for hallways
    {
        return false;
    }
    
    // Maximum aspect ratio check - avoid extremely long narrow hallways
    if (AspectRatio > 10.0f) // Maximum 10:1 ratio
    {
        return false;
    }
    
    return true;
}