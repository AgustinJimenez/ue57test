#pragma once

#include "CoreMinimal.h"
#include "IRoomGenerationStrategy.h"

/**
 * Strategy for generating standard square-ish rooms
 * 
 * Characteristics:
 * - Square or near-square dimensions (2x2m to 8x8m)
 * - Equal or similar width/length ratios
 * - Standard 3m ceiling height
 * - 4 wall connections (North, South, East, West)
 */
class FStandardRoomStrategy : public IRoomGenerationStrategy
{
public:
    // IRoomGenerationStrategy interface
    virtual FRoomData GenerateRoom(const FBackroomGenerationConfig& Config, int32 RoomIndex) override;
    virtual FRoomData GenerateConnectedRoom(const FBackroomGenerationConfig& Config, 
                                           int32 RoomIndex,
                                           const FRoomData& SourceRoom, 
                                           int32 ConnectionIndex) override;
    virtual ERoomCategory GetRoomCategory() const override { return ERoomCategory::Room; }
    virtual FString GetStrategyName() const override { return TEXT("StandardRoom"); }
    
    virtual bool CanGenerateRoom(const FBackroomGenerationConfig& Config, 
                                const FRoomData* SourceRoom = nullptr) const override;

private:
    /**
     * Generate room dimensions for standard rooms
     * Creates square-ish rooms with configurable size range
     * 
     * @param Config - Configuration with room size limits
     * @param Random - Random stream for consistent generation
     * @param OutWidth - Generated width in meters
     * @param OutLength - Generated length in meters
     */
    void GenerateStandardRoomDimensions(const FBackroomGenerationConfig& Config,
                                       FRandomStream& Random,
                                       float& OutWidth, 
                                       float& OutLength) const;
    
    /**
     * Create room connections for standard rooms
     * Standard rooms have 4 connections (one on each wall)
     * 
     * @param Room - Room to add connections to
     */
    void CreateStandardRoomConnections(FRoomData& Room) const;
    
    /**
     * Validate room dimensions are within acceptable ranges
     * 
     * @param Config - Configuration with size limits
     * @param Width - Proposed width
     * @param Length - Proposed length
     * @return True if dimensions are valid
     */
    bool ValidateRoomDimensions(const FBackroomGenerationConfig& Config,
                               float Width, 
                               float Length) const;
};