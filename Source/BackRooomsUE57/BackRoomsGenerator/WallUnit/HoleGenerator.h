#pragma once

#include "CoreMinimal.h"
#include "WallCommon.h"

/**
 * Universal hole generator that handles all hole types:
 * - Rectangles: 4 points, 45° rotation, 0 irregularity
 * - Circles: 24 points, 0° rotation, 0 irregularity  
 * - Irregular: Any configuration with custom chaos/smoothness
 */
class UHoleGenerator
{
public:
	static void GenerateWallWithHole(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs,
		FVector InnerBL, FVector InnerBR, FVector InnerTR, FVector InnerTL,
		FVector OuterBL, FVector OuterBR, FVector OuterTR, FVector OuterTL,
		float WallWidth, float WallHeight, const FDoorConfig& Door, float WallThickness);

private:
	// Ray casting point-in-polygon detection
	static bool IsPointInIrregularPolygon(const FVector2D& Point, const TArray<FVector2D>& PolygonPoints);
	
	// Generate random polygon points
	static TArray<FVector2D> GenerateIrregularPolygon(const FDoorConfig& Door, float BaseSizeCm);
};