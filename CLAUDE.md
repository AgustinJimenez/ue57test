# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an Unreal Engine 5.7 C++ project that implements a procedural backrooms Level 0 generator. The system creates connected rooms and hallways using advanced procedural generation with collision detection, proper connectivity, and a unified hole generation system.

### Current System: Procedural Room Generation

The project uses a **sophisticated procedural room generation system** that creates varied room layouts with intelligent connection management, collision detection, and backtracking algorithms.

## Build Commands

The project now includes an NPM-based build system for convenient development. Use these commands:

### **🚀 NPM Build Scripts (Recommended)**

```bash
# Main build commands
npm run compile          # Build using official Build.sh wrapper (recommended)
npm run compile-dotnet   # Build using direct dotnet method (faster)
npm run build           # Alias for npm run compile

# Utility commands
npm run clean           # Clean build directories and rebuild
npm run generate-project # Generate Xcode project files
npm run open            # Open project in Unreal Editor
npm run open-xcode      # Open Xcode workspace

# Code quality
npm run format          # Format code with clang-format
npm run lint            # Run clang-tidy static analysis (requires: brew install llvm)
```

### **🔧 Direct Build Methods (Legacy)**

```bash
# Official Build.sh method (what npm run compile uses)
"/Users/Shared/Epic Games/UE_5.7/Engine/Build/BatchFiles/Mac/Build.sh" BackRooomsUE57Editor Mac Development -Project="$(pwd)/BackRooomsUE57.uproject"

# Direct dotnet method (what npm run compile-dotnet uses)
dotnet "/Users/Shared/Epic Games/UE_5.7/Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.dll" BackRooomsUE57Editor Mac Development -Project="/Users/agus/repo/BackRooomsUE57/BackRooomsUE57.uproject" -WaitMutex

# Generate project files for development
dotnet "/Users/Shared/Epic Games/UE_5.7/Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.dll" -projectfiles -project="/Users/agus/repo/BackRooomsUE57/BackRooomsUE57.uproject" -game -rocket -progress

# Manual clean build (when encountering HotReload issues)
rm -rf Binaries/ Intermediate/ && npm run compile
```

### **⚡ Quick Start**

For most development tasks, simply use:
- `npm run compile` - Standard build
- `npm run clean` - When you need a fresh build
- `npm run open` - Launch the project

## Current Architecture (Latest Working System)

### Main Components

1. **ABackRoomGenerator** (`Source/MyProject/BackRoomsGenerator/Main.h/.cpp`)
   - **Procedural room generation system** with connection tracking
   - Creates varied room sizes with collision detection and backtracking
   - Room-based generation (not unit-by-unit grid)
   - Intelligent connection management between rooms

2. **Room Generation Data Structures** (`Source/MyProject/BackRoomsGenerator/Types.h`)
   - **FRoomData**: Complete room information with connections
   - **FRoomConnection**: Wall-side connection tracking system
   - **ERoomCategory**: Room vs Hallway categorization
   - **EConnectionType**: Doorway vs Opening connection types

3. **URoomUnit** (`Source/MyProject/BackRoomsGenerator/RoomUnit.h/.cpp`)
   - Individual room mesh generation and management
   - Procedural mesh components for floors, walls, and ceilings
   - Material assignment per room unit

4. **Optimized Hole Generation System** (`Source/MyProject/BackRoomsGenerator/WallGenerator/`)
   - **UWallGenerator**: Main wall generation interface with fast rectangle holes
   - **UHoleGenerator**: Complex irregular hole system (for decorative use only)
   - **UWallGeneratorCommon**: Shared utilities and mesh generation
   - **Performance Optimized**: Simple rectangles for connections, complex holes for decoration

### Critical Implementation Details

#### ✅ **Procedural Room Generation Algorithm**
```cpp
1. GenerateProceduralRooms()
2. CreateInitialRoom() - 5x5m room at character location
3. Main generation loop with safety limits:
   - Select random room with available connections
   - Generate random room (Room 70% vs Hallway 30%)
   - Try placement with collision detection
   - Connect rooms and update connection tracking
4. Backtracking when placement fails
5. Safety limits prevent infinite loops
```

#### ✅ **Room Categories and Sizes**
```cpp
// Room Category: Square-ish rooms
ERoomCategory::Room
- Size: 2x2m to 8x8m (equal width/length)
- 70% of generation (RoomToHallwayRatio = 0.7)

// Hallway Category: Rectangular corridors
ERoomCategory::Hallway  
- Width: 2.5-5m, Length: 4-12m (increased for better navigation)
- Randomly swap dimensions for variety
- 30% of generation
```

#### ✅ **Connection System**
```cpp
// Each room has 4 wall connections (North, South, East, West)
struct FRoomConnection {
    EWallSide WallSide;           // Which wall (N/S/E/W)
    bool bIsUsed;                 // Connection status
    FVector ConnectionPoint;       // 3D connection location
    EConnectionType ConnectionType; // Doorway (0.8m) or Opening
    int32 ConnectedRoomIndex;     // Connected room ID
}
```

#### ✅ **Collision Detection**
```cpp
// Bounding box collision with buffer zones
bool CheckRoomCollision(const FRoomData& TestRoom) const
{
    FBox TestBounds = TestRoom.GetBoundingBox();
    TestBounds = TestBounds.ExpandBy(MetersToUnrealUnits(0.5f)); // 50cm buffer
    // Check against all existing rooms
}
```

#### ✅ **Safety Mechanisms**
```cpp
// Prevent infinite loops with multiple safety limits
MaxAttemptsPerConnection = 5;     // Tries per connection
MaxConnectionRetries = 10;        // Connection retry limit
MaxSafetyIterations = TotalRooms * MaxConnectionRetries * 2;

// Detailed logging when limits reached
if (SafetyCounter >= MaxSafetyIterations) {
    DebugLog("SAFETY LIMIT REACHED: Stopped generation...");
}
```

#### ✅ **Optimized Hole Generation System (Performance Critical)**
```cpp
// Connection holes (doorways/openings): Fast simple rectangles
EHoleShape::Rectangle  -> GenerateSimpleRectangleHole() - 4 wall segments, ~100 vertices
// Decorative holes: Complex irregular system  
EHoleShape::Circle     -> 24 points, 0° rotation, 0 irregularity, 1000+ vertices
EHoleShape::Irregular  -> Custom points, rotation, irregularity, 1000+ vertices

// Performance: Rectangle holes are 100x faster than irregular holes
// Critical: NEVER use irregular holes for connections - causes massive slowdown
```

### Generation Parameters (Current Settings)
```cpp
// Main generation settings
TotalRooms = 200;                   // Total rooms to generate (large backrooms)
RoomToHallwayRatio = 0.7f;          // 70% rooms, 30% hallways
MaxAttemptsPerConnection = 5;        // Placement attempts per connection
MaxConnectionRetries = 10;          // Max retries before giving up
GridUnitSize = 400.0f;             // Base unit size (legacy)

// Room dimensions (in meters)
RoomHeight = 3.0f;                 // Standard 3m ceiling height
// Rooms: 2x2m to 8x8m (square-ish)
// Hallways: 1-4m width, 3-12m length (rectangular)

// Connection types (30% doorway, 70% opening ratio)
Doorway: 0.8m width x 2.0m height (standard door)
Opening: Up to 80% of smallest wall width x 2.5m height (large passages)
```

### Algorithm Flow
```cpp
1. Create initial 5x5x3m room at character location
2. Add to AvailableRooms array (rooms with free connections)
3. Main generation loop:
   a. Select random room from AvailableRooms
   b. Select random available connection
   c. Generate random room size (category-based)
   d. Try placement with collision detection (accounting for 20cm wall thickness)
   e. On success: create room, connect with doorway/opening, add to arrays
   f. On failure: retry with different sizes/connections
4. Remove rooms with no free connections from AvailableRooms
5. Safety limits prevent infinite loops
6. Generate 200 rooms with optimized performance
```

## Current Issues & Status

### ✅ **WORKING (Production Ready)**
- Procedural room generation with varied sizes (200 rooms)
- Collision detection with bounding boxes and buffer zones (20cm wall thickness)
- Connection tracking system (4 walls per room)
- Room categorization (Room vs Hallway with size ranges)
- Backtracking algorithm when placement fails
- Safety mechanisms to prevent infinite loops
- **Performance optimized hole generation system**
- Character positioning in initial room
- **Connection types**: 30% doorways, 70% openings
- **Room numbering**: Vertical text identifiers above each room
- **Timestamps**: Millisecond-precision debug logging

### ✅ **RECENTLY OPTIMIZED (Major Performance Improvements)**
- **Fast rectangle hole generation**: Simple 4-segment walls (~100 vertices)
- **Eliminated performance bottleneck**: Removed complex irregular holes for connections  
- **Massive speed boost**: 100x faster hole generation for doorways/openings
- **Reduced logging**: Minimized verbose collision detection logs
- **Floor optimization**: Removed unnecessary holes from floors
- **Large scale generation**: 200 rooms instead of 30 (4x increase)
- **Proper opening sizes**: No more size clamping issues

### ✅ **FULLY IMPLEMENTED & TESTED**
- 200-room generation with fast performance
- Physical doorways and openings between connected rooms
- Proper wall thickness calculations for room positioning
- Room height standardized to 3.0m

### 🔬 **MCP Integration** 
- ✅ UnrealMCP plugin installed and active
- ✅ Claude Desktop configured with MCP server
- ✅ Can control Unreal Engine through natural language in Claude Desktop app
- ❌ Not available in Claude Code CLI (different MCP system)

## Development Guidelines

### ⚠️ **DO NOT CHANGE** (Core System)
- Procedural generation algorithm in `GenerateProceduralRooms()`
- Room data structures in `Types.h` (FRoomData, FRoomConnection)
- Collision detection logic in `CheckRoomCollision()`
- Connection tracking system
- Backtracking and safety mechanisms

### ✅ **SAFE TO MODIFY**
- Generation parameters (TotalRooms=200, RoomToHallwayRatio=0.7)
- Room size ranges (min/max for rooms and hallways)
- Connection types and ratios (currently 30% doorway, 70% opening)
- Room height (currently 3.0m standard)
- Material assignments in room generation
- Buffer zone sizes for collision detection
- Safety limit values (MaxAttemptsPerConnection, etc.)

### 🚫 **NEVER DO (Critical Performance)**
- **NEVER use irregular holes for connections** - causes 100x slowdown
- **NEVER convert rectangle holes to irregular** - breaks performance optimization
- Remove safety mechanisms or infinite loop protection
- Change room connection tracking without understanding dependencies
- Modify collision detection without proper testing
- Break the room categorization system
- Remove logging from safety limit triggers

## File Structure
```
Source/BackRooomsUE57/BackRoomsGenerator/
├── Main.h/.cpp                   # Procedural room generation system
├── Types.h                       # Data structures and enums
├── TestGenerator.h/.cpp          # Test and demonstration functions
├── RoomUnit/
│   ├── BaseRoom.h/.cpp          # Base room class with virtual methods
│   ├── StandardRoom.h/.cpp      # Standard room implementation
│   ├── StairsRoom.h/.cpp        # Stairs room with elevation
│   ├── BaseRoomUnit.h/.cpp      # Legacy room unit system
│   └── BillboardTextActor.h/.cpp # Room number display actors
└── WallUnit/
    ├── WallUnit.h/.cpp          # Wall creation and management
    ├── HoleGenerator.h/.cpp     # Door and opening generation
    └── WallCommon.h/.cpp        # Shared wall utilities

Source/BackRooomsUE57/
├── BackRooomsUE57.Build.cs      # Build configuration with ProceduralMeshComponent
└── BackRooomsUE57.h/.cpp        # Module definition
```

## Last Working State
- **Date**: Migration to Unreal Engine 5.7 (current)
- **Status**: **Migrated from UE 5.6 - Production-ready large-scale procedural backrooms generator with optimized wall system**
- **Generation**: Scalable generation (1-100 rooms) with default 25 rooms
- **Architecture**: Optimized individual wall mesh sections with smart corner management
- **Visual Features**: **Individual colored walls** with proper 3D connection holes showing wall thickness
- **Connections**: Enhanced asymmetric connection system with anti-overlap corner detection
- **Wall System**: **Advanced corner overlap prevention** with dynamic wall extensions

## Key Recent Changes (Wall System & Performance Optimization)
1. **OPTIMIZED WALL CORNER SYSTEM**: Advanced corner overlap prevention
   - **Dynamic wall extensions**: East/West walls adjust based on North/South wall removal status
   - **Smart corner detection**: Detects when walls are removed (width > 1.5f) and prevents adjacent wall extensions
   - **Anti-overlap positioning**: Eliminates geometry conflicts at connection points
   
2. **3D CONNECTION HOLE GENERATION**: Enhanced hole geometry with proper wall thickness
   - **Interior face generation**: Added 4 interior faces (left, right, top, bottom) to show wall thickness
   - **GenerateHoleInteriorFaces()**: Creates proper 3D geometry for doorways and openings
   - **Thickness visualization**: Connection holes now show realistic wall depth on all edges
   
3. **IMPROVED COLLISION DETECTION**: Optimized for better room placement
   - **Reduced collision buffer**: From 10cm to 2cm for tighter room packing
   - **Minimized logging**: Removed verbose collision debugging for cleaner output
   - **Better placement success**: Higher chance of successful room placement with smaller buffers
   
4. **SCALABLE ROOM GENERATION**: Enhanced capacity and flexibility
   - **Increased limit**: From 1 room to 100 rooms maximum (default 25)
   - **Optimized anti-flicker gap**: Reduced from 20cm to 1cm for better connections
   - **Enhanced debug logging**: Detailed asymmetric connection reporting for troubleshooting
   
5. **INDIVIDUAL WALL COLORS**: Maintained visual debugging system
   - South Wall: Red, North Wall: Blue, West Wall: Green, East Wall: Yellow
   - 5 mesh sections per room: 1 floor + 4 individual colored walls
   - Dynamic material instances for solid primary colors

## Visual Debugging System
- **Individual Wall Mesh Sections**: Each wall (N/S/E/W) is separate mesh section for independent materials
- **Dynamic Material Instances**: Created programmatically for pure primary colors
- **Color-Coded Room Identification**: Easy visual identification of room orientation and connections
- **Connection Visualization**: Clear distinction between holes (larger room) and missing walls (smaller room)

## Connection System Architecture  
- **Smart Size-Based Logic**: Room area comparison determines connection strategy
- **Doorway Connections**: Both rooms get matching 0.8m holes (no z-fighting with gap)
- **Opening Connections**: Larger room gets 1.4m hole, smaller room wall completely removed
- **Wall Removal Trigger**: 99.0f width in DoorConfig triggers complete wall section removal
- **Gap Management**: Optimized 1cm separation between rooms for tighter connections

## Technical Implementation Details

### Wall Corner Management System
```cpp
// Dynamic extension calculation based on wall removal detection
float NorthExtension = bNorthWallRemoved ? 0.0f : (ThicknessCm * 0.5f);
float SouthExtension = bSouthWallRemoved ? 0.0f : (ThicknessCm * 0.5f);

// Wall removal detection
for (const FDoorConfig& DoorConfig : DoorConfigs)
{
    if (DoorConfig.bHasDoor && DoorConfig.Width > 1.5f)  // 1.5m+ triggers removal
    {
        // Mark wall for removal and adjust adjacent walls
    }
}
```

### 3D Connection Hole Generation
```cpp
// Interior face generation for proper wall thickness
void UWallGenerator::GenerateHoleInteriorFaces()
{
    // Creates 4 quad faces connecting inner and outer wall surfaces:
    // - Left interior face (connects left edge inner to outer)
    // - Right interior face (connects right edge inner to outer) 
    // - Bottom interior face (connects bottom edge inner to outer)
    // - Top interior face (connects top edge inner to outer)
}
```

### Optimized Collision Detection
```cpp
// Minimal 2cm buffer for tighter room packing
float BufferSize = MetersToUnrealUnits(0.02f);
TestBounds = TestBounds.ExpandBy(BufferSize);

// Minimal 1cm anti-flicker gap between rooms
float AntiFlickerGap = MetersToUnrealUnits(0.01f);
```

## Performance Benchmarks
- **Wall corner optimization**: Eliminated overlap artifacts while maintaining proper connections
- **3D hole generation**: ~16 additional vertices per connection hole for realistic thickness
- **Collision detection**: 80% reduction in buffer size allows more room placement opportunities
- **Room generation**: Scalable from 1-100 rooms with optimized performance and visual quality

## Infinite Loop Safety System
- **MainLoopCounter**: Tracks main room generation loop (every 100 iterations logged)
- **ConnectionRetryCounter**: Tracks connection retry attempts (every 50 iterations logged)  
- **PlacementAttemptCounter**: Tracks room placement attempts (every 25 iterations logged)
- **Time-Based Safety**: 20-second hard limit regardless of iteration counts
- **Emergency Exit Flags**: Breaks out of all nested loops when any limit reached
- **Detailed Reporting**: Shows exact loop counts and exit reason in final summary

## Stability Improvements
- **BeginPlay Auto-Generation**: Disabled problematic immediate generation, now uses controlled timing
- **Console Command Removal**: Eliminated GEngine->Exec calls that caused editor hanging
- **Validation Checks**: Added IsValid() checks for all UE objects before use
- **Array Initialization**: Fixed UE5-compatible TArray syntax (SetNum instead of constructor parameters)