#include "RoomConnectionManager.h"
#include "Engine/Engine.h"
#include "../RoomUnit/StandardRoom.h"

FRoomConnectionManager::FRoomConnectionManager()
{
}

bool FRoomConnectionManager::TryPlaceRoom(const FRoomData& SourceRoom, 
                                          int32 ConnectionIndex, 
                                          FRoomData& NewRoom,
                                          const TArray<FRoomData>& ExistingRooms,
                                          const FBackroomGenerationConfig& Config,
                                          ICollisionDetectionService* CollisionService,
                                          TFunction<void(FRoomData&)> RoomCreator)
{
    LogDebug(FString::Printf(TEXT("[DEBUG] TryPlaceRoom: Room %d (%.1fx%.1fm, Cat=%s, Elev=%.1fm) at Connection %d"), 
        NewRoom.RoomIndex, NewRoom.Width, NewRoom.Length, 
        *UEnum::GetValueAsString(NewRoom.Category), NewRoom.Elevation, ConnectionIndex), Config);
    
    // Handle special stairs elevation logic
    HandleStairsElevation(SourceRoom, NewRoom, Config);
    
    // Calculate position based on connection
    NewRoom.Position = CalculateConnectionPosition(SourceRoom, ConnectionIndex, NewRoom, Config);
    
    LogDebug(FString::Printf(TEXT("[DEBUG] Calculated Position: %s (X=%.1f, Y=%.1f, Z=%.1f)"), 
        *NewRoom.Position.ToString(), NewRoom.Position.X, NewRoom.Position.Y, NewRoom.Position.Z), Config);
    
    // Check for collisions (excluding the source room we're connecting to)
    LogDebug(FString::Printf(TEXT("[DEBUG] COLLISION CHECK: About to check room %d against %d existing rooms"), 
        NewRoom.RoomIndex, ExistingRooms.Num()), Config);
    
    bool bHasCollision = CollisionService->CheckRoomCollisionExcluding(NewRoom, ExistingRooms, SourceRoom.RoomIndex, Config);
    LogDebug(FString::Printf(TEXT("[DEBUG] COLLISION RESULT: %s"), bHasCollision ? TEXT("COLLISION DETECTED") : TEXT("NO COLLISION")), Config);
    
    if (!bHasCollision)
    {
        // Create room connections
        CreateRoomConnections(NewRoom, Config);
        
        // Call the room creator function (handles UE-specific room unit creation)
        if (RoomCreator)
        {
            RoomCreator(NewRoom);
        }
        
        return true; // Successfully placed
    }
    
    return false; // Collision prevented placement
}

void FRoomConnectionManager::ConnectRooms(FRoomData& Room1, 
                                          int32 Connection1Index, 
                                          FRoomData& Room2, 
                                          int32 Connection2Index,
                                          const FBackroomGenerationConfig& Config)
{
    // Validate connection indices
    if (Connection1Index < 0 || Connection1Index >= Room1.Connections.Num() ||
        Connection2Index < 0 || Connection2Index >= Room2.Connections.Num())
    {
        LogDebug(TEXT("[ERROR] ConnectRooms: Invalid connection indices"), Config);
        return;
    }
    
    // Determine connection properties
    EConnectionType ConnectionType;
    float ConnectionWidth;
    DetermineConnectionProperties(Room1, Room2, Config, ConnectionType, ConnectionWidth);
    
    // Update both connections
    FRoomConnection& Connection1 = Room1.Connections[Connection1Index];
    FRoomConnection& Connection2 = Room2.Connections[Connection2Index];
    
    Connection1.bIsUsed = true;
    Connection1.ConnectionType = ConnectionType;
    Connection1.ConnectionWidth = ConnectionWidth;
    Connection1.ConnectedRoomIndex = Room2.RoomIndex;
    
    Connection2.bIsUsed = true;
    Connection2.ConnectionType = ConnectionType;
    Connection2.ConnectionWidth = ConnectionWidth;
    Connection2.ConnectedRoomIndex = Room1.RoomIndex;
    
    // Create physical holes in the room meshes
    CreatePhysicalConnection(Room1, Connection1Index, ConnectionType, ConnectionWidth);
    CreatePhysicalConnection(Room2, Connection2Index, ConnectionType, ConnectionWidth);
    
    LogDebug(FString::Printf(TEXT("CONNECTED: Room %d ↔ Room %d (%s, %.2fm width)"), 
        Room1.RoomIndex, Room2.RoomIndex, 
        *UEnum::GetValueAsString(ConnectionType), ConnectionWidth), Config);
}

void FRoomConnectionManager::CreateRoomConnections(FRoomData& Room,
                                                   const FBackroomGenerationConfig& Config)
{
    Room.Connections.Empty();
    
    // Create connections based on room type
    TArray<EWallSide> WallSides;
    
    if (Room.Category == ERoomCategory::Stairs)
    {
        // STAIRS: Only allow connection from the wall OPPOSITE to stair direction (bottom end)
        EWallSide BottomEndWall = EWallSide::None;
        switch (Room.StairDirection)
        {
            case EWallSide::North: BottomEndWall = EWallSide::South; break; // Stairs go north, connect at south (bottom)
            case EWallSide::South: BottomEndWall = EWallSide::North; break; // Stairs go south, connect at north (bottom)
            case EWallSide::East: BottomEndWall = EWallSide::West; break;   // Stairs go east, connect at west (bottom)
            case EWallSide::West: BottomEndWall = EWallSide::East; break;   // Stairs go west, connect at east (bottom)
        }
        
        WallSides.Add(BottomEndWall);
        LogDebug(FString::Printf(TEXT("STAIR CONNECTIONS: Stairs ascend %s, connecting at bottom end (%s wall)"), 
            *UEnum::GetValueAsString(Room.StairDirection), *UEnum::GetValueAsString(BottomEndWall)), Config);
    }
    else
    {
        // REGULAR ROOMS/HALLWAYS: All 4 walls can have connections
        WallSides = {EWallSide::North, EWallSide::South, EWallSide::East, EWallSide::West};
    }
    
    for (EWallSide WallSide : WallSides)
    {
        FRoomConnection Connection;
        Connection.WallSide = WallSide;
        Connection.bIsUsed = false;
        Connection.ConnectionType = EConnectionType::Doorway; // Will be set when actually used
        Connection.ConnectionWidth = Config.StandardDoorwayWidth; // Default door width
        Connection.ConnectedRoomIndex = -1;
        
        // Calculate connection point (center of wall)
        Connection.ConnectionPoint = CalculateWallConnectionPoint(Room, WallSide, Config);
        
        Room.Connections.Add(Connection);
    }
}

FVector FRoomConnectionManager::CalculateConnectionPosition(const FRoomData& SourceRoom,
                                                            int32 ConnectionIndex,
                                                            const FRoomData& NewRoom,
                                                            const FBackroomGenerationConfig& Config)
{
    if (ConnectionIndex < 0 || ConnectionIndex >= SourceRoom.Connections.Num())
    {
        LogDebug(TEXT("[ERROR] CalculateConnectionPosition: Invalid connection index"), Config);
        return FVector::ZeroVector;
    }
    
    const FRoomConnection& Connection = SourceRoom.Connections[ConnectionIndex];
    FVector ConnectionPoint = Connection.ConnectionPoint;
    
    LogDebug(FString::Printf(TEXT("Source room %d connection %d at %s (wall: %s)"), 
        SourceRoom.RoomIndex, ConnectionIndex, *ConnectionPoint.ToString(), 
        *UEnum::GetValueAsString(Connection.WallSide)), Config);
    
    // Calculate offset based on wall side
    FVector Offset = FVector::ZeroVector;
    
    // Calculate Z offset based on room type
    float ZOffset;
    if (SourceRoom.Category == ERoomCategory::Stairs)
    {
        // For stairs: We want the new room's floor to be at ConnectionPoint.Z level
        // The new room will render with its geometry at Position.Z + Elevation
        // So Position.Z should be ConnectionPoint.Z - Elevation, which means ZOffset = -Elevation
        float NewRoomElevationCm = NewRoom.Elevation * BackroomConstants::METERS_TO_UNREAL_UNITS;
        ZOffset = -NewRoomElevationCm;
        LogDebug(FString::Printf(TEXT("STAIR CONNECTION: Setting ZOffset=%.1fcm to align room floor (elevation %.1fm) with connection point at Z=%.1fcm"), 
            ZOffset, NewRoom.Elevation, ConnectionPoint.Z), Config);
    }
    else
    {
        // Standard connection: same floor level as source room
        ZOffset = SourceRoom.Position.Z - ConnectionPoint.Z;
    }
    
    // Wall thickness and gap calculations
    float WallThickness = Config.WallThickness * BackroomConstants::METERS_TO_UNREAL_UNITS;
    float AntiFlickerGap = BackroomConstants::METERS_TO_UNREAL_UNITS * 0.01f; // 1cm gap
    
    // Calculate position offset based on connection wall side
    switch (Connection.WallSide)
    {
        case EWallSide::North:
            Offset = FVector(
                -NewRoom.Width * BackroomConstants::METERS_TO_UNREAL_UNITS * 0.5f,    // Center new room X on connection point X
                WallThickness + AntiFlickerGap, // Push north from connection point
                ZOffset
            );
            break;
            
        case EWallSide::South:
            Offset = FVector(
                -NewRoom.Width * BackroomConstants::METERS_TO_UNREAL_UNITS * 0.5f,    // Center new room X on connection point X
                -NewRoom.Length * BackroomConstants::METERS_TO_UNREAL_UNITS - WallThickness - AntiFlickerGap, // Push south from connection point
                ZOffset
            );
            break;
            
        case EWallSide::East:
            Offset = FVector(
                WallThickness + AntiFlickerGap, // Push east from connection point
                -NewRoom.Length * BackroomConstants::METERS_TO_UNREAL_UNITS * 0.5f,   // Center new room Y on connection point Y
                ZOffset
            );
            break;
            
        case EWallSide::West:
            Offset = FVector(
                -NewRoom.Width * BackroomConstants::METERS_TO_UNREAL_UNITS - WallThickness - AntiFlickerGap, // Push west from connection point
                -NewRoom.Length * BackroomConstants::METERS_TO_UNREAL_UNITS * 0.5f,   // Center new room Y on connection point Y
                ZOffset
            );
            break;
    }
    
    return ConnectionPoint + Offset;
}

void FRoomConnectionManager::LogDebug(const FString& Message, const FBackroomGenerationConfig& Config) const
{
    if (Config.bVerboseLogging)
    {
        // Use UE_LOG for independent logging
        UE_LOG(LogTemp, Warning, TEXT("[ConnectionManager] %s"), *Message);
    }
}

FVector FRoomConnectionManager::CalculateWallConnectionPoint(const FRoomData& Room, 
                                                             EWallSide WallSide,
                                                             const FBackroomGenerationConfig& Config) const
{
    FVector RoomCenter = Room.Position + FVector(
        Room.Width * BackroomConstants::METERS_TO_UNREAL_UNITS * 0.5f,
        Room.Length * BackroomConstants::METERS_TO_UNREAL_UNITS * 0.5f,
        Room.Elevation * BackroomConstants::METERS_TO_UNREAL_UNITS // Floor level at elevation
    );
    
    // Special handling for stairs rooms
    if (Room.Category == ERoomCategory::Stairs)
    {
        // For stairs: connection point at the BOTTOM END (ground level) wall center
        switch (Room.StairDirection)
        {
            case EWallSide::North:
                // Stairs go north, bottom end is south wall
                if (WallSide == EWallSide::South)
                {
                    return RoomCenter - FVector(0, Room.Length * BackroomConstants::METERS_TO_UNREAL_UNITS * 0.5f, 0);
                }
                break;
            case EWallSide::South:
                // Stairs go south, bottom end is north wall  
                if (WallSide == EWallSide::North)
                {
                    return RoomCenter + FVector(0, Room.Length * BackroomConstants::METERS_TO_UNREAL_UNITS * 0.5f, 0);
                }
                break;
            case EWallSide::East:
                // Stairs go east, bottom end is west wall
                if (WallSide == EWallSide::West)
                {
                    return RoomCenter - FVector(Room.Width * BackroomConstants::METERS_TO_UNREAL_UNITS * 0.5f, 0, 0);
                }
                break;
            case EWallSide::West:
                // Stairs go west, bottom end is east wall
                if (WallSide == EWallSide::East)
                {
                    return RoomCenter + FVector(Room.Width * BackroomConstants::METERS_TO_UNREAL_UNITS * 0.5f, 0, 0);
                }
                break;
        }
    }
    
    // Standard connection points (wall centers)
    switch (WallSide)
    {
        case EWallSide::North:
            return RoomCenter + FVector(0, Room.Length * BackroomConstants::METERS_TO_UNREAL_UNITS * 0.5f, 0);
        case EWallSide::South:
            return RoomCenter - FVector(0, Room.Length * BackroomConstants::METERS_TO_UNREAL_UNITS * 0.5f, 0);
        case EWallSide::East:
            return RoomCenter + FVector(Room.Width * BackroomConstants::METERS_TO_UNREAL_UNITS * 0.5f, 0, 0);
        case EWallSide::West:
            return RoomCenter - FVector(Room.Width * BackroomConstants::METERS_TO_UNREAL_UNITS * 0.5f, 0, 0);
        default:
            return RoomCenter;
    }
}

void FRoomConnectionManager::DetermineConnectionProperties(const FRoomData& Room1,
                                                           const FRoomData& Room2,
                                                           const FBackroomGenerationConfig& Config,
                                                           EConnectionType& OutConnectionType,
                                                           float& OutConnectionWidth) const
{
    // Determine connection type: use configured ratio
    FRandomStream Random(FDateTime::Now().GetTicks());
    OutConnectionType = (Random.FRand() < Config.DoorwayConnectionRatio) ? 
        EConnectionType::Doorway : EConnectionType::Opening;
    
    // Calculate connection width based on type
    if (OutConnectionType == EConnectionType::Doorway)
    {
        OutConnectionWidth = Config.StandardDoorwayWidth;
    }
    else
    {
        // For openings, use a percentage of the smallest wall in the connection
        float Room1WallSize = GetWallSize(Room1, Room1.Connections.Num() > 0 ? Room1.Connections[0].WallSide : EWallSide::North);
        float Room2WallSize = GetWallSize(Room2, Room2.Connections.Num() > 0 ? Room2.Connections[0].WallSide : EWallSide::North);
        float SmallestWall = FMath::Min(Room1WallSize, Room2WallSize);
        
        // Use 60-80% of smallest wall for opening width
        float OpeningRatio = Random.FRandRange(0.6f, 0.8f);
        OutConnectionWidth = SmallestWall * OpeningRatio;
        
        // Ensure reasonable bounds
        OutConnectionWidth = FMath::Clamp(OutConnectionWidth, Config.StandardDoorwayWidth, SmallestWall * 0.9f);
    }
}

float FRoomConnectionManager::GetWallSize(const FRoomData& Room, EWallSide WallSide) const
{
    switch (WallSide)
    {
        case EWallSide::North:
        case EWallSide::South:
            return Room.Width;
        case EWallSide::East:
        case EWallSide::West:
            return Room.Length;
        default:
            return Room.Width; // Fallback
    }
}

void FRoomConnectionManager::HandleStairsElevation(const FRoomData& SourceRoom, 
                                                   FRoomData& NewRoom,
                                                   const FBackroomGenerationConfig& Config) const
{
    // SPECIAL HANDLING: If connecting to stairs, set new room elevation to match stair top
    if (SourceRoom.Category == ERoomCategory::Stairs)
    {
        NewRoom.Elevation = SourceRoom.Elevation; // Match stair top elevation
        LogDebug(FString::Printf(TEXT("STAIR CONNECTION: Setting new room elevation to %.1fm (matching stair top)"), 
            NewRoom.Elevation), Config);
    }
}

void FRoomConnectionManager::CreatePhysicalConnection(FRoomData& Room, int32 ConnectionIndex, 
                                                     EConnectionType ConnectionType, float ConnectionWidth)
{
    // Validate room unit exists
    if (!Room.RoomUnit)
    {
        UE_LOG(LogTemp, Warning, TEXT("CreatePhysicalConnection: Room %d has no RoomUnit"), Room.RoomIndex);
        return;
    }
    
    // Validate connection index
    if (ConnectionIndex < 0 || ConnectionIndex >= Room.Connections.Num())
    {
        UE_LOG(LogTemp, Warning, TEXT("CreatePhysicalConnection: Invalid connection index %d for room %d"), 
            ConnectionIndex, Room.RoomIndex);
        return;
    }
    
    const FRoomConnection& Connection = Room.Connections[ConnectionIndex];
    EWallSide WallSide = Connection.WallSide;
    
    // Get the owner (should be the ABackRoomGenerator that created this room unit)
    AActor* Owner = Cast<AActor>(Room.RoomUnit->GetOuter());
    if (!Owner)
    {
        UE_LOG(LogTemp, Error, TEXT("CreatePhysicalConnection: Room %d RoomUnit has no valid AActor owner"), Room.RoomIndex);
        return;
    }
    
    // Create door configuration for the hole
    FDoorConfig DoorConfig;
    DoorConfig.WallSide = WallSide;
    DoorConfig.Width = ConnectionWidth;
    DoorConfig.Height = (ConnectionType == EConnectionType::Doorway) ? 2.0f : 2.5f; // Standard door height vs opening height
    DoorConfig.OffsetFromCenter = 0.0f; // Center the connection
    DoorConfig.bHasDoor = true;
    
    // Create the physical hole in the room mesh
    Room.RoomUnit->AddHoleToWall(Owner, WallSide, DoorConfig);
    
    // UE_LOG(LogTemp, Warning, TEXT("PHYSICAL CONNECTION: Created %s hole in Room %d %s wall (%.1fm wide)"), 
    //    (ConnectionType == EConnectionType::Doorway) ? TEXT("doorway") : TEXT("opening"),
    //    Room.RoomIndex, *UEnum::GetValueAsString(WallSide), ConnectionWidth);
}