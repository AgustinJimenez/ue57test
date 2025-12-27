#include "GenerationOrchestrator.h"

#include "../Strategies/RoomStrategyFactory.h"
#include "Engine/Engine.h"
#include "HAL/PlatformTime.h"

FGenerationOrchestrator::FGenerationOrchestrator() {}

int32 FGenerationOrchestrator::ExecuteProceduralGeneration(const FRoomData& InitialRoom,
                                                           TArray<FRoomData>& OutGeneratedRooms,
                                                           const FBackroomGenerationConfig& Config,
                                                           ICollisionDetectionService* CollisionService,
                                                           IRoomConnectionManager* ConnectionManager,
                                                           TFunction<void(FRoomData&)> RoomCreator)
{
	// Initialize generation state
	InitializeGeneration();

	LogDebug(TEXT(""), Config);
	LogDebug(TEXT("================================================================================"), Config);
	LogDebug(TEXT("🚀 STARTING BACKROOMS GENERATION"), Config);
	LogDebug(TEXT("================================================================================"), Config);
	LogDebug(FString::Printf(TEXT("=== Generating %d units: %.1f%% rooms, %.1f%% hallways, %.1f%% stairs ==="),
	                         Config.TotalRooms,
	                         Config.RoomRatio * 100.0f,
	                         Config.HallwayRatio * 100.0f,
	                         Config.StairRatio * 100.0f),
	         Config);

	// Pre-allocate memory to avoid reallocations during generation
	OutGeneratedRooms.Empty();
	OutGeneratedRooms.Reserve(Config.TotalRooms);
	OutGeneratedRooms.Add(InitialRoom);

	// Initialize random number generator
	FRandomStream Random(FDateTime::Now().GetTicks());

	// Main generation loop
	TArray<int32> AvailableRooms;              // Rooms with available connections
	AvailableRooms.Reserve(Config.TotalRooms); // Pre-allocate for worst case (all rooms available)
	AvailableRooms.Add(0);                     // Start with initial room

	LogDebug(FString::Printf(TEXT("Starting main generation loop. Available rooms: %d"), AvailableRooms.Num()), Config);

	bool bEmergencyExit = false; // Flag to break out of all nested loops

	// Generate rooms up to the configured limit
	for (int32 RoomIndex = 1; RoomIndex < Config.TotalRooms && AvailableRooms.Num() > 0 && !bEmergencyExit; RoomIndex++)
	{
		MainLoopCounter++;

		if (CheckSafetyLimits(Config))
		{
			bEmergencyExit = true;
			break;
		}

		LogDebug(TEXT(""), Config);
		LogDebug(TEXT("================================================================================"), Config);
		LogDebug(FString::Printf(TEXT("🏗️  GENERATING ROOM %d/%d (MainLoop: %d/%d)"),
		                         RoomIndex,
		                         Config.TotalRooms - 1,
		                         MainLoopCounter,
		                         Config.MaxSafetyIterations),
		         Config);
		LogDebug(TEXT("================================================================================"), Config);

		bool bRoomPlaced = false;
		int32 ConnectionRetries = 0;

		LogDebug(FString::Printf(TEXT("Available rooms for connection: %d"), AvailableRooms.Num()), Config);

		// Try to place a new room
		while (!bRoomPlaced && ConnectionRetries < Config.MaxConnectionRetries && AvailableRooms.Num() > 0 &&
		       !bEmergencyExit)
		{
			ConnectionRetries++;
			ConnectionRetryCounter++;

			if (CheckSafetyLimits(Config))
			{
				bEmergencyExit = true;
				break;
			}

			// Randomly select a room with available connections
			// Fix: RandRange is inclusive on both ends, so for array of size N, use 0 to N-1
			if (AvailableRooms.Num() == 0)
			{
				LogDebug(TEXT("ERROR: No available rooms for connection"), Config);
				break;
			}
			int32 RandomRoomIndex = AvailableRooms[FMath::RandRange(0, AvailableRooms.Num() - 1)];

			// Fix: Add bounds checking for OutGeneratedRooms access
			if (RandomRoomIndex < 0 || RandomRoomIndex >= OutGeneratedRooms.Num())
			{
				LogDebug(FString::Printf(TEXT("ERROR: Invalid room index %d (array size: %d)"),
				                         RandomRoomIndex,
				                         OutGeneratedRooms.Num()),
				         Config);
				continue;
			}
			FRoomData& SourceRoom = OutGeneratedRooms[RandomRoomIndex];

			TArray<int32> AvailableConnections = SourceRoom.GetAvailableConnections();

			if (AvailableConnections.Num() == 0)
			{
				LogDebug(FString::Printf(TEXT("Room %d has no available connections, will try other rooms"),
				                         RandomRoomIndex),
				         Config);
				ConnectionRetries++; // Count this as a retry
				continue;
			}

			// Randomly select a connection
			// Fix: Add bounds checking for AvailableConnections array access
			if (AvailableConnections.Num() == 0)
			{
				LogDebug(TEXT("ERROR: No available connections"), Config);
				continue;
			}
			int32 ConnectionIndex = AvailableConnections[FMath::RandRange(0, AvailableConnections.Num() - 1)];
			LogDebug(FString::Printf(TEXT("Selected connection index %d"), ConnectionIndex), Config);

			// Try to generate a connected room
			if (TryGenerateConnectedRoom(RoomIndex,
			                             SourceRoom,
			                             ConnectionIndex,
			                             OutGeneratedRooms,
			                             Config,
			                             CollisionService,
			                             ConnectionManager,
			                             RoomCreator,
			                             Random))
			{
				bRoomPlaced = true;
				// Fix: Ensure RoomIndex is valid before adding to AvailableRooms
				if (RoomIndex >= 0 && RoomIndex < Config.TotalRooms)
				{
					AvailableRooms.Add(RoomIndex); // New room might have connections
				}
				else
				{
					LogDebug(FString::Printf(TEXT("ERROR: Invalid RoomIndex %d when adding to AvailableRooms"),
					                         RoomIndex),
					         Config);
				}

				// Connect the rooms (calculate opposite wall index for second room)
				int32 OppositeConnectionIndex = GetOppositeWallIndex(ConnectionIndex);
				ConnectionManager->ConnectRooms(OutGeneratedRooms[RandomRoomIndex],
				                                ConnectionIndex,
				                                OutGeneratedRooms[RoomIndex],
				                                OppositeConnectionIndex,
				                                Config);

				LogDebug(FString::Printf(TEXT("✅ ROOM %d PLACED: %s connected to room %d"),
				                         RoomIndex,
				                         *UEnum::GetValueAsString(OutGeneratedRooms[RoomIndex].Category),
				                         RandomRoomIndex),
				         Config);
			}
			else
			{
				LogDebug(FString::Printf(TEXT("❌ Failed to place room %d at connection %d, retrying..."),
				                         RoomIndex,
				                         ConnectionIndex),
				         Config);
			}
		}

		if (!bRoomPlaced)
		{
			LogDebug(FString::Printf(TEXT("❌ ROOM %d: Could not place after %d retries"),
			                         RoomIndex,
			                         ConnectionRetries),
			         Config);
		}

		// Clean up rooms with no available connections
		CleanupAvailableRooms(AvailableRooms, OutGeneratedRooms);
	}

	// Calculate final statistics
	ElapsedTime = FPlatformTime::Seconds() - StartTime;

	LogDebug(TEXT("================================================================================"), Config);
	LogDebug(FString::Printf(TEXT("✅ GENERATION COMPLETED: %d/%d rooms generated"),
	                         OutGeneratedRooms.Num(),
	                         Config.TotalRooms),
	         Config);
	LogDebug(FString::Printf(TEXT("⏱️  Generation took %.2f seconds"), ElapsedTime), Config);
	LogDebug(FString::Printf(TEXT("🔄 Loop counters: Main=%d, Connection=%d, Placement=%d"),
	                         MainLoopCounter,
	                         ConnectionRetryCounter,
	                         PlacementAttemptCounter),
	         Config);
	if (bStoppedBySafety)
	{
		LogDebug(TEXT("⚠️  Generation stopped due to safety limits"), Config);
	}
	LogDebug(TEXT("================================================================================"), Config);

	return OutGeneratedRooms.Num();
}

void FGenerationOrchestrator::GetGenerationStats(int32& OutMainLoops,
                                                 int32& OutConnectionRetries,
                                                 int32& OutPlacementAttempts,
                                                 double& OutElapsedTime) const
{
	OutMainLoops = MainLoopCounter;
	OutConnectionRetries = ConnectionRetryCounter;
	OutPlacementAttempts = PlacementAttemptCounter;
	OutElapsedTime = ElapsedTime;
}

bool FGenerationOrchestrator::WasStoppedBySafety() const
{
	return bStoppedBySafety;
}

void FGenerationOrchestrator::LogDebug(const FString& Message, const FBackroomGenerationConfig& Config) const
{
	if (Config.bVerboseLogging)
	{
		// Use UE_LOG for independent logging
		UE_LOG(LogTemp, Warning, TEXT("[GenerationOrchestrator] %s"), *Message);
	}
}

bool FGenerationOrchestrator::CheckSafetyLimits(const FBackroomGenerationConfig& Config)
{
	// TIME-BASED SAFETY CHECK
	double CurrentTime = FPlatformTime::Seconds();
	ElapsedTime = CurrentTime - StartTime;
	if (ElapsedTime > Config.MaxGenerationTime)
	{
		LogDebug(FString::Printf(TEXT("⏰ TIME LIMIT REACHED: Stopping generation after %.1f seconds"), ElapsedTime),
		         Config);
		bStoppedBySafety = true;
		return true;
	}

	// ITERATION SAFETY CHECKS
	if (MainLoopCounter >= Config.MaxSafetyIterations)
	{
		LogDebug(TEXT("❌ INFINITE LOOP DETECTED: MAIN LOOP exceeded maximum iterations!"), Config);
		bStoppedBySafety = true;
		return true;
	}

	if (ConnectionRetryCounter >= Config.MaxSafetyIterations)
	{
		LogDebug(TEXT("❌ INFINITE LOOP DETECTED: CONNECTION RETRY LOOP exceeded maximum iterations!"), Config);
		bStoppedBySafety = true;
		return true;
	}

	if (PlacementAttemptCounter >= Config.MaxSafetyIterations)
	{
		LogDebug(TEXT("❌ INFINITE LOOP DETECTED: PLACEMENT ATTEMPT LOOP exceeded maximum iterations!"), Config);
		bStoppedBySafety = true;
		return true;
	}

	// Periodic progress logging
	if (MainLoopCounter % 100 == 0)
	{
		LogDebug(FString::Printf(TEXT("🔄 MAIN LOOP: %d/%d iterations (%.1fs elapsed)"),
		                         MainLoopCounter,
		                         Config.MaxSafetyIterations,
		                         ElapsedTime),
		         Config);
	}

	if (ConnectionRetryCounter % 50 == 0)
	{
		LogDebug(FString::Printf(TEXT("🔄 CONNECTION RETRY LOOP: %d/%d iterations (%.1fs elapsed)"),
		                         ConnectionRetryCounter,
		                         Config.MaxSafetyIterations,
		                         ElapsedTime),
		         Config);
	}

	if (PlacementAttemptCounter % 25 == 0)
	{
		LogDebug(FString::Printf(TEXT("🔄 PLACEMENT ATTEMPT LOOP: %d/%d iterations (%.1fs elapsed)"),
		                         PlacementAttemptCounter,
		                         Config.MaxSafetyIterations,
		                         ElapsedTime),
		         Config);
	}

	return false; // No limits exceeded
}

void FGenerationOrchestrator::InitializeGeneration()
{
	MainLoopCounter = 0;
	ConnectionRetryCounter = 0;
	PlacementAttemptCounter = 0;
	StartTime = FPlatformTime::Seconds();
	ElapsedTime = 0.0;
	bStoppedBySafety = false;
}

TPair<ERoomCategory, FString> FGenerationOrchestrator::DetermineRoomCategory(const FRoomData& SourceRoom,
                                                                             const FBackroomGenerationConfig& Config,
                                                                             FRandomStream& Random) const
{
	// CONSTRAINT: No stairs after stairs
	bool bSourceIsStair = (SourceRoom.Category == ERoomCategory::Stairs);
	float RandomValue = Random.FRand();
	ERoomCategory Category;
	FString CategoryStr;

	if (bSourceIsStair)
	{
		// If source is stair, can only place Room or Hallway (no consecutive stairs)
		float AdjustedRoomRatio =
		    Config.RoomRatio / (Config.RoomRatio + Config.HallwayRatio); // Normalize room/hallway ratio
		if (RandomValue < AdjustedRoomRatio)
		{
			Category = ERoomCategory::Room;
			CategoryStr = TEXT("Room (no-stair-constraint)");
		}
		else
		{
			Category = ERoomCategory::Hallway;
			CategoryStr = TEXT("Hallway (no-stair-constraint)");
		}
	}
	else
	{
		// Normal three-way distribution when source is not a stair
		if (RandomValue < Config.RoomRatio)
		{
			Category = ERoomCategory::Room;
			CategoryStr = TEXT("Room");
		}
		else if (RandomValue < Config.RoomRatio + Config.HallwayRatio)
		{
			Category = ERoomCategory::Hallway;
			CategoryStr = TEXT("Hallway");
		}
		else
		{
			Category = ERoomCategory::Stairs;
			CategoryStr = TEXT("Stairs");
		}
	}

	return TPair<ERoomCategory, FString>(Category, CategoryStr);
}

bool FGenerationOrchestrator::TryGenerateConnectedRoom(int32 RoomIndex,
                                                       const FRoomData& SourceRoom,
                                                       int32 ConnectionIndex,
                                                       TArray<FRoomData>& OutGeneratedRooms,
                                                       const FBackroomGenerationConfig& Config,
                                                       ICollisionDetectionService* CollisionService,
                                                       IRoomConnectionManager* ConnectionManager,
                                                       TFunction<void(FRoomData&)> RoomCreator,
                                                       FRandomStream& Random)
{
	// Try different room sizes for this connection
	for (int32 Attempt = 0; Attempt < Config.MaxAttemptsPerConnection; Attempt++)
	{
		PlacementAttemptCounter++;

		if (CheckSafetyLimits(Config))
		{
			return false; // Safety limit reached
		}

		LogDebug(FString::Printf(TEXT("Placement attempt %d/%d for connection %d"),
		                         Attempt + 1,
		                         Config.MaxAttemptsPerConnection,
		                         ConnectionIndex),
		         Config);

		// Determine room category based on constraints
		auto [Category, CategoryStr] = DetermineRoomCategory(SourceRoom, Config, Random);
		LogDebug(FString::Printf(TEXT("Generated category: %s"), *CategoryStr), Config);

		// Generate random room (connection-aware for stairs)
		FRoomData NewRoom = GenerateRoomOfCategory(Category,
		                                           RoomIndex,
		                                           Category == ERoomCategory::Stairs ? &SourceRoom : nullptr,
		                                           ConnectionIndex);
		LogDebug(FString::Printf(TEXT("Generated %s: %.1fx%.1fm"), *CategoryStr, NewRoom.Width, NewRoom.Length),
		         Config);

		// Try to place the room
		LogDebug(FString::Printf(TEXT("Attempting to place room at connection...")), Config);
		if (ConnectionManager->TryPlaceRoom(
		        SourceRoom, ConnectionIndex, NewRoom, OutGeneratedRooms, Config, CollisionService, RoomCreator))
		{
			LogDebug(FString::Printf(TEXT("SUCCESS: Room placed successfully!")), Config);
			OutGeneratedRooms.Add(NewRoom);
			return true;
		}
		else
		{
			LogDebug(FString::Printf(TEXT("Failed placement attempt %d"), Attempt + 1), Config);
		}
	}

	return false; // All attempts failed
}

void FGenerationOrchestrator::CleanupAvailableRooms(TArray<int32>& AvailableRooms,
                                                    const TArray<FRoomData>& GeneratedRooms) const
{
	for (int32 i = AvailableRooms.Num() - 1; i >= 0; i--)
	{
		int32 RoomIndex = AvailableRooms[i];
		// Fix: Add bounds checking for both arrays
		if (RoomIndex < 0 || RoomIndex >= GeneratedRooms.Num())
		{
			// Remove invalid room indices
			AvailableRooms.RemoveAt(i);
			continue;
		}

		if (GeneratedRooms[RoomIndex].GetAvailableConnections().Num() == 0)
		{
			AvailableRooms.RemoveAt(i);
		}
	}
}

FRoomData FGenerationOrchestrator::GenerateRoomOfCategory(ERoomCategory Category,
                                                          int32 RoomIndex,
                                                          const FRoomData* SourceRoom,
                                                          int32 ConnectionIndex) const
{
	// Create factory instance and use it to generate the room
	FRoomStrategyFactory StrategyFactory;
	if (IRoomGenerationStrategy* Strategy = StrategyFactory.CreateStrategy(Category))
	{
		// Note: Current strategy interface only supports Config+RoomIndex
		// For stairs, we need to extend the interface or handle context differently
		FBackroomGenerationConfig DummyConfig; // Temporary - should get proper config
		DummyConfig.StandardRoomHeight = 3.0f;
		DummyConfig.MinRoomSize = 2.0f;
		DummyConfig.MaxRoomSize = 8.0f;
		DummyConfig.MinHallwayWidth = 2.5f;
		DummyConfig.MaxHallwayWidth = 5.0f;
		DummyConfig.MinHallwayLength = 4.0f;
		DummyConfig.MaxHallwayLength = 12.0f;

		return Strategy->GenerateRoom(DummyConfig, RoomIndex);
	}

	// Fallback if strategy creation fails
	FRoomData FallbackRoom;
	FallbackRoom.RoomIndex = RoomIndex;
	FallbackRoom.Category = Category;
	FallbackRoom.Width = 5.0f;
	FallbackRoom.Length = 5.0f;
	FallbackRoom.Height = 3.0f;
	FallbackRoom.Elevation = 0.0f;
	return FallbackRoom;
}

int32 FGenerationOrchestrator::GetOppositeWallIndex(int32 WallIndex) const
{
	// Wall indices in room connections: North=0, South=1, East=2, West=3
	// Opposite wall mapping:
	// North (0) ↔ South (1)
	// East (2) ↔ West (3)
	switch (WallIndex)
	{
	case 0:
		return 1; // North → South
	case 1:
		return 0; // South → North
	case 2:
		return 3; // East → West
	case 3:
		return 2; // West → East
	default:
		// Fallback for invalid indices
		return 0;
	}
}