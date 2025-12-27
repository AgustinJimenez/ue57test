#pragma once

#include "CoreMinimal.h"
#include "IGenerationOrchestrator.h"
#include "ICollisionDetectionService.h"
#include "IRoomConnectionManager.h"
#include "../Types.h"
#include "../GenerationConfig.h"
#include "../Strategies/IRoomGenerationStrategy.h"

/**
 * Standard implementation of generation orchestration
 * Handles the main procedural generation loop with safety monitoring
 * 
 * Features:
 * - Multi-level safety monitoring (time limits, iteration limits)
 * - Sophisticated room category selection with constraints
 * - Retry logic for failed placements
 * - Performance statistics tracking
 * - Emergency exit mechanisms for infinite loop prevention
 */
class FGenerationOrchestrator : public IGenerationOrchestrator
{
public:
    /**
     * Constructor - Service is standalone and uses UE_LOG for debugging
     */
    FGenerationOrchestrator();
    
    // IGenerationOrchestrator interface
    virtual int32 ExecuteProceduralGeneration(
        const FRoomData& InitialRoom,
        TArray<FRoomData>& OutGeneratedRooms,
        const FBackroomGenerationConfig& Config,
        ICollisionDetectionService* CollisionService,
        IRoomConnectionManager* ConnectionManager,
        TFunction<void(FRoomData&)> RoomCreator
    ) override;
    
    virtual void GetGenerationStats(int32& OutMainLoops, int32& OutConnectionRetries, 
                                  int32& OutPlacementAttempts, double& OutElapsedTime) const override;
    
    virtual bool WasStoppedBySafety() const override;

private:
    // Generation statistics
    int32 MainLoopCounter = 0;
    int32 ConnectionRetryCounter = 0;
    int32 PlacementAttemptCounter = 0;
    double StartTime = 0.0;
    double ElapsedTime = 0.0;
    bool bStoppedBySafety = false;
    
    /**
     * Log debug information using UE_LOG
     * @param Message - Debug message to log
     * @param Config - Configuration to check logging settings
     */
    void LogDebug(const FString& Message, const FBackroomGenerationConfig& Config) const;
    
    /**
     * Check if any safety limits have been exceeded
     * @param Config - Configuration with safety limit settings
     * @return True if generation should stop due to safety limits
     */
    bool CheckSafetyLimits(const FBackroomGenerationConfig& Config);
    
    /**
     * Initialize generation counters and timing
     */
    void InitializeGeneration();
    
    /**
     * Determine room category based on ratios and constraints
     * @param SourceRoom - Room we're connecting from
     * @param Config - Configuration with category ratios
     * @param Random - Random stream for selection
     * @return Selected room category and description string
     */
    TPair<ERoomCategory, FString> DetermineRoomCategory(
        const FRoomData& SourceRoom,
        const FBackroomGenerationConfig& Config,
        FRandomStream& Random) const;
    
    /**
     * Try to generate a room connected to an existing room
     * @param RoomIndex - Index of room being created
     * @param SourceRoom - Existing room to connect to
     * @param ConnectionIndex - Which connection point to use
     * @param OutGeneratedRooms - Array of generated rooms
     * @param Config - Configuration settings
     * @param CollisionService - Collision detection service
     * @param ConnectionManager - Connection management service
     * @param RoomCreator - Function to create actual room units
     * @param Random - Random stream for generation
     * @return True if room was successfully placed
     */
    bool TryGenerateConnectedRoom(
        int32 RoomIndex,
        const FRoomData& SourceRoom,
        int32 ConnectionIndex,
        TArray<FRoomData>& OutGeneratedRooms,
        const FBackroomGenerationConfig& Config,
        ICollisionDetectionService* CollisionService,
        IRoomConnectionManager* ConnectionManager,
        TFunction<void(FRoomData&)> RoomCreator,
        FRandomStream& Random);
    
    /**
     * Clean up rooms with no available connections from the available list
     * @param AvailableRooms - List of room indices with available connections
     * @param GeneratedRooms - All generated rooms
     */
    void CleanupAvailableRooms(TArray<int32>& AvailableRooms, 
                              const TArray<FRoomData>& GeneratedRooms) const;
    
    /**
     * Generate a room using the appropriate strategy
     * @param Category - Room category to generate
     * @param RoomIndex - Index for the new room
     * @param SourceRoom - Source room for context (stairs generation)
     * @param ConnectionIndex - Connection index for context (stairs generation)
     * @return Generated room data
     */
    FRoomData GenerateRoomOfCategory(ERoomCategory Category, int32 RoomIndex,
                                    const FRoomData* SourceRoom = nullptr,
                                    int32 ConnectionIndex = -1) const;
    
    /**
     * Calculate the opposite wall index for room connections
     * @param WallIndex - Original wall connection index (0-3)
     * @return Opposite wall index for proper room connection
     */
    int32 GetOppositeWallIndex(int32 WallIndex) const;
};