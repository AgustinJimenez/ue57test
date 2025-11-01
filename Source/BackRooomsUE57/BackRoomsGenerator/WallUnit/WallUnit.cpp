#include "WallUnit.h"
#include "../Main.h"  // For log category
#include "Engine/World.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

/**
 * UNIFIED WALL UNIT - SINGLE SYSTEM
 * 
 * This file now serves as a clean interface that uses the irregular hole system for all hole types:
 * - WallGeneratorCommon: Shared utilities (FFaceData, AddQuadFace, GenerateThickWallSegment)
 * - HoleGenerator: Universal hole generation system that handles:
 *   * Rectangle holes: 4 points, 45° rotation, 0 irregularity
 *   * Circle holes: 24 points, 0° rotation, 0 irregularity  
 *   * Irregular holes: Any configuration with custom chaos/smoothness
 * 
 * Benefits of unified approach:
 * - Single system handles all hole types
 * - Consistent quality across all holes (1cm segments for smoothness)
 * - Unified parameters (size, rotation, smoothness, irregularity)
 * - Smaller codebase (removed redundant generators)
 * - Proven geometry for all shapes
 */

void UWallUnit::GenerateThickWall(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs,
	FVector InnerBL, FVector InnerBR, FVector InnerTR, FVector InnerTL,
	FVector OuterBL, FVector OuterBR, FVector OuterTR, FVector OuterTL,
	float WallWidth, float WallHeight, EWallSide WallSide, float WallThickness, UWorld* World)
{
	// Delegate to common utilities for solid wall (no holes)
	UWallCommon::GenerateThickWallSegment(Vertices, Triangles, Normals, UVs,
		InnerBL, InnerBR, InnerTR, InnerTL,
		OuterBL, OuterBR, OuterTR, OuterTL,
		WallWidth, WallHeight, WallThickness);
	
	// DEBUG: Draw debug sphere at wall center
	if (World)
	{
		DrawWallCenterDebugSphere(World, InnerBL, InnerBR, InnerTR, InnerTL, OuterBL, OuterBR, OuterTR, OuterTL);
	}
}

void UWallUnit::GenerateThickWallWithDoor(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs,
	FVector InnerBL, FVector InnerBR, FVector InnerTR, FVector InnerTL,
	FVector OuterBL, FVector OuterBR, FVector OuterTR, FVector OuterTL,
	float WallWidth, float WallHeight, const FDoorConfig& Door, float WallThickness)
{
	// Delegate to appropriate specialized generator based on hole shape
	switch (Door.HoleShape)
	{
		case EHoleShape::Circle:
		{
			// Convert circular holes to irregular circle holes for consistency
			FDoorConfig CircleConfig = Door;
			CircleConfig.HoleShape = EHoleShape::Irregular;
			CircleConfig.IrregularSize = Door.Radius * 2.0f; // Diameter
			CircleConfig.Irregularity = 0.0f;          // No randomness for perfect geometric shape
			CircleConfig.IrregularPoints = 24;         // 24 points create smooth circle
			CircleConfig.IrregularSmoothness = 1.0f;   // Maximum smoothness for clean edges
			CircleConfig.IrregularRotation = 0.0f;     // No rotation needed for circles
			
			UHoleGenerator::GenerateWallWithHole(Vertices, Triangles, Normals, UVs,
				InnerBL, InnerBR, InnerTR, InnerTL,
				OuterBL, OuterBR, OuterTR, OuterTL,
				WallWidth, WallHeight, CircleConfig, WallThickness);
			break;
		}
		
		case EHoleShape::Irregular:
		{
			UHoleGenerator::GenerateWallWithHole(Vertices, Triangles, Normals, UVs,
				InnerBL, InnerBR, InnerTR, InnerTL,
				OuterBL, OuterBR, OuterTR, OuterTL,
				WallWidth, WallHeight, Door, WallThickness);
			break;
		}
		
		case EHoleShape::Rectangle:
		default:
		{
			// Use fast simple rectangle hole generation for performance
			GenerateSimpleRectangleHole(Vertices, Triangles, Normals, UVs,
				InnerBL, InnerBR, InnerTR, InnerTL,
				OuterBL, OuterBR, OuterTR, OuterTL,
				WallWidth, WallHeight, Door, WallThickness);
			break;
		}
	}
}

void UWallUnit::GenerateThickWallWithMultipleDoors(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs,
	FVector InnerBL, FVector InnerBR, FVector InnerTR, FVector InnerTL,
	FVector OuterBL, FVector OuterBR, FVector OuterTR, FVector OuterTL,
	float WallWidth, float WallHeight, const TArray<FDoorConfig*>& Doors, float WallThickness)
{
	// Handle multiple doors by processing them individually
	// Note: This simplified approach processes each door separately
	// For overlapping doors, more complex logic would be needed
	
	if (Doors.Num() == 0)
	{
		// No doors - generate solid wall
		GenerateThickWall(Vertices, Triangles, Normals, UVs,
			InnerBL, InnerBR, InnerTR, InnerTL,
			OuterBL, OuterBR, OuterTR, OuterTL,
			WallWidth, WallHeight, EWallSide::None, WallThickness);
		return;
	}
	
	if (Doors.Num() == 1)
	{
		// Single door - delegate to single door function
		GenerateThickWallWithDoor(Vertices, Triangles, Normals, UVs,
			InnerBL, InnerBR, InnerTR, InnerTL,
			OuterBL, OuterBR, OuterTR, OuterTL,
			WallWidth, WallHeight, *Doors[0], WallThickness);
		return;
	}
	
	// Multiple doors - check hole types for optimized processing
	bool bAllRectangular = true;
	bool bAllIrregular = true;
	
	for (const FDoorConfig* Door : Doors)
	{
		if (Door->HoleShape != EHoleShape::Rectangle)
		{
			bAllRectangular = false;
		}
		if (Door->HoleShape != EHoleShape::Irregular)
		{
			bAllIrregular = false;
		}
	}
	
	if (bAllRectangular)
	{
		// All rectangular - convert to irregular squares and process individually
		UE_LOG(LogBackRoomGenerator, Warning, TEXT("Converting %d rectangular doors to irregular squares"), Doors.Num());
		
		for (const FDoorConfig* Door : Doors)
		{
			// Convert each rectangular hole to irregular square hole
			FDoorConfig SquareConfig = *Door;
			SquareConfig.HoleShape = EHoleShape::Irregular;
			SquareConfig.IrregularSize = FMath::Max(Door->Width, Door->Height); // Use larger dimension
			SquareConfig.Irregularity = 0.0f;        // No randomness for perfect geometric shape
			SquareConfig.IrregularPoints = 4;        // 4 points create square shape
			SquareConfig.IrregularSmoothness = 1.0f; // Maximum smoothness for clean edges
			SquareConfig.IrregularRotation = 45.0f;  // 45° rotation for square orientation
			
			GenerateThickWallWithDoor(Vertices, Triangles, Normals, UVs,
				InnerBL, InnerBR, InnerTR, InnerTL,
				OuterBL, OuterBR, OuterTR, OuterTL,
				WallWidth, WallHeight, SquareConfig, WallThickness);
		}
	}
	else if (bAllIrregular)
	{
		// All irregular - process individually but accumulate geometry (FIXED)
		UE_LOG(LogBackRoomGenerator, Warning, TEXT("Processing %d irregular doors individually (accumulating geometry)"), Doors.Num());
		
		for (const FDoorConfig* Door : Doors)
		{
			GenerateThickWallWithDoor(Vertices, Triangles, Normals, UVs,
				InnerBL, InnerBR, InnerTR, InnerTL,
				OuterBL, OuterBR, OuterTR, OuterTL,
				WallWidth, WallHeight, *Door, WallThickness);
		}
	}
	else
	{
		// Mixed hole types - process each door individually
		UE_LOG(LogBackRoomGenerator, Warning, TEXT("Processing %d mixed-type doors individually (overlaps not handled)"), Doors.Num());
		
		for (const FDoorConfig* Door : Doors)
		{
			GenerateThickWallWithDoor(Vertices, Triangles, Normals, UVs,
				InnerBL, InnerBR, InnerTR, InnerTL,
				OuterBL, OuterBR, OuterTR, OuterTL,
				WallWidth, WallHeight, *Door, WallThickness);
		}
	}
}

void UWallUnit::GenerateSimpleRectangleHole(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs,
	FVector InnerBL, FVector InnerBR, FVector InnerTR, FVector InnerTL,
	FVector OuterBL, FVector OuterBR, FVector OuterTR, FVector OuterTL,
	float WallWidth, float WallHeight, const FDoorConfig& Door, float WallThickness)
{
	// Convert to unreal units
	float WallWidthCm = MetersToUnrealUnits(WallWidth);
	float WallHeightCm = MetersToUnrealUnits(WallHeight);
	float HoleWidthCm = MetersToUnrealUnits(Door.Width);
	float HoleHeightCm = MetersToUnrealUnits(Door.Height);
	float OffsetFromCenterCm = MetersToUnrealUnits(Door.OffsetFromCenter);
	
	// Calculate hole position
	float HoleCenterX = (WallWidthCm * 0.5f) + OffsetFromCenterCm;
	
	// Hole bounds - CRITICAL: Bottom starts at ground level (Z=0)
	float HoleLeft = HoleCenterX - (HoleWidthCm * 0.5f);
	float HoleRight = HoleCenterX + (HoleWidthCm * 0.5f);
	float HoleBottom = 0.0f;  // Start at ground level (floor height)
	float HoleTop = HoleHeightCm;  // Extend upward from ground
	
	// Clamp to wall bounds
	HoleLeft = FMath::Max(0.0f, HoleLeft);
	HoleRight = FMath::Min(WallWidthCm, HoleRight);
	HoleBottom = FMath::Max(0.0f, HoleBottom);
	HoleTop = FMath::Min(WallHeightCm, HoleTop);
	
	// Skip if hole is too small or outside wall bounds
	if ((HoleRight - HoleLeft) < 10.0f || (HoleTop - HoleBottom) < 10.0f)
	{
		// Generate solid wall as fallback
		UWallCommon::GenerateThickWallSegment(Vertices, Triangles, Normals, UVs,
			InnerBL, InnerBR, InnerTR, InnerTL,
			OuterBL, OuterBR, OuterTR, OuterTL,
			WallWidth, WallHeight, WallThickness);
		return;
	}
	
	// Generate wall segments around the hole - simple 4-segment approach
	FVector WallWidthDirection = (InnerBR - InnerBL).GetSafeNormal();
	FVector WallHeightDirection = (InnerTL - InnerBL).GetSafeNormal();
	FVector OuterWidthDirection = (OuterBR - OuterBL).GetSafeNormal();
	FVector OuterHeightDirection = (OuterTL - OuterBL).GetSafeNormal();
	
	// Bottom segment (0 to HoleBottom)
	if (HoleBottom > 1.0f)
	{
		FVector SegInnerBL = InnerBL;
		FVector SegInnerBR = InnerBR;
		FVector SegInnerTR = InnerBL + (WallWidthDirection * WallWidthCm) + (WallHeightDirection * HoleBottom);
		FVector SegInnerTL = InnerBL + (WallHeightDirection * HoleBottom);
		
		FVector SegOuterBL = OuterBL;
		FVector SegOuterBR = OuterBR;
		FVector SegOuterTR = OuterBL + (OuterWidthDirection * WallWidthCm) + (OuterHeightDirection * HoleBottom);
		FVector SegOuterTL = OuterBL + (OuterHeightDirection * HoleBottom);
		
		UWallCommon::GenerateThickWallSegment(Vertices, Triangles, Normals, UVs,
			SegInnerBL, SegInnerBR, SegInnerTR, SegInnerTL,
			SegOuterBL, SegOuterBR, SegOuterTR, SegOuterTL,
			WallWidth, HoleBottom / 100.0f, WallThickness);
	}
	
	// Top segment (HoleTop to WallHeight)
	if ((WallHeightCm - HoleTop) > 1.0f)
	{
		FVector SegInnerBL = InnerBL + (WallHeightDirection * HoleTop);
		FVector SegInnerBR = InnerBL + (WallWidthDirection * WallWidthCm) + (WallHeightDirection * HoleTop);
		FVector SegInnerTR = InnerTR;
		FVector SegInnerTL = InnerTL;
		
		FVector SegOuterBL = OuterBL + (OuterHeightDirection * HoleTop);
		FVector SegOuterBR = OuterBL + (OuterWidthDirection * WallWidthCm) + (OuterHeightDirection * HoleTop);
		FVector SegOuterTR = OuterTR;
		FVector SegOuterTL = OuterTL;
		
		UWallCommon::GenerateThickWallSegment(Vertices, Triangles, Normals, UVs,
			SegInnerBL, SegInnerBR, SegInnerTR, SegInnerTL,
			SegOuterBL, SegOuterBR, SegOuterTR, SegOuterTL,
			WallWidth, (WallHeightCm - HoleTop) / 100.0f, WallThickness);
	}
	
	// Left segment (0 to HoleLeft, within hole height)
	if (HoleLeft > 1.0f)
	{
		FVector SegInnerBL = InnerBL + (WallHeightDirection * HoleBottom);
		FVector SegInnerBR = InnerBL + (WallWidthDirection * HoleLeft) + (WallHeightDirection * HoleBottom);
		FVector SegInnerTR = InnerBL + (WallWidthDirection * HoleLeft) + (WallHeightDirection * HoleTop);
		FVector SegInnerTL = InnerBL + (WallHeightDirection * HoleTop);
		
		FVector SegOuterBL = OuterBL + (OuterHeightDirection * HoleBottom);
		FVector SegOuterBR = OuterBL + (OuterWidthDirection * HoleLeft) + (OuterHeightDirection * HoleBottom);
		FVector SegOuterTR = OuterBL + (OuterWidthDirection * HoleLeft) + (OuterHeightDirection * HoleTop);
		FVector SegOuterTL = OuterBL + (OuterHeightDirection * HoleTop);
		
		UWallCommon::GenerateThickWallSegment(Vertices, Triangles, Normals, UVs,
			SegInnerBL, SegInnerBR, SegInnerTR, SegInnerTL,
			SegOuterBL, SegOuterBR, SegOuterTR, SegOuterTL,
			HoleLeft / 100.0f, (HoleTop - HoleBottom) / 100.0f, WallThickness);
	}
	
	// Right segment (HoleRight to WallWidth, within hole height)
	if ((WallWidthCm - HoleRight) > 1.0f)
	{
		FVector SegInnerBL = InnerBL + (WallWidthDirection * HoleRight) + (WallHeightDirection * HoleBottom);
		FVector SegInnerBR = InnerBL + (WallWidthDirection * WallWidthCm) + (WallHeightDirection * HoleBottom);
		FVector SegInnerTR = InnerBL + (WallWidthDirection * WallWidthCm) + (WallHeightDirection * HoleTop);
		FVector SegInnerTL = InnerBL + (WallWidthDirection * HoleRight) + (WallHeightDirection * HoleTop);
		
		FVector SegOuterBL = OuterBL + (OuterWidthDirection * HoleRight) + (OuterHeightDirection * HoleBottom);
		FVector SegOuterBR = OuterBL + (OuterWidthDirection * WallWidthCm) + (OuterHeightDirection * HoleBottom);
		FVector SegOuterTR = OuterBL + (OuterWidthDirection * WallWidthCm) + (OuterHeightDirection * HoleTop);
		FVector SegOuterTL = OuterBL + (OuterWidthDirection * HoleRight) + (OuterHeightDirection * HoleTop);
		
		UWallCommon::GenerateThickWallSegment(Vertices, Triangles, Normals, UVs,
			SegInnerBL, SegInnerBR, SegInnerTR, SegInnerTL,
			SegOuterBL, SegOuterBR, SegOuterTR, SegOuterTL,
			(WallWidthCm - HoleRight) / 100.0f, (HoleTop - HoleBottom) / 100.0f, WallThickness);
	}
	
	// Generate hole interior faces (the thickness edges of the hole)
	GenerateHoleInteriorFaces(Vertices, Triangles, Normals, UVs,
		InnerBL, WallWidthDirection, WallHeightDirection,
		OuterBL, OuterWidthDirection, OuterHeightDirection,
		HoleLeft, HoleRight, HoleBottom, HoleTop, WallThickness);
}

void UWallUnit::GenerateHoleInteriorFaces(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs,
	FVector InnerBL, FVector WallWidthDirection, FVector WallHeightDirection,
	FVector OuterBL, FVector OuterWidthDirection, FVector OuterHeightDirection,
	float HoleLeft, float HoleRight, float HoleBottom, float HoleTop, float WallThickness)
{
	// Generate the 4 interior faces that show the wall thickness around the hole opening
	
	// Helper function to add a quad face
	auto AddQuadFace = [&](FVector V0, FVector V1, FVector V2, FVector V3, FVector Normal)
	{
		int32 StartIndex = Vertices.Num();
		
		// Add vertices
		Vertices.Add(V0);
		Vertices.Add(V1);
		Vertices.Add(V2);
		Vertices.Add(V3);
		
		// Add normals
		Normals.Add(Normal);
		Normals.Add(Normal);
		Normals.Add(Normal);
		Normals.Add(Normal);
		
		// Add UVs (simple planar mapping)
		UVs.Add(FVector2D(0.0f, 0.0f));
		UVs.Add(FVector2D(1.0f, 0.0f));
		UVs.Add(FVector2D(1.0f, 1.0f));
		UVs.Add(FVector2D(0.0f, 1.0f));
		
		// Add triangles (two triangles per quad)
		Triangles.Add(StartIndex + 0);
		Triangles.Add(StartIndex + 1);
		Triangles.Add(StartIndex + 2);
		
		Triangles.Add(StartIndex + 0);
		Triangles.Add(StartIndex + 2);
		Triangles.Add(StartIndex + 3);
	};
	
	// Calculate hole corner positions on inner and outer surfaces
	FVector InnerBottomLeft = InnerBL + (WallWidthDirection * HoleLeft) + (WallHeightDirection * HoleBottom);
	FVector InnerBottomRight = InnerBL + (WallWidthDirection * HoleRight) + (WallHeightDirection * HoleBottom);
	FVector InnerTopLeft = InnerBL + (WallWidthDirection * HoleLeft) + (WallHeightDirection * HoleTop);
	FVector InnerTopRight = InnerBL + (WallWidthDirection * HoleRight) + (WallHeightDirection * HoleTop);
	
	FVector OuterBottomLeft = OuterBL + (OuterWidthDirection * HoleLeft) + (OuterHeightDirection * HoleBottom);
	FVector OuterBottomRight = OuterBL + (OuterWidthDirection * HoleRight) + (OuterHeightDirection * HoleBottom);
	FVector OuterTopLeft = OuterBL + (OuterWidthDirection * HoleLeft) + (OuterHeightDirection * HoleTop);
	FVector OuterTopRight = OuterBL + (OuterWidthDirection * HoleRight) + (OuterHeightDirection * HoleTop);
	
	// Calculate face normals
	FVector LeftNormal = -WallWidthDirection;  // Points left (into hole)
	FVector RightNormal = WallWidthDirection;  // Points right (into hole)
	FVector BottomNormal = -WallHeightDirection; // Points down (into hole)
	FVector TopNormal = WallHeightDirection;   // Points up (into hole)
	
	// Left interior face (connects left edge from inner to outer surface)
	AddQuadFace(InnerBottomLeft, OuterBottomLeft, OuterTopLeft, InnerTopLeft, LeftNormal);
	
	// Right interior face (connects right edge from inner to outer surface)
	AddQuadFace(OuterBottomRight, InnerBottomRight, InnerTopRight, OuterTopRight, RightNormal);
	
	// Bottom interior face (connects bottom edge from inner to outer surface)
	AddQuadFace(OuterBottomLeft, InnerBottomLeft, InnerBottomRight, OuterBottomRight, BottomNormal);
	
	// Top interior face (connects top edge from inner to outer surface)
	AddQuadFace(InnerTopLeft, OuterTopLeft, OuterTopRight, InnerTopRight, TopNormal);
}

void UWallUnit::DrawWallCenterDebugSphere(UWorld* World, 
	FVector InnerBL, FVector InnerBR, FVector InnerTR, FVector InnerTL,
	FVector OuterBL, FVector OuterBR, FVector OuterTR, FVector OuterTL)
{
	if (!World)
	{
		return;
	}
	
	// Calculate the geometric center of all 8 corner points
	FVector GeometricCenter = (InnerBL + InnerBR + InnerTR + InnerTL + OuterBL + OuterBR + OuterTR + OuterTL) / 8.0f;
	
	// Draw cyan debug sphere at the wall center
	DrawDebugSphere(World, GeometricCenter, 25.0f, 12, FColor::Cyan, true, -1.0f, 0, 2.0f);
}

// === NEW CUSTOM HOLE GENERATION IMPLEMENTATIONS ===

void UWallUnit::GenerateDoorway(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs,
	FVector InnerBL, FVector InnerBR, FVector InnerTR, FVector InnerTL,
	FVector OuterBL, FVector OuterBR, FVector OuterTR, FVector OuterTL,
	float WallWidth, float WallHeight, float WallThickness, UWorld* World,
	float DoorWidth, float DoorHeight, float HorizontalPosition, float VerticalPosition)
{
	// Clean doorway interface that delegates to the internal hole generation
	// HorizontalPosition: 0.0 = left edge, 0.5 = center, 1.0 = right edge
	// VerticalPosition: 0.0 = bottom-aligned, 0.5 = center, 1.0 = top-aligned
	
	// Convert positioning to center-based coordinate system for GenerateWallWithCustomSquareHole
	// VerticalPosition 0.0 = bottom-aligned, needs to be converted to center-based
	// HorizontalPosition 0.5 = centered, should work as-is
	
	float HoleCenterY = VerticalPosition;
	if (VerticalPosition == 0.0f)
	{
		// Bottom-aligned: position center at half the door height from bottom
		HoleCenterY = (DoorHeight * 0.5f) / WallHeight;
	}
	
	GenerateWallWithCustomSquareHole(Vertices, Triangles, Normals, UVs,
		InnerBL, InnerBR, InnerTR, InnerTL,
		OuterBL, OuterBR, OuterTR, OuterTL,
		WallWidth, WallHeight, WallThickness, World,
		DoorWidth, DoorHeight, HorizontalPosition, HoleCenterY);
}

void UWallUnit::GenerateIrregularHole(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs,
	FVector InnerBL, FVector InnerBR, FVector InnerTR, FVector InnerTL,
	FVector OuterBL, FVector OuterBR, FVector OuterTR, FVector OuterTL,
	float WallWidth, float WallHeight, float WallThickness, UWorld* World,
	float HoleSize, float Irregularity, int32 RandomSeed)
{
	// Clean irregular hole interface that delegates to the internal hole generation
	// HoleSize: hole diameter in meters
	// Irregularity: 0.0 = perfect circle, 1.0 = very chaotic shape
	// RandomSeed: seed for reproducible random generation
	
	GenerateWallWithRandomHole(Vertices, Triangles, Normals, UVs,
		InnerBL, InnerBR, InnerTR, InnerTL,
		OuterBL, OuterBR, OuterTR, OuterTL,
		WallWidth, WallHeight, WallThickness, World,
		HoleSize, Irregularity, RandomSeed);
}

void UWallUnit::GenerateCompleteWallWithDoorway(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs,
	const FVector& Position, const FRotator& Rotation, 
	float WallWidth, float WallHeight, float WallThickness, UWorld* World,
	float DoorWidth, float DoorHeight, float HorizontalPosition, float VerticalPosition)
{
	// Convert meters to Unreal units (cm)
	float WidthCm = WallWidth * 100.0f;
	float HeightCm = WallHeight * 100.0f;
	float ThicknessCm = WallThickness * 100.0f;
	
	// Define wall corners centered around origin (for proper rotation)
	FVector InnerBL = FVector(-WidthCm * 0.5f, -ThicknessCm * 0.5f, -HeightCm * 0.5f);
	FVector InnerBR = FVector(WidthCm * 0.5f, -ThicknessCm * 0.5f, -HeightCm * 0.5f);
	FVector InnerTR = FVector(WidthCm * 0.5f, -ThicknessCm * 0.5f, HeightCm * 0.5f);
	FVector InnerTL = FVector(-WidthCm * 0.5f, -ThicknessCm * 0.5f, HeightCm * 0.5f);
	
	FVector OuterBL = FVector(-WidthCm * 0.5f, ThicknessCm * 0.5f, -HeightCm * 0.5f);
	FVector OuterBR = FVector(WidthCm * 0.5f, ThicknessCm * 0.5f, -HeightCm * 0.5f);
	FVector OuterTR = FVector(WidthCm * 0.5f, ThicknessCm * 0.5f, HeightCm * 0.5f);
	FVector OuterTL = FVector(-WidthCm * 0.5f, ThicknessCm * 0.5f, HeightCm * 0.5f);
	
	// Apply rotation around the wall's center (origin)
	FTransform RotationTransform(Rotation);
	InnerBL = RotationTransform.TransformPosition(InnerBL);
	InnerBR = RotationTransform.TransformPosition(InnerBR);
	InnerTR = RotationTransform.TransformPosition(InnerTR);
	InnerTL = RotationTransform.TransformPosition(InnerTL);
	OuterBL = RotationTransform.TransformPosition(OuterBL);
	OuterBR = RotationTransform.TransformPosition(OuterBR);
	OuterTR = RotationTransform.TransformPosition(OuterTR);
	OuterTL = RotationTransform.TransformPosition(OuterTL);
	
	// Apply position offset after rotation
	InnerBL += Position;
	InnerBR += Position;
	InnerTR += Position;
	InnerTL += Position;
	OuterBL += Position;
	OuterBR += Position;
	OuterTR += Position;
	OuterTL += Position;
	
	// Generate the wall with doorway using the clean interface
	GenerateDoorway(Vertices, Triangles, Normals, UVs,
		InnerBL, InnerBR, InnerTR, InnerTL,
		OuterBL, OuterBR, OuterTR, OuterTL,
		WallWidth, WallHeight, WallThickness, World,
		DoorWidth, DoorHeight, HorizontalPosition, VerticalPosition);
}

void UWallUnit::CreateWallMeshWithDoorway(TArray<FVector>& OutVertices, TArray<int32>& OutTriangles, TArray<FVector>& OutNormals, TArray<FVector2D>& OutUVs,
	const FVector& Position, const FRotator& Rotation, 
	float WallWidth, float WallHeight, float WallThickness, UWorld* World,
	float DoorWidth, float DoorHeight, float HorizontalPosition, float VerticalPosition)
{
	// Clear output arrays
	OutVertices.Empty();
	OutTriangles.Empty();
	OutNormals.Empty();
	OutUVs.Empty();
	
	// Generate the wall geometry
	GenerateCompleteWallWithDoorway(OutVertices, OutTriangles, OutNormals, OutUVs,
		Position, Rotation, WallWidth, WallHeight, WallThickness, World,
		DoorWidth, DoorHeight, HorizontalPosition, VerticalPosition);
	
	// Make it double-sided by creating copies and then appending them
	TArray<int32> OriginalTriangles = OutTriangles;
	TArray<FVector> OriginalNormals = OutNormals;
	TArray<FVector2D> OriginalUVs = OutUVs;
	
	// Add reversed triangles (swap order for opposite normal direction)
	for (int32 i = 0; i < OriginalTriangles.Num(); i += 3)
	{
		OutTriangles.Add(OriginalTriangles[i + 2]);
		OutTriangles.Add(OriginalTriangles[i + 1]);
		OutTriangles.Add(OriginalTriangles[i + 0]);
	}
	
	// Add reversed normals for the reversed faces
	for (int32 i = 0; i < OriginalNormals.Num(); i++)
	{
		OutNormals.Add(-OriginalNormals[i]);
	}
	
	// Add duplicate UVs for the reversed faces
	for (int32 i = 0; i < OriginalUVs.Num(); i++)
	{
		OutUVs.Add(OriginalUVs[i]);
	}
}

AActor* UWallUnit::CreateCompleteWallActor(UWorld* World, const FVector& Position, const FRotator& Rotation,
	float WallWidth, float WallHeight, float WallThickness, const FLinearColor& Color,
	float DoorWidth, float DoorHeight, float HorizontalPosition, float VerticalPosition)
{
	if (!World)
	{
		return nullptr;
	}

	UE_LOG(LogBackRoomGenerator, Warning, TEXT("🔧 CreateCompleteWallActor: Creating wall at %s, %.1fx%.1fm hole at pos %.2f,%.2f"), 
		*Position.ToString(), DoorWidth, DoorHeight, HorizontalPosition, VerticalPosition);

	// Generate complete wall mesh data
	TArray<FVector> WallVertices;
	TArray<int32> WallTriangles;
	TArray<FVector> WallNormals;
	TArray<FVector2D> WallUVs;
	
	CreateWallMeshWithDoorway(WallVertices, WallTriangles, WallNormals, WallUVs,
		Position, Rotation, WallWidth, WallHeight, WallThickness, World,
		DoorWidth, DoorHeight, HorizontalPosition, VerticalPosition);
	
	UE_LOG(LogBackRoomGenerator, Warning, TEXT("🔧 CreateWallMeshWithDoorway: Generated %d vertices, %d triangles"), 
		WallVertices.Num(), WallTriangles.Num());
	
	// Create actor and mesh component
	AActor* WallActor = World->SpawnActor<AActor>();
	UProceduralMeshComponent* WallMesh = NewObject<UProceduralMeshComponent>(WallActor);
	WallActor->SetRootComponent(WallMesh);
	
	// Setup collision settings - standard wall configuration
	WallMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WallMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	WallMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WallMesh->bUseComplexAsSimpleCollision = true;
	WallMesh->RegisterComponent();
	
	// Create mesh section
	WallMesh->CreateMeshSection(0, WallVertices, WallTriangles, WallNormals, WallUVs, 
		TArray<FColor>(), TArray<FProcMeshTangent>(), true);
	
	// Apply colored material
	UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
	if (BaseMaterial)
	{
		UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(BaseMaterial, WallActor);
		if (DynMat)
		{
			DynMat->SetVectorParameterValue(TEXT("Color"), Color);
			DynMat->SetVectorParameterValue(TEXT("BaseColor"), Color);
			WallMesh->SetMaterial(0, DynMat);
		}
	}
	
	return WallActor;
}

AActor* UWallUnit::CreateSolidWallActor(UWorld* World, const FVector& Position, const FRotator& Rotation,
	float WallWidth, float WallHeight, float WallThickness, const FLinearColor& Color)
{
	if (!World)
	{
		return nullptr;
	}

	// Convert meters to Unreal units (cm)
	float WidthCm = WallWidth * 100.0f;
	float HeightCm = WallHeight * 100.0f;
	float ThicknessCm = WallThickness * 100.0f;
	
	// Define wall corners centered around origin (for proper rotation)
	FVector InnerBL = FVector(-WidthCm * 0.5f, -ThicknessCm * 0.5f, -HeightCm * 0.5f);
	FVector InnerBR = FVector(WidthCm * 0.5f, -ThicknessCm * 0.5f, -HeightCm * 0.5f);
	FVector InnerTR = FVector(WidthCm * 0.5f, -ThicknessCm * 0.5f, HeightCm * 0.5f);
	FVector InnerTL = FVector(-WidthCm * 0.5f, -ThicknessCm * 0.5f, HeightCm * 0.5f);
	
	FVector OuterBL = FVector(-WidthCm * 0.5f, ThicknessCm * 0.5f, -HeightCm * 0.5f);
	FVector OuterBR = FVector(WidthCm * 0.5f, ThicknessCm * 0.5f, -HeightCm * 0.5f);
	FVector OuterTR = FVector(WidthCm * 0.5f, ThicknessCm * 0.5f, HeightCm * 0.5f);
	FVector OuterTL = FVector(-WidthCm * 0.5f, ThicknessCm * 0.5f, HeightCm * 0.5f);
	
	// Apply rotation around the wall's center (origin)
	FTransform RotationTransform(Rotation);
	InnerBL = RotationTransform.TransformPosition(InnerBL);
	InnerBR = RotationTransform.TransformPosition(InnerBR);
	InnerTR = RotationTransform.TransformPosition(InnerTR);
	InnerTL = RotationTransform.TransformPosition(InnerTL);
	OuterBL = RotationTransform.TransformPosition(OuterBL);
	OuterBR = RotationTransform.TransformPosition(OuterBR);
	OuterTR = RotationTransform.TransformPosition(OuterTR);
	OuterTL = RotationTransform.TransformPosition(OuterTL);
	
	// Apply position offset after rotation
	InnerBL += Position;
	InnerBR += Position;
	InnerTR += Position;
	InnerTL += Position;
	OuterBL += Position;
	OuterBR += Position;
	OuterTR += Position;
	OuterTL += Position;
	
	// Generate solid wall mesh (no holes)
	TArray<FVector> WallVertices;
	TArray<int32> WallTriangles;
	TArray<FVector> WallNormals;
	TArray<FVector2D> WallUVs;
	
	GenerateThickWall(WallVertices, WallTriangles, WallNormals, WallUVs,
		InnerBL, InnerBR, InnerTR, InnerTL,
		OuterBL, OuterBR, OuterTR, OuterTL,
		WallWidth, WallHeight, EWallSide::North, WallThickness, World);
	
	// Make it double-sided by creating copies and then appending them
	TArray<int32> OriginalTriangles = WallTriangles;
	TArray<FVector> OriginalNormals = WallNormals;
	TArray<FVector2D> OriginalUVs = WallUVs;
	
	// Add reversed triangles (swap order for opposite normal direction)
	for (int32 i = 0; i < OriginalTriangles.Num(); i += 3)
	{
		WallTriangles.Add(OriginalTriangles[i + 2]);
		WallTriangles.Add(OriginalTriangles[i + 1]);
		WallTriangles.Add(OriginalTriangles[i + 0]);
	}
	
	// Add reversed normals for the reversed faces
	for (int32 i = 0; i < OriginalNormals.Num(); i++)
	{
		WallNormals.Add(-OriginalNormals[i]);
	}
	
	// Add duplicate UVs for the reversed faces
	for (int32 i = 0; i < OriginalUVs.Num(); i++)
	{
		WallUVs.Add(OriginalUVs[i]);
	}
	
	// Create actor and mesh component
	AActor* WallActor = World->SpawnActor<AActor>();
	UProceduralMeshComponent* WallMesh = NewObject<UProceduralMeshComponent>(WallActor);
	WallActor->SetRootComponent(WallMesh);
	
	// Setup collision settings - standard wall configuration
	WallMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WallMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	WallMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WallMesh->bUseComplexAsSimpleCollision = true;
	WallMesh->RegisterComponent();
	
	// Create mesh section
	WallMesh->CreateMeshSection(0, WallVertices, WallTriangles, WallNormals, WallUVs, 
		TArray<FColor>(), TArray<FProcMeshTangent>(), true);
	
	// Apply colored material
	UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
	if (BaseMaterial)
	{
		UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(BaseMaterial, WallActor);
		if (DynMat)
		{
			DynMat->SetVectorParameterValue(TEXT("Color"), Color);
			DynMat->SetVectorParameterValue(TEXT("BaseColor"), Color);
			WallMesh->SetMaterial(0, DynMat);
		}
	}
	
	return WallActor;
}

void UWallUnit::GenerateWallWithCustomSquareHole(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs,
	FVector InnerBL, FVector InnerBR, FVector InnerTR, FVector InnerTL,
	FVector OuterBL, FVector OuterBR, FVector OuterTR, FVector OuterTL,
	float WallWidth, float WallHeight, float WallThickness, UWorld* World,
	float HoleWidth, float HoleHeight, float HoleCenterX, float HoleCenterY)
{
	// FIXED: Generate wall with proper vertical positioning
	// HoleCenterX: 0.0 = left edge, 0.5 = center, 1.0 = right edge
	// HoleCenterY: 0.0 = bottom edge, 0.5 = center, 1.0 = top edge
	
	// Convert to unreal units
	float WallWidthCm = MetersToUnrealUnits(WallWidth);
	float WallHeightCm = MetersToUnrealUnits(WallHeight);
	float HoleWidthCm = MetersToUnrealUnits(HoleWidth);
	float HoleHeightCm = MetersToUnrealUnits(HoleHeight);
	
	// Calculate hole position from CENTER coordinates
	// Wall coordinate system: InnerBL starts at (-WallWidth/2, -WallHeight/2)
	// Hole positioning: 0.0 = left edge, 0.5 = center, 1.0 = right edge
	// Convert normalized position to distance from left edge (InnerBL)
	
	float HoleCenterXCm = HoleCenterX * WallWidthCm;  // Distance from left edge
	float HoleCenterYCm = HoleCenterY * WallHeightCm; // Distance from bottom edge
	
	// Calculate hole bounds relative to wall's left edge (for segment generation)
	float HoleLeft = HoleCenterXCm - (HoleWidthCm * 0.5f);
	float HoleRight = HoleCenterXCm + (HoleWidthCm * 0.5f);
	float HoleBottom = HoleCenterYCm - (HoleHeightCm * 0.5f);
	float HoleTop = HoleCenterYCm + (HoleHeightCm * 0.5f);
	
	// DEBUG: Log hole positioning calculation
	if (World)
	{
		UE_LOG(LogBackRoomGenerator, Warning, TEXT("HOLE DEBUG: HoleCenterX=%.2f->%.1fcm, HoleLeft=%.1fcm, HoleRight=%.1fcm"), 
			HoleCenterX, HoleCenterXCm, HoleLeft, HoleRight);
	}
	
	// Clamp to wall bounds
	HoleLeft = FMath::Max(0.0f, HoleLeft);
	HoleRight = FMath::Min(WallWidthCm, HoleRight);
	HoleBottom = FMath::Max(0.0f, HoleBottom);
	HoleTop = FMath::Min(WallHeightCm, HoleTop);
	
	// Skip if hole is too small or outside wall bounds
	if ((HoleRight - HoleLeft) < 10.0f || (HoleTop - HoleBottom) < 10.0f)
	{
		// Generate solid wall as fallback
		UWallCommon::GenerateThickWallSegment(Vertices, Triangles, Normals, UVs,
			InnerBL, InnerBR, InnerTR, InnerTL,
			OuterBL, OuterBR, OuterTR, OuterTL,
			WallWidth, WallHeight, WallThickness);
		return;
	}
	
	// Generate wall segments around the hole - simple 4-segment approach
	FVector WallWidthDirection = (InnerBR - InnerBL).GetSafeNormal();
	FVector WallHeightDirection = (InnerTL - InnerBL).GetSafeNormal();
	FVector OuterWidthDirection = (OuterBR - OuterBL).GetSafeNormal();
	FVector OuterHeightDirection = (OuterTL - OuterBL).GetSafeNormal();
	
	// Bottom segment (0 to HoleBottom)
	if (HoleBottom > 1.0f)
	{
		FVector SegInnerBL = InnerBL;
		FVector SegInnerBR = InnerBR;
		FVector SegInnerTR = InnerBL + (WallWidthDirection * WallWidthCm) + (WallHeightDirection * HoleBottom);
		FVector SegInnerTL = InnerBL + (WallHeightDirection * HoleBottom);
		
		FVector SegOuterBL = OuterBL;
		FVector SegOuterBR = OuterBR;
		FVector SegOuterTR = OuterBL + (OuterWidthDirection * WallWidthCm) + (OuterHeightDirection * HoleBottom);
		FVector SegOuterTL = OuterBL + (OuterHeightDirection * HoleBottom);
		
		UWallCommon::GenerateThickWallSegment(Vertices, Triangles, Normals, UVs,
			SegInnerBL, SegInnerBR, SegInnerTR, SegInnerTL,
			SegOuterBL, SegOuterBR, SegOuterTR, SegOuterTL,
			WallWidth, HoleBottom / 100.0f, WallThickness);
	}
	
	// Top segment (HoleTop to WallHeight)
	if ((WallHeightCm - HoleTop) > 1.0f)
	{
		FVector SegInnerBL = InnerBL + (WallHeightDirection * HoleTop);
		FVector SegInnerBR = InnerBL + (WallWidthDirection * WallWidthCm) + (WallHeightDirection * HoleTop);
		FVector SegInnerTR = InnerTR;
		FVector SegInnerTL = InnerTL;
		
		FVector SegOuterBL = OuterBL + (OuterHeightDirection * HoleTop);
		FVector SegOuterBR = OuterBL + (OuterWidthDirection * WallWidthCm) + (OuterHeightDirection * HoleTop);
		FVector SegOuterTR = OuterTR;
		FVector SegOuterTL = OuterTL;
		
		UWallCommon::GenerateThickWallSegment(Vertices, Triangles, Normals, UVs,
			SegInnerBL, SegInnerBR, SegInnerTR, SegInnerTL,
			SegOuterBL, SegOuterBR, SegOuterTR, SegOuterTL,
			WallWidth, (WallHeightCm - HoleTop) / 100.0f, WallThickness);
	}
	
	// Left segment (0 to HoleLeft, within hole height)
	if (HoleLeft > 1.0f)
	{
		FVector SegInnerBL = InnerBL + (WallHeightDirection * HoleBottom);
		FVector SegInnerBR = InnerBL + (WallWidthDirection * HoleLeft) + (WallHeightDirection * HoleBottom);
		FVector SegInnerTR = InnerBL + (WallWidthDirection * HoleLeft) + (WallHeightDirection * HoleTop);
		FVector SegInnerTL = InnerBL + (WallHeightDirection * HoleTop);
		
		FVector SegOuterBL = OuterBL + (OuterHeightDirection * HoleBottom);
		FVector SegOuterBR = OuterBL + (OuterWidthDirection * HoleLeft) + (OuterHeightDirection * HoleBottom);
		FVector SegOuterTR = OuterBL + (OuterWidthDirection * HoleLeft) + (OuterHeightDirection * HoleTop);
		FVector SegOuterTL = OuterBL + (OuterHeightDirection * HoleTop);
		
		UWallCommon::GenerateThickWallSegment(Vertices, Triangles, Normals, UVs,
			SegInnerBL, SegInnerBR, SegInnerTR, SegInnerTL,
			SegOuterBL, SegOuterBR, SegOuterTR, SegOuterTL,
			HoleLeft / 100.0f, (HoleTop - HoleBottom) / 100.0f, WallThickness);
	}
	
	// Right segment (HoleRight to WallWidth, within hole height)
	if ((WallWidthCm - HoleRight) > 1.0f)
	{
		FVector SegInnerBL = InnerBL + (WallWidthDirection * HoleRight) + (WallHeightDirection * HoleBottom);
		FVector SegInnerBR = InnerBL + (WallWidthDirection * WallWidthCm) + (WallHeightDirection * HoleBottom);
		FVector SegInnerTR = InnerBL + (WallWidthDirection * WallWidthCm) + (WallHeightDirection * HoleTop);
		FVector SegInnerTL = InnerBL + (WallWidthDirection * HoleRight) + (WallHeightDirection * HoleTop);
		
		FVector SegOuterBL = OuterBL + (OuterWidthDirection * HoleRight) + (OuterHeightDirection * HoleBottom);
		FVector SegOuterBR = OuterBL + (OuterWidthDirection * WallWidthCm) + (OuterHeightDirection * HoleBottom);
		FVector SegOuterTR = OuterBL + (OuterWidthDirection * WallWidthCm) + (OuterHeightDirection * HoleTop);
		FVector SegOuterTL = OuterBL + (OuterWidthDirection * HoleRight) + (OuterHeightDirection * HoleTop);
		
		UWallCommon::GenerateThickWallSegment(Vertices, Triangles, Normals, UVs,
			SegInnerBL, SegInnerBR, SegInnerTR, SegInnerTL,
			SegOuterBL, SegOuterBR, SegOuterTR, SegOuterTL,
			(WallWidthCm - HoleRight) / 100.0f, (HoleTop - HoleBottom) / 100.0f, WallThickness);
	}
	
	// Generate hole interior faces (the thickness edges of the hole)
	GenerateHoleInteriorFaces(Vertices, Triangles, Normals, UVs,
		InnerBL, WallWidthDirection, WallHeightDirection,
		OuterBL, OuterWidthDirection, OuterHeightDirection,
		HoleLeft, HoleRight, HoleBottom, HoleTop, WallThickness);
	
	// Draw debug sphere if World is provided
	if (World)
	{
		DrawWallCenterDebugSphere(World, InnerBL, InnerBR, InnerTR, InnerTL, OuterBL, OuterBR, OuterTR, OuterTL);
	}
}

void UWallUnit::GenerateWallWithRandomHole(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs,
	FVector InnerBL, FVector InnerBR, FVector InnerTR, FVector InnerTL,
	FVector OuterBL, FVector OuterBR, FVector OuterTR, FVector OuterTL,
	float WallWidth, float WallHeight, float WallThickness, UWorld* World,
	float HoleSize, float Irregularity, int32 RandomSeed)
{
	// Create custom FDoorConfig for random irregular hole
	FDoorConfig RandomDoor;
	RandomDoor.bHasDoor = true;
	RandomDoor.HoleShape = EHoleShape::Irregular;
	RandomDoor.IrregularSize = HoleSize;      // Size in meters
	RandomDoor.Irregularity = Irregularity;   // 0.0 = circle, 1.0 = very chaotic
	RandomDoor.IrregularPoints = 8 + (int32)(Irregularity * 12); // 8-20 points based on irregularity
	RandomDoor.IrregularSmoothness = 1.0f - Irregularity;        // Smoother when less irregular
	RandomDoor.RandomSeed = RandomSeed;       // Seed for reproducible randomness
	RandomDoor.IrregularRotation = FMath::FRand() * 360.0f;      // Random rotation
	
	// Use HoleGenerator for complex irregular shapes (performance warning: use sparingly!)
	UHoleGenerator::GenerateWallWithHole(Vertices, Triangles, Normals, UVs,
		InnerBL, InnerBR, InnerTR, InnerTL,
		OuterBL, OuterBR, OuterTR, OuterTL,
		WallWidth, WallHeight, RandomDoor, WallThickness);
	
	// Draw debug sphere if World is provided
	if (World)
	{
		DrawWallCenterDebugSphere(World, InnerBL, InnerBR, InnerTR, InnerTL, OuterBL, OuterBR, OuterTR, OuterTL);
	}
}

// === ADVANCED HOLE CONFIGURATION IMPLEMENTATIONS ===

AActor* UWallUnit::CreateWallWithHole(UWorld* World, const FVector& Position, const FRotator& Rotation,
	float WallWidth, float WallHeight, float WallThickness, const FLinearColor& Color,
	const FWallHoleConfig& HoleConfig)
{
	if (!World)
	{
		return nullptr;
	}

	// HYBRID HOLE SYSTEM: Use best system for each shape type
	if (HoleConfig.Shape == EHoleShape::Irregular)
	{
		// Convert FWallHoleConfig to FDoorConfig for irregular hole generation
		FDoorConfig DoorConfig;
		DoorConfig.bHasDoor = true;
		DoorConfig.HoleShape = EHoleShape::Irregular;
		DoorConfig.IrregularSize = FMath::Max(HoleConfig.Width, HoleConfig.Height); // Use larger dimension as base size
		
		// Configure shape parameters based on hole name for specific shapes
		if (HoleConfig.HoleName == TEXT("Circle"))
		{
			DoorConfig.IrregularPoints = 24;
			DoorConfig.Irregularity = 0.0f;
			DoorConfig.IrregularSmoothness = 1.0f;
			DoorConfig.IrregularRotation = 0.0f;
			DoorConfig.RandomSeed = 12345; // Fixed seed for consistent circles
		}
		else if (HoleConfig.HoleName == TEXT("Triangle"))
		{
			DoorConfig.IrregularPoints = 3;
			DoorConfig.Irregularity = 0.1f;
			DoorConfig.IrregularSmoothness = 0.1f;
			DoorConfig.IrregularRotation = 0.0f;
			DoorConfig.RandomSeed = 11111;
		}
		else if (HoleConfig.HoleName == TEXT("Square"))
		{
			DoorConfig.IrregularPoints = 4;
			DoorConfig.Irregularity = 0.0f;
			DoorConfig.IrregularSmoothness = 0.2f;
			DoorConfig.IrregularRotation = 45.0f; // Diamond orientation
			DoorConfig.RandomSeed = 22222;
		}
		else if (HoleConfig.HoleName == TEXT("Hexagon"))
		{
			DoorConfig.IrregularPoints = 6;
			DoorConfig.Irregularity = 0.0f;
			DoorConfig.IrregularSmoothness = 0.5f;
			DoorConfig.IrregularRotation = 0.0f;
			DoorConfig.RandomSeed = 33333;
		}
		else if (HoleConfig.HoleName == TEXT("Star"))
		{
			DoorConfig.IrregularPoints = 8;
			DoorConfig.Irregularity = 0.5f;
			DoorConfig.IrregularSmoothness = 0.1f;
			DoorConfig.IrregularRotation = 22.5f; // Slight rotation for star effect
			DoorConfig.RandomSeed = 44444;
		}
		else if (HoleConfig.HoleName == TEXT("Flower"))
		{
			DoorConfig.IrregularPoints = 12;
			DoorConfig.Irregularity = 0.4f;
			DoorConfig.IrregularSmoothness = 0.8f;
			DoorConfig.IrregularRotation = 15.0f;
			DoorConfig.RandomSeed = 55555;
		}
		else if (HoleConfig.HoleName == TEXT("Blob"))
		{
			DoorConfig.IrregularPoints = 10;
			DoorConfig.Irregularity = 0.8f;
			DoorConfig.IrregularSmoothness = 0.9f;
			DoorConfig.IrregularRotation = FMath::RandRange(0.0f, 360.0f);
			DoorConfig.RandomSeed = 66666;
		}
		else if (HoleConfig.HoleName == TEXT("Crystal"))
		{
			DoorConfig.IrregularPoints = 6;
			DoorConfig.Irregularity = 0.6f;
			DoorConfig.IrregularSmoothness = 0.0f;
			DoorConfig.IrregularRotation = 30.0f;
			DoorConfig.RandomSeed = 77777;
		}
		else
		{
			// Default random irregular hole (for Row 6 random shapes)
			DoorConfig.Irregularity = 0.7f; // Good amount of randomness for organic shapes
			DoorConfig.IrregularPoints = 12; // Good balance between smooth and interesting
			DoorConfig.IrregularSmoothness = 0.3f; // Some smoothing but keep organic feel
			DoorConfig.IrregularRotation = FMath::RandRange(0.0f, 360.0f); // Random rotation
			DoorConfig.RandomSeed = FMath::RandRange(1000, 99999); // Random seed for unique shapes
		}
		
		// Convert hole config to normalized position
		float FinalHorizontalPos, FinalVerticalPos;
		HoleConfig.GetNormalizedPosition(WallWidth, WallHeight, FinalHorizontalPos, FinalVerticalPos);
		
		// Convert to offset from center for irregular hole system
		// FinalHorizontalPos = 0.5 means center, < 0.5 means left, > 0.5 means right
		float HorizontalOffsetMeters = (FinalHorizontalPos - 0.5f) * WallWidth;
		DoorConfig.OffsetFromCenter = HorizontalOffsetMeters;
		
		// For irregular holes, we need to use a different approach
		// Create wall with irregular hole by using the hole generator system directly
		TArray<FVector> WallVertices;
		TArray<int32> WallTriangles; 
		TArray<FVector> WallNormals;
		TArray<FVector2D> WallUVs;
		
		// Convert position and setup wall corners
		float WidthCm = WallWidth * 100.0f;
		float HeightCm = WallHeight * 100.0f;
		float ThicknessCm = WallThickness * 100.0f;
		
		FVector InnerBL = FVector(-WidthCm * 0.5f, -ThicknessCm * 0.5f, -HeightCm * 0.5f);
		FVector InnerBR = FVector(WidthCm * 0.5f, -ThicknessCm * 0.5f, -HeightCm * 0.5f);
		FVector InnerTR = FVector(WidthCm * 0.5f, -ThicknessCm * 0.5f, HeightCm * 0.5f);
		FVector InnerTL = FVector(-WidthCm * 0.5f, -ThicknessCm * 0.5f, HeightCm * 0.5f);
		
		FVector OuterBL = FVector(-WidthCm * 0.5f, ThicknessCm * 0.5f, -HeightCm * 0.5f);
		FVector OuterBR = FVector(WidthCm * 0.5f, ThicknessCm * 0.5f, -HeightCm * 0.5f);
		FVector OuterTR = FVector(WidthCm * 0.5f, ThicknessCm * 0.5f, HeightCm * 0.5f);
		FVector OuterTL = FVector(-WidthCm * 0.5f, ThicknessCm * 0.5f, HeightCm * 0.5f);
		
		// Apply rotation
		FTransform RotationTransform(Rotation);
		InnerBL = RotationTransform.TransformPosition(InnerBL) + Position;
		InnerBR = RotationTransform.TransformPosition(InnerBR) + Position;
		InnerTR = RotationTransform.TransformPosition(InnerTR) + Position;
		InnerTL = RotationTransform.TransformPosition(InnerTL) + Position;
		OuterBL = RotationTransform.TransformPosition(OuterBL) + Position;
		OuterBR = RotationTransform.TransformPosition(OuterBR) + Position;
		OuterTR = RotationTransform.TransformPosition(OuterTR) + Position;
		OuterTL = RotationTransform.TransformPosition(OuterTL) + Position;
		
		// Generate wall with irregular hole using HoleGenerator
		UHoleGenerator::GenerateWallWithHole(WallVertices, WallTriangles, WallNormals, WallUVs,
			InnerBL, InnerBR, InnerTR, InnerTL,
			OuterBL, OuterBR, OuterTR, OuterTL,
			WallWidth, WallHeight, DoorConfig, WallThickness);
		
		// Make it double-sided by creating copies and then appending them (same as rectangular system)
		TArray<int32> OriginalTriangles = WallTriangles;
		TArray<FVector> OriginalNormals = WallNormals;
		TArray<FVector2D> OriginalUVs = WallUVs;
		
		// Add reversed triangles (swap order for opposite normal direction)
		for (int32 i = 0; i < OriginalTriangles.Num(); i += 3)
		{
			WallTriangles.Add(OriginalTriangles[i + 2]);
			WallTriangles.Add(OriginalTriangles[i + 1]);
			WallTriangles.Add(OriginalTriangles[i + 0]);
		}
		
		// Add reversed normals for the reversed faces
		for (int32 i = 0; i < OriginalNormals.Num(); i++)
		{
			WallNormals.Add(-OriginalNormals[i]);
		}
		
		// Add duplicate UVs for the reversed faces
		for (int32 i = 0; i < OriginalUVs.Num(); i++)
		{
			WallUVs.Add(OriginalUVs[i]);
		}
		
		// Create actor and setup
		AActor* WallActor = World->SpawnActor<AActor>();
		UProceduralMeshComponent* WallMesh = NewObject<UProceduralMeshComponent>(WallActor);
		WallActor->SetRootComponent(WallMesh);
		
		// Setup collision
		WallMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WallMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
		WallMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
		WallMesh->bUseComplexAsSimpleCollision = true;
		WallMesh->RegisterComponent();
		
		// Create mesh section
		WallMesh->CreateMeshSection(0, WallVertices, WallTriangles, WallNormals, WallUVs, 
			TArray<FColor>(), TArray<FProcMeshTangent>(), true);
		
		// Apply colored material
		UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
		if (BaseMaterial)
		{
			UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(BaseMaterial, WallActor);
			if (DynMat)
			{
				DynMat->SetVectorParameterValue(TEXT("Color"), Color);
				DynMat->SetVectorParameterValue(TEXT("BaseColor"), Color);
				WallMesh->SetMaterial(0, DynMat);
			}
		}
		
		return WallActor;
	}
	else if (HoleConfig.Shape == EHoleShape::Circle)
	{
		// ENHANCED CIRCLES: Use irregular system for perfect circles
		FDoorConfig CircleConfig;
		CircleConfig.bHasDoor = true;
		CircleConfig.HoleShape = EHoleShape::Irregular;
		CircleConfig.IrregularSize = FMath::Max(HoleConfig.Width, HoleConfig.Height);
		CircleConfig.IrregularPoints = 16; // Good performance/quality balance
		CircleConfig.Irregularity = 0.0f;
		CircleConfig.IrregularSmoothness = 1.0f;
		CircleConfig.IrregularRotation = 0.0f;
		CircleConfig.RandomSeed = 30000;
		
		// Convert position and use irregular system for enhanced circles
		float FinalHorizontalPos, FinalVerticalPos;
		HoleConfig.GetNormalizedPosition(WallWidth, WallHeight, FinalHorizontalPos, FinalVerticalPos);
		float HorizontalOffsetMeters = (FinalHorizontalPos - 0.5f) * WallWidth;
		CircleConfig.OffsetFromCenter = HorizontalOffsetMeters;
		
		// Use same setup as irregular holes
		TArray<FVector> WallVertices;
		TArray<int32> WallTriangles; 
		TArray<FVector> WallNormals;
		TArray<FVector2D> WallUVs;
		
		// Setup wall corners (same as irregular system)
		float WidthCm = WallWidth * 100.0f;
		float HeightCm = WallHeight * 100.0f;
		float ThicknessCm = WallThickness * 100.0f;
		
		FVector InnerBL = FVector(-WidthCm * 0.5f, -ThicknessCm * 0.5f, -HeightCm * 0.5f);
		FVector InnerBR = FVector(WidthCm * 0.5f, -ThicknessCm * 0.5f, -HeightCm * 0.5f);
		FVector InnerTR = FVector(WidthCm * 0.5f, -ThicknessCm * 0.5f, HeightCm * 0.5f);
		FVector InnerTL = FVector(-WidthCm * 0.5f, -ThicknessCm * 0.5f, HeightCm * 0.5f);
		
		FVector OuterBL = FVector(-WidthCm * 0.5f, ThicknessCm * 0.5f, -HeightCm * 0.5f);
		FVector OuterBR = FVector(WidthCm * 0.5f, ThicknessCm * 0.5f, -HeightCm * 0.5f);
		FVector OuterTR = FVector(WidthCm * 0.5f, ThicknessCm * 0.5f, HeightCm * 0.5f);
		FVector OuterTL = FVector(-WidthCm * 0.5f, ThicknessCm * 0.5f, HeightCm * 0.5f);
		
		// Apply rotation and position
		FTransform RotationTransform(Rotation);
		InnerBL = RotationTransform.TransformPosition(InnerBL) + Position;
		InnerBR = RotationTransform.TransformPosition(InnerBR) + Position;
		InnerTR = RotationTransform.TransformPosition(InnerTR) + Position;
		InnerTL = RotationTransform.TransformPosition(InnerTL) + Position;
		OuterBL = RotationTransform.TransformPosition(OuterBL) + Position;
		OuterBR = RotationTransform.TransformPosition(OuterBR) + Position;
		OuterTR = RotationTransform.TransformPosition(OuterTR) + Position;
		OuterTL = RotationTransform.TransformPosition(OuterTL) + Position;
		
		// Generate enhanced circle using irregular system
		UHoleGenerator::GenerateWallWithHole(WallVertices, WallTriangles, WallNormals, WallUVs,
			InnerBL, InnerBR, InnerTR, InnerTL,
			OuterBL, OuterBR, OuterTR, OuterTL,
			WallWidth, WallHeight, CircleConfig, WallThickness);
		
		// Apply double-siding and create actor (same as irregular)
		TArray<int32> OriginalTriangles = WallTriangles;
		TArray<FVector> OriginalNormals = WallNormals;
		TArray<FVector2D> OriginalUVs = WallUVs;
		
		for (int32 i = 0; i < OriginalTriangles.Num(); i += 3)
		{
			WallTriangles.Add(OriginalTriangles[i + 2]);
			WallTriangles.Add(OriginalTriangles[i + 1]);
			WallTriangles.Add(OriginalTriangles[i + 0]);
		}
		
		for (int32 i = 0; i < OriginalNormals.Num(); i++)
		{
			WallNormals.Add(-OriginalNormals[i]);
		}
		
		for (int32 i = 0; i < OriginalUVs.Num(); i++)
		{
			WallUVs.Add(OriginalUVs[i]);
		}
		
		// Create actor with enhanced circle
		AActor* WallActor = World->SpawnActor<AActor>();
		UProceduralMeshComponent* WallMesh = NewObject<UProceduralMeshComponent>(WallActor);
		WallActor->SetRootComponent(WallMesh);
		
		WallMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WallMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
		WallMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
		WallMesh->bUseComplexAsSimpleCollision = true;
		WallMesh->RegisterComponent();
		
		WallMesh->CreateMeshSection(0, WallVertices, WallTriangles, WallNormals, WallUVs, 
			TArray<FColor>(), TArray<FProcMeshTangent>(), true);
		
		// Apply material
		UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
		if (BaseMaterial)
		{
			UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(BaseMaterial, WallActor);
			if (DynMat)
			{
				DynMat->SetVectorParameterValue(TEXT("Color"), Color);
				DynMat->SetVectorParameterValue(TEXT("BaseColor"), Color);
				WallMesh->SetMaterial(0, DynMat);
			}
		}
		
		return WallActor;
	}
	else
	{
		// For rectangle holes, use existing fast system (optimized for proper rectangles)
		float FinalHorizontalPos, FinalVerticalPos;
		HoleConfig.GetNormalizedPosition(WallWidth, WallHeight, FinalHorizontalPos, FinalVerticalPos);
		return CreateCompleteWallActor(World, Position, Rotation, WallWidth, WallHeight, WallThickness, Color,
			HoleConfig.Width, HoleConfig.Height, FinalHorizontalPos, FinalVerticalPos);
	}
}

AActor* UWallUnit::CreateWallWithMultipleHoles(UWorld* World, const FVector& Position, const FRotator& Rotation,
	float WallWidth, float WallHeight, float WallThickness, const FLinearColor& Color,
	const TArray<FWallHoleConfig>& HoleConfigs)
{
	if (!World)
	{
		return nullptr;
	}

	// Handle different cases based on number of holes
	if (HoleConfigs.Num() == 0)
	{
		// No holes - create solid wall
		return CreateSolidWallActor(World, Position, Rotation, WallWidth, WallHeight, WallThickness, Color);
	}
	else if (HoleConfigs.Num() == 1)
	{
		// Single hole - use single hole method
		return CreateWallWithHole(World, Position, Rotation, WallWidth, WallHeight, WallThickness, Color, HoleConfigs[0]);
	}
	else
	{
		// Multiple holes - for now, create the first hole only
		// TODO: Implement true multiple holes by combining geometry
		UE_LOG(LogBackRoomGenerator, Warning, TEXT("Multiple holes requested (%d), creating first hole only. Full multiple holes implementation pending."), HoleConfigs.Num());
		return CreateWallWithHole(World, Position, Rotation, WallWidth, WallHeight, WallThickness, Color, HoleConfigs[0]);
	}
}
