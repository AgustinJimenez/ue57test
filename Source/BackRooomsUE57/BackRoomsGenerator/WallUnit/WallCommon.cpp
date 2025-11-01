#include "WallCommon.h"
#include "../Main.h"  // For log category

// Optimized helper function for quad face generation
void UWallCommon::AddQuadFace(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs, const FFaceData& Face)
{
	if (Face.Vertices.Num() != 4 || Face.UVs.Num() != 4)
	{
		return; // Invalid face data
	}
	
	int32 BaseIndex = Vertices.Num();
	
	// Add vertices
	Vertices.Append(Face.Vertices);
	
	// Add normals (same for all 4 vertices)
	for (int32 i = 0; i < 4; i++)
	{
		Normals.Add(Face.Normal);
	}
	
	// Add UVs
	UVs.Append(Face.UVs);
	
	// Add triangles (2 triangles per quad)
	if (Face.bReverseWinding)
	{
		// Reversed winding for outer faces
		Triangles.Add(BaseIndex + 0);
		Triangles.Add(BaseIndex + 2);
		Triangles.Add(BaseIndex + 1);
		Triangles.Add(BaseIndex + 0);
		Triangles.Add(BaseIndex + 3);
		Triangles.Add(BaseIndex + 2);
	}
	else
	{
		// Normal winding for inner faces
		Triangles.Add(BaseIndex + 0);
		Triangles.Add(BaseIndex + 1);
		Triangles.Add(BaseIndex + 2);
		Triangles.Add(BaseIndex + 0);
		Triangles.Add(BaseIndex + 2);
		Triangles.Add(BaseIndex + 3);
	}
}

void UWallCommon::AddDoorFrame(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs,
	FVector InnerV1, FVector OuterV1, FVector OuterV2, FVector InnerV2, float FrameThickness, float FrameSize)
{
	int32 FrameIndex = Vertices.Num();
	Vertices.Add(InnerV1);
	Vertices.Add(OuterV1);
	Vertices.Add(OuterV2);
	Vertices.Add(InnerV2);
	
	FVector FrameNormal = FVector::CrossProduct((OuterV1 - InnerV1).GetSafeNormal(), (InnerV2 - InnerV1).GetSafeNormal());
	for (int32 i = 0; i < 4; i++)
	{
		Normals.Add(FrameNormal);
	}
	
	UVs.Add(FVector2D(0, 0));
	UVs.Add(FVector2D(FrameThickness, 0));
	UVs.Add(FVector2D(FrameThickness, FrameSize));
	UVs.Add(FVector2D(0, FrameSize));
	
	Triangles.Add(FrameIndex + 0);
	Triangles.Add(FrameIndex + 1);
	Triangles.Add(FrameIndex + 2);
	Triangles.Add(FrameIndex + 0);
	Triangles.Add(FrameIndex + 2);
	Triangles.Add(FrameIndex + 3);
}

void UWallCommon::GenerateThickWallSegment(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs,
	FVector InnerBL, FVector InnerBR, FVector InnerTR, FVector InnerTL,
	FVector OuterBL, FVector OuterBR, FVector OuterTR, FVector OuterTL,
	float SegmentWidth, float SegmentHeight, float WallThickness)
{
	// Calculate normals
	FVector EdgeA = InnerBR - InnerBL;
	FVector EdgeB = InnerTL - InnerBL;
	FVector InnerNormal = FVector::CrossProduct(EdgeA, EdgeB).GetSafeNormal();
	FVector OuterNormal = -InnerNormal;
	
	float ThicknessUV = WallThickness;
	
	// Create all faces using shared utilities
	TArray<FFaceData> AllFaces;
	AllFaces.Reserve(6); // Inner, Outer, Bottom, Top, Left, Right
	
	// === INNER FACE (visible from inside room) ===
	FFaceData InnerFace;
	InnerFace.Vertices = {InnerBL, InnerBR, InnerTR, InnerTL};
	InnerFace.Normal = InnerNormal;
	InnerFace.UVs = {FVector2D(0, 0), FVector2D(SegmentWidth, 0), FVector2D(SegmentWidth, SegmentHeight), FVector2D(0, SegmentHeight)};
	InnerFace.bReverseWinding = false;
	AllFaces.Add(InnerFace);
	
	// === OUTER FACE (visible from outside room) ===
	FFaceData OuterFace;
	OuterFace.Vertices = {OuterBL, OuterBR, OuterTR, OuterTL};
	OuterFace.Normal = OuterNormal;
	OuterFace.UVs = {FVector2D(0, 0), FVector2D(SegmentWidth, 0), FVector2D(SegmentWidth, SegmentHeight), FVector2D(0, SegmentHeight)};
	OuterFace.bReverseWinding = true; // Reversed winding for outer face
	AllFaces.Add(OuterFace);
	
	// === BOTTOM SIDE FACE ===
	FFaceData BottomFace;
	BottomFace.Vertices = {InnerBL, OuterBL, OuterBR, InnerBR};
	BottomFace.Normal = FVector(0, 0, -1);
	BottomFace.UVs = {FVector2D(0, 0), FVector2D(ThicknessUV, 0), FVector2D(ThicknessUV, SegmentWidth), FVector2D(0, SegmentWidth)};
	BottomFace.bReverseWinding = false;
	AllFaces.Add(BottomFace);
	
	// === TOP SIDE FACE ===
	FFaceData TopFace;
	TopFace.Vertices = {InnerTL, InnerTR, OuterTR, OuterTL};
	TopFace.Normal = FVector(0, 0, 1);
	TopFace.UVs = {FVector2D(0, 0), FVector2D(SegmentWidth, 0), FVector2D(SegmentWidth, ThicknessUV), FVector2D(0, ThicknessUV)};
	TopFace.bReverseWinding = false;
	AllFaces.Add(TopFace);
	
	// === LEFT SIDE FACE ===
	FFaceData LeftFace;
	LeftFace.Vertices = {InnerTL, OuterTL, OuterBL, InnerBL};
	LeftFace.Normal = (OuterBL - InnerBL).GetSafeNormal();
	LeftFace.UVs = {FVector2D(0, SegmentHeight), FVector2D(ThicknessUV, SegmentHeight), FVector2D(ThicknessUV, 0), FVector2D(0, 0)};
	LeftFace.bReverseWinding = false;
	AllFaces.Add(LeftFace);
	
	// === RIGHT SIDE FACE ===
	FFaceData RightFace;
	RightFace.Vertices = {InnerBR, OuterBR, OuterTR, InnerTR};
	RightFace.Normal = (OuterBR - InnerBR).GetSafeNormal();
	RightFace.UVs = {FVector2D(0, 0), FVector2D(ThicknessUV, 0), FVector2D(ThicknessUV, SegmentHeight), FVector2D(0, SegmentHeight)};
	RightFace.bReverseWinding = true; // Special winding for right face
	AllFaces.Add(RightFace);
	
	// Generate all faces using shared utility
	for (const FFaceData& Face : AllFaces)
	{
		AddQuadFace(Vertices, Triangles, Normals, UVs, Face);
	}
}