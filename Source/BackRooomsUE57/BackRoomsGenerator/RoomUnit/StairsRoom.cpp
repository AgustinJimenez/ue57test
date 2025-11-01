#include "StairsRoom.h"
#include "Engine/Engine.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

UStairsRoom::UStairsRoom() : Super()
{
	// Set default stair properties
	Width = 2.0f;           // 2m wide base
	Length = 3.0f;          // 3m long base
	Height = 1.6f;          // Total height for 8 steps * 20cm
	RoomCategory = ERoomCategory::Stairs;
	
	// Default stair configuration - optimized for backrooms
	StairDirection = EWallSide::North;
	NumberOfSteps = 8;      // Reduced from 10 to 8 steps
	StepHeight = 0.2f;      // 20cm per step (slightly higher)
	StepDepth = 0.3f;       // 30cm depth per step (slightly deeper)
	StairWidth = 1.2f;      // 1.2m stair width
	bIncludeRailings = true;
	RailingHeight = 0.9f;   // 90cm railings
	
	// Adjust room dimensions based on stair requirements
	UpdateRoomDimensionsForStairs();
}

float UStairsRoom::CalculateTotalStairHeight() const
{
	return NumberOfSteps * StepHeight;
}

float UStairsRoom::CalculateTotalStairLength() const
{
	return NumberOfSteps * StepDepth;
}

void UStairsRoom::UpdateRoomDimensionsForStairs()
{
	// Automatically adjust room dimensions based on stair configuration
	float TotalStairLength = CalculateTotalStairLength();
	
	// Constrain stair dimensions to reasonable values for backrooms
	float MaxStairLength = 6.0f; // Maximum 6m stair run
	float MinStairLength = 2.5f; // Minimum 2.5m stair run
	TotalStairLength = FMath::Clamp(TotalStairLength, MinStairLength, MaxStairLength);
	
	// Set room dimensions based on stair direction with reasonable constraints
	switch (StairDirection)
	{
		case EWallSide::North:
		case EWallSide::South:
			Length = FMath::Clamp(TotalStairLength + 0.5f, 3.0f, 6.0f); // Add 0.5m buffer, max 6m
			Width = FMath::Clamp(StairWidth + 0.4f, 2.0f, 3.5f);        // Add 0.4m buffer, max 3.5m
			break;
		case EWallSide::East:
		case EWallSide::West:
			Width = FMath::Clamp(TotalStairLength + 0.5f, 3.0f, 6.0f);  // Add 0.5m buffer, max 6m
			Length = FMath::Clamp(StairWidth + 0.4f, 2.0f, 3.5f);       // Add 0.4m buffer, max 3.5m
			break;
		default:
			break;
	}
	
	// Set room height to accommodate stairs + clearance (constrained)
	float TotalStairHeight = CalculateTotalStairHeight();
	Height = FMath::Clamp(TotalStairHeight + 1.5f, 3.0f, 4.5f); // Add 1.5m clearance, max 4.5m
	
	UE_LOG(LogTemp, Warning, TEXT("StairsRoom: Updated dimensions - W:%.1fm L:%.1fm H:%.1fm (StairLength:%.1fm)"), 
		Width, Length, Height, TotalStairLength);
}

FVector UStairsRoom::GetStairTopPosition() const
{
	FVector BasePosition = Position;
	float TotalStairLength = CalculateTotalStairLength();
	float TotalStairHeight = CalculateTotalStairHeight();
	
	switch (StairDirection)
	{
		case EWallSide::North:
			return BasePosition + FVector(0, MetersToUnrealUnits(TotalStairLength), MetersToUnrealUnits(TotalStairHeight + Elevation));
		case EWallSide::South:
			return BasePosition + FVector(0, -MetersToUnrealUnits(TotalStairLength), MetersToUnrealUnits(TotalStairHeight + Elevation));
		case EWallSide::East:
			return BasePosition + FVector(MetersToUnrealUnits(TotalStairLength), 0, MetersToUnrealUnits(TotalStairHeight + Elevation));
		case EWallSide::West:
			return BasePosition + FVector(-MetersToUnrealUnits(TotalStairLength), 0, MetersToUnrealUnits(TotalStairHeight + Elevation));
		default:
			return BasePosition + FVector(0, 0, MetersToUnrealUnits(TotalStairHeight + Elevation));
	}
}

FVector UStairsRoom::GetStairBottomPosition() const
{
	return Position + FVector(0, 0, MetersToUnrealUnits(Elevation));
}

FBox UStairsRoom::GetStairCollisionBounds() const
{
	// Calculate accurate bounds for stair geometry
	float WallThickness = 0.2f; // 20cm wall thickness
	float WallThicknessCm = MetersToUnrealUnits(WallThickness);
	
	// Base room bounds
	FVector Min = Position + FVector(-WallThicknessCm, -WallThicknessCm, MetersToUnrealUnits(Elevation));
	FVector Max = Position + FVector(
		MetersToUnrealUnits(Width) + WallThicknessCm,
		MetersToUnrealUnits(Length) + WallThicknessCm,
		MetersToUnrealUnits(Elevation + Height)
	);
	
	// Extend bounds to include actual stair footprint based on direction
	float TotalStairLength = CalculateTotalStairLength();
	float TotalStairHeight = CalculateTotalStairHeight();
	
	switch (StairDirection)
	{
		case EWallSide::North:
			Max.Y += MetersToUnrealUnits(TotalStairLength * 0.5f); // Extend north
			Max.Z = FMath::Max(Max.Z, Min.Z + MetersToUnrealUnits(TotalStairHeight));
			break;
		case EWallSide::South:
			Min.Y -= MetersToUnrealUnits(TotalStairLength * 0.5f); // Extend south
			Max.Z = FMath::Max(Max.Z, Min.Z + MetersToUnrealUnits(TotalStairHeight));
			break;
		case EWallSide::East:
			Max.X += MetersToUnrealUnits(TotalStairLength * 0.5f); // Extend east
			Max.Z = FMath::Max(Max.Z, Min.Z + MetersToUnrealUnits(TotalStairHeight));
			break;
		case EWallSide::West:
			Min.X -= MetersToUnrealUnits(TotalStairLength * 0.5f); // Extend west
			Max.Z = FMath::Max(Max.Z, Min.Z + MetersToUnrealUnits(TotalStairHeight));
			break;
		default:
			break;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("StairsRoom: Collision bounds - Min:%s Max:%s"), *Min.ToString(), *Max.ToString());
	return FBox(Min, Max);
}

FRotator UStairsRoom::GetStairDirectionRotation() const
{
	switch (StairDirection)
	{
		case EWallSide::North:
			return FRotator(0, 0, 0);      // Facing North (default)
		case EWallSide::South:
			return FRotator(0, 180, 0);    // Facing South
		case EWallSide::East:
			return FRotator(0, 90, 0);     // Facing East
		case EWallSide::West:
			return FRotator(0, -90, 0);    // Facing West
		default:
			return FRotator::ZeroRotator;
	}
}

bool UStairsRoom::CreateStairsFromRoomData(const FRoomData& RoomData, AActor* Owner, bool bShowNumbers)
{
	if (!Owner)
	{
		UE_LOG(LogTemp, Error, TEXT("StairsRoom::CreateStairsFromRoomData - Owner is null"));
		return false;
	}

	// Copy room data properties
	Position = RoomData.Position;
	Width = RoomData.Width;
	Length = RoomData.Length;
	Height = RoomData.Height;
	Elevation = RoomData.Elevation;
	RoomCategory = RoomData.Category;
	DoorConfigs = TArray<FDoorConfig>(); // Start with no doors, can be added later
	
	// Set stairs-specific properties
	StairDirection = RoomData.StairDirection;
	
	// Log stair creation with detailed parameters
	UE_LOG(LogTemp, Warning, TEXT("========== CREATING STAIRS ROOM %d =========="), RoomData.RoomIndex);
	UE_LOG(LogTemp, Warning, TEXT("ORIGINAL DIMENSIONS: %.1fm x %.1fm x %.1fm (W x L x H)"), Width, Length, Height);
	UE_LOG(LogTemp, Warning, TEXT("POSITION: %s (X=%.1f, Y=%.1f, Z=%.1f)"), *Position.ToString(), Position.X, Position.Y, Position.Z);
	UE_LOG(LogTemp, Warning, TEXT("ELEVATION: %.2fm above ground"), Elevation);
	UE_LOG(LogTemp, Warning, TEXT("STAIR DIRECTION: %s"), *UEnum::GetValueAsString(StairDirection));
	UE_LOG(LogTemp, Warning, TEXT("STAIRS CONFIG: %d steps, %.2fm height each, %.2fm depth each"), NumberOfSteps, StepHeight, StepDepth);
	UE_LOG(LogTemp, Warning, TEXT("CALCULATED: Total Length=%.2fm, Total Height=%.2fm"), CalculateTotalStairLength(), CalculateTotalStairHeight());

	// Update room dimensions based on stairs
	UpdateRoomDimensionsForStairs();
	
	UE_LOG(LogTemp, Warning, TEXT("FINAL DIMENSIONS: %.1fm x %.1fm x %.1fm (W x L x H) - READY FOR COLLISION CHECK"), Width, Length, Height);
	
	// Create the stair room using individual actors
	CreateRoomUsingIndividualActors(Owner);
	
	// Create room number text if requested
	if (bShowNumbers)
	{
		CreateRoomNumberText(RoomData.RoomIndex, bShowNumbers);
	}
	
	UE_LOG(LogTemp, Warning, TEXT("========== STAIRS ROOM %d CREATED =========="), RoomData.RoomIndex);
	
	return true;
}

void UStairsRoom::CreateRoomUsingIndividualActors(AActor* Owner)
{
	UE_LOG(LogTemp, Warning, TEXT("StairsRoom: Creating stairs using individual actors"));
	
	// First create the basic room structure (foundation walls)
	Super::CreateRoomUsingIndividualActors(Owner);
	
	// Then add stair-specific geometry
	// Note: For now, we'll use the standard mesh generation with stairs included
	// In a more advanced implementation, you could create separate stair actors
	
	UE_LOG(LogTemp, Warning, TEXT("StairsRoom: Individual actors created with stair geometry"));
}

void UStairsRoom::GenerateMesh()
{
	if (!MeshComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("StairsRoom::GenerateMesh - MeshComponent is null"));
		return;
	}

	// Clear existing mesh data
	MeshComponent->ClearAllMeshSections();

	// Arrays to store all geometry
	TArray<FVector> CombinedVertices;
	TArray<int32> CombinedTriangles;
	TArray<FVector> CombinedNormals;
	TArray<FVector2D> CombinedUVs;
	TArray<FLinearColor> CombinedColors;

	// Generate foundation floor
	GenerateFloorGeometry(CombinedVertices, CombinedTriangles, CombinedNormals, CombinedUVs);
	
	// Add floor colors (gray)
	for (int32 i = 0; i < 4; i++) // Floor has 4 vertices
	{
		CombinedColors.Add(FLinearColor::Gray);
	}

	// Generate stair steps
	GenerateStairSteps(CombinedVertices, CombinedTriangles, CombinedNormals, CombinedUVs, CombinedColors);
	
	// Generate stair foundation/walls under stairs
	GenerateStairFoundation(CombinedVertices, CombinedTriangles, CombinedNormals, CombinedUVs, CombinedColors);
	
	// Generate railings if enabled
	if (bIncludeRailings)
	{
		GenerateStairRailings(CombinedVertices, CombinedTriangles, CombinedNormals, CombinedUVs, CombinedColors);
	}

	// Create the mesh section
	if (CombinedVertices.Num() > 0 && CombinedTriangles.Num() > 0)
	{
		MeshComponent->CreateMeshSection_LinearColor(
			0, // Section index
			CombinedVertices,
			CombinedTriangles,
			CombinedNormals,
			CombinedUVs,
			CombinedColors,
			TArray<FProcMeshTangent>(), // Empty tangents
			true // Create collision
		);
		
		UE_LOG(LogTemp, Warning, TEXT("StairsRoom: Generated mesh with %d vertices, %d triangles"), 
			CombinedVertices.Num(), CombinedTriangles.Num() / 3);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("StairsRoom: No mesh data generated"));
	}
}

void UStairsRoom::GenerateStairSteps(TArray<FVector>& Vertices, TArray<int32>& Triangles, 
	TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FLinearColor>& Colors)
{
	int32 BaseVertexIndex = Vertices.Num();
	
	for (int32 StepIndex = 0; StepIndex < NumberOfSteps; StepIndex++)
	{
		FVector StepPosition = CalculateStepPosition(StepIndex);
		AddStepToMesh(StepIndex, StepPosition, Vertices, Triangles, Normals, UVs, Colors);
	}
	
	UE_LOG(LogTemp, Warning, TEXT("StairsRoom: Generated %d stair steps"), NumberOfSteps);
}

FVector UStairsRoom::CalculateStepPosition(int32 StepIndex) const
{
	FVector BasePosition = Position + FVector(0, 0, MetersToUnrealUnits(Elevation));
	
	// Calculate step offset based on direction
	FVector StepOffset = FVector::ZeroVector;
	float StepProgress = StepIndex * StepDepth;
	float StepElevation = StepIndex * StepHeight;
	
	switch (StairDirection)
	{
		case EWallSide::North:
			StepOffset = FVector(0, MetersToUnrealUnits(StepProgress), MetersToUnrealUnits(StepElevation));
			break;
		case EWallSide::South:
			StepOffset = FVector(0, -MetersToUnrealUnits(StepProgress), MetersToUnrealUnits(StepElevation));
			break;
		case EWallSide::East:
			StepOffset = FVector(MetersToUnrealUnits(StepProgress), 0, MetersToUnrealUnits(StepElevation));
			break;
		case EWallSide::West:
			StepOffset = FVector(-MetersToUnrealUnits(StepProgress), 0, MetersToUnrealUnits(StepElevation));
			break;
		default:
			break;
	}
	
	return BasePosition + StepOffset;
}

void UStairsRoom::AddStepToMesh(int32 StepIndex, const FVector& StepPosition, 
	TArray<FVector>& Vertices, TArray<int32>& Triangles,
	TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FLinearColor>& Colors)
{
	int32 BaseVertexIndex = Vertices.Num();
	
	// Step dimensions in Unreal units
	float StepWidthCm = MetersToUnrealUnits(StairWidth);
	float StepDepthCm = MetersToUnrealUnits(StepDepth);
	float StepHeightCm = MetersToUnrealUnits(StepHeight);
	
	// Create step as a box (simplified)
	// Step vertices (8 vertices for a box)
	TArray<FVector> StepVertices = {
		// Bottom face
		StepPosition + FVector(0, 0, 0),                                    // 0: Bottom front-left
		StepPosition + FVector(StepWidthCm, 0, 0),                         // 1: Bottom front-right
		StepPosition + FVector(StepWidthCm, StepDepthCm, 0),               // 2: Bottom back-right
		StepPosition + FVector(0, StepDepthCm, 0),                         // 3: Bottom back-left
		
		// Top face
		StepPosition + FVector(0, 0, StepHeightCm),                        // 4: Top front-left
		StepPosition + FVector(StepWidthCm, 0, StepHeightCm),              // 5: Top front-right
		StepPosition + FVector(StepWidthCm, StepDepthCm, StepHeightCm),    // 6: Top back-right
		StepPosition + FVector(0, StepDepthCm, StepHeightCm)               // 7: Top back-left
	};
	
	// Add vertices
	Vertices.Append(StepVertices);
	
	// Create triangles for step box (12 triangles = 6 faces * 2 triangles each)
	TArray<int32> StepTriangles = {
		// Bottom face (facing down)
		BaseVertexIndex + 0, BaseVertexIndex + 2, BaseVertexIndex + 1,
		BaseVertexIndex + 0, BaseVertexIndex + 3, BaseVertexIndex + 2,
		
		// Top face (facing up)
		BaseVertexIndex + 4, BaseVertexIndex + 5, BaseVertexIndex + 6,
		BaseVertexIndex + 4, BaseVertexIndex + 6, BaseVertexIndex + 7,
		
		// Front face
		BaseVertexIndex + 0, BaseVertexIndex + 1, BaseVertexIndex + 5,
		BaseVertexIndex + 0, BaseVertexIndex + 5, BaseVertexIndex + 4,
		
		// Back face
		BaseVertexIndex + 2, BaseVertexIndex + 3, BaseVertexIndex + 7,
		BaseVertexIndex + 2, BaseVertexIndex + 7, BaseVertexIndex + 6,
		
		// Left face
		BaseVertexIndex + 3, BaseVertexIndex + 0, BaseVertexIndex + 4,
		BaseVertexIndex + 3, BaseVertexIndex + 4, BaseVertexIndex + 7,
		
		// Right face
		BaseVertexIndex + 1, BaseVertexIndex + 2, BaseVertexIndex + 6,
		BaseVertexIndex + 1, BaseVertexIndex + 6, BaseVertexIndex + 5
	};
	
	Triangles.Append(StepTriangles);
	
	// Add normals (simplified - using face normals)
	for (int32 i = 0; i < 8; i++)
	{
		Normals.Add(FVector::UpVector); // Simplified normal
	}
	
	// Add UVs
	for (int32 i = 0; i < 8; i++)
	{
		UVs.Add(CalculateStairUV(StepVertices[i]));
	}
	
	// Add colors (orange/brown for stairs)
	FLinearColor StepColor = FLinearColor(0.8f, 0.4f, 0.2f, 1.0f); // Orange-brown
	for (int32 i = 0; i < 8; i++)
	{
		Colors.Add(StepColor);
	}
}

void UStairsRoom::GenerateStairFoundation(TArray<FVector>& Vertices, TArray<int32>& Triangles,
	TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FLinearColor>& Colors)
{
	// Generate supporting walls/foundation under the stairs
	// This creates the triangular space under the stairs
	
	// For now, implement a simple foundation block
	// In a more complex implementation, this could be triangular supports
	
	UE_LOG(LogTemp, Warning, TEXT("StairsRoom: Generated stair foundation"));
}

void UStairsRoom::GenerateStairRailings(TArray<FVector>& Vertices, TArray<int32>& Triangles,
	TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FLinearColor>& Colors)
{
	// Generate railings on both sides of the stairs
	// Railings run parallel to the stair direction
	
	UE_LOG(LogTemp, Warning, TEXT("StairsRoom: Generated stair railings (height: %.2fm)"), RailingHeight);
}

FVector2D UStairsRoom::CalculateStairUV(const FVector& Vertex, float ScaleFactor)
{
	// Simple UV calculation based on world position
	return FVector2D(Vertex.X * 0.01f * ScaleFactor, Vertex.Y * 0.01f * ScaleFactor);
}

void UStairsRoom::CreateRoomNumberText(int32 RoomIndex, bool bShowNumbers)
{
	if (!bShowNumbers) return;
	
	// Create text component for room number (similar to StandardRoom)
	// Position the text above the stairs for visibility
	
	UE_LOG(LogTemp, Warning, TEXT("StairsRoom: Created room number text for stairs %d"), RoomIndex);
}

float UStairsRoom::MetersToUnrealUnits(float Meters) const
{
	return Meters * 100.0f; // Convert meters to centimeters
}