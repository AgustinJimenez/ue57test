#pragma once

#include "CoreMinimal.h"
#include "ICollisionDetectionService.h"
#include "../Types.h"
#include "../GenerationConfig.h"

/**
 * Standard implementation of collision detection service
 * Handles bounding box collision detection with configurable buffers
 * 
 * Features:
 * - Bounding box collision with configurable buffer zones
 * - Elevation-aware collision detection for multi-level generation
 * - Detailed debug logging for troubleshooting placement issues
 * - Room validation for bounds checking
 */
class FCollisionDetectionService : public ICollisionDetectionService
{
public:
    /**
     * Constructor - Service is standalone and doesn't require external logger
     */
    FCollisionDetectionService();
    
    // ICollisionDetectionService interface
    virtual bool CheckRoomCollision(const FRoomData& TestRoom,
                                   const TArray<FRoomData>& ExistingRooms,
                                   const FBackroomGenerationConfig& Config) const override;
    
    virtual bool CheckRoomCollisionExcluding(const FRoomData& TestRoom,
                                            const TArray<FRoomData>& ExistingRooms,
                                            int32 ExcludeRoomIndex,
                                            const FBackroomGenerationConfig& Config) const override;
    
    virtual bool ValidateRoomBounds(const FRoomData& Room,
                                   const FBackroomGenerationConfig& Config) const override;

private:
    /**
     * Internal collision detection implementation
     * Shared logic between collision check methods
     * 
     * @param TestRoom - Room to test for collisions
     * @param ExistingRooms - Array of already placed rooms
     * @param ExcludeRoomIndex - Index to exclude (-1 for none)
     * @param Config - Configuration settings
     * @return True if collision detected
     */
    bool InternalCheckCollision(const FRoomData& TestRoom,
                               const TArray<FRoomData>& ExistingRooms,
                               int32 ExcludeRoomIndex,
                               const FBackroomGenerationConfig& Config) const;
    
    /**
     * Log collision detection debug information
     * Uses UE_LOG for independent logging
     * 
     * @param Message - Debug message to log
     * @param Config - Configuration to check logging settings
     */
    void LogDebug(const FString& Message, const FBackroomGenerationConfig& Config) const;
    
    /**
     * Create expanded bounding box with buffer
     * Applies collision buffer from configuration
     * 
     * @param Room - Room to get bounds for
     * @param Config - Configuration with buffer size
     * @return Expanded bounding box
     */
    FBox GetExpandedBounds(const FRoomData& Room, const FBackroomGenerationConfig& Config) const;
    
    /**
     * Check if two rooms have significant elevation difference
     * Determines if rooms are on different vertical levels
     * 
     * @param Room1 - First room
     * @param Room2 - Second room
     * @param MinSeparation - Minimum elevation difference to consider separate levels
     * @return True if rooms are on different levels
     */
    bool AreRoomsOnDifferentLevels(const FRoomData& Room1,
                                  const FRoomData& Room2,
                                  float MinSeparation = 200.0f) const; // 2m default
};