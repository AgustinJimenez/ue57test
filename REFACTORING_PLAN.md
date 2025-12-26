# BackRooms UE5.7 Refactoring Plan

## Current Status: Phase 1 - In Progress ✅

**Last Updated**: December 26, 2025  
**Session State**: Active - Creating configuration extraction

---

## 📋 Complete Refactoring Roadmap

### ✅ Phase 1: Extract Configuration Constants (IN PROGRESS)
**Goal**: Centralize all magic numbers and configuration values

**Status**: 
- ✅ Created `GenerationConfig.h` with centralized configuration
- ⏳ **NEXT**: Update `Main.h` to use `FBackroomGenerationConfig` 
- ⏳ **NEXT**: Update `Main.cpp` constructor to use config
- ⏳ **NEXT**: Replace all hardcoded values in generation methods

**Files to Modify**:
- [✅] `GenerationConfig.h` - Created
- [ ] `Main.h` - Replace individual config properties with single config struct
- [ ] `Main.cpp` - Update constructor and all methods to use config values
- [ ] `BaseRoom.cpp` - Use config for room size ranges

**Key Changes Needed**:
```cpp
// BEFORE (Main.h):
int32 TotalRooms = 100;
float RoomRatio = 0.30f;
float HallwayRatio = 0.30f;
// ... 15+ individual config properties

// AFTER (Main.h):
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
FBackroomGenerationConfig Config;
```

---

### ⏳ Phase 2: Create Strategy Interfaces (PENDING)
**Goal**: Apply Strategy Pattern for room generation (Open/Closed Principle)

**Files to Create**:
- [ ] `IRoomGenerationStrategy.h` - Abstract base for room generation
- [ ] `StandardRoomStrategy.h/.cpp` - Standard room generation logic
- [ ] `HallwayStrategy.h/.cpp` - Hallway generation logic  
- [ ] `StairsStrategy.h/.cpp` - Stairs generation logic
- [ ] `RoomStrategyFactory.h/.cpp` - Factory to create appropriate strategy

**Key Interfaces**:
```cpp
class IRoomGenerationStrategy 
{
public:
    virtual ~IRoomGenerationStrategy() = default;
    virtual FRoomData GenerateRoom(const FBackroomGenerationConfig& Config, int32 RoomIndex) = 0;
    virtual FRoomData GenerateConnectedRoom(const FBackroomGenerationConfig& Config, 
                                           int32 RoomIndex, 
                                           const FRoomData& SourceRoom, 
                                           int32 ConnectionIndex) = 0;
};
```

---

### ⏳ Phase 3: Extract Collision Detection Service (PENDING)
**Goal**: Single Responsibility - separate collision logic

**Files to Create**:
- [ ] `CollisionDetectionService.h/.cpp` - Centralized collision detection
- [ ] `ICollisionDetectionService.h` - Interface for testing

**Key Methods**:
```cpp
class FCollisionDetectionService
{
public:
    bool CheckRoomCollision(const FRoomData& TestRoom, 
                           const TArray<FRoomData>& ExistingRooms,
                           const FBackroomGenerationConfig& Config) const;
    
    bool CheckRoomCollisionExcluding(const FRoomData& TestRoom,
                                    const TArray<FRoomData>& ExistingRooms,
                                    int32 ExcludeRoomIndex,
                                    const FBackroomGenerationConfig& Config) const;
};
```

---

### ⏳ Phase 4: Extract Room Connection Manager (PENDING)
**Goal**: Separate connection and placement logic

**Files to Create**:
- [ ] `RoomConnectionManager.h/.cpp` - Handle room connections
- [ ] `IRoomConnectionManager.h` - Interface for testing

**Key Methods**:
```cpp
class FRoomConnectionManager
{
public:
    void ConnectRooms(FRoomData& Room1, int32 Connection1Index, 
                     FRoomData& Room2, int32 Connection2Index);
    
    bool TryPlaceRoom(const FRoomData& SourceRoom, int32 ConnectionIndex, 
                     FRoomData& NewRoom, const TArray<FRoomData>& ExistingRooms,
                     const FBackroomGenerationConfig& Config);
    
    FVector CalculateConnectionPosition(const FRoomData& SourceRoom, 
                                       int32 ConnectionIndex, 
                                       const FRoomData& NewRoom);
};
```

---

### ⏳ Phase 5: Extract Safety & Validation Services (PENDING)
**Goal**: Centralize safety checks and logging

**Files to Create**:
- [ ] `GenerationSafetyService.h/.cpp` - Time limits, iteration limits
- [ ] `BackroomLogger.h/.cpp` - Centralized logging
- [ ] `GenerationValidator.h/.cpp` - Validate generation results

**Key Classes**:
```cpp
class FGenerationSafetyService
{
private:
    double StartTime;
    int32 MainLoopCounter = 0;
    int32 ConnectionRetryCounter = 0;
    int32 PlacementAttemptCounter = 0;
    
public:
    bool CheckTimeLimit(const FBackroomGenerationConfig& Config) const;
    bool CheckIterationLimits(const FBackroomGenerationConfig& Config) const;
    void LogProgress(const FBackroomGenerationConfig& Config);
    void IncrementCounters(/* specific counter type */);
};
```

---

### ⏳ Phase 6: Refactor Main Generator Class (PENDING)
**Goal**: Dependency Injection, clean separation of concerns

**Target Main.h Structure**:
```cpp
class ABackRoomGenerator : public AActor
{
private:
    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
    FBackroomGenerationConfig Config;
    
    // Services (Dependency Injection)
    TUniquePtr<FCollisionDetectionService> CollisionService;
    TUniquePtr<FRoomConnectionManager> ConnectionManager;
    TUniquePtr<FGenerationSafetyService> SafetyService;
    TUniquePtr<FBackroomLogger> Logger;
    TUniquePtr<FRoomStrategyFactory> StrategyFactory;
    
    // Data
    UPROPERTY()
    TArray<UStandardRoom*> RoomUnits;
    
    UPROPERTY()
    TArray<FRoomData> GeneratedRooms;

public:
    // Simplified public interface
    UFUNCTION(BlueprintCallable, Category = "Generation")
    void GenerateBackrooms();
    
private:
    // Clean, focused private methods
    void InitializeServices();
    FRoomData CreateInitialRoom(const FVector& CharacterLocation);
    void GenerateProceduralRooms(); // Now ~20 lines instead of 200+
    void CleanupGeneration();
};
```

**Target Main.cpp Structure**:
```cpp
void ABackRoomGenerator::GenerateProceduralRooms()
{
    // Initialize
    const FRoomData InitialRoom = CreateInitialRoom(GetCharacterLocation());
    GeneratedRooms.Add(InitialRoom);
    TArray<int32> AvailableRooms = {0};
    
    SafetyService->StartGeneration();
    
    // Main generation loop (simplified)
    for (int32 RoomIndex = 1; RoomIndex < Config.TotalRooms && AvailableRooms.Num() > 0; RoomIndex++)
    {
        if (!SafetyService->CheckLimits(Config)) break;
        
        if (TryGenerateConnectedRoom(RoomIndex, AvailableRooms))
        {
            Logger->LogRoomPlacement(GeneratedRooms.Last(), RoomIndex);
        }
        else
        {
            Logger->LogPlacementFailure(RoomIndex);
            CleanupAvailableRooms(AvailableRooms);
        }
    }
    
    Logger->LogGenerationComplete(GeneratedRooms.Num(), SafetyService->GetElapsedTime());
}
```

---

### ⏳ Phase 7: Add Unit Tests (PENDING)
**Goal**: Test each service independently

**Files to Create**:
- [ ] `Tests/CollisionDetectionServiceTest.cpp`
- [ ] `Tests/RoomConnectionManagerTest.cpp`
- [ ] `Tests/GenerationConfigTest.cpp`
- [ ] `Tests/RoomStrategiesTest.cpp`

---

### ⏳ Phase 8: Setup Linting & Code Standards (PENDING)
**Goal**: Enforce consistent code quality

**Files to Create**:
- [ ] `.clang-format` - Code formatting rules
- [ ] `.clang-tidy` - Static analysis rules
- [ ] `pre-commit-hook.sh` - Git hooks for automatic formatting
- [ ] `CODING_STANDARDS.md` - Team coding guidelines

---

## 🎯 Benefits After Completion

### SOLID Compliance:
- ✅ **Single Responsibility**: Each class has one clear purpose
- ✅ **Open/Closed**: Easy to add new room types via strategy pattern
- ✅ **Liskov Substitution**: Strategies are interchangeable
- ✅ **Interface Segregation**: Small, focused interfaces
- ✅ **Dependency Inversion**: Depend on abstractions, not concrete classes

### Code Quality:
- ✅ **Testable**: Each service can be unit tested independently
- ✅ **Maintainable**: Small, focused classes easier to understand
- ✅ **Extensible**: Add new features without modifying existing code
- ✅ **Configurable**: All settings in one place
- ✅ **Debuggable**: Clear separation of concerns makes debugging easier

---

## 🚨 Current Session Recovery Instructions

**If session ends abruptly, resume with**:

1. **Complete Phase 1**: 
   - Update `Main.h` to use `FBackroomGenerationConfig Config;` instead of individual properties
   - Update `Main.cpp` constructor to initialize config
   - Replace hardcoded values with `Config.PropertyName`

2. **Test Compilation**: 
   - Build project to ensure configuration changes work
   - Fix any compilation errors from config migration

3. **Continue to Phase 2**:
   - Create strategy interfaces for room generation
   - Start with `IRoomGenerationStrategy.h`

**Current Files Status**:
- ✅ `GenerationConfig.h` - Complete
- ⏳ `Main.h` - Needs config integration  
- ⏳ `Main.cpp` - Needs config integration

**Last Working Commit**: `c404579` - Fix variable shadowing compilation errors