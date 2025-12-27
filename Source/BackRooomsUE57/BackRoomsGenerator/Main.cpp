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
	
	// Initialize with default configuration values
	// All settings now centralized in Config struct
	Config = FBackroomGenerationConfig();
	
	// Initialize services
	CollisionService = MakeUnique<FCollisionDetectionService>();
	ConnectionManager = MakeUnique<FRoomConnectionManager>();
	GenerationOrchestrator = MakeUnique<FGenerationOrchestrator>();
	
	// Configuration is set via GenerationConfig.h defaults (currently 4 rooms for testing)
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
	// Clear any existing rooms and pre-allocate memory
	RoomUnits.Empty();
	RoomUnits.Reserve(Config.TotalRooms);
	GeneratedRooms.Empty();
	GeneratedRooms.Reserve(Config.TotalRooms);
	
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
	// Pre-allocate memory to avoid reallocations during generation
	RoomUnits.Empty();
	RoomUnits.Reserve(Config.TotalRooms);
	GeneratedRooms.Empty();
	GeneratedRooms.Reserve(Config.TotalRooms);
	
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
	
	// Use the Generation Orchestrator service to handle the main generation loop
	int32 GeneratedCount = GenerationOrchestrator->ExecuteProceduralGeneration(
		InitialRoom,
		GeneratedRooms,
		Config,
		CollisionService.Get(),
		ConnectionManager.Get(),
		[this](FRoomData& Room) {
			// Room creator function - create the actual UE room unit
			UStandardRoom* RoomUnit = NewObject<UStandardRoom>(this);
			if (RoomUnit && RoomUnit->CreateFromRoomData(Room, this, Config.bShowRoomNumbers))
			{
				Room.RoomUnit = RoomUnit;
				RoomUnits.Add(RoomUnit);
			}
		}
	);
	
	// Get generation statistics
	int32 MainLoops, ConnectionRetries, PlacementAttempts;
	double ElapsedTime;
	GenerationOrchestrator->GetGenerationStats(MainLoops, ConnectionRetries, PlacementAttempts, ElapsedTime);
	
	// Log summary
	DebugLog(FString::Printf(TEXT("✅ GENERATION COMPLETED: %d/%d rooms generated"), 
		GeneratedCount, Config.TotalRooms));
	DebugLog(FString::Printf(TEXT("⏱️  Generation took %.2f seconds"), ElapsedTime));
	DebugLog(FString::Printf(TEXT("🔄 Loop counters: Main=%d, Connection=%d, Placement=%d"), 
		MainLoops, ConnectionRetries, PlacementAttempts));
	
	if (GenerationOrchestrator->WasStoppedBySafety())
	{
		DebugLog(TEXT("⚠️  Generation stopped due to safety limits"));
	}
	
	// Ensure we don't have more rooms than intended
	if (GeneratedRooms.Num() > Config.TotalRooms)
	{
		DebugLog(FString::Printf(TEXT("WARNING: Generated %d rooms but target was %d!"), 
			GeneratedRooms.Num(), Config.TotalRooms));
	}
	
	// Print comprehensive room size summary
	UE_LOG(LogTemp, Warning, TEXT("🚀 About to call PrintRoomSizeSummary()..."));
	PrintRoomSizeSummary();
	UE_LOG(LogTemp, Warning, TEXT("✅ PrintRoomSizeSummary() completed"));
}
void ABackRoomGenerator::GenerateBackroomsInTestMode()
{
	// Clear any existing rooms
	RoomUnits.Empty();
	GeneratedRooms.Empty();
	
	DebugLog(TEXT("=== MAIN TEST MODE: Spawning TestGenerator for WallUnit tests ==="));
	
	// Spawn the TestGenerator Actor to perform automated wall tests
	if (UWorld* World = GetWorld())
	{
		if (ATestGenerator* TestGen = World->SpawnActor<ATestGenerator>())
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
	InitialRoom.Width = BackroomConstants::INITIAL_ROOM_SIZE;  // Configurable starting room size
	InitialRoom.Length = BackroomConstants::INITIAL_ROOM_SIZE;
	InitialRoom.Height = Config.StandardRoomHeight;
	InitialRoom.RoomIndex = 0;
	
	// Position room so character stands on the floor surface  
	InitialRoom.Position = FVector(
		CharacterLocation.X - MetersToUnrealUnits(InitialRoom.Width) * 0.5f,
		CharacterLocation.Y - MetersToUnrealUnits(InitialRoom.Length) * 0.5f,
		CharacterLocation.Z - MetersToUnrealUnits(BackroomConstants::INITIAL_ROOM_FLOOR_OFFSET) // Room floor offset below character
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
	ConnectionManager->CreateRoomConnections(InitialRoom, Config);
	
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
		const SphereConfig& SphereConf = SphereConfigs[i];
		
		// Create sphere component
		UStaticMeshComponent* SphereComponent = NewObject<UStaticMeshComponent>(this, 
			*FString::Printf(TEXT("IdentifierSphere_%d"), i));
		
		SphereComponent->SetStaticMesh(SphereMesh);
		SphereComponent->SetupAttachment(GetRootComponent());
		SphereComponent->RegisterComponent();
		SphereComponent->SetWorldLocation(SphereConf.Position);
		SphereComponent->SetWorldScale3D(FVector(0.5f)); // 50cm diameter spheres
		
		// Create dynamic material with primary color
		UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, 
			TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
		
		if (BaseMaterial)
		{
			UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			if (DynamicMaterial)
			{
				DynamicMaterial->SetVectorParameterValue(TEXT("Color"), SphereConf.Color);
				SphereComponent->SetMaterial(0, DynamicMaterial);
			}
		}
		
		DebugLog(FString::Printf(TEXT("Created identifier sphere for %s at %s"), 
			*SphereConf.Description, *SphereConf.Position.ToString()));
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
	ConnectionManager->CreateRoomConnections(SmallRoom, Config);
	
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
	ConnectionManager->CreateRoomConnections(LargeRoom, Config);
	
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
	
	ConnectionManager->ConnectRooms(GeneratedRooms[0], SmallRoomConnectionIndex, GeneratedRooms[1], LargeRoomConnectionIndex, Config);
	
	DebugLog(TEXT("================================================================================"));
	DebugLog(TEXT("🧪 BOUNDARY TEST COMPLETED - Check logs above for hole positioning"));
	DebugLog(TEXT("================================================================================"));
}

void ABackRoomGenerator::PrintRoomSizeSummary()
{
	UE_LOG(LogTemp, Warning, TEXT("🔍 PrintRoomSizeSummary called - GeneratedRooms.Num()=%d"), GeneratedRooms.Num());
	
	if (GeneratedRooms.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("⚠️ No rooms found in GeneratedRooms array!"));
		return;
	}
	
	DebugLog(TEXT(""));
	DebugLog(TEXT("📏 ================ ROOM SIZE SUMMARY ================"));
	
	// Count room categories
	int32 TotalRooms = 0;
	int32 StandardRooms = 0;
	int32 ShortHallways = 0;
	int32 MediumHallways = 0;
	int32 LongHallways = 0;
	int32 StairRooms = 0;
	
	// Process each room and log its size
	for (const FRoomData& Room : GeneratedRooms)
	{
		TotalRooms++;
		
		if (Room.Category == ERoomCategory::Room)
		{
			StandardRooms++;
			UE_LOG(LogTemp, Warning, TEXT("🔸 Room %d: %.1fm x %.1fm (W x L) - Standard Room"), 
				Room.RoomIndex + 1, Room.Width, Room.Length);
		}
		else if (Room.Category == ERoomCategory::Hallway)
		{
			// Determine hallway category based on longer dimension
			float LongerDimension = FMath::Max(Room.Width, Room.Length);
			FString Category;
			
			if (LongerDimension < Config.MediumHallwayThreshold)
			{
				ShortHallways++;
				Category = TEXT("SHORT");
			}
			else if (LongerDimension < Config.LongHallwayThreshold)
			{
				MediumHallways++;
				Category = TEXT("MEDIUM");
			}
			else
			{
				LongHallways++;
				Category = TEXT("LONG");
			}
			
			UE_LOG(LogTemp, Warning, TEXT("🔹 Room %d: %.1fm x %.1fm (W x L) - %s Hallway"), 
				Room.RoomIndex + 1, Room.Width, Room.Length, *Category);
		}
		else if (Room.Category == ERoomCategory::Stairs)
		{
			StairRooms++;
			UE_LOG(LogTemp, Warning, TEXT("🔺 Room %d: %.1fm x %.1fm (W x L) - Stairs (Elevation: %.1fm)"), 
				Room.RoomIndex + 1, Room.Width, Room.Length, Room.Elevation);
		}
	}
	
	// Print category statistics
	DebugLog(TEXT(""));
	DebugLog(TEXT("📊 ================ GENERATION STATISTICS ================"));
	DebugLog(FString::Printf(TEXT("🔸 Standard Rooms: %d (%.1f%%)"), 
		StandardRooms, TotalRooms > 0 ? (StandardRooms * 100.0f / TotalRooms) : 0.0f));
	
	int32 TotalHallways = ShortHallways + MediumHallways + LongHallways;
	DebugLog(FString::Printf(TEXT("🔹 Total Hallways: %d (%.1f%%)"), 
		TotalHallways, TotalRooms > 0 ? (TotalHallways * 100.0f / TotalRooms) : 0.0f));
		
	if (TotalHallways > 0)
	{
		DebugLog(FString::Printf(TEXT("   • Short Hallways (12-20m): %d (%.1f%%)"), 
			ShortHallways, (ShortHallways * 100.0f / TotalHallways)));
		DebugLog(FString::Printf(TEXT("   • Medium Hallways (20-35m): %d (%.1f%%)"), 
			MediumHallways, (MediumHallways * 100.0f / TotalHallways)));
		DebugLog(FString::Printf(TEXT("   • Long Hallways (35-50m): %d (%.1f%%)"), 
			LongHallways, (LongHallways * 100.0f / TotalHallways)));
	}
	
	if (StairRooms > 0)
	{
		DebugLog(FString::Printf(TEXT("🔺 Stair Rooms: %d (%.1f%%)"), 
			StairRooms, TotalRooms > 0 ? (StairRooms * 100.0f / TotalRooms) : 0.0f));
	}
	
	DebugLog(FString::Printf(TEXT("📏 Total Generated: %d/%d rooms"), TotalRooms, Config.TotalRooms));
	DebugLog(TEXT("========================================================="));
	DebugLog(TEXT(""));
}

