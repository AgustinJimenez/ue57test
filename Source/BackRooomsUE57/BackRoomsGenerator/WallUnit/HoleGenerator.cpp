#include "HoleGenerator.h"
#include "../Main.h"  // For log category

void UHoleGenerator::GenerateWallWithHole(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs,
	FVector InnerBL, FVector InnerBR, FVector InnerTR, FVector InnerTL,
	FVector OuterBL, FVector OuterBR, FVector OuterTR, FVector OuterTL,
	float WallWidth, float WallHeight, const FDoorConfig& Door, float WallThickness)
{
	// Generate irregular polygon using random parameters
	float BaseSizeCm = MetersToUnrealUnits(Door.IrregularSize);
	float OffsetFromCenterCm = MetersToUnrealUnits(Door.OffsetFromCenter);
	float WallWidthCm = MetersToUnrealUnits(WallWidth);
	float WallHeightCm = MetersToUnrealUnits(WallHeight);
	
	// Calculate hole center position
	float WallCenterOffsetCm = (WallWidthCm * 0.5f) + OffsetFromCenterCm;
	float HoleHeightCm = WallHeightCm * 0.5f; // Center vertically
	
	// Bounds checking - allow very large holes for testing
	float MaxAllowableSize = FMath::Min(WallWidthCm, WallHeightCm) * 0.95f; // Max 95% of smallest wall dimension
	if (BaseSizeCm > MaxAllowableSize)
	{
		UE_LOG(LogBackRoomGenerator, Warning, TEXT("Irregular hole size %.0fcm too large for wall %.0fx%.0fcm, clamping to %.0fcm"), 
			BaseSizeCm, WallWidthCm, WallHeightCm, MaxAllowableSize);
		BaseSizeCm = MaxAllowableSize;
	}
	
	// Generate irregular polygon points
	TArray<FVector2D> IrregularPoints = GenerateIrregularPolygon(Door, BaseSizeCm);
	
	UE_LOG(LogBackRoomGenerator, Log, TEXT("Generated irregular hole with %d points, size=%.0fcm, irregularity=%.2f, seed=%d"), 
		Door.IrregularPoints, BaseSizeCm, Door.Irregularity, Door.RandomSeed);
	
	// OPTIMIZED: Use hole-based sizing with reasonable minimums for performance
	// Segment size based on hole detail needs, not wall size
	float SegmentSize;
	if (Door.IrregularSmoothness >= 0.8f)
	{
		// Smooth holes: Smaller segments for detail, but not crazy small
		SegmentSize = FMath::Max(BaseSizeCm * 0.1f, 15.0f); // Min 15cm for smooth holes
	}
	else
	{
		// Jagged holes: Larger segments are fine for rough edges
		SegmentSize = FMath::Max(BaseSizeCm * 0.2f, 25.0f); // Min 25cm for jagged holes
	}
	
	// Cap maximum segment size to prevent too few segments
	SegmentSize = FMath::Min(SegmentSize, BaseSizeCm * 0.4f); // Max 40% of hole size
	
	int32 GridXSize = FMath::CeilToInt(WallWidthCm / SegmentSize);
	int32 GridZSize = FMath::CeilToInt(WallHeightCm / SegmentSize);
	
	// Performance monitoring
	int32 TotalGridCells = GridXSize * GridZSize;
	UE_LOG(LogBackRoomGenerator, Log, TEXT("OPTIMIZED irregular hole: %.0fcm segments, %dx%d grid (%d cells) for %.0fx%.0fcm wall"), 
		SegmentSize, GridXSize, GridZSize, TotalGridCells, WallWidthCm, WallHeightCm);
	
	float GridStartX = 0.0f;
	float GridStartZ = 0.0f;
	
	int32 GeneratedSegments = 0;
	
	// Generate wall segments, skipping those that overlap the irregular polygon
	for (int32 GridX = 0; GridX < GridXSize; GridX++)
	{
		for (int32 GridZ = 0; GridZ < GridZSize; GridZ++)
		{
			float SegStartX = GridStartX + (GridX * SegmentSize);
			float SegEndX = SegStartX + SegmentSize;
			float SegStartZ = GridStartZ + (GridZ * SegmentSize);
			float SegEndZ = SegStartZ + SegmentSize;
			
			// Check if this segment overlaps with the irregular polygon
			bool bSegmentOverlapsHole = false;
			
			TArray<FVector2D> TestPoints = {
				FVector2D(SegStartX, SegStartZ),  // Bottom-left
				FVector2D(SegEndX, SegStartZ),    // Bottom-right  
				FVector2D(SegEndX, SegEndZ),      // Top-right
				FVector2D(SegStartX, SegEndZ),    // Top-left
				FVector2D((SegStartX + SegEndX) * 0.5f, (SegStartZ + SegEndZ) * 0.5f) // Center
			};
			
			for (const FVector2D& TestPoint : TestPoints)
			{
				// Convert test point to local coordinates relative to hole center
				FVector2D LocalPoint(
					TestPoint.X - WallCenterOffsetCm,
					TestPoint.Y - HoleHeightCm
				);
				
				// Check if point is inside irregular polygon
				if (IsPointInIrregularPolygon(LocalPoint, IrregularPoints))
				{
					bSegmentOverlapsHole = true;
					break;
				}
			}
			
			if (bSegmentOverlapsHole)
			{
				continue; // Skip this segment - it overlaps the hole
			}
			
			// Clamp segment to wall bounds
			SegStartX = FMath::Max(0.0f, SegStartX);
			SegEndX = FMath::Min(WallWidthCm, SegEndX);
			SegStartZ = FMath::Max(0.0f, SegStartZ);
			SegEndZ = FMath::Min(WallHeightCm, SegEndZ);
			
			// Skip if segment becomes too small
			if ((SegEndX - SegStartX) < 1.0f || (SegEndZ - SegStartZ) < 1.0f)
			{
				continue;
			}
			
			// Generate this wall segment
			FVector WallWidthDirection = (InnerBR - InnerBL).GetSafeNormal();
			FVector WallHeightDirection = (InnerTL - InnerBL).GetSafeNormal();
			
			FVector SegInnerBL = InnerBL + (WallWidthDirection * SegStartX) + (WallHeightDirection * SegStartZ);
			FVector SegInnerBR = InnerBL + (WallWidthDirection * SegEndX) + (WallHeightDirection * SegStartZ);
			FVector SegInnerTR = InnerBL + (WallWidthDirection * SegEndX) + (WallHeightDirection * SegEndZ);
			FVector SegInnerTL = InnerBL + (WallWidthDirection * SegStartX) + (WallHeightDirection * SegEndZ);
			
			FVector OuterWidthDirection = (OuterBR - OuterBL).GetSafeNormal();
			FVector OuterHeightDirection = (OuterTL - OuterBL).GetSafeNormal();
			
			FVector SegOuterBL = OuterBL + (OuterWidthDirection * SegStartX) + (OuterHeightDirection * SegStartZ);
			FVector SegOuterBR = OuterBL + (OuterWidthDirection * SegEndX) + (OuterHeightDirection * SegStartZ);
			FVector SegOuterTR = OuterBL + (OuterWidthDirection * SegEndX) + (OuterHeightDirection * SegEndZ);
			FVector SegOuterTL = OuterBL + (OuterWidthDirection * SegStartX) + (OuterHeightDirection * SegEndZ);
			
			// Generate wall segment using shared utilities
			UWallCommon::FFaceData InnerFace, OuterFace, BottomFace, TopFace, LeftFace, RightFace;
			
			// Inner face
			InnerFace.Vertices = {SegInnerBL, SegInnerBR, SegInnerTR, SegInnerTL};
			InnerFace.Normal = FVector::CrossProduct((SegInnerBR - SegInnerBL).GetSafeNormal(), (SegInnerTL - SegInnerBL).GetSafeNormal());
			InnerFace.UVs = {FVector2D(0, 0), FVector2D(SegmentSize/100.0f, 0), FVector2D(SegmentSize/100.0f, SegmentSize/100.0f), FVector2D(0, SegmentSize/100.0f)};
			InnerFace.bReverseWinding = false;
			UWallCommon::AddQuadFace(Vertices, Triangles, Normals, UVs, InnerFace);
			
			// Outer face
			OuterFace.Vertices = {SegOuterBL, SegOuterBR, SegOuterTR, SegOuterTL};
			OuterFace.Normal = -InnerFace.Normal;
			OuterFace.UVs = {FVector2D(0, 0), FVector2D(SegmentSize/100.0f, 0), FVector2D(SegmentSize/100.0f, SegmentSize/100.0f), FVector2D(0, SegmentSize/100.0f)};
			OuterFace.bReverseWinding = true;
			UWallCommon::AddQuadFace(Vertices, Triangles, Normals, UVs, OuterFace);
			
			// Add side faces
			float ThicknessUV = WallThickness;
			
			// Bottom face
			BottomFace.Vertices = {SegInnerBL, SegOuterBL, SegOuterBR, SegInnerBR};
			BottomFace.Normal = FVector(0, 0, -1);
			BottomFace.UVs = {FVector2D(0, 0), FVector2D(ThicknessUV, 0), FVector2D(ThicknessUV, SegmentSize/100.0f), FVector2D(0, SegmentSize/100.0f)};
			BottomFace.bReverseWinding = false;
			UWallCommon::AddQuadFace(Vertices, Triangles, Normals, UVs, BottomFace);
			
			// Top face
			TopFace.Vertices = {SegInnerTL, SegInnerTR, SegOuterTR, SegOuterTL};
			TopFace.Normal = FVector(0, 0, 1);
			TopFace.UVs = {FVector2D(0, 0), FVector2D(SegmentSize/100.0f, 0), FVector2D(SegmentSize/100.0f, ThicknessUV), FVector2D(0, ThicknessUV)};
			TopFace.bReverseWinding = false;
			UWallCommon::AddQuadFace(Vertices, Triangles, Normals, UVs, TopFace);
			
			// Left face
			LeftFace.Vertices = {SegInnerTL, SegOuterTL, SegOuterBL, SegInnerBL};
			LeftFace.Normal = (SegOuterBL - SegInnerBL).GetSafeNormal();
			LeftFace.UVs = {FVector2D(0, SegmentSize/100.0f), FVector2D(ThicknessUV, SegmentSize/100.0f), FVector2D(ThicknessUV, 0), FVector2D(0, 0)};
			LeftFace.bReverseWinding = false;
			UWallCommon::AddQuadFace(Vertices, Triangles, Normals, UVs, LeftFace);
			
			// Right face
			RightFace.Vertices = {SegInnerBR, SegOuterBR, SegOuterTR, SegInnerTR};
			RightFace.Normal = (SegOuterBR - SegInnerBR).GetSafeNormal();
			RightFace.UVs = {FVector2D(0, 0), FVector2D(ThicknessUV, 0), FVector2D(ThicknessUV, SegmentSize/100.0f), FVector2D(0, SegmentSize/100.0f)};
			RightFace.bReverseWinding = true;
			UWallCommon::AddQuadFace(Vertices, Triangles, Normals, UVs, RightFace);
			
			GeneratedSegments++;
		}
	}
	
	// Check if we generated enough segments
	int32 TotalPossibleSegments = GridXSize * GridZSize;
	float WallCoveragePercent = (float)GeneratedSegments / (float)TotalPossibleSegments * 100.0f;
	
	if (GeneratedSegments < 8) // Lower threshold for irregular holes
	{
		UE_LOG(LogBackRoomGenerator, Error, TEXT("Irregular hole too large! Only %d segments generated (%.1f%% coverage). Falling back to solid wall."), 
			GeneratedSegments, WallCoveragePercent);
		
		// Generate fallback solid wall
		UWallCommon::FFaceData InnerFace, OuterFace;
		
		InnerFace.Vertices = {InnerBL, InnerBR, InnerTR, InnerTL};
		InnerFace.Normal = FVector::CrossProduct((InnerBR - InnerBL).GetSafeNormal(), (InnerTL - InnerBL).GetSafeNormal());
		InnerFace.UVs = {FVector2D(0, 0), FVector2D(WallWidth, 0), FVector2D(WallWidth, WallHeight), FVector2D(0, WallHeight)};
		InnerFace.bReverseWinding = false;
		UWallCommon::AddQuadFace(Vertices, Triangles, Normals, UVs, InnerFace);
		
		OuterFace.Vertices = {OuterBL, OuterBR, OuterTR, OuterTL};
		OuterFace.Normal = -InnerFace.Normal;
		OuterFace.UVs = {FVector2D(0, 0), FVector2D(WallWidth, 0), FVector2D(WallWidth, WallHeight), FVector2D(0, WallHeight)};
		OuterFace.bReverseWinding = true;
		UWallCommon::AddQuadFace(Vertices, Triangles, Normals, UVs, OuterFace);
		
		return;
	}
	
	// Add edge faces for hole borders - only for segments that have exposed edges
	FVector WallWidthDirection = (InnerBR - InnerBL).GetSafeNormal();
	FVector WallHeightDirection = (InnerTL - InnerBL).GetSafeNormal();
	
	for (int32 GridX = 0; GridX < GridXSize; GridX++)
	{
		for (int32 GridZ = 0; GridZ < GridZSize; GridZ++)
		{
			float SegStartX = GridStartX + (GridX * SegmentSize);
			float SegEndX = SegStartX + SegmentSize;
			float SegStartZ = GridStartZ + (GridZ * SegmentSize);
			float SegEndZ = SegStartZ + SegmentSize;
			
			// Check if this segment exists (not part of hole)
			bool bSegmentExists = true;
			TArray<FVector2D> TestPoints = {
				FVector2D(SegStartX, SegStartZ),  
				FVector2D(SegEndX, SegStartZ),    
				FVector2D(SegEndX, SegEndZ),      
				FVector2D(SegStartX, SegEndZ),    
				FVector2D((SegStartX + SegEndX) * 0.5f, (SegStartZ + SegEndZ) * 0.5f)
			};
			
			for (const FVector2D& TestPoint : TestPoints)
			{
				FVector2D LocalPoint(TestPoint.X - WallCenterOffsetCm, TestPoint.Y - HoleHeightCm);
				if (IsPointInIrregularPolygon(LocalPoint, IrregularPoints))
				{
					bSegmentExists = false;
					break;
				}
			}
			
			if (!bSegmentExists)
			{
				continue; // Skip hole segments
			}
			
			// Check if segment is within wall bounds
			if (SegStartX < 0 || SegEndX > WallWidthCm || SegStartZ < 0 || SegEndZ > WallHeightCm)
			{
				continue;
			}
			
			// Helper function to check if a neighboring segment is missing
			auto IsNeighborMissing = [&](int32 NeighborGridX, int32 NeighborGridZ) -> bool {
				// Outside grid bounds
				if (NeighborGridX < 0 || NeighborGridX >= GridXSize || NeighborGridZ < 0 || NeighborGridZ >= GridZSize)
				{
					return true;
				}
				
				// Calculate neighbor segment
				float NeighborStartX = GridStartX + (NeighborGridX * SegmentSize);
				float NeighborEndX = NeighborStartX + SegmentSize;
				float NeighborStartZ = GridStartZ + (NeighborGridZ * SegmentSize);
				float NeighborEndZ = NeighborStartZ + SegmentSize;
				
				// Outside wall bounds
				if (NeighborStartX < 0 || NeighborEndX > WallWidthCm || NeighborStartZ < 0 || NeighborEndZ > WallHeightCm)
				{
					return true;
				}
				
				// Check if neighbor overlaps with polygon
				TArray<FVector2D> NeighborTestPoints = {
					FVector2D(NeighborStartX, NeighborStartZ),  
					FVector2D(NeighborEndX, NeighborStartZ),    
					FVector2D(NeighborEndX, NeighborEndZ),      
					FVector2D(NeighborStartX, NeighborEndZ),    
					FVector2D((NeighborStartX + NeighborEndX) * 0.5f, (NeighborStartZ + NeighborEndZ) * 0.5f)
				};
				
				for (const FVector2D& TestPoint : NeighborTestPoints)
				{
					FVector2D LocalPoint(TestPoint.X - WallCenterOffsetCm, TestPoint.Y - HoleHeightCm);
					if (IsPointInIrregularPolygon(LocalPoint, IrregularPoints))
					{
						return true; // Neighbor is part of hole
					}
				}
				
				return false; // Neighbor exists
			};
			
			// Clamp segment to wall bounds
			SegStartX = FMath::Max(0.0f, SegStartX);
			SegEndX = FMath::Min(WallWidthCm, SegEndX);
			SegStartZ = FMath::Max(0.0f, SegStartZ);
			SegEndZ = FMath::Min(WallHeightCm, SegEndZ);
			
			if ((SegEndX - SegStartX) < 1.0f || (SegEndZ - SegStartZ) < 1.0f)
			{
				continue;
			}
			
			// Calculate segment corners
			FVector SegInnerBL = InnerBL + (WallWidthDirection * SegStartX) + (WallHeightDirection * SegStartZ);
			FVector SegInnerBR = InnerBL + (WallWidthDirection * SegEndX) + (WallHeightDirection * SegStartZ);
			FVector SegInnerTR = InnerBL + (WallWidthDirection * SegEndX) + (WallHeightDirection * SegEndZ);
			FVector SegInnerTL = InnerBL + (WallWidthDirection * SegStartX) + (WallHeightDirection * SegEndZ);
			
			FVector OuterWidthDirection = (OuterBR - OuterBL).GetSafeNormal();
			FVector OuterHeightDirection = (OuterTL - OuterBL).GetSafeNormal();
			
			FVector SegOuterBL = OuterBL + (OuterWidthDirection * SegStartX) + (OuterHeightDirection * SegStartZ);
			FVector SegOuterBR = OuterBL + (OuterWidthDirection * SegEndX) + (OuterHeightDirection * SegStartZ);
			FVector SegOuterTR = OuterBL + (OuterWidthDirection * SegEndX) + (OuterHeightDirection * SegEndZ);
			FVector SegOuterTL = OuterBL + (OuterWidthDirection * SegStartX) + (OuterHeightDirection * SegEndZ);
			
			// Add edge faces only for edges that border missing segments
			if (IsNeighborMissing(GridX - 1, GridZ)) // Left edge
			{
				UWallCommon::AddDoorFrame(Vertices, Triangles, Normals, UVs, 
					SegInnerBL, SegOuterBL, SegOuterTL, SegInnerTL, WallThickness, (SegEndZ - SegStartZ) / 100.0f);
			}
			
			if (IsNeighborMissing(GridX + 1, GridZ)) // Right edge
			{
				UWallCommon::AddDoorFrame(Vertices, Triangles, Normals, UVs, 
					SegInnerBR, SegOuterBR, SegOuterTR, SegInnerTR, WallThickness, (SegEndZ - SegStartZ) / 100.0f);
			}
			
			if (IsNeighborMissing(GridX, GridZ - 1)) // Bottom edge
			{
				UWallCommon::AddDoorFrame(Vertices, Triangles, Normals, UVs, 
					SegInnerBL, SegOuterBL, SegOuterBR, SegInnerBR, WallThickness, (SegEndX - SegStartX) / 100.0f);
			}
			
			if (IsNeighborMissing(GridX, GridZ + 1)) // Top edge
			{
				UWallCommon::AddDoorFrame(Vertices, Triangles, Normals, UVs, 
					SegInnerTL, SegOuterTL, SegOuterTR, SegInnerTR, WallThickness, (SegEndX - SegStartX) / 100.0f);
			}
		}
	}
	
	UE_LOG(LogBackRoomGenerator, Log, TEXT("OPTIMIZED irregular hole complete: %d segments (%.1f%% coverage) from %d grid cells, %d polygon points, seed %d"), 
		GeneratedSegments, WallCoveragePercent, TotalGridCells, Door.IrregularPoints, Door.RandomSeed);
}

bool UHoleGenerator::IsPointInIrregularPolygon(const FVector2D& Point, const TArray<FVector2D>& PolygonPoints)
{
	// Ray casting algorithm for point-in-polygon detection
	bool bInsidePolygon = false;
	int32 j = PolygonPoints.Num() - 1;
	
	for (int32 i = 0; i < PolygonPoints.Num(); i++)
	{
		if (((PolygonPoints[i].Y > Point.Y) != (PolygonPoints[j].Y > Point.Y)) &&
			(Point.X < (PolygonPoints[j].X - PolygonPoints[i].X) * (Point.Y - PolygonPoints[i].Y) / (PolygonPoints[j].Y - PolygonPoints[i].Y) + PolygonPoints[i].X))
		{
			bInsidePolygon = !bInsidePolygon;
		}
		j = i;
	}
	
	return bInsidePolygon;
}

TArray<FVector2D> UHoleGenerator::GenerateIrregularPolygon(const FDoorConfig& Door, float BaseSizeCm)
{
	// Set up deterministic random generator using the provided seed
	FRandomStream RandomStream(Door.RandomSeed);
	
	// Generate random points for irregular polygon
	TArray<FVector2D> BasePoints;
	float AngleStep = 2.0f * PI / Door.IrregularPoints;
	
	for (int32 i = 0; i < Door.IrregularPoints; i++)
	{
		float Angle = i * AngleStep;
		
		// Base circle position
		float BaseX = FMath::Cos(Angle);
		float BaseZ = FMath::Sin(Angle);
		
		// Add random variation based on irregularity factor
		float RandomRadius = RandomStream.FRandRange(
			BaseSizeCm * (1.0f - Door.Irregularity), 
			BaseSizeCm * (1.0f + Door.Irregularity)
		);
		
		// Random angle offset for more chaos
		float AngleOffset = RandomStream.FRandRange(-Door.Irregularity * 0.5f, Door.Irregularity * 0.5f);
		float FinalAngle = Angle + AngleOffset;
		
		FVector2D Point(
			FMath::Cos(FinalAngle) * RandomRadius,
			FMath::Sin(FinalAngle) * RandomRadius
		);
		
		BasePoints.Add(Point);
	}
	
	// Apply smoothness by adding interpolated points between base points
	TArray<FVector2D> IrregularPoints;
	
	if (Door.IrregularSmoothness <= 0.0f)
	{
		// No smoothness - return original points
		return BasePoints;
	}
	
	// Calculate number of interpolation points based on smoothness
	int32 InterpPointsPerSegment = FMath::RoundToInt(Door.IrregularSmoothness * 4.0f); // 0-4 points between each base point
	
	for (int32 i = 0; i < BasePoints.Num(); i++)
	{
		// Add the current base point
		IrregularPoints.Add(BasePoints[i]);
		
		// Add interpolated points to next base point (for smoothness)
		if (InterpPointsPerSegment > 0)
		{
			int32 NextIndex = (i + 1) % BasePoints.Num(); // Wrap around to first point
			FVector2D CurrentPoint = BasePoints[i];
			FVector2D NextPoint = BasePoints[NextIndex];
			
			// Add interpolated points between current and next
			for (int32 j = 1; j <= InterpPointsPerSegment; j++)
			{
				float Alpha = (float)j / (float)(InterpPointsPerSegment + 1);
				FVector2D InterpPoint = FMath::Lerp(CurrentPoint, NextPoint, Alpha);
				IrregularPoints.Add(InterpPoint);
			}
		}
	}
	
	// Apply rotation if specified
	if (Door.IrregularRotation != 0.0f)
	{
		float RotationRadians = FMath::DegreesToRadians(Door.IrregularRotation);
		float CosRot = FMath::Cos(RotationRadians);
		float SinRot = FMath::Sin(RotationRadians);
		
		for (FVector2D& Point : IrregularPoints)
		{
			// Rotate point around origin
			float NewX = Point.X * CosRot - Point.Y * SinRot;
			float NewY = Point.X * SinRot + Point.Y * CosRot;
			Point.X = NewX;
			Point.Y = NewY;
		}
	}
	
	return IrregularPoints;
}