#include "Main.h"
#include "RoomUnit/BaseRoom.h"
#include "RoomUnit/StandardRoom.h"
#include "TestGenerator.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "DrawDebugHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"

DEFINE_LOG_CATEGORY(LogBackRoomGenerator);

ABackRoomGenerator::ABackRoomGenerator()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// Basic settings - 5 rooms for collision debugging
	TotalRooms = 10; // Generate 5 rooms for testing
	RoomRatio = 0.30f; // 30% rooms 
	HallwayRatio = 0.30f; // 30% hallways 
	StairRatio = 0.40f; // 40% stairs (ENABLED)
	MaxAttemptsPerConnection = 5;
	MaxConnectionRetries = 10;
	GridUnitSize = 400.0f;
}

void ABackRoomGenerator::BeginPlay()
{
	Super::BeginPlay();
	
	// Auto-generate immediately using procedural generation
	GenerateProceduralRooms(); // ENABLED - normal backrooms generation with boundary fix
	// GenerateBackroomsInTestMode(); // DISABLED - use TestGenerator for stair testing
}

void ABackRoomGenerator::GenerateBackrooms()
{
	// Clear any existing rooms
	RoomUnits.Empty();
	GeneratedRooms.Empty();
	
	// Get character location
	FVector CharacterLocation = FVector::ZeroVector;
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}
	
	APlayerController* PC = World->GetFirstPlayerController();
	if (IsValid(PC))
	{
		ACharacter* Character = PC->GetCharacter();
		if (IsValid(Character))
		{
			CharacterLocation = Character->GetActorLocation();
		}
	}
	
	if (CharacterLocation.IsZero())
	{
		CharacterLocation = FVector::ZeroVector;
	}
	
	// Generate all rooms using procedural generation
	// GenerateProceduralRooms(); // DISABLED - procedural generation with boundary fix
	GenerateBackroomsInTestMode(); // ENABLED - test mode for stairs
	DebugLog(TEXT("GenerateBackrooms completed - boundary test mode"));
}

void ABackRoomGenerator::GenerateProceduralRooms()
{
	DebugLog(TEXT(""));
	DebugLog(TEXT("================================================================================"));
	DebugLog(TEXT("🚀 STARTING BACKROOMS GENERATION"));
	DebugLog(TEXT("================================================================================"));
	DebugLog(FString::Printf(TEXT("=== Generating %d units: %.1f%% rooms, %.1f%% hallways, %.1f%% stairs ==="), 
		TotalRooms, RoomRatio * 100.0f, HallwayRatio * 100.0f, StairRatio * 100.0f));
	DebugLog(FString::Printf(TEXT("DEBUG: TotalRooms value is %d"), TotalRooms));
	
	// Get character location for initial room
	FVector CharacterLocation = FVector::ZeroVector;
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (ACharacter* Character = PC->GetCharacter())
			{
				CharacterLocation = Character->GetActorLocation();
			}
		}
	}
	
	// Create initial room
	FRoomData InitialRoom = CreateInitialRoom(CharacterLocation);
	GeneratedRooms.Add(InitialRoom);
	
	// Initialize random number generator
	FRandomStream Random(FDateTime::Now().GetTicks());
	
	// Main generation loop
	TArray<int32> AvailableRooms; // Rooms with available connections
	AvailableRooms.Add(0); // Start with initial room
	
	DebugLog(FString::Printf(TEXT("Starting main generation loop. Available rooms: %d"), AvailableRooms.Num()));
	
	// LOOP COUNTERS - Each with 2000 limit to detect infinite loops
	int32 MainLoopCounter = 0;
	int32 ConnectionRetryCounter = 0;  
	int32 PlacementAttemptCounter = 0;
	const int32 LOOP_LIMIT = 2000;
	bool bEmergencyExit = false; // Flag to break out of all nested loops
	
	// TIME-BASED SAFETY: Stop after 20 seconds regardless of what's happening
	double StartTime = FPlatformTime::Seconds();
	const double MAX_GENERATION_TIME = 20.0; // 20 seconds max
	
	// Generate rooms up to the configured limit  
	for (int32 RoomIndex = 1; RoomIndex < TotalRooms && MainLoopCounter < LOOP_LIMIT && AvailableRooms.Num() > 0 && !bEmergencyExit; RoomIndex++)
	{
		MainLoopCounter++;
		
		// TIME-BASED SAFETY CHECK
		double CurrentTime = FPlatformTime::Seconds();
		double ElapsedTime = CurrentTime - StartTime;
		if (ElapsedTime > MAX_GENERATION_TIME)
		{
			DebugLog(FString::Printf(TEXT("⏰ TIME LIMIT REACHED: Stopping generation after %.1f seconds"), ElapsedTime));
			bEmergencyExit = true;
			break;
		}
		
		if (MainLoopCounter % 100 == 0)
		{
			DebugLog(FString::Printf(TEXT("🔄 MAIN LOOP: %d/%d iterations (%.1fs elapsed)"), MainLoopCounter, LOOP_LIMIT, ElapsedTime));
		}
		
		if (MainLoopCounter >= LOOP_LIMIT)
		{
			DebugLog(TEXT("❌ INFINITE LOOP DETECTED: MAIN LOOP exceeded 2000 iterations!"));
			bEmergencyExit = true;
			break;
		}
		
		DebugLog(TEXT(""));
		DebugLog(TEXT("================================================================================"));
		DebugLog(FString::Printf(TEXT("🏗️  GENERATING ROOM %d/%d (MainLoop: %d/%d)"), 
			RoomIndex, TotalRooms-1, MainLoopCounter, LOOP_LIMIT));
		DebugLog(TEXT("================================================================================"));
		
		bool bRoomPlaced = false;
		int32 ConnectionRetries = 0;
		
		DebugLog(FString::Printf(TEXT("Available rooms for connection: %d"), AvailableRooms.Num()));
		
		// Try to place a new room
		while (!bRoomPlaced && ConnectionRetries < MaxConnectionRetries && AvailableRooms.Num() > 0 && !bEmergencyExit)
		{
			ConnectionRetries++;
			ConnectionRetryCounter++;
			
			// TIME-BASED SAFETY CHECK
			double CurrentTime = FPlatformTime::Seconds();
			double ElapsedTime = CurrentTime - StartTime;
			if (ElapsedTime > MAX_GENERATION_TIME)
			{
				DebugLog(FString::Printf(TEXT("⏰ TIME LIMIT REACHED in CONNECTION LOOP: Stopping after %.1f seconds"), ElapsedTime));
				bEmergencyExit = true;
				break;
			}
			
			if (ConnectionRetryCounter % 50 == 0)
			{
				DebugLog(FString::Printf(TEXT("🔄 CONNECTION RETRY LOOP: %d/%d iterations (%.1fs elapsed)"), ConnectionRetryCounter, LOOP_LIMIT, ElapsedTime));
			}
			
			if (ConnectionRetryCounter >= LOOP_LIMIT)
			{
				DebugLog(TEXT("❌ INFINITE LOOP DETECTED: CONNECTION RETRY LOOP exceeded 2000 iterations!"));
				bEmergencyExit = true;
				break;
			}
			
			// Randomly select a room with available connections
			int32 RandomRoomIndex = AvailableRooms[Random.RandRange(0, AvailableRooms.Num() - 1)];
			FRoomData& SourceRoom = GeneratedRooms[RandomRoomIndex];
			
			TArray<int32> AvailableConnections = SourceRoom.GetAvailableConnections();
			
			if (AvailableConnections.Num() == 0)
			{
				DebugLog(FString::Printf(TEXT("Room %d has no available connections, will try other rooms"), RandomRoomIndex));
				// Don't remove immediately - let the main retry logic handle it
				ConnectionRetries++; // Count this as a retry
				continue;
			}
			
			// Randomly select a connection
			int32 ConnectionIndex = AvailableConnections[Random.RandRange(0, AvailableConnections.Num() - 1)];
			DebugLog(FString::Printf(TEXT("Selected connection index %d"), ConnectionIndex));
			
			// Try different room sizes for this connection
			for (int32 Attempt = 0; Attempt < MaxAttemptsPerConnection && !bEmergencyExit; Attempt++)
			{
				PlacementAttemptCounter++;
				
				// TIME-BASED SAFETY CHECK
				double CurrentTime = FPlatformTime::Seconds();
				double ElapsedTime = CurrentTime - StartTime;
				if (ElapsedTime > MAX_GENERATION_TIME)
				{
					DebugLog(FString::Printf(TEXT("⏰ TIME LIMIT REACHED in PLACEMENT LOOP: Stopping after %.1f seconds"), ElapsedTime));
					bEmergencyExit = true;
					break;
				}
				
				if (PlacementAttemptCounter % 25 == 0)
				{
					DebugLog(FString::Printf(TEXT("🔄 PLACEMENT ATTEMPT LOOP: %d/%d iterations (%.1fs elapsed)"), PlacementAttemptCounter, LOOP_LIMIT, ElapsedTime));
				}
				
				if (PlacementAttemptCounter >= LOOP_LIMIT)
				{
					DebugLog(TEXT("❌ INFINITE LOOP DETECTED: PLACEMENT ATTEMPT LOOP exceeded 2000 iterations!"));
					bEmergencyExit = true;
					break;
				}
				
				DebugLog(FString::Printf(TEXT("Placement attempt %d/%d for connection %d"), Attempt+1, MaxAttemptsPerConnection, ConnectionIndex));
				
				// Determine room category based on three-way ratio
				// CONSTRAINT: No stairs after stairs
				bool bSourceIsStair = (SourceRoom.Category == ERoomCategory::Stairs);
				float RandomValue = Random.FRand();
				ERoomCategory Category;
				FString CategoryStr;
				
				if (bSourceIsStair)
				{
					// If source is stair, can only place Room or Hallway (no consecutive stairs)
					float AdjustedRoomRatio = RoomRatio / (RoomRatio + HallwayRatio); // Normalize room/hallway ratio
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
					if (RandomValue < RoomRatio)
					{
						Category = ERoomCategory::Room;
						CategoryStr = TEXT("Room");
					}
					else if (RandomValue < RoomRatio + HallwayRatio)
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
				DebugLog(FString::Printf(TEXT("Generated category: %s"), *CategoryStr));
				
				// Generate random room (connection-aware for stairs)
				FRoomData NewRoom = (Category == ERoomCategory::Stairs) ? 
					GenerateRandomRoom(Category, RoomIndex, SourceRoom, ConnectionIndex) :
					GenerateRandomRoom(Category, RoomIndex);
				DebugLog(FString::Printf(TEXT("Generated %s: %.1fx%.1fm"), *CategoryStr, NewRoom.Width, NewRoom.Length));
				
				// Try to place the room
				DebugLog(FString::Printf(TEXT("Attempting to place room at connection...")));
				if (TryPlaceRoom(SourceRoom, ConnectionIndex, NewRoom))
				{
					DebugLog(FString::Printf(TEXT("SUCCESS: Room placed successfully!")));
					// Success! Add the room
					GeneratedRooms.Add(NewRoom);
					AvailableRooms.Add(RoomIndex);
					
					// Mark the connection as used and connect the rooms
					ConnectRooms(GeneratedRooms[RandomRoomIndex], ConnectionIndex, GeneratedRooms[RoomIndex], 0);
					
					bRoomPlaced = true;
					
					DebugLog(FString::Printf(TEXT("✅ ROOM %d PLACED: %s (%.1fx%.1fm) connected to room %d"), 
						RoomIndex,
						(Category == ERoomCategory::Room) ? TEXT("square") : TEXT("hallway"),
						NewRoom.Width, NewRoom.Length, RandomRoomIndex));
					DebugLog(TEXT("================================================================================"));
					break;
				}
				else
				{
					DebugLog(FString::Printf(TEXT("FAILED: Room placement failed (collision detected)")));
				}
			}
			
			if (!bRoomPlaced)
			{
				ConnectionRetries++;
			}
		}
		
		if (!bRoomPlaced)
		{
			DebugLog(FString::Printf(TEXT("❌ ROOM %d FAILED: Could not place after %d retries"), RoomIndex, MaxConnectionRetries));
			
			// Clean up available rooms - remove rooms with no connections
			for (int32 i = AvailableRooms.Num() - 1; i >= 0; i--)
			{
				int32 RoomIdx = AvailableRooms[i];
				if (RoomIdx < GeneratedRooms.Num() && GeneratedRooms[RoomIdx].GetAvailableConnections().Num() == 0)
				{
					DebugLog(FString::Printf(TEXT("Removing room %d from available list (no connections)"), RoomIdx));
					AvailableRooms.RemoveAt(i);
				}
			}
			
			// Only stop if we have no more rooms to try
			if (AvailableRooms.Num() == 0)
			{
				DebugLog(TEXT("No more rooms with available connections - stopping generation"));
				DebugLog(TEXT("================================================================================"));
				break;
			}
			
			// Continue trying with remaining rooms
			DebugLog(FString::Printf(TEXT("Continuing with %d remaining rooms with connections"), AvailableRooms.Num()));
			DebugLog(TEXT("================================================================================"));
			RoomIndex--; // Retry this room index with cleaned up available rooms
		}
		
		// Emergency exit if we hit any loop limit
		if (MainLoopCounter >= LOOP_LIMIT || ConnectionRetryCounter >= LOOP_LIMIT || PlacementAttemptCounter >= LOOP_LIMIT)
		{
			DebugLog(FString::Printf(TEXT("❌ EMERGENCY EXIT: Loop limit reached - Main:%d Connection:%d Placement:%d"), 
				MainLoopCounter, ConnectionRetryCounter, PlacementAttemptCounter));
			break;
		}
	}
	
	// Final summary of loop iterations
	DebugLog(FString::Printf(TEXT("📊 LOOP SUMMARY - Main:%d Connection:%d Placement:%d"), 
		MainLoopCounter, ConnectionRetryCounter, PlacementAttemptCounter));
	
	if (MainLoopCounter >= LOOP_LIMIT || ConnectionRetryCounter >= LOOP_LIMIT || PlacementAttemptCounter >= LOOP_LIMIT)
	{
		DebugLog(TEXT("❌ INFINITE LOOP DETECTED: Generation stopped due to loop limit!"));
	}
	
	DebugLog(FString::Printf(TEXT("Procedural generation complete: %d rooms created (Target: %d)"), GeneratedRooms.Num(), TotalRooms));
	
	// LOOP COUNTER SUMMARY REPORT
	double TotalTime = FPlatformTime::Seconds() - StartTime;
	DebugLog(TEXT("================================================================================"));
	DebugLog(TEXT("📊 GENERATION SUMMARY"));
	DebugLog(TEXT("================================================================================"));
	DebugLog(FString::Printf(TEXT("⏱️  Total Generation Time: %.2f seconds"), TotalTime));
	DebugLog(FString::Printf(TEXT("🔄 Main Loop Counter: %d/%d iterations"), MainLoopCounter, LOOP_LIMIT));
	DebugLog(FString::Printf(TEXT("🔄 Connection Retry Counter: %d/%d iterations"), ConnectionRetryCounter, LOOP_LIMIT));
	DebugLog(FString::Printf(TEXT("🔄 Placement Attempt Counter: %d/%d iterations"), PlacementAttemptCounter, LOOP_LIMIT));
	if (bEmergencyExit)
	{
		if (TotalTime >= MAX_GENERATION_TIME)
		{
			DebugLog(TEXT("⏰ TIME LIMIT EXIT: Generation stopped after 20 seconds!"));
		}
		else
		{
			DebugLog(TEXT("❌ LOOP LIMIT EXIT: Generation stopped due to infinite loop detection!"));
		}
	}
	else
	{
		DebugLog(TEXT("✅ NORMAL EXIT: Generation completed successfully"));
	}
	DebugLog(TEXT("================================================================================"));
	DebugLog(TEXT("🎯 BACKROOMS GENERATION COMPLETED"));
	DebugLog(TEXT("================================================================================"));
	DebugLog(TEXT(""));
	
	// Ensure we don't have more rooms than intended
	if (GeneratedRooms.Num() > TotalRooms)
	{
		DebugLog(FString::Printf(TEXT("WARNING: Generated %d rooms but target was %d!"), GeneratedRooms.Num(), TotalRooms));
	}
}

void ABackRoomGenerator::GenerateBackroomsInTestMode()
{
	// Clear any existing rooms
	RoomUnits.Empty();
	GeneratedRooms.Empty();
	
	DebugLog(TEXT("=== MAIN TEST MODE: Spawning TestGenerator for WallUnit tests ==="));
	
	// Spawn TestGenerator actor to handle WallUnit testing
	if (UWorld* World = GetWorld())
	{
		ATestGenerator* TestGen = World->SpawnActor<ATestGenerator>();
		if (TestGen)
		{
			DebugLog(TEXT("✅ TestGenerator spawned successfully - it will auto-generate WallUnit tests"));
		}
		else
		{
			DebugLog(TEXT("❌ Failed to spawn TestGenerator"));
		}
	}
	
	DebugLog(TEXT("✅ Test mode complete - TestGenerator should create rotating walls"));
}

FRoomData ABackRoomGenerator::CreateInitialRoom(const FVector& CharacterLocation)
{
	FRoomData InitialRoom;
	InitialRoom.Category = ERoomCategory::Room;
	InitialRoom.Width = 5.0f;  // 5x5 meter starting room
	InitialRoom.Length = 5.0f;
	InitialRoom.Height = 3.0f;
	InitialRoom.RoomIndex = 0;
	
	// Position room so character stands on the floor surface  
	InitialRoom.Position = FVector(
		CharacterLocation.X - MetersToUnrealUnits(InitialRoom.Width) * 0.5f,
		CharacterLocation.Y - MetersToUnrealUnits(InitialRoom.Length) * 0.5f,
		CharacterLocation.Z - MetersToUnrealUnits(0.5f) // Room floor 0.5m below character
	);
	
	// Create the actual room unit using new architecture
	UStandardRoom* InitialRoomUnit = NewObject<UStandardRoom>(this);
	InitialRoomUnit->Width = InitialRoom.Width;
	InitialRoomUnit->Length = InitialRoom.Length;
	InitialRoomUnit->Height = InitialRoom.Height;
	InitialRoomUnit->Position = InitialRoom.Position;
	InitialRoomUnit->RoomCategory = InitialRoom.Category;
	InitialRoomUnit->Elevation = InitialRoom.Elevation;
	
	InitialRoom.RoomUnit = InitialRoomUnit;
	InitialRoom.RoomUnit->CreateRoom(this);
	InitialRoom.RoomUnit->SetMaterial(nullptr); // Apply color-coded materials
	RoomUnits.Add(InitialRoom.RoomUnit);
	
	// Add room number identifier
	CreateRoomNumberIdentifier(InitialRoom);
	
	// Create connections for all 4 walls
	CreateRoomConnections(InitialRoom);
	
	// Position character above the room floor
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (ACharacter* Character = PC->GetCharacter())
			{
				FVector FloorCenter = InitialRoom.Position + FVector(
					MetersToUnrealUnits(InitialRoom.Width) * 0.5f,
					MetersToUnrealUnits(InitialRoom.Length) * 0.5f,
					MetersToUnrealUnits(0.6f) // 0.6m above room floor (0.5m below original + 0.1m clearance)
				);
				Character->SetActorLocation(FloorCenter);
			}
		}
	}
	
	DebugLog(FString::Printf(TEXT("Created initial room (%.1fx%.1fm) at %s"), 
		InitialRoom.Width, InitialRoom.Length, *InitialRoom.Position.ToString()));
	
	return InitialRoom;
}

FRoomData ABackRoomGenerator::GenerateRandomRoom(ERoomCategory Category, int32 RoomIndex)
{
	// Use more varied seed to prevent identical room generation
	FRandomStream Random(FDateTime::Now().GetTicks() + RoomIndex * 12345 + FMath::Rand());
	
	// Use comprehensive initialization with full scale for massive backrooms
	FRoomData Room;
	UBaseRoom::InitializeRandomRoomWithElevation(Room, Category, RoomIndex, Random, true); // true = use full scale
	
	return Room;
}

FRoomData ABackRoomGenerator::GenerateRandomRoom(ERoomCategory Category, int32 RoomIndex, const FRoomData& SourceRoom, int32 ConnectionIndex)
{
	// Use more varied seed to prevent identical room generation
	FRandomStream Random(FDateTime::Now().GetTicks() + RoomIndex * 12345 + FMath::Rand());
	
	FRoomData Room;
	Room.Category = Category;
	Room.RoomIndex = RoomIndex;
	
	if (Category == ERoomCategory::Stairs)
	{
		// Connection-aware stair generation
		const FRoomConnection& Connection = SourceRoom.Connections[ConnectionIndex];
		EWallSide ConnectionWall = Connection.WallSide;
		
		// Determine stair direction based on source room connection
		EWallSide StairDirection = EWallSide::None;
		bool bStairsRunNorthSouth = false;
		
		switch (ConnectionWall)
		{
			case EWallSide::North:  // Source room's north wall connects to stairs' south wall, so stairs ascend north
				StairDirection = EWallSide::North;
				bStairsRunNorthSouth = true;
				break;
			case EWallSide::South:  // Source room's south wall connects to stairs' north wall, so stairs ascend south  
				StairDirection = EWallSide::South;
				bStairsRunNorthSouth = true;
				break;
			case EWallSide::East:   // Source room's east wall connects to stairs' west wall, so stairs ascend east
				StairDirection = EWallSide::East;
				bStairsRunNorthSouth = false;
				break;
			case EWallSide::West:   // Source room's west wall connects to stairs' east wall, so stairs ascend west
				StairDirection = EWallSide::West;
				bStairsRunNorthSouth = false;
				break;
		}
		
		Room.StairDirection = StairDirection;
		
		// Stair dimensions: 2-4m width, ensure length gives minimum 4m height
		Room.Width = Random.FRandRange(2.0f, 4.0f);
		Room.Height = 3.0f; // Standard ceiling height
		
		// Calculate minimum length needed for stairs to reach above room height
		float StepDepth = 0.3f; // 30cm step depth
		float StepHeight = 0.15f; // 15cm step rise
		float RoomHeight = 3.0f; // Standard room height
		float MinElevation = RoomHeight + 1.0f; // Must reach at least 4m total (1m above room height)
		float MaxElevation = RoomHeight + 3.0f; // Up to 6m total for variety
		
		// Calculate length range that gives desired elevation range
		int32 MinStepsNeeded = FMath::CeilToInt(MinElevation / StepHeight); // 27 steps for 4m
		int32 MaxStepsDesired = FMath::FloorToInt(MaxElevation / StepHeight); // 40 steps for 6m
		float MinLength = MinStepsNeeded * StepDepth; // 8.1m minimum length
		float MaxLength = MaxStepsDesired * StepDepth; // 12m maximum length
		
		// Generate length that ensures stairs go above room height
		Room.Length = Random.FRandRange(MinLength, MaxLength);
		
		// Calculate actual elevation based on final stair length
		float StairLength = FMath::Max(Room.Width, Room.Length);
		int32 NumSteps = FMath::FloorToInt(StairLength / StepDepth);
		Room.Elevation = NumSteps * StepHeight; // Total elevation gained (guaranteed ≥4m)
		
		DebugLog(FString::Printf(TEXT("STAIR GENERATION: Connection from %s wall, stairs ascend %s, dimensions %.1fx%.1fm, elevation %.1fm (%d steps)"), 
			*UEnum::GetValueAsString(ConnectionWall),
			*UEnum::GetValueAsString(StairDirection),
			Room.Width, Room.Length, Room.Elevation, NumSteps));
	}
	else
	{
		// Use the standard generation for non-stairs
		return GenerateRandomRoom(Category, RoomIndex);
	}
	
	return Room;
}

bool ABackRoomGenerator::TryPlaceRoom(const FRoomData& SourceRoom, int32 ConnectionIndex, FRoomData& NewRoom)
{
	DebugLog(FString::Printf(TEXT("[DEBUG] TryPlaceRoom: Room %d (%.1fx%.1fm, Cat=%s, Elev=%.1fm) at Connection %d"), 
		NewRoom.RoomIndex, NewRoom.Width, NewRoom.Length, 
		*UEnum::GetValueAsString(NewRoom.Category), NewRoom.Elevation, ConnectionIndex));
	
	// SPECIAL HANDLING: If connecting to stairs, set new room elevation to match stair top
	if (SourceRoom.Category == ERoomCategory::Stairs)
	{
		NewRoom.Elevation = SourceRoom.Elevation; // Match stair top elevation
		DebugLog(FString::Printf(TEXT("STAIR CONNECTION: Setting new room elevation to %.1fm (matching stair top)"), 
			NewRoom.Elevation));
	}
	
	// Calculate position based on connection
	NewRoom.Position = CalculateConnectionPosition(SourceRoom, ConnectionIndex, NewRoom);
	
	DebugLog(FString::Printf(TEXT("[DEBUG] Calculated Position: %s (X=%.1f, Y=%.1f, Z=%.1f)"), 
		*NewRoom.Position.ToString(), NewRoom.Position.X, NewRoom.Position.Y, NewRoom.Position.Z));
	
	// Check for collisions (excluding the source room we're connecting to)
	DebugLog(FString::Printf(TEXT("[DEBUG] COLLISION CHECK: About to check room %d against %d existing rooms"), 
		NewRoom.RoomIndex, GeneratedRooms.Num()));
	bool bHasCollision = CheckRoomCollisionExcluding(NewRoom, SourceRoom.RoomIndex);
	DebugLog(FString::Printf(TEXT("[DEBUG] COLLISION RESULT: %s"), bHasCollision ? TEXT("COLLISION DETECTED") : TEXT("NO COLLISION")));
	
	if (!bHasCollision)
	{
		// Create room unit with unified method (includes numbering and creation)
		UStandardRoom* RoomUnit = NewObject<UStandardRoom>(this);
		bool bRoomCreated = RoomUnit->CreateFromRoomData(NewRoom, this, bShowRoomNumbers);
		
		if (bRoomCreated)
		{
			NewRoom.RoomUnit = RoomUnit;
			RoomUnits.Add(RoomUnit);
			
			// Create connections
			CreateRoomConnections(NewRoom);
			
			return true;
		}
	}
	
	return false;
}

bool ABackRoomGenerator::CheckRoomCollision(const FRoomData& TestRoom) const
{
	FBox TestBounds = TestRoom.GetBoundingBox();
	
	// Add minimal buffer to prevent rooms from touching
	float BufferSize = MetersToUnrealUnits(0.02f); // 2cm buffer for more room placement opportunities
	TestBounds = TestBounds.ExpandBy(BufferSize);
	
	for (int32 i = 0; i < GeneratedRooms.Num(); i++)
	{
		const FRoomData& ExistingRoom = GeneratedRooms[i];
		FBox ExistingBounds = ExistingRoom.GetBoundingBox();
		
		if (TestBounds.Intersect(ExistingBounds))
		{
			// Detailed collision debugging
			FString TestCategory = UEnum::GetValueAsString(TestRoom.Category);
			FString ExistingCategory = UEnum::GetValueAsString(ExistingRoom.Category);
			
			DebugLog(FString::Printf(TEXT("[X] COLLISION detected between rooms:")));
			DebugLog(FString::Printf(TEXT("   Test Room: %s, Elevation=%.2fm, Bounds Min=%s Max=%s"), 
				*TestCategory, TestRoom.Elevation, *TestBounds.Min.ToString(), *TestBounds.Max.ToString()));
			DebugLog(FString::Printf(TEXT("   Existing Room %d: %s, Elevation=%.2fm, Bounds Min=%s Max=%s"), 
				i, *ExistingCategory, ExistingRoom.Elevation, *ExistingBounds.Min.ToString(), *ExistingBounds.Max.ToString()));
			
			// Check if it's a vertical collision (different elevations)
			float VerticalSeparation = FMath::Abs(TestRoom.Elevation - ExistingRoom.Elevation);
			if (VerticalSeparation > 2.0f) // More than 2m elevation difference
			{
				DebugLog(FString::Printf(TEXT("   [!] SUSPICIOUS: %.2fm elevation difference should prevent collision!"), VerticalSeparation));
			}
			
			return true; // Collision detected
		}
	}
	
	return false; // No collision
}

bool ABackRoomGenerator::CheckRoomCollisionExcluding(const FRoomData& TestRoom, int32 ExcludeRoomIndex) const
{
	FBox TestBounds = TestRoom.GetBoundingBox();
	
	DebugLog(FString::Printf(TEXT("[DEBUG] BOUNDS: Room %d bounds Min=%s Max=%s"), 
		TestRoom.RoomIndex, *TestBounds.Min.ToString(), *TestBounds.Max.ToString()));
	
	// Add minimal buffer to prevent rooms from touching  
	float BufferSize = MetersToUnrealUnits(0.02f); // 2cm buffer for more room placement opportunities
	TestBounds = TestBounds.ExpandBy(BufferSize);
	
	DebugLog(FString::Printf(TEXT("[DEBUG] BUFFERED: Room %d buffered bounds Min=%s Max=%s"), 
		TestRoom.RoomIndex, *TestBounds.Min.ToString(), *TestBounds.Max.ToString()));
	
	// Debug boxes removed for cleaner visualization
	
	// Check against all existing rooms (excluding specified room)
	for (int32 i = 0; i < GeneratedRooms.Num(); i++)
	{
		// Skip the room we're connecting to
		if (i == ExcludeRoomIndex)
		{
			continue;
		}
		
		const FRoomData& ExistingRoom = GeneratedRooms[i];
		FBox ExistingBounds = ExistingRoom.GetBoundingBox();
		
		DebugLog(FString::Printf(TEXT("[DEBUG] CHECK: Room %d vs Room %d (%.1fx%.1fm, Elev=%.1fm, Bounds Min=%s Max=%s)"), 
			TestRoom.RoomIndex, i, ExistingRoom.Width, ExistingRoom.Length, ExistingRoom.Elevation,
			*ExistingBounds.Min.ToString(), *ExistingBounds.Max.ToString()));
		
		// Debug boxes removed for cleaner visualization
		
		if (TestBounds.Intersect(ExistingBounds))
		{
			// Debug boxes removed for cleaner visualization
			
			// Detailed collision debugging
			FString TestCategory = UEnum::GetValueAsString(TestRoom.Category);
			FString ExistingCategory = UEnum::GetValueAsString(ExistingRoom.Category);
			
			DebugLog(FString::Printf(TEXT("[X] COLLISION detected between rooms:")));
			DebugLog(FString::Printf(TEXT("   Test Room %d: %s, Elevation=%.2fm, Bounds Min=%s Max=%s"), 
				TestRoom.RoomIndex, *TestCategory, TestRoom.Elevation, *TestBounds.Min.ToString(), *TestBounds.Max.ToString()));
			DebugLog(FString::Printf(TEXT("   Existing Room %d: %s, Elevation=%.2fm, Bounds Min=%s Max=%s"), 
				i, *ExistingCategory, ExistingRoom.Elevation, *ExistingBounds.Min.ToString(), *ExistingBounds.Max.ToString()));
			
			// Check if it's a vertical collision (different elevations)
			float VerticalSeparation = FMath::Abs(TestRoom.Elevation - ExistingRoom.Elevation);
			if (VerticalSeparation > 2.0f) // More than 2m elevation difference
			{
				DebugLog(FString::Printf(TEXT("   [!] SUSPICIOUS: %.2fm elevation difference should prevent collision!"), VerticalSeparation));
			}
			
			return true; // Collision detected
		}
	}
	
	DebugLog(FString::Printf(TEXT("[DEBUG] NO COLLISION: Room %d passed all collision checks"), TestRoom.RoomIndex));
	return false; // No collision
}

void ABackRoomGenerator::CreateRoomConnections(FRoomData& Room)
{
	Room.Connections.Empty();
	
	// Create connections based on room type
	TArray<EWallSide> WallSides;
	
	if (Room.Category == ERoomCategory::Stairs)
	{
		// STAIRS: Only allow connection from the wall OPPOSITE to stair direction (bottom end)
		EWallSide BottomEndWall = EWallSide::None;
		switch (Room.StairDirection)
		{
			case EWallSide::North: BottomEndWall = EWallSide::South; break; // Stairs go north, connect at south (bottom)
			case EWallSide::South: BottomEndWall = EWallSide::North; break; // Stairs go south, connect at north (bottom)
			case EWallSide::East: BottomEndWall = EWallSide::West; break;   // Stairs go east, connect at west (bottom)
			case EWallSide::West: BottomEndWall = EWallSide::East; break;   // Stairs go west, connect at east (bottom)
		}
		
		WallSides.Add(BottomEndWall);
		DebugLog(FString::Printf(TEXT("STAIR CONNECTIONS: Stairs ascend %s, connecting at bottom end (%s wall)"), 
			*UEnum::GetValueAsString(Room.StairDirection), *UEnum::GetValueAsString(BottomEndWall)));
	}
	else
	{
		// REGULAR ROOMS/HALLWAYS: All 4 walls can have connections
		WallSides = {EWallSide::North, EWallSide::South, EWallSide::East, EWallSide::West};
	}
	
	for (EWallSide WallSide : WallSides)
	{
		FRoomConnection Connection;
		Connection.WallSide = WallSide;
		Connection.bIsUsed = false;
		Connection.ConnectionType = EConnectionType::Doorway; // Will be set when actually used
		Connection.ConnectionWidth = 0.8f; // Default door width (will be updated for openings)
		Connection.ConnectedRoomIndex = -1;
		
		// Calculate connection point (center of wall)
		if (Room.Category == ERoomCategory::Stairs)
		{
			// For stairs: connection point at the BOTTOM END (ground level) wall center
			FVector StairBottomCenter = Room.Position + FVector(
				MetersToUnrealUnits(Room.Width) * 0.5f,
				MetersToUnrealUnits(Room.Length) * 0.5f,
				MetersToUnrealUnits(Room.Height) * 0.5f // At standard room height (not elevated)
			);
			
			// Calculate connection point on the wall OPPOSITE to stair direction (bottom end)
			switch (Room.StairDirection)
			{
				case EWallSide::North:
					// Stairs go north, connection on south wall (bottom end)
					Connection.ConnectionPoint = FVector(
						StairBottomCenter.X, // Center X
						Room.Position.Y, // South wall Y position (bottom end)
						StairBottomCenter.Z  // At ground level
					);
					break;
				case EWallSide::South:
					// Stairs go south, connection on north wall (bottom end)
					Connection.ConnectionPoint = FVector(
						StairBottomCenter.X, // Center X
						Room.Position.Y + MetersToUnrealUnits(Room.Length), // North wall Y position (bottom end)
						StairBottomCenter.Z  // At ground level
					);
					break;
				case EWallSide::East:
					// Stairs go east, connection on west wall (bottom end)
					Connection.ConnectionPoint = FVector(
						Room.Position.X, // West wall X position (bottom end)
						StairBottomCenter.Y, // Center Y
						StairBottomCenter.Z  // At ground level
					);
					break;
				case EWallSide::West:
					// Stairs go west, connection on east wall (bottom end)
					Connection.ConnectionPoint = FVector(
						Room.Position.X + MetersToUnrealUnits(Room.Width), // East wall X position (bottom end)
						StairBottomCenter.Y, // Center Y
						StairBottomCenter.Z  // At ground level
					);
					break;
			}
		}
		else
		{
			// Standard room connection point calculation
			FVector RoomCenter = Room.Position + FVector(
				MetersToUnrealUnits(Room.Width) * 0.5f,
				MetersToUnrealUnits(Room.Length) * 0.5f,
				MetersToUnrealUnits(Room.Height) * 0.5f
			);
			
			switch (WallSide)
			{
				case EWallSide::North:
					Connection.ConnectionPoint = RoomCenter + FVector(0, MetersToUnrealUnits(Room.Length) * 0.5f, 0);
					break;
				case EWallSide::South:
					Connection.ConnectionPoint = RoomCenter - FVector(0, MetersToUnrealUnits(Room.Length) * 0.5f, 0);
					break;
				case EWallSide::East:
					Connection.ConnectionPoint = RoomCenter + FVector(MetersToUnrealUnits(Room.Width) * 0.5f, 0, 0);
					break;
				case EWallSide::West:
					Connection.ConnectionPoint = RoomCenter - FVector(MetersToUnrealUnits(Room.Width) * 0.5f, 0, 0);
					break;
			}
		}
		
		Room.Connections.Add(Connection);
	}
}

FVector ABackRoomGenerator::CalculateConnectionPosition(const FRoomData& SourceRoom, int32 ConnectionIndex, const FRoomData& NewRoom)
{
	const FRoomConnection& Connection = SourceRoom.Connections[ConnectionIndex];
	FVector ConnectionPoint = Connection.ConnectionPoint;
	
	DebugLog(FString::Printf(TEXT("Source room %d connection %d at %s (wall: %s)"), 
		SourceRoom.RoomIndex, ConnectionIndex, *ConnectionPoint.ToString(), 
		*UEnum::GetValueAsString(Connection.WallSide)));
	
	// Calculate offset based on wall side
	FVector Offset = FVector::ZeroVector;
	
	// Calculate Z offset based on room type
	float ZOffset;
	if (SourceRoom.Category == ERoomCategory::Stairs)
	{
		// For stairs: We want the new room's floor to be at ConnectionPoint.Z level
		// The new room will render with its geometry at Position.Z + Elevation
		// So Position.Z should be ConnectionPoint.Z - Elevation, which means ZOffset = -Elevation
		float NewRoomElevationCm = MetersToUnrealUnits(NewRoom.Elevation);
		ZOffset = -NewRoomElevationCm;
		DebugLog(FString::Printf(TEXT("STAIR CONNECTION: Setting ZOffset=%.1fcm to align room floor (elevation %.1fm) with connection point at Z=%.1fcm"), 
			ZOffset, NewRoom.Elevation, ConnectionPoint.Z));
	}
	else
	{
		// Standard connection: same floor level as source room
		ZOffset = SourceRoom.Position.Z - ConnectionPoint.Z;
	}
	
	// Wall thickness - rooms have 20cm thick walls
	float WallThickness = MetersToUnrealUnits(0.2f); // 20cm = 0.2m
	
	// Add a minimal gap to prevent wall z-fighting 
	float AntiFlickerGap = MetersToUnrealUnits(0.01f); // 1cm gap - minimal to prevent z-fighting
	
	// Position rooms - special handling for stairs
	if (SourceRoom.Category == ERoomCategory::Stairs)
	{
		// STAIR CONNECTION: Position new room at the TOP END of stairs facing same direction
		// The connection must be from the wall that faces the stair direction
		
		// Use the existing connection point from the stair (already calculated correctly)
		// The connection point is already at the proper location and elevation
		
		switch (SourceRoom.StairDirection)
		{
			case EWallSide::North:
				// Place new room north of the stair connection point
				Offset = FVector(
					-MetersToUnrealUnits(NewRoom.Width) * 0.5f,    // Center new room X on connection point X
					WallThickness + AntiFlickerGap,                // Push north from connection point
					ZOffset                                        // Use calculated Z offset
				);
				break;
			case EWallSide::South:
				// Place new room south of the stair connection point
				Offset = FVector(
					-MetersToUnrealUnits(NewRoom.Width) * 0.5f,    // Center new room X on connection point X
					-MetersToUnrealUnits(NewRoom.Length) - WallThickness - AntiFlickerGap, // Push south from connection point
					ZOffset                                        // Use calculated Z offset
				);
				break;
			case EWallSide::East:
				// Place new room east of the stair connection point
				Offset = FVector(
					WallThickness + AntiFlickerGap,                // Push east from connection point
					-MetersToUnrealUnits(NewRoom.Length) * 0.5f,   // Center new room Y on connection point Y
					ZOffset                                        // Use calculated Z offset
				);
				break;
			case EWallSide::West:
				// Place new room west of the stair connection point
				Offset = FVector(
					-MetersToUnrealUnits(NewRoom.Width) - WallThickness - AntiFlickerGap, // Push west from connection point
					-MetersToUnrealUnits(NewRoom.Length) * 0.5f,   // Center new room Y on connection point Y
					ZOffset                                        // Use calculated Z offset
				);
				break;
		}
		
		DebugLog(FString::Printf(TEXT("STAIR CONNECTION: Using connection point %s, offset %s"), 
			*ConnectionPoint.ToString(), *Offset.ToString()));
	}
	else
	{
		// STANDARD CONNECTION: Position rooms normally based on wall side
		switch (Connection.WallSide)
		{
			case EWallSide::North:
				Offset = FVector(
					-MetersToUnrealUnits(NewRoom.Width) * 0.5f,
					WallThickness + AntiFlickerGap,
					ZOffset
				);
				break;
			case EWallSide::South:
				Offset = FVector(
					-MetersToUnrealUnits(NewRoom.Width) * 0.5f,
					-MetersToUnrealUnits(NewRoom.Length) - WallThickness - AntiFlickerGap,
					ZOffset
				);
				break;
			case EWallSide::East:
				Offset = FVector(
					WallThickness + AntiFlickerGap,
					-MetersToUnrealUnits(NewRoom.Length) * 0.5f,
					ZOffset
				);
				break;
			case EWallSide::West:
				Offset = FVector(
					-MetersToUnrealUnits(NewRoom.Width) - WallThickness - AntiFlickerGap,
					-MetersToUnrealUnits(NewRoom.Length) * 0.5f,
					ZOffset
				);
				break;
		}
	}
	
	FVector FinalPosition = ConnectionPoint + Offset;
	DebugLog(FString::Printf(TEXT("Calculated offset: %s, Final position: %s"), 
		*Offset.ToString(), *FinalPosition.ToString()));
	
	return FinalPosition;
}

void ABackRoomGenerator::ConnectRooms(FRoomData& Room1, int32 Connection1Index, FRoomData& Room2, int32 Connection2Index)
{
	// Determine connection type: 30% doorway, 70% opening (balanced backrooms feel)
	FRandomStream Random(FDateTime::Now().GetTicks());
	EConnectionType ConnectionType = (Random.FRand() < 0.3f) ? EConnectionType::Doorway : EConnectionType::Opening;
	
	// Calculate connection width based on type
	float ConnectionWidth = 0.8f; // Default doorway width
	if (ConnectionType == EConnectionType::Opening)
	{
		// For openings, use up to the smallest wall width in the connection
		float Room1WallSize = (Room1.Connections[Connection1Index].WallSide == EWallSide::North || 
		                      Room1.Connections[Connection1Index].WallSide == EWallSide::South) ? 
		                      Room1.Width : Room1.Length;
		float Room2WallSize = (Room2.Connections.Num() > 0) ? 
		                      ((Room2.Connections[0].WallSide == EWallSide::North || 
		                        Room2.Connections[0].WallSide == EWallSide::South) ? 
		                        Room2.Width : Room2.Length) : Room2.Width;
		
		// Use 60-100% of the largest wall for opening width (very large openings)
		float LargestWallSize = FMath::Max(Room1WallSize, Room2WallSize);
		float OpeningPercentage = Random.FRandRange(0.6f, 1.0f); // 60% to 100%
		ConnectionWidth = LargestWallSize * OpeningPercentage;
		ConnectionWidth = FMath::Clamp(ConnectionWidth, 1.0f, 100.0f); // Min 1m, max 100m opening
	}
	
	// Mark connections as used with determined type and width
	Room1.Connections[Connection1Index].bIsUsed = true;
	Room1.Connections[Connection1Index].ConnectedRoomIndex = Room2.RoomIndex;
	Room1.Connections[Connection1Index].ConnectionType = ConnectionType;
	Room1.Connections[Connection1Index].ConnectionWidth = ConnectionWidth;
	
	// Find matching connection on Room2 (opposite wall)
	EWallSide OppositeWall = EWallSide::None;
	switch (Room1.Connections[Connection1Index].WallSide)
	{
		case EWallSide::North: OppositeWall = EWallSide::South; break;
		case EWallSide::South: OppositeWall = EWallSide::North; break;
		case EWallSide::East: OppositeWall = EWallSide::West; break;
		case EWallSide::West: OppositeWall = EWallSide::East; break;
	}
	
	// Find and mark the opposite connection
	for (int32 i = 0; i < Room2.Connections.Num(); i++)
	{
		if (Room2.Connections[i].WallSide == OppositeWall)
		{
			Room2.Connections[i].bIsUsed = true;
			Room2.Connections[i].ConnectedRoomIndex = Room1.RoomIndex;
			Room2.Connections[i].ConnectionType = ConnectionType;
			Room2.Connections[i].ConnectionWidth = ConnectionWidth;
			break;
		}
	}
	
	// Determine which wall is smaller at connection point - remove smaller wall, put hole in larger wall
	float Room1WallSize = (Room1.Connections[Connection1Index].WallSide == EWallSide::North || 
	                      Room1.Connections[Connection1Index].WallSide == EWallSide::South) ? 
	                      Room1.Width : Room1.Length;
	float Room2WallSize = (OppositeWall == EWallSide::North || OppositeWall == EWallSide::South) ? 
	                      Room2.Width : Room2.Length;
	
	DebugLog(FString::Printf(TEXT("WALL SIZE COMPARISON: Room %d %s wall=%.1fm vs Room %d %s wall=%.1fm"), 
		Room1.RoomIndex, *UEnum::GetValueAsString(Room1.Connections[Connection1Index].WallSide), Room1WallSize,
		Room2.RoomIndex, *UEnum::GetValueAsString(OppositeWall), Room2WallSize));
	
	// OPTIMIZED: Remove smaller wall, extend larger wall thickness to eliminate overlap
	float SmallerWallSize = FMath::Min(Room1WallSize, Room2WallSize);
	float SafeConnectionWidth = FMath::Min(ConnectionWidth, SmallerWallSize - 0.5f); // Leave 0.5m margin
	SafeConnectionWidth = FMath::Max(SafeConnectionWidth, 0.6f); // Minimum 0.6m hole for navigation
	
	// Determine which room has the larger wall
	bool Room1HasLargerWall = (Room1WallSize >= Room2WallSize);
	FRoomData& LargerWallRoom = Room1HasLargerWall ? Room1 : Room2;
	FRoomData& SmallerWallRoom = Room1HasLargerWall ? Room2 : Room1;
	EWallSide LargerWallSide = Room1HasLargerWall ? Room1.Connections[Connection1Index].WallSide : OppositeWall;
	EWallSide SmallerWallSide = Room1HasLargerWall ? OppositeWall : Room1.Connections[Connection1Index].WallSide;
	
	DebugLog(FString::Printf(TEXT("OVERLAP ELIMINATION: Room %d (%s wall %.1fm) removes wall, Room %d (%s wall %.1fm) gets thick wall with hole"), 
		SmallerWallRoom.RoomIndex, *UEnum::GetValueAsString(SmallerWallSide), SmallerWallSize,
		LargerWallRoom.RoomIndex, *UEnum::GetValueAsString(LargerWallSide), Room1HasLargerWall ? Room1WallSize : Room2WallSize));
	
	// Remove the smaller wall completely (no wall at all)
	FDoorConfig RemovalConfig;
	RemovalConfig.bHasDoor = true;
	RemovalConfig.WallSide = SmallerWallSide;
	RemovalConfig.Width = 999.0f; // Special value for complete wall removal
	RemovalConfig.Height = SmallerWallRoom.Height;
	RemovalConfig.OffsetFromCenter = 0.0f;
	SmallerWallRoom.RoomUnit->AddHoleToWall(this, SmallerWallSide, RemovalConfig);
	
	// Create the larger wall with hole and double thickness (0.4m instead of 0.2m)
	CreateConnectionInRoomWallWithThickness(LargerWallRoom, LargerWallSide, ConnectionType, SafeConnectionWidth, 0.4f, SmallerWallSize);
	
	FString ConnectionTypeStr = (ConnectionType == EConnectionType::Doorway) ? TEXT("doorway") : TEXT("opening");
	DebugLog(FString::Printf(TEXT("Connected room %d (%s wall) to room %d (%s wall) with %s (%.1fm wide)"), 
		Room1.RoomIndex, 
		*UEnum::GetValueAsString(Room1.Connections[Connection1Index].WallSide),
		Room2.RoomIndex,
		*UEnum::GetValueAsString(OppositeWall),
		*ConnectionTypeStr,
		ConnectionWidth));
}

void ABackRoomGenerator::DebugLog(const FString& Message) const
{
	// Get current timestamp with milliseconds
	FDateTime Now = FDateTime::Now();
	FString TimeStamp = FString::Printf(TEXT("[%02d:%02d:%02d.%03d]"), 
		Now.GetHour(), Now.GetMinute(), Now.GetSecond(), Now.GetMillisecond());
	
	UE_LOG(LogBackRoomGenerator, Log, TEXT("%s %s"), *TimeStamp, *Message);
}

void ABackRoomGenerator::CreateRoomNumberIdentifier(const FRoomData& Room)
{
	if (!Room.RoomUnit) return;
	
	// Create text render component for the room number
	UTextRenderComponent* NumberText = NewObject<UTextRenderComponent>(this, 
		*FString::Printf(TEXT("RoomNumberText_%d"), Room.RoomIndex));
	
	if (!NumberText)
	{
		DebugLog(TEXT("Failed to create text render component"));
		return;
	}
	
	// Set the text to display the room number
	NumberText->SetText(FText::FromString(FString::Printf(TEXT("%d"), Room.RoomIndex)));
	
	// Configure text appearance
	NumberText->SetTextRenderColor(FColor::White);
	NumberText->SetWorldSize(200.0f); // Large text size
	NumberText->SetHorizontalAlignment(EHTA_Center);
	NumberText->SetVerticalAlignment(EVRTA_TextCenter);
	
	// Setup attachment and register
	NumberText->SetupAttachment(GetRootComponent());
	NumberText->RegisterComponent();
	
	// Position above room center
	FVector RoomCenter = Room.Position + FVector(
		MetersToUnrealUnits(Room.Width) * 0.5f,
		MetersToUnrealUnits(Room.Length) * 0.5f,
		MetersToUnrealUnits(Room.Height) + MetersToUnrealUnits(1.5f) // 1.5m above room
	);
	
	NumberText->SetWorldLocation(RoomCenter);
	
	// Make text vertical (standing upright) - no rotation needed for vertical text
	NumberText->SetWorldRotation(FRotator(0.0f, 0.0f, 0.0f));
	
	DebugLog(FString::Printf(TEXT("Created room number text '%d' at %s"), 
		Room.RoomIndex, *RoomCenter.ToString()));
}

void ABackRoomGenerator::CreateConnectionInRoomWall(FRoomData& Room, EWallSide WallSide, EConnectionType ConnectionType, float ConnectionWidth)
{
	if (!Room.RoomUnit) return;
	
	// Create a door/opening configuration for this wall
	FDoorConfig DoorConfig;
	DoorConfig.bHasDoor = true;
	DoorConfig.WallSide = WallSide;
	DoorConfig.HoleShape = EHoleShape::Rectangle; // Use simple rectangles for performance
	DoorConfig.OffsetFromCenter = 0.0f; // Centered on wall
	
	if (ConnectionType == EConnectionType::Doorway)
	{
		// Standard doorway: 0.8m wide x 2.0m high
		DoorConfig.Width = 0.8f;
		DoorConfig.Height = 2.0f;
	}
	else // EConnectionType::Opening
	{
		// Large opening: custom width x full room height (minus small margin)
		DoorConfig.Width = ConnectionWidth;
		DoorConfig.Height = Room.Height - 0.5f; // Leave 50cm from ceiling for structural integrity
		DoorConfig.Height = FMath::Max(DoorConfig.Height, 2.0f); // Minimum 2m height
	}
	
	// Add the connection configuration to the room unit
	Room.RoomUnit->DoorConfigs.Add(DoorConfig);
	
	// Connection added successfully
	
	// Instead of regenerating entire room, just recreate the specific wall with the connection
	RegenerateSpecificWall(Room, WallSide, DoorConfig);
	
	FString ConnectionTypeStr = (ConnectionType == EConnectionType::Doorway) ? TEXT("doorway") : TEXT("opening");
	DebugLog(FString::Printf(TEXT("Created %s in room %d on %s wall (%.1fm wide x %.1fm high)"), 
		*ConnectionTypeStr, Room.RoomIndex, *UEnum::GetValueAsString(WallSide), 
		DoorConfig.Width, DoorConfig.Height));
}

void ABackRoomGenerator::CreateConnectionInRoomWallWithThickness(FRoomData& Room, EWallSide WallSide, EConnectionType ConnectionType, float ConnectionWidth, float WallThickness, float SmallerWallSize)
{
	if (!Room.RoomUnit) return;
	
	DebugLog(FString::Printf(TEXT("🔧 Creating thick wall: Room %d %s wall with %.1fm hole, %.1fm thickness (smaller wall=%.1fm)"), 
		Room.RoomIndex, *UEnum::GetValueAsString(WallSide), ConnectionWidth, WallThickness, SmallerWallSize));
	
	// Create a door/opening configuration for this wall with custom thickness
	FDoorConfig DoorConfig;
	DoorConfig.bHasDoor = true;
	DoorConfig.WallSide = WallSide;
	DoorConfig.HoleShape = EHoleShape::Rectangle; // Use simple rectangles for performance
	DoorConfig.OffsetFromCenter = 0.0f; // Centered on wall
	
	if (ConnectionType == EConnectionType::Doorway)
	{
		// Standard doorway: 0.8m wide x 2.0m high
		DoorConfig.Width = 0.8f;
		DoorConfig.Height = 2.0f;
	}
	else
	{
		// Opening: Use provided width, standard height
		DoorConfig.Width = ConnectionWidth;
		DoorConfig.Height = 2.5f;
	}
	
	// Add the connection configuration to the room unit
	Room.RoomUnit->DoorConfigs.Add(DoorConfig);
	
	// Use AddHoleToWallWithThickness method to create wall with custom thickness
	Room.RoomUnit->AddHoleToWallWithThickness(this, WallSide, DoorConfig, WallThickness, SmallerWallSize);
	
	FString ConnectionTypeStr = (ConnectionType == EConnectionType::Doorway) ? TEXT("doorway") : TEXT("opening");
	DebugLog(FString::Printf(TEXT("Created thick %s in room %d on %s wall (%.1fm wide x %.1fm high, %.1fm thick)"), 
		*ConnectionTypeStr, Room.RoomIndex, *UEnum::GetValueAsString(WallSide), 
		DoorConfig.Width, DoorConfig.Height, WallThickness));
}

void ABackRoomGenerator::RegenerateSpecificWall(FRoomData& Room, EWallSide WallSide, const FDoorConfig& DoorConfig)
{
	UWorld* World = GetWorld();
	if (!World || !Room.RoomUnit) 
	{
		DebugLog(TEXT("RegenerateSpecificWall: Invalid World or RoomUnit"));
		return;
	}

	DebugLog(FString::Printf(TEXT("🔧 RegenerateSpecificWall: Room %d, Wall %s, Width %.1fm"), 
		Room.RoomIndex, *UEnum::GetValueAsString(WallSide), DoorConfig.Width));

	// Use the new AddHoleToWall method instead of full room regeneration
	Room.RoomUnit->AddHoleToWall(this, WallSide, DoorConfig);
}

void ABackRoomGenerator::CreateIdentifierSpheres(UStandardRoom* Room)
{
	if (!Room) return;
	
	// Load sphere mesh
	UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere"));
	if (!SphereMesh)
	{
		DebugLog(TEXT("Failed to load sphere mesh for identifiers"));
		return;
	}
	
	// Room dimensions in cm
	float RoomWidthCm = MetersToUnrealUnits(Room->Width);
	float RoomLengthCm = MetersToUnrealUnits(Room->Length);
	float RoomHeightCm = MetersToUnrealUnits(Room->Height);
	
	// Sphere configurations: {position, color, description}
	struct SphereConfig
	{
		FVector Position;
		FLinearColor Color;
		FString Description;
	};
	
	TArray<SphereConfig> SphereConfigs = {
		// North wall (red sphere) - center of north wall, outside
		{
			Room->Position + FVector(RoomWidthCm * 0.5f, RoomLengthCm + 100.0f, RoomHeightCm * 0.5f),
			FLinearColor::Red,
			TEXT("North Wall (RED - 0° rotation)")
		},
		// South wall (blue sphere) - center of south wall, outside  
		{
			Room->Position + FVector(RoomWidthCm * 0.5f, -100.0f, RoomHeightCm * 0.5f),
			FLinearColor::Blue,
			TEXT("South Wall (BLUE - 45° rotation)")
		},
		// East wall (yellow sphere) - center of east wall, outside
		{
			Room->Position + FVector(RoomWidthCm + 100.0f, RoomLengthCm * 0.5f, RoomHeightCm * 0.5f),
			FLinearColor::Yellow,
			TEXT("East Wall (YELLOW - 30° rotation)")
		},
		// West wall (green sphere) - center of west wall, outside
		{
			Room->Position + FVector(-100.0f, RoomLengthCm * 0.5f, RoomHeightCm * 0.5f),
			FLinearColor::Green,
			TEXT("West Wall (GREEN - 60° rotation)")
		},
		// Floor (white sphere) - above room center
		{
			Room->Position + FVector(RoomWidthCm * 0.5f, RoomLengthCm * 0.5f, RoomHeightCm + 100.0f),
			FLinearColor::White,
			TEXT("Floor (WHITE - 45° rotation)")
		}
	};
	
	// Create spheres
	for (int32 i = 0; i < SphereConfigs.Num(); i++)
	{
		const SphereConfig& Config = SphereConfigs[i];
		
		// Create sphere component
		UStaticMeshComponent* SphereComponent = NewObject<UStaticMeshComponent>(this, 
			*FString::Printf(TEXT("IdentifierSphere_%d"), i));
		
		SphereComponent->SetStaticMesh(SphereMesh);
		SphereComponent->SetupAttachment(GetRootComponent());
		SphereComponent->RegisterComponent();
		SphereComponent->SetWorldLocation(Config.Position);
		SphereComponent->SetWorldScale3D(FVector(0.5f)); // 50cm diameter spheres
		
		// Create dynamic material with primary color
		UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, 
			TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
		
		if (BaseMaterial)
		{
			UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			if (DynamicMaterial)
			{
				DynamicMaterial->SetVectorParameterValue(TEXT("Color"), Config.Color);
				SphereComponent->SetMaterial(0, DynamicMaterial);
			}
		}
		
		DebugLog(FString::Printf(TEXT("Created identifier sphere for %s at %s"), 
			*Config.Description, *Config.Position.ToString()));
	}
}

void ABackRoomGenerator::TestSmallToLargeRoomConnection()
{
	DebugLog(TEXT("================================================================================"));
	DebugLog(TEXT("🧪 STARTING BOUNDARY TEST: Small Room (5m) to Large Room (20m) Connection"));
	DebugLog(TEXT("================================================================================"));
	
	// Clear any existing data
	RoomUnits.Empty();
	GeneratedRooms.Empty();
	
	// Get character location for positioning
	FVector CharacterLocation = FVector::ZeroVector;
	UWorld* World = GetWorld();
	if (IsValid(World))
	{
		APlayerController* PC = World->GetFirstPlayerController();
		if (IsValid(PC))
		{
			ACharacter* Character = PC->GetCharacter();
			if (IsValid(Character))
			{
				CharacterLocation = Character->GetActorLocation();
			}
		}
	}
	
	if (CharacterLocation.IsZero())
	{
		CharacterLocation = FVector::ZeroVector;
	}
	
	// Create small room (5x5m) - the one that will have its wall removed
	FRoomData SmallRoom;
	SmallRoom.RoomIndex = 0;
	SmallRoom.Position = CharacterLocation + FVector(-250.0f, -250.0f, -50.0f); // 2.5m west, 2.5m south, 0.5m below user
	SmallRoom.Width = 5.0f;   // 5 meters wide
	SmallRoom.Length = 5.0f;  // 5 meters long
	SmallRoom.Height = 3.0f;  // Standard height
	SmallRoom.Category = ERoomCategory::Room;
	SmallRoom.Elevation = 0.0f;
	
	// Create the room unit
	SmallRoom.RoomUnit = NewObject<UStandardRoom>(this);
	SmallRoom.RoomUnit->Position = SmallRoom.Position;
	SmallRoom.RoomUnit->Width = SmallRoom.Width;
	SmallRoom.RoomUnit->Length = SmallRoom.Length;
	SmallRoom.RoomUnit->Height = SmallRoom.Height;
	SmallRoom.RoomUnit->RoomCategory = SmallRoom.Category;
	SmallRoom.RoomUnit->Elevation = SmallRoom.Elevation;
	
	// Create connections for small room
	CreateRoomConnections(SmallRoom);
	
	// Create the small room geometry
	SmallRoom.RoomUnit->CreateRoomUsingIndividualActors(this);
	
	// Add to arrays
	GeneratedRooms.Add(SmallRoom);
	RoomUnits.Add(SmallRoom.RoomUnit);
	
	DebugLog(FString::Printf(TEXT("✅ Created small room (%.1fx%.1fm) at %s"), 
		SmallRoom.Width, SmallRoom.Length, *SmallRoom.Position.ToString()));
	
	// Create large room (20x20m) - the one that will get the thick wall with constrained hole
	FRoomData LargeRoom;
	LargeRoom.RoomIndex = 1;
	LargeRoom.Position = CharacterLocation + FVector(520.0f, -250.0f, -50.0f); // Adjacent to small room east wall
	LargeRoom.Width = 20.0f;  // 20 meters wide
	LargeRoom.Length = 20.0f; // 20 meters long
	LargeRoom.Height = 3.0f;  // Standard height
	LargeRoom.Category = ERoomCategory::Room;
	LargeRoom.Elevation = 0.0f;
	
	// Create the room unit
	LargeRoom.RoomUnit = NewObject<UStandardRoom>(this);
	LargeRoom.RoomUnit->Position = LargeRoom.Position;
	LargeRoom.RoomUnit->Width = LargeRoom.Width;
	LargeRoom.RoomUnit->Length = LargeRoom.Length;
	LargeRoom.RoomUnit->Height = LargeRoom.Height;
	LargeRoom.RoomUnit->RoomCategory = LargeRoom.Category;
	LargeRoom.RoomUnit->Elevation = LargeRoom.Elevation;
	
	// Create connections for large room
	CreateRoomConnections(LargeRoom);
	
	// Create the large room geometry
	LargeRoom.RoomUnit->CreateRoomUsingIndividualActors(this);
	
	// Add to arrays
	GeneratedRooms.Add(LargeRoom);
	RoomUnits.Add(LargeRoom.RoomUnit);
	
	DebugLog(FString::Printf(TEXT("✅ Created large room (%.1fx%.1fm) at %s"), 
		LargeRoom.Width, LargeRoom.Length, *LargeRoom.Position.ToString()));
	
	// Now test the connection: Small room East wall (5m) to Large room West wall (20m)
	// The hole should be constrained to the 5m smaller wall bounds
	
	DebugLog(TEXT("🔗 Testing connection: Small room East wall (5m) → Large room West wall (20m)"));
	DebugLog(TEXT("🎯 Expected: Hole should be positioned within 5m bounds, not randomly across 20m wall"));
	
	// Connect rooms using the wall overlap elimination system
	int32 SmallRoomConnectionIndex = 1; // East wall of small room
	int32 LargeRoomConnectionIndex = 3; // West wall of large room
	
	ConnectRooms(GeneratedRooms[0], SmallRoomConnectionIndex, GeneratedRooms[1], LargeRoomConnectionIndex);
	
	DebugLog(TEXT("================================================================================"));
	DebugLog(TEXT("🧪 BOUNDARY TEST COMPLETED - Check logs above for hole positioning"));
	DebugLog(TEXT("================================================================================"));
}

