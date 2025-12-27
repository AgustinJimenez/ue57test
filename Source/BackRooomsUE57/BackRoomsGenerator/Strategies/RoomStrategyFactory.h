#pragma once

#include "CoreMinimal.h"
#include "IRoomGenerationStrategy.h"
#include "StandardRoomStrategy.h"
#include "HallwayStrategy.h"
#include "StairsStrategy.h"

/**
 * Factory for creating room generation strategies
 * Implements Factory Pattern for strategy creation and management
 * 
 * Handles probability-based strategy selection and strategy caching
 * Supports configuration-driven strategy selection ratios
 */
class FRoomStrategyFactory
{
public:
    /**
     * Constructor
     * Initializes strategy instances for reuse
     */
    FRoomStrategyFactory();
    
    /**
     * Destructor
     * Cleans up strategy instances
     */
    ~FRoomStrategyFactory();

    /**
     * Create strategy based on room category
     * Returns appropriate strategy for specific room type
     * 
     * @param Category - Room category to create strategy for
     * @return Pointer to strategy instance, or nullptr if invalid category
     */
    IRoomGenerationStrategy* CreateStrategy(ERoomCategory Category);
    
    /**
     * Create strategy based on configuration probabilities
     * Uses configured ratios to randomly select appropriate strategy
     * 
     * @param Config - Configuration with room generation ratios
     * @param Random - Random stream for consistent selection
     * @return Pointer to selected strategy instance
     */
    IRoomGenerationStrategy* CreateStrategyByProbability(const FBackroomGenerationConfig& Config, 
                                                        FRandomStream& Random);
    
    /**
     * Create strategy for connected room generation
     * Considers context from source room for better room flow
     * 
     * @param Config - Configuration with room generation ratios
     * @param SourceRoom - Existing room that new room will connect to
     * @param Random - Random stream for consistent selection
     * @return Pointer to contextually appropriate strategy instance
     */
    IRoomGenerationStrategy* CreateConnectedRoomStrategy(const FBackroomGenerationConfig& Config,
                                                        const FRoomData& SourceRoom,
                                                        FRandomStream& Random);
    
    /**
     * Validate if a strategy can generate rooms with given configuration
     * Checks strategy-specific generation requirements
     * 
     * @param Strategy - Strategy to validate
     * @param Config - Configuration to validate against
     * @param SourceRoom - Optional source room for connection validation
     * @return True if strategy can generate valid rooms
     */
    bool ValidateStrategy(IRoomGenerationStrategy* Strategy,
                         const FBackroomGenerationConfig& Config,
                         const FRoomData* SourceRoom = nullptr) const;
    
    /**
     * Get all available strategies
     * Returns array of all strategy instances for validation or testing
     * 
     * @return Array of strategy pointers
     */
    TArray<IRoomGenerationStrategy*> GetAllStrategies() const;

private:
    // Strategy instances (created once, reused for performance)
    TUniquePtr<FStandardRoomStrategy> StandardRoomStrategy;
    TUniquePtr<FHallwayStrategy> HallwayStrategy;
    TUniquePtr<FStairsStrategy> StairsStrategy;
    
    /**
     * Normalize probabilities to ensure they sum to 1.0
     * Handles configuration edge cases where ratios don't add up correctly
     * 
     * @param Config - Configuration with room ratios
     * @param OutStandardRatio - Normalized standard room probability
     * @param OutHallwayRatio - Normalized hallway probability  
     * @param OutStairsRatio - Normalized stairs probability
     */
    void CalculateNormalizedProbabilities(const FBackroomGenerationConfig& Config,
                                         float& OutStandardRatio,
                                         float& OutHallwayRatio,
                                         float& OutStairsRatio) const;
    
    /**
     * Apply context-based strategy selection biases
     * Modifies probabilities based on source room type for better flow
     * 
     * @param SourceRoom - Room that new room will connect to
     * @param StandardRatio - Standard room probability (modified by reference)
     * @param HallwayRatio - Hallway probability (modified by reference)
     * @param StairsRatio - Stairs probability (modified by reference)
     */
    void ApplyContextBias(const FRoomData& SourceRoom,
                         float& StandardRatio,
                         float& HallwayRatio,
                         float& StairsRatio) const;
};