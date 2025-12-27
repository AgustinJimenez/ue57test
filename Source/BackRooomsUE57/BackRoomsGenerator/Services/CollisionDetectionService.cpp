#include "CollisionDetectionService.h"
#include "Engine/Engine.h"

FCollisionDetectionService::FCollisionDetectionService()
{
}

bool FCollisionDetectionService::CheckRoomCollision(const FRoomData& TestRoom,
                                                    const TArray<FRoomData>& ExistingRooms,
                                                    const FBackroomGenerationConfig& Config) const
{
    return InternalCheckCollision(TestRoom, ExistingRooms, -1, Config);
}

bool FCollisionDetectionService::CheckRoomCollisionExcluding(const FRoomData& TestRoom,
                                                             const TArray<FRoomData>& ExistingRooms,
                                                             int32 ExcludeRoomIndex,
                                                             const FBackroomGenerationConfig& Config) const
{
    return InternalCheckCollision(TestRoom, ExistingRooms, ExcludeRoomIndex, Config);
}

bool FCollisionDetectionService::ValidateRoomBounds(const FRoomData& Room,
                                                    const FBackroomGenerationConfig& Config) const
{
    // Check for reasonable room dimensions
    if (Room.Width <= 0.0f || Room.Length <= 0.0f || Room.Height <= 0.0f)
    {
        LogDebug(FString::Printf(TEXT("[BOUNDS] Invalid room dimensions: %.2fx%.2fx%.2fm"), 
            Room.Width, Room.Length, Room.Height), Config);
        return false;
    }
    
    // Check for excessively large rooms (potential memory/performance issues)
    const float MaxDimension = 100.0f; // 100m maximum
    if (Room.Width > MaxDimension || Room.Length > MaxDimension || Room.Height > MaxDimension)
    {
        LogDebug(FString::Printf(TEXT("[BOUNDS] Room too large: %.2fx%.2fx%.2fm (max %.2fm)"), 
            Room.Width, Room.Length, Room.Height, MaxDimension), Config);
        return false;
    }
    
    // Check for extreme elevations (potential precision issues)
    const float MaxElevation = 10000.0f; // 100m absolute elevation limit
    if (FMath::Abs(Room.Elevation) > MaxElevation)
    {
        LogDebug(FString::Printf(TEXT("[BOUNDS] Room elevation too extreme: %.2fm (max ±%.2fm)"), 
            Room.Elevation, MaxElevation), Config);
        return false;
    }
    
    // Check bounding box validity
    FBox RoomBounds = Room.GetBoundingBox();
    if (!RoomBounds.IsValid)
    {
        LogDebug(TEXT("[BOUNDS] Room produces invalid bounding box"), Config);
        return false;
    }
    
    return true;
}

bool FCollisionDetectionService::InternalCheckCollision(const FRoomData& TestRoom,
                                                        const TArray<FRoomData>& ExistingRooms,
                                                        int32 ExcludeRoomIndex,
                                                        const FBackroomGenerationConfig& Config) const
{
    // Validate test room bounds first
    if (!ValidateRoomBounds(TestRoom, Config))
    {
        LogDebug(TEXT("[COLLISION] Test room failed bounds validation"), Config);
        return true; // Treat invalid rooms as collisions
    }
    
    // Get expanded bounds for test room
    FBox TestBounds = GetExpandedBounds(TestRoom, Config);
    
    // LogDebug(FString::Printf(TEXT("[DEBUG] BOUNDS: Room %d bounds Min=%s Max=%s"), 
    //     TestRoom.RoomIndex, *TestBounds.Min.ToString(), *TestBounds.Max.ToString()), Config);
    
    // Check against all existing rooms
    for (int32 i = 0; i < ExistingRooms.Num(); i++)
    {
        // Skip excluded room (for connected room placement)
        if (i == ExcludeRoomIndex)
        {
            // LogDebug(FString::Printf(TEXT("[DEBUG] SKIP: Excluding room %d from collision check"), i), Config);
            continue;
        }
        
        const FRoomData& ExistingRoom = ExistingRooms[i];
        
        // Validate existing room bounds
        if (!ValidateRoomBounds(ExistingRoom, Config))
        {
            LogDebug(FString::Printf(TEXT("[COLLISION] Existing room %d failed bounds validation"), i), Config);
            continue; // Skip invalid existing rooms
        }
        
        FBox ExistingBounds = ExistingRoom.GetBoundingBox();
        
        LogDebug(FString::Printf(TEXT("[DEBUG] CHECK: Room %d vs Room %d (%.1fx%.1fm, Elev=%.1fm, Bounds Min=%s Max=%s)"), 
            TestRoom.RoomIndex, i, ExistingRoom.Width, ExistingRoom.Length, ExistingRoom.Elevation,
            *ExistingBounds.Min.ToString(), *ExistingBounds.Max.ToString()), Config);
        
        // Check for 3D bounding box intersection
        if (TestBounds.Intersect(ExistingBounds))
        {
            // Detailed collision reporting
            FString TestCategory = UEnum::GetValueAsString(TestRoom.Category);
            FString ExistingCategory = UEnum::GetValueAsString(ExistingRoom.Category);
            
            LogDebug(TEXT("[X] COLLISION detected between rooms:"), Config);
            LogDebug(FString::Printf(TEXT("   Test Room %d: %s, Elevation=%.2fm, Bounds Min=%s Max=%s"), 
                TestRoom.RoomIndex, *TestCategory, TestRoom.Elevation, 
                *TestBounds.Min.ToString(), *TestBounds.Max.ToString()), Config);
            LogDebug(FString::Printf(TEXT("   Existing Room %d: %s, Elevation=%.2fm, Bounds Min=%s Max=%s"), 
                i, *ExistingCategory, ExistingRoom.Elevation, 
                *ExistingBounds.Min.ToString(), *ExistingBounds.Max.ToString()), Config);
            
            // Check for suspicious vertical collisions
            if (AreRoomsOnDifferentLevels(TestRoom, ExistingRoom))
            {
                float VerticalSeparation = FMath::Abs(TestRoom.Elevation - ExistingRoom.Elevation);
                LogDebug(FString::Printf(TEXT("   [!] SUSPICIOUS: %.2fm elevation difference should prevent collision!"), 
                    VerticalSeparation), Config);
            }
            
            return true; // Collision detected
        }
    }
    
    LogDebug(FString::Printf(TEXT("[✓] NO COLLISION: Room %d placement is safe"), TestRoom.RoomIndex), Config);
    return false; // No collision
}

void FCollisionDetectionService::LogDebug(const FString& Message, const FBackroomGenerationConfig& Config) const
{
    // Only log if verbose logging is enabled
    if (Config.bVerboseLogging)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CollisionService] %s"), *Message);
    }
}

FBox FCollisionDetectionService::GetExpandedBounds(const FRoomData& Room, const FBackroomGenerationConfig& Config) const
{
    FBox RoomBounds = Room.GetBoundingBox();
    
    // Apply collision buffer from configuration
    // Convert buffer from meters to Unreal units (centimeters)
    float BufferSize = Config.CollisionBuffer * BackroomConstants::METERS_TO_UNREAL_UNITS;
    
    return RoomBounds.ExpandBy(BufferSize);
}

bool FCollisionDetectionService::AreRoomsOnDifferentLevels(const FRoomData& Room1,
                                                           const FRoomData& Room2,
                                                           float MinSeparation) const
{
    float ElevationDifference = FMath::Abs(Room1.Elevation - Room2.Elevation);
    return ElevationDifference > MinSeparation;
}