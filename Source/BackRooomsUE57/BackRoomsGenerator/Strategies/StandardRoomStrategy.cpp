#include "StandardRoomStrategy.h"
#include "../RoomUnit/BaseRoom.h"

FRoomData FStandardRoomStrategy::GenerateRoom(const FBackroomGenerationConfig& Config, int32 RoomIndex)
{
    FRoomData Room;
    InitializeBaseRoomData(Room, ERoomCategory::Room, RoomIndex, Config);
    
    // Generate random dimensions within standard room constraints
    FRandomStream Random = CreateRandomStream(RoomIndex);
    GenerateStandardRoomDimensions(Config, Random, Room.Width, Room.Length);
    
    // Create standard room connections
    CreateStandardRoomConnections(Room);
    
    return Room;
}

FRoomData FStandardRoomStrategy::GenerateConnectedRoom(const FBackroomGenerationConfig& Config, 
                                                      int32 RoomIndex,
                                                      const FRoomData& SourceRoom, 
                                                      int32 ConnectionIndex)
{
    // For standard rooms, connection-aware generation is same as standalone generation
    // The connection logic is handled by the placement system, not the generation
    FRoomData Room = GenerateRoom(Config, RoomIndex);
    
    // Future enhancement: Could add logic to avoid generating rooms that are too large
    // for the connection context, but current system handles this via collision detection
    
    return Room;
}

bool FStandardRoomStrategy::CanGenerateRoom(const FBackroomGenerationConfig& Config, 
                                           const FRoomData* SourceRoom) const
{
    // Standard rooms can always be generated as long as size ranges are valid
    bool bValidSizeRange = (Config.MinRoomSize > 0.0f && 
                           Config.MaxRoomSize > Config.MinRoomSize &&
                           Config.MaxRoomSize <= 50.0f); // Reasonable upper limit
    
    // Additional validation: Ensure we're not trying to connect to a stair room 
    // that explicitly disallows room connections (future enhancement)
    if (SourceRoom && SourceRoom->Category == ERoomCategory::Stairs)
    {
        // Could add stair-specific connection rules here
        // For now, allow all connections
    }
    
    return bValidSizeRange;
}

void FStandardRoomStrategy::GenerateStandardRoomDimensions(const FBackroomGenerationConfig& Config,
                                                          FRandomStream& Random,
                                                          float& OutWidth, 
                                                          float& OutLength) const
{
    // Generate base size within configured range
    const float MinSize = FMath::Max(1.0f, Config.MinRoomSize);
    const float MaxSize = FMath::Max(MinSize + 0.5f, Config.MaxRoomSize);
    
    // Standard rooms are square-ish, so generate one base dimension
    const float BaseSize = Random.FRandRange(MinSize, MaxSize);
    
    // Add slight variation to make rooms not perfectly square
    // Variation is 0-20% of base size to maintain square-ish appearance
    const float VariationPercent = Random.FRandRange(0.0f, 0.2f);
    const float Variation = BaseSize * VariationPercent;
    
    // Randomly decide which dimension gets the variation
    // IMPORTANT: Only apply positive variation to prevent rooms going below MinSize
    if (Random.RandRange(0, 1) == 0)
    {
        OutWidth = BaseSize;
        OutLength = BaseSize + Variation; // Only positive variation
    }
    else
    {
        OutWidth = BaseSize + Variation; // Only positive variation
        OutLength = BaseSize;
    }
    
    // Ensure both dimensions stay within bounds (should already be valid now)
    OutWidth = FMath::Clamp(OutWidth, MinSize, MaxSize);
    OutLength = FMath::Clamp(OutLength, MinSize, MaxSize);
    
    // Final validation
    if (!ValidateRoomDimensions(Config, OutWidth, OutLength))
    {
        // Fallback to safe dimensions
        OutWidth = FMath::Clamp(BaseSize, MinSize, MaxSize);
        OutLength = OutWidth; // Perfect square fallback
    }
    
    // Note: Individual size logging removed - will be shown in generation summary
}

void FStandardRoomStrategy::CreateStandardRoomConnections(FRoomData& Room) const
{
    // Standard rooms have 4 connections (one per wall)
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

bool FStandardRoomStrategy::ValidateRoomDimensions(const FBackroomGenerationConfig& Config,
                                                  float Width, 
                                                  float Length) const
{
    // Basic range validation
    if (Width < 0.5f || Length < 0.5f || Width > 100.0f || Length > 100.0f)
    {
        return false;
    }
    
    // Check against configuration bounds
    const float MinSize = FMath::Max(0.5f, Config.MinRoomSize);
    const float MaxSize = FMath::Min(100.0f, Config.MaxRoomSize);
    
    if (Width < MinSize || Width > MaxSize || Length < MinSize || Length > MaxSize)
    {
        return false;
    }
    
    // Ensure dimensions are reasonable for standard rooms
    // Standard rooms should not be extremely rectangular (aspect ratio check)
    const float AspectRatio = FMath::Max(Width, Length) / FMath::Max(0.1f, FMath::Min(Width, Length));
    if (AspectRatio > 3.0f) // Allow up to 3:1 aspect ratio for standard rooms
    {
        return false;
    }
    
    return true;
}