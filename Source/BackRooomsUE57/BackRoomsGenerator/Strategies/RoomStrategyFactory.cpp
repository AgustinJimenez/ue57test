#include "RoomStrategyFactory.h"

FRoomStrategyFactory::FRoomStrategyFactory()
{
    // Initialize strategy instances for reuse
    StandardRoomStrategy = MakeUnique<FStandardRoomStrategy>();
    HallwayStrategy = MakeUnique<FHallwayStrategy>();
    StairsStrategy = MakeUnique<FStairsStrategy>();
}

FRoomStrategyFactory::~FRoomStrategyFactory()
{
    // Unique pointers handle cleanup automatically
}

IRoomGenerationStrategy* FRoomStrategyFactory::CreateStrategy(ERoomCategory Category)
{
    switch (Category)
    {
    case ERoomCategory::Room:
        return StandardRoomStrategy.Get();
    case ERoomCategory::Hallway:
        return HallwayStrategy.Get();
    case ERoomCategory::Stairs:
        return StairsStrategy.Get();
    default:
        // Default to standard room for unknown categories
        return StandardRoomStrategy.Get();
    }
}

IRoomGenerationStrategy* FRoomStrategyFactory::CreateStrategyByProbability(const FBackroomGenerationConfig& Config, 
                                                                          FRandomStream& Random)
{
    // Calculate normalized probabilities
    float StandardRatio, HallwayRatio, StairsRatio;
    CalculateNormalizedProbabilities(Config, StandardRatio, HallwayRatio, StairsRatio);
    
    // Generate random value for selection
    const float RandomValue = Random.RandRange(0.0f, 1.0f);
    
    // Select strategy based on cumulative probabilities
    if (RandomValue < StandardRatio)
    {
        return StandardRoomStrategy.Get();
    }
    else if (RandomValue < StandardRatio + HallwayRatio)
    {
        return HallwayStrategy.Get();
    }
    else
    {
        return StairsStrategy.Get();
    }
}

IRoomGenerationStrategy* FRoomStrategyFactory::CreateConnectedRoomStrategy(const FBackroomGenerationConfig& Config,
                                                                          const FRoomData& SourceRoom,
                                                                          FRandomStream& Random)
{
    // Start with base probabilities
    float StandardRatio, HallwayRatio, StairsRatio;
    CalculateNormalizedProbabilities(Config, StandardRatio, HallwayRatio, StairsRatio);
    
    // Apply context-based biases for better room flow
    ApplyContextBias(SourceRoom, StandardRatio, HallwayRatio, StairsRatio);
    
    // Renormalize after applying biases
    const float Total = StandardRatio + HallwayRatio + StairsRatio;
    if (Total > 0.0f)
    {
        StandardRatio /= Total;
        HallwayRatio /= Total;
        StairsRatio /= Total;
    }
    
    // Generate random value for selection
    const float RandomValue = Random.RandRange(0.0f, 1.0f);
    
    // Select strategy based on modified cumulative probabilities
    if (RandomValue < StandardRatio)
    {
        return StandardRoomStrategy.Get();
    }
    else if (RandomValue < StandardRatio + HallwayRatio)
    {
        return HallwayStrategy.Get();
    }
    else
    {
        return StairsStrategy.Get();
    }
}

bool FRoomStrategyFactory::ValidateStrategy(IRoomGenerationStrategy* Strategy,
                                           const FBackroomGenerationConfig& Config,
                                           const FRoomData* SourceRoom) const
{
    if (!Strategy)
    {
        return false;
    }
    
    // Use strategy's own validation method
    return Strategy->CanGenerateRoom(Config, SourceRoom);
}

TArray<IRoomGenerationStrategy*> FRoomStrategyFactory::GetAllStrategies() const
{
    return {
        StandardRoomStrategy.Get(),
        HallwayStrategy.Get(),
        StairsStrategy.Get()
    };
}

void FRoomStrategyFactory::CalculateNormalizedProbabilities(const FBackroomGenerationConfig& Config,
                                                           float& OutStandardRatio,
                                                           float& OutHallwayRatio,
                                                           float& OutStairsRatio) const
{
    // Get raw ratios from config
    OutStandardRatio = FMath::Max(0.0f, Config.RoomRatio);
    OutHallwayRatio = FMath::Max(0.0f, Config.HallwayRatio);
    OutStairsRatio = FMath::Max(0.0f, Config.StairRatio);
    
    // Calculate total for normalization
    const float Total = OutStandardRatio + OutHallwayRatio + OutStairsRatio;
    
    // Normalize to sum to 1.0, or fall back to equal distribution
    if (Total > 0.0f)
    {
        OutStandardRatio /= Total;
        OutHallwayRatio /= Total;
        OutStairsRatio /= Total;
    }
    else
    {
        // Fallback: equal distribution if all ratios are 0
        OutStandardRatio = 0.33f;
        OutHallwayRatio = 0.33f;
        OutStairsRatio = 0.34f;
    }
}

void FRoomStrategyFactory::ApplyContextBias(const FRoomData& SourceRoom,
                                           float& StandardRatio,
                                           float& HallwayRatio,
                                           float& StairsRatio) const
{
    // Apply biases based on source room type for better architectural flow
    switch (SourceRoom.Category)
    {
    case ERoomCategory::Room:
        // Standard rooms slightly favor hallways for connectivity
        HallwayRatio *= 1.3f;  // 30% boost to hallway probability
        StairsRatio *= 0.8f;   // 20% reduction to stair probability
        break;
        
    case ERoomCategory::Hallway:
        // Hallways favor rooms and stairs for destinations
        StandardRatio *= 1.4f; // 40% boost to room probability
        StairsRatio *= 1.2f;   // 20% boost to stair probability  
        HallwayRatio *= 0.6f;  // 40% reduction to prevent long hallway chains
        break;
        
    case ERoomCategory::Stairs:
        // Stairs strongly favor rooms for destinations
        StandardRatio *= 1.8f; // 80% boost to room probability
        HallwayRatio *= 1.1f;  // 10% boost to hallway probability
        StairsRatio *= 0.3f;   // 70% reduction to prevent stair chains
        break;
        
    default:
        // No bias for unknown categories
        break;
    }
    
    // Additional bias based on source room elevation for stairs
    if (SourceRoom.Category == ERoomCategory::Stairs)
    {
        // Reduce stair probability if already at extreme elevations
        const float AbsElevation = FMath::Abs(SourceRoom.Elevation);
        if (AbsElevation > 1500.0f) // 15m above/below start
        {
            StairsRatio *= 0.1f; // Strong bias against more stairs
            StandardRatio *= 1.5f; // Bias towards rooms for destinations
        }
        else if (AbsElevation > 800.0f) // 8m above/below start
        {
            StairsRatio *= 0.5f; // Moderate bias against more stairs
            StandardRatio *= 1.2f;
        }
    }
    
    // Prevent negative probabilities after bias application
    StandardRatio = FMath::Max(0.01f, StandardRatio);
    HallwayRatio = FMath::Max(0.01f, HallwayRatio);
    StairsRatio = FMath::Max(0.01f, StairsRatio);
}