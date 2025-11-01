#pragma once

#include "CoreMinimal.h"
#include "../Types.h"

/**
 * Common utilities and data structures shared by all wall units
 */
class UWallCommon
{
public:
	// Face generation optimization structure
	struct FFaceData
	{
		TArray<FVector> Vertices;     // 4 vertices for quad face
		FVector Normal;               // Normal vector for all vertices
		TArray<FVector2D> UVs;        // 4 UV coordinates
		bool bReverseWinding = false; // For proper triangle orientation
		
		FFaceData() { Vertices.Reserve(4); UVs.Reserve(4); }
	};

	// Optimized helper functions
	static void AddQuadFace(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs, const FFaceData& Face);
	
	// Helper function for door frames
	static void AddDoorFrame(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs,
		FVector InnerV1, FVector OuterV1, FVector OuterV2, FVector InnerV2, float FrameThickness, float FrameSize);
	
	// Helper function to generate solid thick wall segment
	static void GenerateThickWallSegment(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs,
		FVector InnerBL, FVector InnerBR, FVector InnerTR, FVector InnerTL,
		FVector OuterBL, FVector OuterBR, FVector OuterTR, FVector OuterTL,
		float SegmentWidth, float SegmentHeight, float WallThickness);
};