#pragma once

#include "CoreMinimal.h"
#include "IRoomConnectionManager.h"
#include "ICollisionDetectionService.h"
#include "../Types.h"
#include "../GenerationConfig.h"

/**
 * Standard implementation of room connection management
 * Handles room placement, connection creation, and position calculation
 * 
 * Features:
 * - Smart room positioning with wall thickness consideration
 * - Elevation-aware connections for multi-level generation
 * - Different connection patterns for different room types
 * - Configurable connection types and widths
 * - Support for stairs rooms with special connection logic
 */
class FRoomConnectionManager : public IRoomConnectionManager
{
public:
    /**
     * Constructor - Service is standalone and uses UE_LOG for debugging
     */
    FRoomConnectionManager();
    
    // IRoomConnectionManager interface
    virtual bool TryPlaceRoom(const FRoomData& SourceRoom, 
                             int32 ConnectionIndex, 
                             FRoomData& NewRoom,
                             const TArray<FRoomData>& ExistingRooms,
                             const FBackroomGenerationConfig& Config,
                             ICollisionDetectionService* CollisionService,
                             TFunction<void(FRoomData&)> RoomCreator) override;
    
    virtual void ConnectRooms(FRoomData& Room1, 
                             int32 Connection1Index, 
                             FRoomData& Room2, 
                             int32 Connection2Index,
                             const FBackroomGenerationConfig& Config) override;
    
    virtual void CreateRoomConnections(FRoomData& Room,
                                      const FBackroomGenerationConfig& Config) override;
    
    virtual FVector CalculateConnectionPosition(const FRoomData& SourceRoom,
                                               int32 ConnectionIndex,
                                               const FRoomData& NewRoom,
                                               const FBackroomGenerationConfig& Config) override;

private:
    /**
     * Log debug information using UE_LOG
     * @param Message - Debug message to log
     * @param Config - Configuration to check logging settings
     */
    void LogDebug(const FString& Message, const FBackroomGenerationConfig& Config) const;
    
    /**
     * Calculate connection point position on a room's wall
     * @param Room - Room to calculate connection point for
     * @param WallSide - Which wall to calculate for
     * @param Config - Configuration with dimension settings
     * @return World position of connection point
     */
    FVector CalculateWallConnectionPoint(const FRoomData& Room, 
                                        EWallSide WallSide,
                                        const FBackroomGenerationConfig& Config) const;
    
    /**
     * Determine connection type and width based on room sizes and configuration
     * @param Room1 - First room in connection
     * @param Room2 - Second room in connection
     * @param Config - Configuration with connection ratios
     * @param OutConnectionType - Resulting connection type
     * @param OutConnectionWidth - Resulting connection width
     */
    void DetermineConnectionProperties(const FRoomData& Room1,
                                      const FRoomData& Room2,
                                      const FBackroomGenerationConfig& Config,
                                      EConnectionType& OutConnectionType,
                                      float& OutConnectionWidth) const;
    
    /**
     * Get wall dimensions for connection calculations
     * @param Room - Room to get wall size for
     * @param WallSide - Which wall to measure
     * @return Wall dimension in meters
     */
    float GetWallSize(const FRoomData& Room, EWallSide WallSide) const;
    
    /**
     * Handle special elevation logic for stairs connections
     * @param SourceRoom - Source room (potentially stairs)
     * @param NewRoom - New room to adjust elevation for
     * @param Config - Configuration settings
     */
    void HandleStairsElevation(const FRoomData& SourceRoom, 
                              FRoomData& NewRoom,
                              const FBackroomGenerationConfig& Config) const;
    
    /**
     * Create physical hole in room mesh for connection
     * @param Room - Room to create hole in
     * @param ConnectionIndex - Index of the connection
     * @param ConnectionType - Type of connection (doorway/opening)
     * @param ConnectionWidth - Width of the connection
     */
    void CreatePhysicalConnection(FRoomData& Room, int32 ConnectionIndex, 
                                 EConnectionType ConnectionType, float ConnectionWidth);
};