#pragma once

#include "CoreMinimal.h"
#include "IRoomGenerationStrategy.h"

/**
 * Strategy for generating hallway rooms
 * 
 * Characteristics:
 * - Rectangular dimensions with higher length-to-width ratios
 * - Width: 2.5-5m, Length: 4-12m  
 * - Standard 3m ceiling height
 * - 4 wall connections (North, South, East, West)
 * - Optimized for corridor-like navigation
 */
class FHallwayStrategy : public IRoomGenerationStrategy
{
public:
    // IRoomGenerationStrategy interface
    virtual FRoomData GenerateRoom(const FBackroomGenerationConfig& Config, int32 RoomIndex) override;
    virtual FRoomData GenerateConnectedRoom(const FBackroomGenerationConfig& Config, 
                                           int32 RoomIndex,
                                           const FRoomData& SourceRoom, 
                                           int32 ConnectionIndex) override;
    virtual ERoomCategory GetRoomCategory() const override { return ERoomCategory::Hallway; }
    virtual FString GetStrategyName() const override { return TEXT("Hallway"); }
    
    virtual bool CanGenerateRoom(const FBackroomGenerationConfig& Config, 
                                const FRoomData* SourceRoom = nullptr) const override;

private:
    /**
     * Generate room dimensions for hallways
     * Creates rectangular corridors with configurable size ranges
     * 
     * @param Config - Configuration with room size limits
     * @param Random - Random stream for consistent generation
     * @param OutWidth - Generated width in meters (shorter dimension)
     * @param OutLength - Generated length in meters (longer dimension)
     */
    void GenerateHallwayDimensions(const FBackroomGenerationConfig& Config,
                                  FRandomStream& Random,
                                  float& OutWidth, 
                                  float& OutLength) const;
    
    /**
     * Create room connections for hallways
     * Hallways have 4 connections (one on each wall) like standard rooms
     * 
     * @param Room - Room to add connections to
     */
    void CreateHallwayConnections(FRoomData& Room) const;
    
    /**
     * Validate hallway dimensions are within acceptable ranges
     * Ensures proper rectangular aspect ratios for navigation
     * 
     * @param Config - Configuration with size limits
     * @param Width - Proposed width (should be shorter dimension)
     * @param Length - Proposed length (should be longer dimension)
     * @return True if dimensions are valid for hallway
     */
    bool ValidateHallwayDimensions(const FBackroomGenerationConfig& Config,
                                  float Width, 
                                  float Length) const;
};