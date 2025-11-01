#include "BaseRoom.h"
#include "StandardRoom.h"
#include "../WallUnit/WallUnit.h"
#include "Engine/Engine.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"

UBaseRoom::UBaseRoom()
{
	// Base room defaults
	Width = 5.0f;
	Length = 5.0f;
	Height = 3.0f;
	WallThickness = 0.2f;
	Position = FVector::ZeroVector;
	RoomCategory = ERoomCategory::Room;
	Elevation = 0.0f;
	MeshComponent = nullptr;
}

void UBaseRoom::CreateRoom(AActor* Owner)
{
	UE_LOG(LogTemp, Warning, TEXT("=== BASEROOM: Creating room at %s (%.1fx%.1fx%.1fm) ==="), 
		*Position.ToString(), Width, Length, Height);

	// Use the individual actor approach for walls
	CreateRoomUsingIndividualActors(Owner);

	UE_LOG(LogTemp, Warning, TEXT("BaseRoom: Successfully created room using individual actors"));
}

void UBaseRoom::SetMaterial(UMaterialInterface* Material)
{
	ApplyMaterialToMesh(Material);
}

void UBaseRoom::InitializeMeshComponent(AActor* Owner)
{
	if (!Owner || !Owner->GetWorld())
	{
		UE_LOG(LogTemp, Error, TEXT("BaseRoom: Invalid Owner or World"));
		return;
	}

	// Create procedural mesh component
	MeshComponent = NewObject<UProceduralMeshComponent>(Owner, UProceduralMeshComponent::StaticClass(), 
		*FString::Printf(TEXT("BaseRoomMesh_%d"), FMath::Rand()));

	if (!MeshComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("BaseRoom: Failed to create mesh component"));
		return;
	}

	// Always set as root component for room units (they should be standalone actors)
	Owner->SetRootComponent(MeshComponent);

	// Set basic properties
	MeshComponent->SetWorldLocation(Position);
	
	// Setup collision and rendering
	SetupCollisionSettings();
	
	// CRITICAL: Register the component with the world so it's rendered and has collision
	MeshComponent->RegisterComponent();

	UE_LOG(LogTemp, Log, TEXT("BaseRoom: Initialized mesh component at position %s"), 
		*Position.ToString());
}

void UBaseRoom::ApplyMaterialToMesh(UMaterialInterface* Material)
{
	if (!MeshComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("BaseRoom: Cannot apply material - no mesh component"));
		return;
	}

	if (Material)
	{
		MeshComponent->SetMaterial(0, Material);
		UE_LOG(LogTemp, Log, TEXT("BaseRoom: Applied material to mesh"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("BaseRoom: No material provided - using default"));
	}
}

void UBaseRoom::SetupCollisionSettings()
{
	if (!MeshComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("BaseRoom: Cannot setup collision - no mesh component"));
		return;
	}

	// Enable collision for walking on floors
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	MeshComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);

	// Use complex collision for accurate walking
	MeshComponent->bUseComplexAsSimpleCollision = true;

	UE_LOG(LogTemp, Warning, TEXT("BaseRoom: Configured collision settings - Complex collision enabled"));
}

void UBaseRoom::GenerateMesh()
{
	if (!MeshComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("BaseRoom: Cannot generate mesh - no mesh component"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("BaseRoom: Generating room mesh"));
	
	// Generate combined mesh data
	TArray<FVector> CombinedVertices;
	TArray<int32> CombinedTriangles;
	TArray<FVector> CombinedNormals;
	TArray<FVector2D> CombinedUVs;
	TArray<FLinearColor> CombinedColors;
	
	// Generate floor using derived class implementation
	GenerateFloorGeometry(CombinedVertices, CombinedTriangles, CombinedNormals, CombinedUVs);
	
	// Add vertex colors for floor (white/gray)
	int32 FloorVertexCount = CombinedVertices.Num();
	for (int32 i = 0; i < FloorVertexCount; i++)
	{
		CombinedColors.Add(FLinearColor::Gray);
	}
	
	// Wall generation is implemented in derived classes (StandardRoom)
	
	// Create the mesh section - convert FLinearColor to FColor
	if (CombinedVertices.Num() > 0 && CombinedTriangles.Num() > 0)
	{
		// Convert FLinearColor to FColor
		TArray<FColor> VertexColors;
		VertexColors.Reserve(CombinedColors.Num());
		for (const FLinearColor& LinearColor : CombinedColors)
		{
			VertexColors.Add(LinearColor.ToFColor(true));
		}
		
		MeshComponent->CreateMeshSection(0, CombinedVertices, CombinedTriangles, 
			CombinedNormals, CombinedUVs, VertexColors, TArray<FProcMeshTangent>(), true);
		
		UE_LOG(LogTemp, Log, TEXT("BaseRoom: Created mesh section with %d vertices, %d triangles"), 
			CombinedVertices.Num(), CombinedTriangles.Num() / 3);
	}
	
	UE_LOG(LogTemp, Log, TEXT("BaseRoom: Created room mesh"));
}


void UBaseRoom::CreateRoomUsingIndividualActors(AActor* Owner)
{
	UE_LOG(LogTemp, Warning, TEXT("BaseRoom: Creating room using individual actors approach"));
	
	// This method will be moved from StandardRoom and enhanced
	// For now, basic implementation
	
	UWorld* World = Owner->GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("BaseRoom: No valid world"));
		return;
	}

	// Convert dimensions
	float WidthCm = MetersToUnrealUnits(Width);
	float LengthCm = MetersToUnrealUnits(Length);
	float HeightCm = MetersToUnrealUnits(Height);
	float HalfWidth = WidthCm * 0.5f;
	float HalfLength = LengthCm * 0.5f;

	// Convert from corner position to center position
	FVector RoomCenter = Position + FVector(HalfWidth, HalfLength, HeightCm * 0.5f);
	
	// Create walls using WallUnit system (same as StandardRoom)
	FLinearColor SouthWallColor = FLinearColor::Green;
	FLinearColor NorthWallColor = FLinearColor::Red;
	FLinearColor EastWallColor = FLinearColor::Blue;
	FLinearColor WestWallColor = FLinearColor::Yellow;

	float WallThicknessCm = MetersToUnrealUnits(WallThickness);

	// Create 4 walls (simplified for now)
	// South Wall
	FVector SouthWallPos = RoomCenter + FVector(0, -HalfLength, 0);
	AActor* SouthWallActor = UWallUnit::CreateSolidWallActor(World, SouthWallPos, FRotator(0, 0, 0), 
		Width, Height, WallThickness, SouthWallColor);
	// Wall actor tracking removed - individual actors managed by StandardRoom

	UE_LOG(LogTemp, Warning, TEXT("BaseRoom: Created room structure with individual wall actors"));
}




void UBaseRoom::AddHoleToWall(AActor* Owner, EWallSide WallSide, const FDoorConfig& DoorConfig)
{
	// Implementation will be moved from StandardRoom
	UE_LOG(LogTemp, Warning, TEXT("BaseRoom: AddHoleToWall called - to be implemented"));
}

void UBaseRoom::AddHoleToWallWithThickness(AActor* Owner, EWallSide WallSide, const FDoorConfig& DoorConfig, float CustomThickness, float SmallerWallSize, UStandardRoom* TargetRoom)
{
	// Implementation will be moved from StandardRoom
	UE_LOG(LogTemp, Warning, TEXT("BaseRoom: AddHoleToWallWithThickness called - to be implemented"));
}

bool UBaseRoom::ShouldRemoveWall(const FDoorConfig* DoorConfig) const
{
	return DoorConfig && DoorConfig->bHasDoor && DoorConfig->Width > 50.0f;
}

void UBaseRoom::GenerateFloorGeometry(TArray<FVector>& Vertices, TArray<int32>& Triangles, 
	TArray<FVector>& Normals, TArray<FVector2D>& UVs)
{
	// Default floor implementation - flat rectangular floor
	float WidthCm = MetersToUnrealUnits(Width);
	float LengthCm = MetersToUnrealUnits(Length);
	
	// Floor vertices (4 corners at Z=0)
	int32 BaseIndex = Vertices.Num();
	Vertices.Add(FVector(0, 0, 0));           // Bottom-left
	Vertices.Add(FVector(WidthCm, 0, 0));     // Bottom-right
	Vertices.Add(FVector(WidthCm, LengthCm, 0)); // Top-right
	Vertices.Add(FVector(0, LengthCm, 0));    // Top-left
	
	// Floor triangles (2 triangles making a rectangle)
	Triangles.Add(BaseIndex + 0);
	Triangles.Add(BaseIndex + 1);
	Triangles.Add(BaseIndex + 2);
	
	Triangles.Add(BaseIndex + 0);
	Triangles.Add(BaseIndex + 2);
	Triangles.Add(BaseIndex + 3);
	
	// Normals (pointing up)
	for (int32 i = 0; i < 4; i++)
	{
		Normals.Add(FVector::UpVector);
	}
	
	// UVs (standard texture mapping)
	UVs.Add(FVector2D(0, 0));
	UVs.Add(FVector2D(1, 0));
	UVs.Add(FVector2D(1, 1));
	UVs.Add(FVector2D(0, 1));
	
	UE_LOG(LogTemp, Log, TEXT("BaseRoom: Generated default floor geometry"));
}

// Static room size generation methods - centralized from Main.cpp logic
void UBaseRoom::GenerateRandomRoomSize(ERoomCategory Category, float& OutWidth, float& OutLength, FRandomStream& Random)
{
	if (Category == ERoomCategory::Room)
	{
		GenerateStandardRoomSize(OutWidth, OutLength, Random);
	}
	else if (Category == ERoomCategory::Hallway)
	{
		GenerateHallwaySize(OutWidth, OutLength, Random);
	}
	else if (Category == ERoomCategory::Stairs)
	{
		GenerateHallwaySize(OutWidth, OutLength, Random);
	}
	else
	{
		// Default to standard room
		GenerateStandardRoomSize(OutWidth, OutLength, Random);
	}
}

void UBaseRoom::GenerateStandardRoomSize(float& OutWidth, float& OutLength, FRandomStream& Random)
{
	// Square-ish rooms: 2x2 to 8x8 meters (reasonable size for testing)
	float Size = Random.FRandRange(2.0f, 8.0f);
	OutWidth = Size;
	OutLength = Size;
}

void UBaseRoom::GenerateHallwaySize(float& OutWidth, float& OutLength, FRandomStream& Random)
{
	// Rectangular hallways: 2-4m width, 8-16m length (narrow corridors)
	OutWidth = Random.FRandRange(2.0f, 4.0f);
	OutLength = Random.FRandRange(8.0f, 16.0f);
	
	// Randomly swap width/length for variety (25% chance)
	if (Random.FRand() < 0.25f)
	{
		float Temp = OutWidth;
		OutWidth = OutLength;
		OutLength = Temp;
	}
}


// Static room size generation methods - Full Scale (massive backrooms)
void UBaseRoom::GenerateRandomRoomSizeFullScale(ERoomCategory Category, float& OutWidth, float& OutLength, FRandomStream& Random)
{
	if (Category == ERoomCategory::Room)
	{
		GenerateStandardRoomSizeFullScale(OutWidth, OutLength, Random);
	}
	else if (Category == ERoomCategory::Hallway)
	{
		GenerateHallwaySizeFullScale(OutWidth, OutLength, Random);
	}
	else if (Category == ERoomCategory::Stairs)
	{
		GenerateHallwaySizeFullScale(OutWidth, OutLength, Random);
	}
	else
	{
		// Default to standard room
		GenerateStandardRoomSizeFullScale(OutWidth, OutLength, Random);
	}
}

void UBaseRoom::GenerateStandardRoomSizeFullScale(float& OutWidth, float& OutLength, FRandomStream& Random)
{
	// Square-ish rooms: 2x2 to 50x50 meters (massive backrooms spaces)
	float Size = Random.FRandRange(2.0f, 50.0f);
	OutWidth = Size;
	OutLength = Size;
}

void UBaseRoom::GenerateHallwaySizeFullScale(float& OutWidth, float& OutLength, FRandomStream& Random)
{
	// Rectangular hallways: 2.5-5m width, 6-100m length (extremely long for vast backrooms feel)
	OutWidth = Random.FRandRange(2.5f, 5.0f);
	OutLength = Random.FRandRange(6.0f, 100.0f);
	
	// Randomly swap width/length for variety
	if (Random.FRand() < 0.5f)
	{
		float Temp = OutWidth;
		OutWidth = OutLength;
		OutLength = Temp;
	}
}


// Comprehensive room generation methods - automatically assign all properties based on category
void UBaseRoom::InitializeRandomRoom(FRoomData& Room, ERoomCategory Category, int32 RoomIndex, FRandomStream& Random, bool bUseFullScale)
{
	// Set basic properties
	Room.RoomIndex = RoomIndex;
	Room.Category = Category;
	Room.Height = 3.0f; // Standard height for all room types
	Room.Elevation = 0.0f; // Default elevation (stairs will override this)
	
	// Generate random size based on category and scale
	if (bUseFullScale)
	{
		GenerateRandomRoomSizeFullScale(Category, Room.Width, Room.Length, Random);
	}
	else
	{
		GenerateRandomRoomSize(Category, Room.Width, Room.Length, Random);
	}
	
	// Initialize all connections as unused
	Room.Connections.SetNum(4);
	for (int32 i = 0; i < 4; i++)
	{
		Room.Connections[i].WallSide = static_cast<EWallSide>(i);
		Room.Connections[i].bIsUsed = false;
		Room.Connections[i].ConnectionPoint = FVector::ZeroVector;
		Room.Connections[i].ConnectionType = EConnectionType::Doorway;
		Room.Connections[i].ConnectedRoomIndex = -1;
	}
	
	// Note: Position will need to be set separately by the caller as it depends on world placement
	Room.Position = FVector::ZeroVector; // Caller must set this
	Room.RoomUnit = nullptr; // Caller must create this
}

void UBaseRoom::InitializeRandomRoomWithElevation(FRoomData& Room, ERoomCategory Category, int32 RoomIndex, FRandomStream& Random, bool bUseFullScale)
{
	// Initialize basic properties first
	InitializeRandomRoom(Room, Category, RoomIndex, Random, bUseFullScale);
	
	// Handle special elevation calculation for stairs
	if (Category == ERoomCategory::Stairs)
	{
		// Create vertical elevation based on stair length (longer stairs = higher)
		// Standard stair rise: ~15cm per step, step depth: ~30cm
		// Steps = Length / 0.3m, Height = Steps * 0.15m
		float StepDepth = 0.3f; // 30cm step depth
		float StepHeight = 0.15f; // 15cm step rise
		int32 NumSteps = FMath::FloorToInt(Room.Length / StepDepth);
		Room.Elevation = NumSteps * StepHeight; // Elevation gained
		
		// Set random stair direction
		TArray<EWallSide> StairDirections = {EWallSide::North, EWallSide::South, EWallSide::East, EWallSide::West};
		Room.StairDirection = StairDirections[Random.RandRange(0, StairDirections.Num() - 1)];
	}
}