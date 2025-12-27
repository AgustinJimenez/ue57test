#pragma once

#include "CoreMinimal.h"
#include "../Types.h"
#include "../GenerationConfig.h"

/**
 * Interface for generation orchestration services
 * Handles the main procedural generation loop with safety monitoring
 */
class IGenerationOrchestrator
{
public:
    virtual ~IGenerationOrchestrator() = default;
    
    /**
     * Execute the main procedural generation loop
     * Creates rooms, hallways, and stairs using configured ratios
     * 
     * @param InitialRoom - Starting room for generation
     * @param OutGeneratedRooms - Array to populate with generated rooms
     * @param Config - Configuration settings
     * @param CollisionService - Service for collision detection
     * @param ConnectionManager - Service for room connections
     * @param RoomCreator - Callback function to create actual UE room units
     * @return Number of rooms successfully generated
     */
    virtual int32 ExecuteProceduralGeneration(
        const FRoomData& InitialRoom,
        TArray<FRoomData>& OutGeneratedRooms,
        const FBackroomGenerationConfig& Config,
        class ICollisionDetectionService* CollisionService,
        class IRoomConnectionManager* ConnectionManager,
        TFunction<void(FRoomData&)> RoomCreator
    ) = 0;
    
    /**
     * Get generation statistics from the last run
     * @param OutMainLoops - Number of main loop iterations
     * @param OutConnectionRetries - Number of connection retry attempts  
     * @param OutPlacementAttempts - Number of placement attempts
     * @param OutElapsedTime - Total generation time in seconds
     */
    virtual void GetGenerationStats(int32& OutMainLoops, int32& OutConnectionRetries, 
                                  int32& OutPlacementAttempts, double& OutElapsedTime) const = 0;
    
    /**
     * Check if the last generation was stopped due to safety limits
     * @return True if generation stopped due to time or iteration limits
     */
    virtual bool WasStoppedBySafety() const = 0;
};