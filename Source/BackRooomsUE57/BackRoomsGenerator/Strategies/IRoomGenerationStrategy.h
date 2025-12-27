#pragma once

#include "CoreMinimal.h"
#include "../Types.h"
#include "../GenerationConfig.h"

/**
 * Abstract interface for room generation strategies
 * Implements Strategy Pattern for Open/Closed Principle compliance
 * 
 * Each room type (Room, Hallway, Stairs) has its own strategy implementation
 * Allows adding new room types without modifying existing code
 */
class IRoomGenerationStrategy
{
public:
    virtual ~IRoomGenerationStrategy() = default;
    
    /**
     * Generate a standalone room without connection context
     * Used for initial room creation and testing
     * 
     * @param Config - Generation configuration settings
     * @param RoomIndex - Sequential index of the room being generated
     * @return Generated room data ready for placement
     */
    virtual FRoomData GenerateRoom(const FBackroomGenerationConfig& Config, int32 RoomIndex) = 0;
    
    /**
     * Generate a room that connects to an existing room
     * Connection-aware generation for proper placement and sizing
     * 
     * @param Config - Generation configuration settings  
     * @param RoomIndex - Sequential index of the room being generated
     * @param SourceRoom - Existing room to connect to
     * @param ConnectionIndex - Which wall connection on the source room
     * @return Generated room data optimized for the connection
     */
    virtual FRoomData GenerateConnectedRoom(const FBackroomGenerationConfig& Config, 
                                           int32 RoomIndex,
                                           const FRoomData& SourceRoom, 
                                           int32 ConnectionIndex) = 0;
    
    /**
     * Get the room category this strategy handles
     * Used by factory for strategy selection
     * 
     * @return Room category enum value
     */
    virtual ERoomCategory GetRoomCategory() const = 0;
    
    /**
     * Get human-readable name for debugging and logging
     * 
     * @return Strategy name for logging purposes
     */
    virtual FString GetStrategyName() const = 0;
    
    /**
     * Validate if this strategy can generate a room with given constraints
     * Optional validation hook for complex generation rules
     * 
     * @param Config - Generation configuration settings
     * @param SourceRoom - Optional source room for connection validation
     * @return True if strategy can generate valid room
     */
    virtual bool CanGenerateRoom(const FBackroomGenerationConfig& Config, 
                                const FRoomData* SourceRoom = nullptr) const
    {
        // Default implementation - most strategies can always generate
        return true;
    }

protected:
    /**
     * Utility function for random number generation with seed
     * Creates consistent randomization for testing and debugging
     * 
     * @param RoomIndex - Room index for seed variation
     * @return Initialized random stream
     */
    FRandomStream CreateRandomStream(int32 RoomIndex) const
    {
        return FRandomStream(FDateTime::Now().GetTicks() + RoomIndex * 12345 + FMath::Rand());
    }
    
    /**
     * Initialize common room data properties
     * Reduces code duplication across strategy implementations
     * 
     * @param OutRoom - Room to initialize
     * @param Category - Room category
     * @param RoomIndex - Sequential room index
     * @param Config - Generation configuration
     */
    void InitializeBaseRoomData(FRoomData& OutRoom, 
                               ERoomCategory Category,
                               int32 RoomIndex, 
                               const FBackroomGenerationConfig& Config) const
    {
        OutRoom.Category = Category;
        OutRoom.RoomIndex = RoomIndex;
        OutRoom.Height = Config.StandardRoomHeight;
        OutRoom.Elevation = 0.0f; // Base elevation, can be overridden
        OutRoom.StairDirection = EWallSide::None; // Non-stair default
        OutRoom.Position = FVector::ZeroVector; // Set by placement logic
        OutRoom.Connections.Empty();
        OutRoom.RoomUnit = nullptr; // Set during room creation
    }
};