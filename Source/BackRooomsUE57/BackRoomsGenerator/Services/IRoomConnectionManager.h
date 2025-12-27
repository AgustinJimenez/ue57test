#pragma once

#include "CoreMinimal.h"
#include "../Types.h"
#include "../GenerationConfig.h"

/**
 * Interface for room connection management services
 * Handles room placement, connection logic, and connection creation
 */
class IRoomConnectionManager
{
public:
    virtual ~IRoomConnectionManager() = default;
    
    /**
     * Try to place a new room connected to an existing room
     * Handles position calculation, collision checking, and room creation
     * 
     * @param SourceRoom - Existing room to connect to
     * @param ConnectionIndex - Which connection point on the source room
     * @param NewRoom - Room to be placed (will be modified with final position)
     * @param ExistingRooms - Array of all existing rooms for collision checking
     * @param Config - Configuration settings
     * @param CollisionService - Service for collision detection
     * @param RoomCreator - Callback function to create the actual room unit
     * @return True if room was successfully placed
     */
    virtual bool TryPlaceRoom(const FRoomData& SourceRoom, 
                             int32 ConnectionIndex, 
                             FRoomData& NewRoom,
                             const TArray<FRoomData>& ExistingRooms,
                             const FBackroomGenerationConfig& Config,
                             class ICollisionDetectionService* CollisionService,
                             TFunction<void(FRoomData&)> RoomCreator) = 0;
    
    /**
     * Connect two rooms together with appropriate connection type and width
     * Creates bidirectional connections between rooms
     * 
     * @param Room1 - First room to connect
     * @param Connection1Index - Connection point index on first room
     * @param Room2 - Second room to connect  
     * @param Connection2Index - Connection point index on second room
     * @param Config - Configuration for connection ratios and settings
     */
    virtual void ConnectRooms(FRoomData& Room1, 
                             int32 Connection1Index, 
                             FRoomData& Room2, 
                             int32 Connection2Index,
                             const FBackroomGenerationConfig& Config) = 0;
    
    /**
     * Create connection points for a room based on its category
     * Different room types have different connection patterns
     * 
     * @param Room - Room to create connections for (will be modified)
     * @param Config - Configuration with connection settings
     */
    virtual void CreateRoomConnections(FRoomData& Room,
                                      const FBackroomGenerationConfig& Config) = 0;
    
    /**
     * Calculate the world position for a new room based on connection point
     * Handles different room types and elevation calculations
     * 
     * @param SourceRoom - Room that the new room connects to
     * @param ConnectionIndex - Which connection point on source room
     * @param NewRoom - New room with dimensions (used for positioning calculations)
     * @param Config - Configuration with wall thickness settings
     * @return World position for the new room
     */
    virtual FVector CalculateConnectionPosition(const FRoomData& SourceRoom,
                                               int32 ConnectionIndex,
                                               const FRoomData& NewRoom,
                                               const FBackroomGenerationConfig& Config) = 0;
};