# Coding Standards - BackRooms UE5.7 Project

## Overview

This document defines coding standards for the BackRooms UE5.7 procedural generation project. These standards ensure code consistency, maintainability, and alignment with Unreal Engine best practices.

## 🛠️ Tools Setup

### Required Tools
- **clang-format**: Code formatting (`brew install clang-format`)
- **clang-tidy**: Static analysis (`brew install llvm`)

### Configuration Files
- `.clang-format`: Automatic code formatting rules
- `.clang-tidy`: Static analysis and linting rules  
- `pre-commit-hook.sh`: Automated pre-commit checks

### Installation Instructions
```bash
# Install tools
brew install clang-format llvm

# Setup pre-commit hook (run from project root)
cp pre-commit-hook.sh .git/hooks/pre-commit
chmod +x .git/hooks/pre-commit

# Test formatting
clang-format -i Source/**/*.cpp Source/**/*.h

# Test static analysis
clang-tidy Source/**/*.cpp -- -std=c++17 -ISource
```

## 📋 Naming Conventions

### Unreal Engine Naming Standards

#### Classes and Structs
- **Classes**: `CamelCase` (e.g., `ABackRoomGenerator`, `UStandardRoom`)
- **Structs**: `FCamelCase` with `F` prefix (e.g., `FRoomData`, `FBackroomGenerationConfig`)
- **Enums**: `ECamelCase` with `E` prefix (e.g., `ERoomCategory`, `EWallSide`)
- **Interfaces**: `ICamelCase` with `I` prefix (e.g., `ICollisionDetectionService`)

#### Variables and Functions
- **Functions**: `CamelCase` (e.g., `GenerateProceduralRooms`, `CheckRoomCollision`)
- **Variables**: `CamelCase` (e.g., `TotalRooms`, `RoomWidth`, `bIsValid`)
- **Member Variables**: No special prefix - use descriptive names
- **Boolean Variables**: `b` prefix (e.g., `bShowNumbers`, `bEmergencyExit`)
- **Constants**: `CamelCase` (e.g., `MaxSafetyIterations`, `DefaultRoomHeight`)

#### Files and Directories
- **Header Files**: `.h` extension (e.g., `Main.h`, `GenerationConfig.h`)
- **Source Files**: `.cpp` extension (e.g., `Main.cpp`, `StandardRoom.cpp`)
- **Directories**: `CamelCase` (e.g., `Services`, `Strategies`, `RoomUnit`)

### Examples
```cpp
// ✅ Good Examples
class ABackRoomGenerator : public AActor {};
struct FRoomData {};
enum class ERoomCategory { Room, Hallway, Stairs };
interface ICollisionDetectionService {};

bool bShowNumbers = true;
int32 TotalRooms = 100;
float RoomWidth = 5.0f;
TArray<FRoomData> GeneratedRooms;

void GenerateProceduralRooms();
bool CheckRoomCollision(const FRoomData& TestRoom);

// ❌ Bad Examples  
class backroom_generator {};  // Wrong case
struct roomData {};          // Missing F prefix
enum roomType {};           // Missing E prefix, wrong case
bool showNumbers;           // Missing b prefix
int total_rooms;            // Wrong case
void generate_rooms();      // Wrong case
```

## 🏗️ Code Structure and Architecture

### SOLID Principles Implementation

Our codebase follows SOLID principles through service-oriented architecture:

#### Single Responsibility Principle
- **CollisionDetectionService**: Only handles spatial collision detection
- **RoomConnectionManager**: Only manages room connections and placement
- **GenerationOrchestrator**: Only orchestrates the generation flow

#### Open/Closed Principle
- **Strategy Pattern**: Add new room types via `IRoomGenerationStrategy` without modifying existing code
- **Interface Segregation**: Small, focused interfaces (`ICollisionDetectionService`, etc.)

#### Dependency Inversion
- Services depend on interfaces, not concrete implementations
- Main class uses dependency injection pattern

### Service Architecture Example
```cpp
// ✅ Good: Dependency injection with interfaces
class ABackRoomGenerator : public AActor
{
private:
    TUniquePtr<ICollisionDetectionService> CollisionService;
    TUniquePtr<IRoomConnectionManager> ConnectionManager;
    TUniquePtr<IGenerationOrchestrator> GenerationOrchestrator;

public:
    ABackRoomGenerator()
    {
        CollisionService = MakeUnique<FCollisionDetectionService>();
        ConnectionManager = MakeUnique<FRoomConnectionManager>();
        GenerationOrchestrator = MakeUnique<FGenerationOrchestrator>();
    }
};
```

## 📝 Code Formatting

### Braces and Indentation
- **Brace Style**: Allman style (braces on new lines)
- **Indentation**: 4 spaces (tabs for indentation, spaces for alignment)
- **Line Length**: 120 characters maximum

```cpp
// ✅ Good: Allman brace style
void GenerateProceduralRooms()
{
    if (Config.TotalRooms > 0)
    {
        for (int32 i = 0; i < Config.TotalRooms; ++i)
        {
            GenerateRoom(i);
        }
    }
}

// ❌ Bad: Wrong brace placement
void GenerateProceduralRooms() {
    if (Config.TotalRooms > 0) {
        // ...
    }
}
```

### Function Declarations
- **Parameter Alignment**: Align parameters when breaking lines
- **Const Correctness**: Use `const` for read-only parameters and methods

```cpp
// ✅ Good: Parameter alignment and const correctness
bool CheckRoomCollisionExcluding(const FRoomData& TestRoom,
                                const TArray<FRoomData>& ExistingRooms,
                                int32 ExcludeRoomIndex,
                                const FBackroomGenerationConfig& Config) const;

// ❌ Bad: Poor alignment and missing const
bool CheckRoomCollisionExcluding(FRoomData TestRoom, TArray<FRoomData> ExistingRooms, int32 ExcludeRoomIndex);
```

### Include Organization
- **Order**: Unreal headers, system headers, project headers
- **Grouping**: Related headers together
- **Forward Declarations**: Use when possible to reduce compile dependencies

```cpp
// ✅ Good: Organized includes
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

#include <memory>
#include <vector>

#include "Types.h"
#include "GenerationConfig.h"
#include "Services/ICollisionDetectionService.h"

#include "Main.generated.h"
```

## 🧪 Testing and Quality

### Unit Testing Requirements
- **Service Testing**: Each service must have unit tests
- **Strategy Testing**: Each room generation strategy must have tests
- **Mocking**: Use interfaces for easy mocking in tests

### Static Analysis Rules
- **clang-tidy**: All checks must pass before commit
- **Performance Checks**: No unnecessary copies or allocations in hot paths
- **Memory Safety**: Proper use of smart pointers and RAII

### Code Review Checklist
- [ ] Follows naming conventions
- [ ] Uses appropriate Unreal Engine types (`int32`, `float`, `TArray`, etc.)
- [ ] Has proper const correctness
- [ ] Includes necessary UPROPERTY/UFUNCTION macros
- [ ] Passes clang-format and clang-tidy checks
- [ ] Has clear, descriptive variable and function names
- [ ] Follows single responsibility principle
- [ ] Includes appropriate error handling

## 🔍 Performance Guidelines

### Unreal Engine Specific
- **Use UE Types**: Prefer `int32` over `int`, `TArray` over `std::vector`
- **Memory Management**: Use `TUniquePtr`, `TSharedPtr` for automatic cleanup
- **String Handling**: Use `FString`, `FName`, `FText` appropriately
- **Avoid**: Raw pointers, manual memory management, C-style arrays

### Performance Patterns
```cpp
// ✅ Good: Reserve array capacity, use const references
void ProcessRooms(const TArray<FRoomData>& Rooms)
{
    TArray<FProcessedRoom> Results;
    Results.Reserve(Rooms.Num()); // Pre-allocate
    
    for (const FRoomData& Room : Rooms) // Const reference iteration
    {
        Results.Add(ProcessRoom(Room));
    }
}

// ❌ Bad: No reservation, unnecessary copies
void ProcessRooms(TArray<FRoomData> Rooms) // Copy instead of reference
{
    TArray<FProcessedRoom> Results; // No reservation
    
    for (FRoomData Room : Rooms) // Copy on each iteration
    {
        Results.Add(ProcessRoom(Room));
    }
}
```

## 📚 Documentation Standards

### Header Comments
```cpp
/**
 * Centralized collision detection service for room placement validation.
 * 
 * Provides efficient spatial collision checking with configurable buffer zones
 * and support for excluding specific rooms from collision tests.
 * 
 * Used by GenerationOrchestrator to validate room placement during generation.
 */
class FCollisionDetectionService : public ICollisionDetectionService
{
    // ...
};
```

### Function Comments
```cpp
/**
 * Checks if a test room collides with any existing rooms, excluding one specific room.
 * 
 * @param TestRoom - Room data to test for collisions
 * @param ExistingRooms - Array of already placed rooms to check against
 * @param ExcludeRoomIndex - Index of room to ignore during collision check (-1 for none)
 * @param Config - Generation configuration containing buffer zone settings
 * @return true if collision detected, false if placement is safe
 */
bool CheckRoomCollisionExcluding(const FRoomData& TestRoom,
                                const TArray<FRoomData>& ExistingRooms,
                                int32 ExcludeRoomIndex,
                                const FBackroomGenerationConfig& Config) const override;
```

## 🚀 Git Workflow Integration

### Pre-commit Hooks
The `pre-commit-hook.sh` automatically:
1. **Formats code** with clang-format
2. **Runs static analysis** with clang-tidy  
3. **Checks basic file standards** (trailing whitespace, newlines)
4. **Prevents commits** with formatting or analysis issues

### Manual Commands
```bash
# Format all source files
find Source -name "*.cpp" -o -name "*.h" | xargs clang-format -i

# Run static analysis
clang-tidy Source/**/*.cpp -- -std=c++17 -ISource

# Check specific file
clang-tidy Source/BackRooomsUE57/BackRoomsGenerator/Main.cpp -- -std=c++17 -ISource

# Bypass pre-commit checks (use sparingly)
git commit --no-verify -m "Emergency commit"
```

## ✅ Benefits of These Standards

### Code Quality
- **Consistency**: All code follows same patterns and conventions
- **Readability**: Clear naming and structure makes code self-documenting
- **Maintainability**: SOLID principles make code easy to modify and extend

### Team Collaboration
- **Automated Enforcement**: Tools prevent style inconsistencies
- **Reduced Review Time**: Focus on logic instead of formatting
- **Knowledge Sharing**: Standards document team agreements

### Unreal Engine Integration
- **Engine Compatibility**: Follows Epic Games coding standards
- **Performance**: Uses appropriate UE types and patterns
- **Best Practices**: Aligns with engine architecture and conventions

---

**Remember**: These standards are enforced automatically by our tooling. Focus on writing clean, logical code - the tools will handle formatting and basic quality checks!