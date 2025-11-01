#pragma once

#include "CoreMinimal.h"
#include "../Types.h"
#include "WallCommon.h"
#include "HoleGenerator.h"
#include "DrawDebugHelpers.h"

/**
 * Main wall unit interface that delegates to specialized generators
 * This provides a clean API while utilizing modular hole generators
 */
class UWallUnit
{
public:
	// Main wall generation functions - delegates to specialized generators
	static void GenerateThickWall(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs,
		FVector InnerBL, FVector InnerBR, FVector InnerTR, FVector InnerTL,
		FVector OuterBL, FVector OuterBR, FVector OuterTR, FVector OuterTL,
		float WallWidth, float WallHeight, EWallSide WallSide, float WallThickness, UWorld* World = nullptr);

	static void GenerateThickWallWithDoor(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs,
		FVector InnerBL, FVector InnerBR, FVector InnerTR, FVector InnerTL,
		FVector OuterBL, FVector OuterBR, FVector OuterTR, FVector OuterTL,
		float WallWidth, float WallHeight, const FDoorConfig& Door, float WallThickness);

	static void GenerateThickWallWithMultipleDoors(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs,
		FVector InnerBL, FVector InnerBR, FVector InnerTR, FVector InnerTL,
		FVector OuterBL, FVector OuterBR, FVector OuterTR, FVector OuterTL,
		float WallWidth, float WallHeight, const TArray<FDoorConfig*>& Doors, float WallThickness);

	// === NEW CUSTOM HOLE GENERATION SECTION ===
	
	// Generate wall with doorway (clean interface for doorway creation)
	static void GenerateDoorway(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs,
		FVector InnerBL, FVector InnerBR, FVector InnerTR, FVector InnerTL,
		FVector OuterBL, FVector OuterBR, FVector OuterTR, FVector OuterTL,
		float WallWidth, float WallHeight, float WallThickness, UWorld* World = nullptr,
		float DoorWidth = 0.8f, float DoorHeight = 2.0f, float HorizontalPosition = 0.5f, float VerticalPosition = 0.0f);

	// Generate wall with irregular hole (clean interface for irregular hole creation)
	static void GenerateIrregularHole(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs,
		FVector InnerBL, FVector InnerBR, FVector InnerTR, FVector InnerTL,
		FVector OuterBL, FVector OuterBR, FVector OuterTR, FVector OuterTL,
		float WallWidth, float WallHeight, float WallThickness, UWorld* World = nullptr,
		float HoleSize = 1.0f, float Irregularity = 0.5f, int32 RandomSeed = 12345);

	// === SIMPLIFIED INTERFACE FOR TESTS ===

	// Generate complete wall with doorway (handles all positioning and rotation internally)
	static void GenerateCompleteWallWithDoorway(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs,
		const FVector& Position, const FRotator& Rotation, 
		float WallWidth, float WallHeight, float WallThickness, UWorld* World = nullptr,
		float DoorWidth = 0.8f, float DoorHeight = 2.0f, float HorizontalPosition = 0.5f, float VerticalPosition = 0.0f);

	// Ultra-simplified interface - returns ready-to-use mesh data
	static void CreateWallMeshWithDoorway(TArray<FVector>& OutVertices, TArray<int32>& OutTriangles, TArray<FVector>& OutNormals, TArray<FVector2D>& OutUVs,
		const FVector& Position, const FRotator& Rotation, 
		float WallWidth, float WallHeight, float WallThickness, UWorld* World = nullptr,
		float DoorWidth = 0.8f, float DoorHeight = 2.0f, float HorizontalPosition = 0.5f, float VerticalPosition = 0.0f);

	// Complete wall creation - returns fully configured wall actor ready to use (with doorway)
	static AActor* CreateCompleteWallActor(UWorld* World, const FVector& Position, const FRotator& Rotation,
		float WallWidth, float WallHeight, float WallThickness, const FLinearColor& Color,
		float DoorWidth = 0.8f, float DoorHeight = 2.0f, float HorizontalPosition = 0.5f, float VerticalPosition = 0.0f);

	// Complete solid wall creation - returns fully configured solid wall actor (no holes)
	static AActor* CreateSolidWallActor(UWorld* World, const FVector& Position, const FRotator& Rotation,
		float WallWidth, float WallHeight, float WallThickness, const FLinearColor& Color);

	// === ADVANCED HOLE CONFIGURATION SYSTEM ===

	// Create wall with single hole using advanced positioning
	static AActor* CreateWallWithHole(UWorld* World, const FVector& Position, const FRotator& Rotation,
		float WallWidth, float WallHeight, float WallThickness, const FLinearColor& Color,
		const FWallHoleConfig& HoleConfig);

	// Create wall with multiple holes using advanced positioning
	static AActor* CreateWallWithMultipleHoles(UWorld* World, const FVector& Position, const FRotator& Rotation,
		float WallWidth, float WallHeight, float WallThickness, const FLinearColor& Color,
		const TArray<FWallHoleConfig>& HoleConfigs);

private:
	// Fast simple rectangle hole generation for performance
	static void GenerateSimpleRectangleHole(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs,
		FVector InnerBL, FVector InnerBR, FVector InnerTR, FVector InnerTL,
		FVector OuterBL, FVector OuterBR, FVector OuterTR, FVector OuterTL,
		float WallWidth, float WallHeight, const FDoorConfig& Door, float WallThickness);
	
	// Generate the interior faces that show wall thickness around holes
	static void GenerateHoleInteriorFaces(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs,
		FVector InnerBL, FVector WallWidthDirection, FVector WallHeightDirection,
		FVector OuterBL, FVector OuterWidthDirection, FVector OuterHeightDirection,
		float HoleLeft, float HoleRight, float HoleBottom, float HoleTop, float WallThickness);
	
	// Debug function to show wall center
	static void DrawWallCenterDebugSphere(UWorld* World, 
		FVector InnerBL, FVector InnerBR, FVector InnerTR, FVector InnerTL,
		FVector OuterBL, FVector OuterBR, FVector OuterTR, FVector OuterTL);

private:
	// Generate wall with custom positioned square hole (for precise doorways)
	static void GenerateWallWithCustomSquareHole(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs,
		FVector InnerBL, FVector InnerBR, FVector InnerTR, FVector InnerTL,
		FVector OuterBL, FVector OuterBR, FVector OuterTR, FVector OuterTL,
		float WallWidth, float WallHeight, float WallThickness, UWorld* World = nullptr,
		float HoleWidth = 0.8f, float HoleHeight = 2.0f, float HoleCenterX = 0.5f, float HoleCenterY = 0.5f);
	
	// Generate wall with random irregular hole (for organic openings)
	static void GenerateWallWithRandomHole(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs,
		FVector InnerBL, FVector InnerBR, FVector InnerTR, FVector InnerTL,
		FVector OuterBL, FVector OuterBR, FVector OuterTR, FVector OuterTL,
		float WallWidth, float WallHeight, float WallThickness, UWorld* World = nullptr,
		float HoleSize = 1.0f, float Irregularity = 0.5f, int32 RandomSeed = 12345);

public:
};