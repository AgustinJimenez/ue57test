# BackRooms Performance Optimizations

## Overview
Documentation of potential performance improvements for the procedural BackRooms generation system.

## Current Performance Characteristics
- **Small batches (1-50 rooms)**: Fast generation (~1-5 seconds)
- **Large batches (100-200 rooms)**: Slower due to collision detection complexity
- **Collision checking**: O(n) per room attempt, becomes O(n²) overall

## Strategy 1: Progressive/Streaming Generation (RECOMMENDED)

### Concept
Generate rooms in batches while the user plays, providing immediate gameplay while expanding the world.

### Implementation Plan
```cpp
// Add to Main.h
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
int32 InitialBatchSize = 50; // Rooms generated immediately

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation") 
int32 StreamingBatchSize = 10; // Rooms per streaming batch

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
float StreamingInterval = 3.0f; // Seconds between batches

// Streaming state
FTimerHandle StreamingTimer;
int32 CurrentRoomCount = 0;
int32 TargetRoomCount = 500; // Total rooms to eventually generate
bool bStreamingActive = false;
```

### Benefits
- ✅ **Immediate gameplay**: User can start exploring right away
- ✅ **Seamless expansion**: World grows as player explores
- ✅ **No frame drops**: Small batches don't cause stuttering
- ✅ **Scalable**: Can generate thousands of rooms over time
- ✅ **Controllable**: Adjustable batch sizes and intervals

### User Experience
1. **Game starts**: 50 rooms generated instantly (~2-3 seconds)
2. **Player explores**: While playing, 10 more rooms added every 3 seconds
3. **Endless world**: Rooms continue generating in background
4. **No interruption**: Player never notices the generation happening

### Code Structure
```cpp
void BeginPlay() override
{
    // Generate initial batch for immediate play
    GenerateInitialBatch(InitialBatchSize);
    
    // Start streaming generation
    StartStreamingGeneration();
}

void GenerateInitialBatch(int32 BatchSize)
{
    TotalRooms = BatchSize;
    GenerateProceduralRooms();
    CurrentRoomCount = BatchSize;
}

void StartStreamingGeneration()
{
    if (CurrentRoomCount < TargetRoomCount)
    {
        bStreamingActive = true;
        GetWorld()->GetTimerManager().SetTimer(
            StreamingTimer, 
            this, 
            &ABackRoomGenerator::GenerateStreamingBatch, 
            StreamingInterval, 
            true // Loop
        );
    }
}

void GenerateStreamingBatch()
{
    if (CurrentRoomCount >= TargetRoomCount)
    {
        StopStreamingGeneration();
        return;
    }
    
    // Generate next batch
    int32 RoomsToGenerate = FMath::Min(StreamingBatchSize, TargetRoomCount - CurrentRoomCount);
    GenerateRoomBatch(RoomsToGenerate);
    CurrentRoomCount += RoomsToGenerate;
}
```

## Strategy 2: Spatial Partitioning Optimization (FUTURE)

### Concept
Optimize collision detection using spatial grid to handle large room counts efficiently.

### Current Collision Performance
- **Room #1**: Check against 0 rooms
- **Room #50**: Check against 49 rooms  
- **Room #200**: Check against 199 rooms (up to 10 retries = 1,990 checks)

### Optimized Collision Performance
- **Any room**: Check against ~4 nearby rooms only
- **Room #200**: Up to 40 collision checks (vs 1,990 current)

### Implementation Plan
```cpp
// Add to Main.h private:
TMap<FIntPoint, TArray<int32>> SpatialGrid; // Grid cell -> room indices
float GridCellSize = 2000.0f; // 20m cells (larger than biggest room)

// Optimized collision checking
bool CheckRoomCollisionOptimized(const FRoomData& TestRoom) const
{
    FBox TestBounds = TestRoom.GetBoundingBox();
    TSet<int32> NearbyRooms = GetRoomsInBounds(TestBounds);
    
    for (int32 RoomIndex : NearbyRooms) // Much smaller set!
    {
        if (TestBounds.Intersect(GeneratedRooms[RoomIndex].GetBoundingBox()))
            return true;
    }
    return false;
}

TSet<int32> GetRoomsInBounds(const FBox& Bounds) const
{
    TSet<int32> NearbyRooms;
    
    // Get grid cell range that overlaps with bounds
    FIntPoint MinCell = WorldToGridCell(Bounds.Min);
    FIntPoint MaxCell = WorldToGridCell(Bounds.Max);
    
    // Check all overlapping cells
    for (int32 X = MinCell.X; X <= MaxCell.X; X++)
    {
        for (int32 Y = MinCell.Y; Y <= MaxCell.Y; Y++)
        {
            FIntPoint CellKey(X, Y);
            if (const TArray<int32>* RoomsInCell = SpatialGrid.Find(CellKey))
            {
                for (int32 RoomIndex : *RoomsInCell)
                {
                    NearbyRooms.Add(RoomIndex);
                }
            }
        }
    }
    
    return NearbyRooms;
}
```

### Benefits
- ✅ **100x faster**: Collision detection becomes O(1) instead of O(n)
- ✅ **Scales infinitely**: Works with thousands of rooms
- ✅ **No algorithm changes**: Keeps all creative generation logic
- ✅ **Backwards compatible**: Can be added without breaking existing code

### When to Implement
- When single-batch generation of 200+ rooms is needed
- When streaming generation batches become too slow
- When real-time room deletion/modification is required

## Strategy 3: Memory Optimizations (IMPLEMENTED)

### Pre-allocation ✅ IMPLEMENTED
```cpp
// Reserve memory upfront to avoid reallocations
RoomUnits.Empty();
RoomUnits.Reserve(TotalRooms);
GeneratedRooms.Empty();
GeneratedRooms.Reserve(TotalRooms);
AvailableRooms.Reserve(TotalRooms);
```

**Status**: Implemented in `GenerateProceduralRooms()` and `GenerateBackrooms()`
**Impact**: Eliminates 50-200 memory reallocations during generation

### Cached Random Streams
```cpp
// Cache random stream instead of creating new ones
FRandomStream CachedRandom;
```

### Conditional Logging
```cpp
// Reduce string operations in tight loops
#define ROOM_LOG_LEVEL 1 // 0=none, 1=important, 2=verbose
```

## Recommended Implementation Order

1. **Phase 1**: Implement **Progressive/Streaming Generation** (immediate benefits)
2. **Phase 2**: Add **Memory Optimizations** (easy wins)  
3. **Phase 3**: Implement **Spatial Partitioning** (when massive scale needed)

## Performance Targets

### Current (Single Batch)
- 50 rooms: ~2-3 seconds ✅
- 100 rooms: ~8-12 seconds ⚠️
- 200 rooms: ~30-60 seconds ❌

### With Streaming (Progressive)
- Initial 50 rooms: ~2-3 seconds ✅
- Total 500 rooms: Generated over ~2-3 minutes of gameplay ✅
- User experience: Seamless, no waiting ✅

### With Spatial Optimization (Future)
- 1000+ rooms: Single batch in ~10-20 seconds ✅
- Real-time generation: Add rooms on-demand ✅

## Notes
- Streaming generation provides better UX than faster single-batch generation
- Spatial optimization becomes valuable for real-time editing or massive worlds
- Memory optimizations are easy wins that can be implemented alongside streaming