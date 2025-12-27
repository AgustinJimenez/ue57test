#pragma once

#include "CoreMinimal.h"
#include "IRoomGenerationStrategy.h"

/**
 * Strategy for generating stairs rooms with elevation changes
 * 
 * Characteristics:
 * - Square dimensions similar to standard rooms (3x3m to 6x6m)
 * - Variable height based on stair configuration
 * - Elevation changes for multi-level generation
 * - 3 wall connections (stairs occupy one wall)
 * - Directional stairs (North, South, East, West)
 */
class FStairsStrategy : public IRoomGenerationStrategy
{
public:
    // IRoomGenerationStrategy interface
    virtual FRoomData GenerateRoom(const FBackroomGenerationConfig& Config, int32 RoomIndex) override;
    virtual FRoomData GenerateConnectedRoom(const FBackroomGenerationConfig& Config, 
                                           int32 RoomIndex,
                                           const FRoomData& SourceRoom, 
                                           int32 ConnectionIndex) override;
    virtual ERoomCategory GetRoomCategory() const override { return ERoomCategory::Stairs; }
    virtual FString GetStrategyName() const override { return TEXT("Stairs"); }
    
    virtual bool CanGenerateRoom(const FBackroomGenerationConfig& Config, 
                                const FRoomData* SourceRoom = nullptr) const override;

private:
    /**
     * Generate room dimensions for stairs rooms
     * Creates square-ish rooms suitable for stair placement
     * 
     * @param Config - Configuration with room size limits
     * @param Random - Random stream for consistent generation
     * @param OutWidth - Generated width in meters
     * @param OutLength - Generated length in meters
     */
    void GenerateStairsDimensions(const FBackroomGenerationConfig& Config,
                                 FRandomStream& Random,
                                 float& OutWidth, 
                                 float& OutLength) const;
    
    /**
     * Create room connections for stairs rooms
     * Stairs rooms have 3 connections (stairs occupy one wall)
     * 
     * @param Room - Room to add connections to
     * @param StairDirection - Which wall the stairs occupy
     */
    void CreateStairsConnections(FRoomData& Room, EWallSide StairDirection) const;
    
    /**
     * Calculate elevation change for stairs
     * Determines how much elevation change this stair room provides
     * 
     * @param Config - Configuration with stair settings
     * @param Random - Random stream for consistent generation
     * @param bGoingUp - Whether stairs go up or down
     * @return Elevation change in Unreal units
     */
    float CalculateStairElevation(const FBackroomGenerationConfig& Config,
                                 FRandomStream& Random,
                                 bool bGoingUp) const;
    
    /**
     * Determine stair direction based on connection context
     * Chooses which wall should contain the stairs
     * 
     * @param SourceRoom - Room that stairs will connect to
     * @param ConnectionIndex - Which wall connection on source room
     * @param Random - Random stream for consistent generation
     * @return Direction stairs should face
     */
    EWallSide DetermineStairDirection(const FRoomData& SourceRoom,
                                     int32 ConnectionIndex,
                                     FRandomStream& Random) const;
    
    /**
     * Validate stairs room dimensions are within acceptable ranges
     * Ensures rooms are large enough for stair placement
     * 
     * @param Config - Configuration with size limits
     * @param Width - Proposed width
     * @param Length - Proposed length
     * @return True if dimensions are valid for stairs
     */
    bool ValidateStairsDimensions(const FBackroomGenerationConfig& Config,
                                 float Width, 
                                 float Length) const;
};