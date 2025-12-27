#pragma once

#include "CoreMinimal.h"
#include "../Types.h"
#include "../GenerationConfig.h"

/**
 * Interface for collision detection services
 * Abstraction for testing and future extensibility
 */
class ICollisionDetectionService
{
public:
    virtual ~ICollisionDetectionService() = default;
    
    /**
     * Check if a test room collides with any existing rooms
     * Used for initial room placement validation
     * 
     * @param TestRoom - Room to test for collisions
     * @param ExistingRooms - Array of already placed rooms
     * @param Config - Configuration settings for buffer sizes and debug logging
     * @return True if collision detected, false if placement is safe
     */
    virtual bool CheckRoomCollision(const FRoomData& TestRoom,
                                   const TArray<FRoomData>& ExistingRooms,
                                   const FBackroomGenerationConfig& Config) const = 0;
    
    /**
     * Check room collision while excluding a specific room from collision checks
     * Used when placing rooms connected to existing rooms
     * 
     * @param TestRoom - Room to test for collisions
     * @param ExistingRooms - Array of already placed rooms
     * @param ExcludeRoomIndex - Index of room to ignore during collision check
     * @param Config - Configuration settings for buffer sizes and debug logging
     * @return True if collision detected, false if placement is safe
     */
    virtual bool CheckRoomCollisionExcluding(const FRoomData& TestRoom,
                                            const TArray<FRoomData>& ExistingRooms,
                                            int32 ExcludeRoomIndex,
                                            const FBackroomGenerationConfig& Config) const = 0;
    
    /**
     * Validate that room bounds are reasonable and within world limits
     * Prevents invalid room configurations from being processed
     * 
     * @param Room - Room to validate
     * @param Config - Configuration with world size limits
     * @return True if room is valid for collision checking
     */
    virtual bool ValidateRoomBounds(const FRoomData& Room,
                                   const FBackroomGenerationConfig& Config) const = 0;
};