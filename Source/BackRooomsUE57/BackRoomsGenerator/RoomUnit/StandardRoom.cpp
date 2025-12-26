#include "StandardRoom.h"
#include "Engine/Engine.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "BillboardTextActor.h"
#include "../WallUnit/WallUnit.h"

UStandardRoom::UStandardRoom() : Super()
{
	// StandardRoom-specific defaults (inherit base properties from UBaseRoom)
	Length = 10.0f; // Override default length (base is 5.0f, StandardRoom uses 10.0f)
}





void UStandardRoom::GenerateMesh()
{
	if (!MeshComponent)
	{
		// UE_LOG(LogTemp, Error, TEXT("StandardRoom: Cannot generate mesh - no mesh component"));
		return;
	}

	// Use individual wall sections approach (like the working TestGenerator system)
	// UE_LOG(LogTemp, Warning, TEXT("StandardRoom: Generating room using individual wall sections"));
	
	// Generate individual wall sections  
	TArray<TArray<FVector>> WallVertices;
	TArray<TArray<int32>> WallTriangles;
	TArray<TArray<FVector>> WallNormals;
	TArray<TArray<FVector2D>> WallUVs;
	
	GenerateIndividualWalls(WallVertices, WallTriangles, WallNormals, WallUVs);
	
	// Create mesh sections for each wall
	for (int32 SectionIndex = 0; SectionIndex < WallVertices.Num(); SectionIndex++)
	{
		if (WallVertices[SectionIndex].Num() > 0)
		{
			// UE5.6 CreateMeshSection signature: (SectionIndex, Vertices, Triangles, Normals, UV0, VertexColors, Tangents, bCreateCollision)
			TArray<FColor> VertexColors;
			VertexColors.SetNum(WallVertices[SectionIndex].Num());
			for (int32 i = 0; i < VertexColors.Num(); i++)
			{
				VertexColors[i] = FColor::White; // Default white color
			}
			
			MeshComponent->CreateMeshSection(SectionIndex, WallVertices[SectionIndex], WallTriangles[SectionIndex], 
				WallNormals[SectionIndex], WallUVs[SectionIndex], VertexColors, TArray<FProcMeshTangent>(), true);
			
			// UE_LOG(LogTemp, Log, TEXT("StandardRoom: Created mesh section %d with %d vertices, %d triangles"), 
				// SectionIndex, WallVertices[SectionIndex].Num(), WallTriangles[SectionIndex].Num() / 3);
		}
	}
	
	// UE_LOG(LogTemp, Log, TEXT("StandardRoom: Created room with %d mesh sections"), WallVertices.Num());
}

void UStandardRoom::GenerateWallGeometry(TArray<FVector>& CombinedVertices, TArray<int32>& CombinedTriangles,
	TArray<FVector>& CombinedNormals, TArray<FVector2D>& CombinedUVs, TArray<FLinearColor>& CombinedColors)
{
	// Convert room dimensions to Unreal units (cm)
	float WidthCm = Width * 100.0f;
	float LengthCm = Length * 100.0f;
	float HeightCm = Height * 100.0f;
	float HalfWidth = WidthCm * 0.5f;
	float HalfLength = LengthCm * 0.5f;
	
	// Wall colors (consistent with TestGenerator)
	FLinearColor SouthWallColor = FLinearColor::Green;   // South wall = Green
	FLinearColor NorthWallColor = FLinearColor::Red;     // North wall = Red  
	FLinearColor EastWallColor = FLinearColor::Blue;     // East wall = Blue
	FLinearColor WestWallColor = FLinearColor::Yellow;   // West wall = Yellow
	FLinearColor FloorColor = FLinearColor::White * 0.6f + FLinearColor(0.3f, 0.3f, 0.3f, 1.0f);
	FLinearColor CeilingColor = FLinearColor::White * 0.9f;
	
	// Create door configurations for walls that need them
	FWallHoleConfig* DoorConfig = nullptr;
	for (const FDoorConfig& Config : DoorConfigs)
	{
		if (Config.bHasDoor && Config.Width > 0.1f)
		{
			DoorConfig = new FWallHoleConfig(Config.Width, Config.Height, TEXT("Door"));
			DoorConfig->Shape = EHoleShape::Rectangle;
			break; // Use first valid door config for now
		}
	}
	
	// Generate South Wall (Y = -HalfLength)
	FVector SouthWallPos = FVector(0, -HalfLength, 0);
	AddWallToMesh(SouthWallPos, FRotator(0, 0, 0), Width, Height, SouthWallColor, DoorConfig,
		CombinedVertices, CombinedTriangles, CombinedNormals, CombinedUVs, CombinedColors);
	
	// Generate North Wall (Y = +HalfLength)
	FVector NorthWallPos = FVector(0, HalfLength, 0);
	AddWallToMesh(NorthWallPos, FRotator(0, 180, 0), Width, Height, NorthWallColor, DoorConfig,
		CombinedVertices, CombinedTriangles, CombinedNormals, CombinedUVs, CombinedColors);
	
	// Generate East Wall (X = +HalfWidth)
	FVector EastWallPos = FVector(HalfWidth, 0, 0);
	AddWallToMesh(EastWallPos, FRotator(0, 90, 0), Length, Height, EastWallColor, DoorConfig,
		CombinedVertices, CombinedTriangles, CombinedNormals, CombinedUVs, CombinedColors);
	
	// Generate West Wall (X = -HalfWidth)
	FVector WestWallPos = FVector(-HalfWidth, 0, 0);
	AddWallToMesh(WestWallPos, FRotator(0, 270, 0), Length, Height, WestWallColor, DoorConfig,
		CombinedVertices, CombinedTriangles, CombinedNormals, CombinedUVs, CombinedColors);
	
	// Generate Floor (Z = -Height/2)
	FVector FloorPos = FVector(0, 0, -(HeightCm * 0.5f + WallThickness * 100.0f * 0.5f + 2.0f));
	AddWallToMesh(FloorPos, FRotator(0, 0, 90), Width, Length, FloorColor, nullptr,
		CombinedVertices, CombinedTriangles, CombinedNormals, CombinedUVs, CombinedColors);
	
	// Generate Ceiling (Z = +Height/2)
	FVector CeilingPos = FVector(0, 0, HeightCm * 0.5f + (WallThickness * 100.0f * 0.5f + 2.0f));
	AddWallToMesh(CeilingPos, FRotator(0, 0, 270), Width, Length, CeilingColor, nullptr,
		CombinedVertices, CombinedTriangles, CombinedNormals, CombinedUVs, CombinedColors);
	
	// Clean up
	if (DoorConfig)
	{
		delete DoorConfig;
	}
	
	// UE_LOG(LogTemp, Log, TEXT("StandardRoom: Generated room geometry with %d vertices total"), 
		// CombinedVertices.Num());
}

void UStandardRoom::AddWallToMesh(const FVector& WallPosition, const FRotator& WallRotation, float WallWidth, float WallHeight,
	const FLinearColor& WallColor, const FWallHoleConfig* HoleConfig,
	TArray<FVector>& CombinedVertices, TArray<int32>& CombinedTriangles,
	TArray<FVector>& CombinedNormals, TArray<FVector2D>& CombinedUVs, TArray<FLinearColor>& CombinedColors)
{
	// Get wall geometry from WallUnit
	TArray<FVector> WallVertices;
	TArray<int32> WallTriangles;
	TArray<FVector> WallNormals;
	TArray<FVector2D> WallUVs;
	
	// Use WallUnit's static geometry generation methods
	// For now, use a simpler approach - generate basic wall geometry
	// This is a simplified implementation that creates basic rectangular walls
	
	// Calculate wall corners
	float HalfWidth = WallWidth * 100.0f * 0.5f; // Convert to cm and get half
	float HalfThickness = WallThickness * 100.0f * 0.5f;
	float HeightCm = WallHeight * 100.0f;
	
	// Generate vertices for a simple rectangular wall
	WallVertices.Empty();
	WallTriangles.Empty();
	WallNormals.Empty();
	WallUVs.Empty();
	
	// Create a simple quad wall (front face)
	WallVertices.Add(FVector(-HalfWidth, -HalfThickness, 0));           // Bottom left
	WallVertices.Add(FVector(HalfWidth, -HalfThickness, 0));            // Bottom right  
	WallVertices.Add(FVector(HalfWidth, -HalfThickness, HeightCm));     // Top right
	WallVertices.Add(FVector(-HalfWidth, -HalfThickness, HeightCm));    // Top left
	
	// Create triangles (two triangles for the quad)
	WallTriangles.Add(0); WallTriangles.Add(1); WallTriangles.Add(2);  // First triangle
	WallTriangles.Add(0); WallTriangles.Add(2); WallTriangles.Add(3);  // Second triangle
	
	// Generate normals (pointing forward for front face)
	for (int32 i = 0; i < 4; i++)
	{
		WallNormals.Add(FVector(0, -1, 0)); // Normal pointing toward negative Y
	}
	
	// Generate UVs
	WallUVs.Add(FVector2D(0, 0)); // Bottom left
	WallUVs.Add(FVector2D(1, 0)); // Bottom right
	WallUVs.Add(FVector2D(1, 1)); // Top right  
	WallUVs.Add(FVector2D(0, 1)); // Top left
	
	// Transform wall geometry to room position and rotation
	FTransform WallTransform(WallRotation, Position + WallPosition);
	
	int32 VertexOffset = CombinedVertices.Num();
	
	// Add transformed vertices
	for (const FVector& Vertex : WallVertices)
	{
		FVector TransformedVertex = WallTransform.TransformPosition(Vertex);
		CombinedVertices.Add(TransformedVertex);
		CombinedColors.Add(WallColor);
	}
	
	// Add transformed normals
	for (const FVector& Normal : WallNormals)
	{
		FVector TransformedNormal = WallTransform.TransformVectorNoScale(Normal);
		CombinedNormals.Add(TransformedNormal);
	}
	
	// Add UVs (unchanged)
	CombinedUVs.Append(WallUVs);
	
	// Add triangles with vertex offset
	for (int32 Triangle : WallTriangles)
	{
		CombinedTriangles.Add(Triangle + VertexOffset);
	}
	
	// UE_LOG(LogTemp, Verbose, TEXT("StandardRoom: Added wall with %d vertices at position %s"), 
		// WallVertices.Num(), *WallPosition.ToString());
}

void UStandardRoom::GenerateIndividualWalls(TArray<TArray<FVector>>& WallVertices, TArray<TArray<int32>>& WallTriangles, 
	TArray<TArray<FVector>>& WallNormals, TArray<TArray<FVector2D>>& WallUVs)
{
	// Initialize arrays for 5 sections: Floor(0), North(1), South(2), East(3), West(4)
	WallVertices.SetNum(5);
	WallTriangles.SetNum(5);
	WallNormals.SetNum(5);
	WallUVs.SetNum(5);

	// UE_LOG(LogTemp, Warning, TEXT("🔧 GEOMETRY DEBUG: Starting wall generation for 5 sections using WallUnit system"));

	// Generate floor first
	// UE_LOG(LogTemp, Warning, TEXT("🔧 GEOMETRY DEBUG: Generating floor (section 0)"));
	GenerateFloor(WallVertices[0], WallTriangles[0], WallNormals[0], WallUVs[0]);
	// UE_LOG(LogTemp, Warning, TEXT("🔧 GEOMETRY DEBUG: Floor generated - %d vertices, %d triangles"), 
		// WallVertices[0].Num(), WallTriangles[0].Num() / 3);

	// Convert to unreal units for mesh generation
	float WidthCm = Width * 100.0f;
	float LengthCm = Length * 100.0f;
	float HeightCm = Height * 100.0f;
	float ThicknessCm = WallThickness * 100.0f;

	// Generate walls using WallUnit system with proper hole handling
	TArray<EWallSide> WallSides = {EWallSide::North, EWallSide::South, EWallSide::East, EWallSide::West};
	
	for (int32 WallIndex = 0; WallIndex < WallSides.Num(); WallIndex++)
	{
		EWallSide CurrentWall = WallSides[WallIndex];
		int32 SectionIndex = WallIndex + 1; // Offset by 1 (floor is section 0)
		
		// UE_LOG(LogTemp, Warning, TEXT("🔧 GEOMETRY DEBUG: Generating %s wall (section %d) with WallUnit"), 
			// *UEnum::GetValueAsString(CurrentWall), SectionIndex);
		
		// Generate wall with holes using WallUnit system
		GenerateWallWithHoles(CurrentWall, SectionIndex, 
			WallVertices[SectionIndex], WallTriangles[SectionIndex], 
			WallNormals[SectionIndex], WallUVs[SectionIndex]);

		// UE_LOG(LogTemp, Warning, TEXT("🔧 GEOMETRY DEBUG: %s wall generated - %d vertices, %d triangles"), 
			// *UEnum::GetValueAsString(CurrentWall), WallVertices[SectionIndex].Num(), WallTriangles[SectionIndex].Num() / 3);
	}
	
	// UE_LOG(LogTemp, Warning, TEXT("🔧 GEOMETRY DEBUG: All walls generated with holes. Summary:"));
	// for (int32 i = 0; i < 5; i++)
	// {
		// UE_LOG(LogTemp, Warning, TEXT("🔧   Section %d: %d vertices, %d triangles"), 
			// i, WallVertices[i].Num(), WallTriangles[i].Num() / 3);
	// }
}

void UStandardRoom::GenerateFloor(TArray<FVector>& Vertices, TArray<int32>& Triangles, 
	TArray<FVector>& Normals, TArray<FVector2D>& UVs)
{
	// EXPERIMENTAL: Generate step pattern floor instead of flat floor
	float WidthCm = Width * 100.0f;
	float LengthCm = Length * 100.0f;
	
	// Step configuration for floor pattern
	float StepWidth = 60.0f;   // 60cm wide steps
	float StepDepth = 60.0f;   // 60cm deep steps  
	float StepHeight = 10.0f;  // 10cm high steps (low height for floor)
	
	// Calculate how many steps fit in each direction
	int32 StepsX = FMath::Max(1, FMath::FloorToInt(WidthCm / StepWidth));
	int32 StepsY = FMath::Max(1, FMath::FloorToInt(LengthCm / StepDepth));
	
	// Generate grid of steps across the floor
	for (int32 Y = 0; Y < StepsY; Y++)
	{
		for (int32 X = 0; X < StepsX; X++)
		{
			int32 BaseVertexIndex = Vertices.Num();
			
			// Calculate step position
			float StepX = X * StepWidth;
			float StepY = Y * StepDepth;
			FVector StepPosition = FVector(StepX, StepY, 0);
			
			// Create step box vertices (8 vertices per step)
			TArray<FVector> StepVertices = {
				// Bottom face
				StepPosition + FVector(0, 0, 0),                           // 0: Bottom front-left
				StepPosition + FVector(StepWidth, 0, 0),                   // 1: Bottom front-right
				StepPosition + FVector(StepWidth, StepDepth, 0),           // 2: Bottom back-right
				StepPosition + FVector(0, StepDepth, 0),                   // 3: Bottom back-left
				
				// Top face
				StepPosition + FVector(0, 0, StepHeight),                  // 4: Top front-left
				StepPosition + FVector(StepWidth, 0, StepHeight),          // 5: Top front-right
				StepPosition + FVector(StepWidth, StepDepth, StepHeight),  // 6: Top back-right
				StepPosition + FVector(0, StepDepth, StepHeight)           // 7: Top back-left
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
			
			// Add normals (simplified - using up vector)
			for (int32 i = 0; i < 8; i++)
			{
				Normals.Add(FVector::UpVector);
			}
			
			// Add UVs (simple mapping)
			for (int32 i = 0; i < 8; i++)
			{
				UVs.Add(FVector2D(StepX / WidthCm, StepY / LengthCm));
			}
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("StandardRoom: Generated step pattern floor with %d steps (%dx%d grid)"), 
		StepsX * StepsY, StepsX, StepsY);
}




void UStandardRoom::GenerateSimpleWall(EWallSide WallSide, float WidthCm, float LengthCm, float HeightCm,
	TArray<FVector>& Vertices, TArray<int32>& Triangles, 
	TArray<FVector>& Normals, TArray<FVector2D>& UVs)
{
	int32 BaseIndex = Vertices.Num();
	float WallThicknessCm = WallThickness * 100.0f; // 20cm default thickness
	
	// UE_LOG(LogTemp, Warning, TEXT("🔧 WALL DEBUG: Generating %s wall with thickness %.1fcm"), 
		// *UEnum::GetValueAsString(WallSide), WallThicknessCm);
	
	// Generate walls with thickness - 8 vertices per wall (inner + outer faces)
	switch (WallSide)
	{
		case EWallSide::North:
			// North wall - inner face (at Y = LengthCm)
			Vertices.Add(FVector(0, LengthCm, 0));              // Inner bottom-left
			Vertices.Add(FVector(WidthCm, LengthCm, 0));        // Inner bottom-right
			Vertices.Add(FVector(WidthCm, LengthCm, HeightCm)); // Inner top-right
			Vertices.Add(FVector(0, LengthCm, HeightCm));       // Inner top-left
			// North wall - outer face (at Y = LengthCm + thickness)
			Vertices.Add(FVector(WidthCm, LengthCm + WallThicknessCm, 0));        // Outer bottom-right
			Vertices.Add(FVector(0, LengthCm + WallThicknessCm, 0));              // Outer bottom-left
			Vertices.Add(FVector(0, LengthCm + WallThicknessCm, HeightCm));       // Outer top-left
			Vertices.Add(FVector(WidthCm, LengthCm + WallThicknessCm, HeightCm)); // Outer top-right
			break;
		case EWallSide::South:
			// South wall - inner face (at Y = 0)
			Vertices.Add(FVector(WidthCm, 0, 0));        // Inner bottom-right
			Vertices.Add(FVector(0, 0, 0));              // Inner bottom-left
			Vertices.Add(FVector(0, 0, HeightCm));       // Inner top-left
			Vertices.Add(FVector(WidthCm, 0, HeightCm)); // Inner top-right
			// South wall - outer face (at Y = -thickness)
			Vertices.Add(FVector(0, -WallThicknessCm, 0));              // Outer bottom-left
			Vertices.Add(FVector(WidthCm, -WallThicknessCm, 0));        // Outer bottom-right
			Vertices.Add(FVector(WidthCm, -WallThicknessCm, HeightCm)); // Outer top-right
			Vertices.Add(FVector(0, -WallThicknessCm, HeightCm));       // Outer top-left
			break;
		case EWallSide::East:
			// East wall - inner face (at X = WidthCm)
			Vertices.Add(FVector(WidthCm, 0, 0));              // Inner bottom-front
			Vertices.Add(FVector(WidthCm, LengthCm, 0));       // Inner bottom-back
			Vertices.Add(FVector(WidthCm, LengthCm, HeightCm)); // Inner top-back
			Vertices.Add(FVector(WidthCm, 0, HeightCm));       // Inner top-front
			// East wall - outer face (at X = WidthCm + thickness)
			Vertices.Add(FVector(WidthCm + WallThicknessCm, LengthCm, 0));       // Outer bottom-back
			Vertices.Add(FVector(WidthCm + WallThicknessCm, 0, 0));              // Outer bottom-front
			Vertices.Add(FVector(WidthCm + WallThicknessCm, 0, HeightCm));       // Outer top-front
			Vertices.Add(FVector(WidthCm + WallThicknessCm, LengthCm, HeightCm)); // Outer top-back
			break;
		case EWallSide::West:
			// West wall - inner face (at X = 0)
			Vertices.Add(FVector(0, LengthCm, 0));       // Inner bottom-back
			Vertices.Add(FVector(0, 0, 0));              // Inner bottom-front
			Vertices.Add(FVector(0, 0, HeightCm));       // Inner top-front
			Vertices.Add(FVector(0, LengthCm, HeightCm)); // Inner top-back
			// West wall - outer face (at X = -thickness)
			Vertices.Add(FVector(-WallThicknessCm, 0, 0));              // Outer bottom-front
			Vertices.Add(FVector(-WallThicknessCm, LengthCm, 0));       // Outer bottom-back
			Vertices.Add(FVector(-WallThicknessCm, LengthCm, HeightCm)); // Outer top-back
			Vertices.Add(FVector(-WallThicknessCm, 0, HeightCm));       // Outer top-front
			break;
	}
	
	// Generate 6 faces for thick 3D wall (like a box)
	// Face 1: Inner face (visible from inside room) - vertices 0,1,2,3
	Triangles.Append({
		BaseIndex + 0, BaseIndex + 2, BaseIndex + 1,  // Inner face triangle 1
		BaseIndex + 0, BaseIndex + 3, BaseIndex + 2   // Inner face triangle 2
	});
	
	// Face 2: Outer face (visible from outside room) - vertices 4,5,6,7
	Triangles.Append({
		BaseIndex + 4, BaseIndex + 5, BaseIndex + 6,  // Outer face triangle 1
		BaseIndex + 4, BaseIndex + 6, BaseIndex + 7   // Outer face triangle 2
	});
	
	// Face 3: Bottom face (connects bottom edges of inner and outer)
	Triangles.Append({
		BaseIndex + 0, BaseIndex + 1, BaseIndex + 5,  // Bottom triangle 1
		BaseIndex + 0, BaseIndex + 5, BaseIndex + 4   // Bottom triangle 2
	});
	
	// Face 4: Top face (connects top edges of inner and outer) - wall-specific
	switch (WallSide)
	{
		case EWallSide::North:
			// North: Inner top = 2,3  Outer top = 6,7
			Triangles.Append({
				BaseIndex + 3, BaseIndex + 2, BaseIndex + 6,  // Top triangle 1
				BaseIndex + 3, BaseIndex + 6, BaseIndex + 7   // Top triangle 2
			});
			break;
		case EWallSide::South:
			// South: Inner top = 2,3  Outer top = 6,7
			Triangles.Append({
				BaseIndex + 2, BaseIndex + 3, BaseIndex + 7,  // Top triangle 1
				BaseIndex + 2, BaseIndex + 7, BaseIndex + 6   // Top triangle 2
			});
			break;
		case EWallSide::East:
			// East: Inner top = 2,3  Outer top = 6,7
			Triangles.Append({
				BaseIndex + 2, BaseIndex + 3, BaseIndex + 6,  // Top triangle 1
				BaseIndex + 3, BaseIndex + 7, BaseIndex + 6   // Top triangle 2
			});
			break;
		case EWallSide::West:
			// West: Inner top = 2,3  Outer top = 6,7
			Triangles.Append({
				BaseIndex + 3, BaseIndex + 2, BaseIndex + 7,  // Top triangle 1
				BaseIndex + 2, BaseIndex + 6, BaseIndex + 7   // Top triangle 2
			});
			break;
	}
	
	// Face 5: Left face (connects left edges of inner and outer)
	Triangles.Append({
		BaseIndex + 0, BaseIndex + 4, BaseIndex + 3,  // Left triangle 1
		BaseIndex + 3, BaseIndex + 4, BaseIndex + 7   // Left triangle 2
	});
	
	// Face 6: Right face (connects right edges of inner and outer)
	Triangles.Append({
		BaseIndex + 1, BaseIndex + 2, BaseIndex + 5,  // Right triangle 1
		BaseIndex + 2, BaseIndex + 6, BaseIndex + 5   // Right triangle 2
	});
	
	// Add normals for all 8 vertices
	FVector InwardNormal = FVector::ZeroVector;
	FVector OutwardNormal = FVector::ZeroVector;
	switch (WallSide)
	{
		case EWallSide::North: 
			InwardNormal = -FVector::ForwardVector;  // Point inward (-Y)
			OutwardNormal = FVector::ForwardVector;  // Point outward (+Y)
			break;
		case EWallSide::South: 
			InwardNormal = FVector::ForwardVector;   // Point inward (+Y)
			OutwardNormal = -FVector::ForwardVector; // Point outward (-Y)
			break;
		case EWallSide::East: 
			InwardNormal = -FVector::RightVector;    // Point inward (-X)
			OutwardNormal = FVector::RightVector;    // Point outward (+X)
			break;
		case EWallSide::West: 
			InwardNormal = FVector::RightVector;     // Point inward (+X)
			OutwardNormal = -FVector::RightVector;   // Point outward (-X)
			break;
	}
	
	// Add normals for inner face vertices (0,1,2,3)
	for (int32 i = 0; i < 4; i++)
	{
		Normals.Add(InwardNormal);
	}
	
	// Add normals for outer face vertices (4,5,6,7)
	for (int32 i = 0; i < 4; i++)
	{
		Normals.Add(OutwardNormal);
	}
	
	// Add UVs for all 8 vertices
	// Inner face UVs
	UVs.Add(FVector2D(0, 0));
	UVs.Add(FVector2D(1, 0));
	UVs.Add(FVector2D(1, 1));
	UVs.Add(FVector2D(0, 1));
	// Outer face UVs
	UVs.Add(FVector2D(0, 0));
	UVs.Add(FVector2D(1, 0));
	UVs.Add(FVector2D(1, 1));
	UVs.Add(FVector2D(0, 1));
	
	// UE_LOG(LogTemp, Warning, TEXT("🔧 WALL DEBUG: %s wall completed - added 8 vertices, 12 triangles (thick wall with inner/outer faces + connecting sides)"), 
		// *UEnum::GetValueAsString(WallSide));
}

void UStandardRoom::GenerateWallWithHoles(EWallSide WallSide, int32 SectionIndex,
	TArray<FVector>& Vertices, TArray<int32>& Triangles,
	TArray<FVector>& Normals, TArray<FVector2D>& UVs)
{
	// Use the same simple thick wall approach as test mode for consistency
	float WidthCm = Width * 100.0f;
	float LengthCm = Length * 100.0f;
	float HeightCm = Height * 100.0f;
	
	// Generate simple thick walls without holes for now (to match test mode geometry)
	// This ensures all wall sections render properly
	GenerateSimpleWall(WallSide, WidthCm, LengthCm, HeightCm, Vertices, Triangles, Normals, UVs);
	
	// UE_LOG(LogTemp, Warning, TEXT("🔧 SIMPLE WALL: %s wall generated using thick wall approach"), 
		// *UEnum::GetValueAsString(WallSide));
}

void UStandardRoom::CreateRoomUsingIndividualActors(AActor* Owner)
{
	if (!Owner || !Owner->GetWorld())
	{
		UE_LOG(LogTemp, Error, TEXT("StandardRoom: Cannot create room - invalid owner or world"));
		return;
	}

	UWorld* World = Owner->GetWorld();
	float WallThickness = 0.2f; // 20cm thick walls

	// Convert to Unreal units (cm)
	float HalfWidth = Width * 0.5f * 100.0f;
	float HalfLength = Length * 0.5f * 100.0f;
	float HeightCm = Height * 100.0f;

	// COORDINATE SYSTEM FIX: Main flow gives us corner position, but we need center position
	// Convert from corner position (main flow) to center position (test mode)
	FVector RoomCenter = Position + FVector(HalfWidth, HalfLength, HeightCm * 0.5f);
	
	// UE_LOG(LogTemp, Warning, TEXT("StandardRoom: Converting corner position %s to center position %s"), 
		// *Position.ToString(), *RoomCenter.ToString());

	// Individual wall colors (same as test mode)
	FLinearColor SouthWallColor = FLinearColor::Green;   // South wall = Green
	FLinearColor NorthWallColor = FLinearColor::Red;     // North wall = Red  
	FLinearColor EastWallColor = FLinearColor::Blue;     // East wall = Blue
	FLinearColor WestWallColor = FLinearColor::Yellow;   // West wall = Yellow
	FLinearColor FloorColor = FLinearColor::Gray;        // Floor = Gray
	FLinearColor CeilingColor = FLinearColor::White * 0.9f; // Ceiling = Light gray

	// === CREATE 4 WALLS (only with doors where DoorConfigs specify) ===
	
	UE_LOG(LogTemp, Warning, TEXT("WALL GENERATION DETAILS:"));
	UE_LOG(LogTemp, Warning, TEXT("   Room Center: %s"), *RoomCenter.ToString());
	UE_LOG(LogTemp, Warning, TEXT("   Wall Colors: South=Green, North=Red, East=Blue, West=Yellow"));
	UE_LOG(LogTemp, Warning, TEXT("   DoorConfigs: %d door configurations"), DoorConfigs.Num());

	// Helper to find DoorConfig for a specific wall side
	auto FindDoorConfig = [&](EWallSide WallSide) -> FDoorConfig* {
		for (FDoorConfig& Config : DoorConfigs) {
			if (Config.bHasDoor && Config.WallSide == WallSide) {
				return &Config;
			}
		}
		return nullptr;
	};

	// Helper to check if wall should be completely removed (large width triggers removal)
	auto ShouldRemoveWall = [](FDoorConfig* DoorConfig) -> bool {
		return DoorConfig && DoorConfig->Width >= 99.0f;
	};

	// South Wall (GREEN) - check if it should have a doorway
	FVector SouthWallPos = RoomCenter + FVector(0, -HalfLength, 0);
	FRotator SouthWallRot = FRotator(0, 0, 0);
	FDoorConfig* SouthDoor = FindDoorConfig(EWallSide::South);
	UE_LOG(LogTemp, Warning, TEXT("   [S] SOUTH WALL: Pos=%s, Rot=%s, Color=Green"), *SouthWallPos.ToString(), *SouthWallRot.ToString());
	if (ShouldRemoveWall(SouthDoor)) {
		UE_LOG(LogTemp, Warning, TEXT("      [X] REMOVED (Width=%.1fm triggers removal)"), SouthDoor->Width);
	} else if (SouthDoor) {
		FWallHoleConfig SouthDoorConfig = FWallHoleConfig::CreateCustom(
			SouthDoor->Width, SouthDoor->Height, Width * 0.5f, Height * 0.5f, TEXT("SouthDoor"));
		UWallUnit::CreateWallWithHole(World, SouthWallPos, SouthWallRot, 
			Width, Height, WallThickness, SouthWallColor, SouthDoorConfig);
		UE_LOG(LogTemp, Warning, TEXT("      [O] WITH HOLE (%.1fx%.1fm doorway)"), SouthDoor->Width, SouthDoor->Height);
	} else {
		AActor* SouthWallActor = UWallUnit::CreateSolidWallActor(World, SouthWallPos, SouthWallRot, 
			Width, Height, WallThickness, SouthWallColor);
		WallActors.Add(EWallSide::South, SouthWallActor);
		UE_LOG(LogTemp, Warning, TEXT("      [#] SOLID (no connections)"));
	}

	// North Wall (RED) - check if it should have a doorway
	FVector NorthWallPos = RoomCenter + FVector(0, HalfLength, 0);
	FRotator NorthWallRot = FRotator(0, 180, 0);
	FDoorConfig* NorthDoor = FindDoorConfig(EWallSide::North);
	UE_LOG(LogTemp, Warning, TEXT("   [N] NORTH WALL: Pos=%s, Rot=%s, Color=Red"), *NorthWallPos.ToString(), *NorthWallRot.ToString());
	if (ShouldRemoveWall(NorthDoor)) {
		UE_LOG(LogTemp, Warning, TEXT("      [X] REMOVED (Width=%.1fm triggers removal)"), NorthDoor->Width);
	} else if (NorthDoor) {
		float HoleCenterX = Width * 0.5f;
		float HoleCenterY = Height * 0.5f;
		FWallHoleConfig NorthDoorConfig = FWallHoleConfig::CreateCustom(
			NorthDoor->Width, NorthDoor->Height, HoleCenterX, HoleCenterY, TEXT("NorthDoor"));
		AActor* WallActor = UWallUnit::CreateWallWithHole(World, NorthWallPos, NorthWallRot, 
			Width, Height, WallThickness, NorthWallColor, NorthDoorConfig);
		UE_LOG(LogTemp, Warning, TEXT("      [O] WITH HOLE (%.1fx%.1fm doorway)"), NorthDoor->Width, NorthDoor->Height);
	} else {
		AActor* NorthWallActor = UWallUnit::CreateSolidWallActor(World, NorthWallPos, NorthWallRot, 
			Width, Height, WallThickness, NorthWallColor);
		WallActors.Add(EWallSide::North, NorthWallActor);
		UE_LOG(LogTemp, Warning, TEXT("      [#] SOLID (no connections)"));
	}

	// East Wall (BLUE) - check if it should have a doorway
	FVector EastWallPos = RoomCenter + FVector(HalfWidth, 0, 0);
	FRotator EastWallRot = FRotator(0, 90, 0);
	FDoorConfig* EastDoor = FindDoorConfig(EWallSide::East);
	UE_LOG(LogTemp, Warning, TEXT("   [E] EAST WALL: Pos=%s, Rot=%s, Color=Blue"), *EastWallPos.ToString(), *EastWallRot.ToString());
	if (ShouldRemoveWall(EastDoor)) {
		UE_LOG(LogTemp, Warning, TEXT("      [X] REMOVED (Width=%.1fm triggers removal)"), EastDoor->Width);
	} else if (EastDoor) {
		FWallHoleConfig EastDoorConfig = FWallHoleConfig::CreateCustom(
			EastDoor->Width, EastDoor->Height, Length * 0.5f, Height * 0.5f, TEXT("EastDoor"));
		AActor* WallActor = UWallUnit::CreateWallWithHole(World, EastWallPos, EastWallRot, 
			Length, Height, WallThickness, EastWallColor, EastDoorConfig);
		UE_LOG(LogTemp, Warning, TEXT("      [O] WITH HOLE (%.1fx%.1fm doorway)"), EastDoor->Width, EastDoor->Height);
	} else {
		AActor* EastWallActor = UWallUnit::CreateSolidWallActor(World, EastWallPos, EastWallRot, 
			Length, Height, WallThickness, EastWallColor);
		WallActors.Add(EWallSide::East, EastWallActor);
		UE_LOG(LogTemp, Warning, TEXT("      [#] SOLID (no connections)"));
	}

	// West Wall (YELLOW) - check if it should have a doorway
	FVector WestWallPos = RoomCenter + FVector(-HalfWidth, 0, 0);
	FRotator WestWallRot = FRotator(0, 270, 0);
	FDoorConfig* WestDoor = FindDoorConfig(EWallSide::West);
	UE_LOG(LogTemp, Warning, TEXT("   [W] WEST WALL: Pos=%s, Rot=%s, Color=Yellow"), *WestWallPos.ToString(), *WestWallRot.ToString());
	if (ShouldRemoveWall(WestDoor)) {
		UE_LOG(LogTemp, Warning, TEXT("      [X] REMOVED (Width=%.1fm triggers removal)"), WestDoor->Width);
	} else if (WestDoor) {
		FWallHoleConfig WestDoorConfig = FWallHoleConfig::CreateCustom(
			WestDoor->Width, WestDoor->Height, Length * 0.5f, Height * 0.5f, TEXT("WestDoor"));
		AActor* WallActor = UWallUnit::CreateWallWithHole(World, WestWallPos, WestWallRot, 
			Length, Height, WallThickness, WestWallColor, WestDoorConfig);
		UE_LOG(LogTemp, Warning, TEXT("      [O] WITH HOLE (%.1fx%.1fm doorway)"), WestDoor->Width, WestDoor->Height);
	} else {
		AActor* WestWallActor = UWallUnit::CreateSolidWallActor(World, WestWallPos, WestWallRot, 
			Length, Height, WallThickness, WestWallColor);
		WallActors.Add(EWallSide::West, WestWallActor);
		UE_LOG(LogTemp, Warning, TEXT("      [#] SOLID (no connections)"));
	}

	// === CREATE FLOOR AND CEILING (same as test mode) ===

	// Floor (positioned below room center)
	FVector FloorPos = RoomCenter + FVector(0, 0, -(HeightCm * 0.5f + WallThickness * 100.0f * 0.5f + 2.0f));
	FRotator FloorRot = FRotator(0, 0, 90);
	UWallUnit::CreateSolidWallActor(World, FloorPos, FloorRot, 
		Width, Length, WallThickness, FloorColor);
	UE_LOG(LogTemp, Warning, TEXT("   [F] FLOOR: Pos=%s, Rot=%s, Color=Gray"), *FloorPos.ToString(), *FloorRot.ToString());

	// Ceiling (positioned above room center) - COMMENTED OUT for better visibility
	/*
	FVector CeilingPos = RoomCenter + FVector(0, 0, HeightCm * 0.5f + (WallThickness * 100.0f * 0.5f + 2.0f));
	FRotator CeilingRot = FRotator(0, 0, 270);
	UWallUnit::CreateSolidWallActor(World, CeilingPos, CeilingRot, 
		Width, Length, WallThickness, CeilingColor);
	UE_LOG(LogTemp, Warning, TEXT("   ⬜ CEILING: Pos=%s, Rot=%s, Color=LightGray"), *CeilingPos.ToString(), *CeilingRot.ToString());
	*/
	UE_LOG(LogTemp, Warning, TEXT("   [X] CEILING: Disabled for better visibility"));
	
	UE_LOG(LogTemp, Warning, TEXT("*** ROOM CREATION COMPLETED: 4 walls + floor created successfully ***"));
}



FVector2D UStandardRoom::CalculateUV(const FVector& Vertex, float ScaleFactor)
{
	// Simple UV calculation for room texturing
	return FVector2D(Vertex.X / (Width * 100.0f) * ScaleFactor, Vertex.Y / (Length * 100.0f) * ScaleFactor);
}

EWallSide UStandardRoom::GetOppositeWall(EWallSide WallSide) const
{
	switch (WallSide)
	{
		case EWallSide::North: return EWallSide::South;
		case EWallSide::South: return EWallSide::North;
		case EWallSide::East: return EWallSide::West;
		case EWallSide::West: return EWallSide::East;
		default: return EWallSide::None;
	}
}

bool UStandardRoom::CreateFromRoomData(const FRoomData& RoomData, AActor* Owner, bool bShowNumbers)
{
	// Set all properties from RoomData
	Width = RoomData.Width;
	Length = RoomData.Length;
	Height = RoomData.Height;
	Position = RoomData.Position;
	RoomCategory = RoomData.Category;
	Elevation = RoomData.Elevation;
	
	// DETAILED ROOM PROPERTIES LOG
	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT("========== CREATING ROOM %d =========="), RoomData.RoomIndex);
	UE_LOG(LogTemp, Warning, TEXT("DIMENSIONS: %.1fm x %.1fm x %.1fm (W x L x H)"), Width, Length, Height);
	UE_LOG(LogTemp, Warning, TEXT("POSITION: %s (X=%.1f, Y=%.1f, Z=%.1f)"), *Position.ToString(), Position.X, Position.Y, Position.Z);
	UE_LOG(LogTemp, Warning, TEXT("CATEGORY: %s"), *UEnum::GetValueAsString(RoomCategory));
	UE_LOG(LogTemp, Warning, TEXT("ELEVATION: %.2fm above ground"), Elevation);
	if (RoomData.StairDirection != EWallSide::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("STAIR DIRECTION: %s"), *UEnum::GetValueAsString(RoomData.StairDirection));
	}
	UE_LOG(LogTemp, Warning, TEXT("CONNECTIONS: %d wall connections available"), RoomData.Connections.Num());
	UE_LOG(LogTemp, Warning, TEXT("WALL THICKNESS: %.1fcm"), WallThickness * 100.0f);
	
	// Calculate room area and volume for reference
	float RoomArea = Width * Length;
	float RoomVolume = Width * Length * Height;
	UE_LOG(LogTemp, Warning, TEXT("STATS: Area=%.1fm2, Volume=%.1fm3"), RoomArea, RoomVolume);
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	
	// Create the room using individual actors approach
	this->CreateRoomUsingIndividualActors(Owner);
	
	// Note: Collision detection is handled by the main flow before calling this method
	
	// Create room number identifier automatically if enabled
	CreateRoomNumberText(RoomData.RoomIndex, bShowNumbers);
	
	UE_LOG(LogTemp, Warning, TEXT("========== ROOM %d COMPLETED =========="), RoomData.RoomIndex);
	UE_LOG(LogTemp, Warning, TEXT(""));
	return true;
}

void UStandardRoom::AddHoleToWall(AActor* Owner, EWallSide WallSide, const FDoorConfig& DoorConfig)
{
	if (!Owner || !Owner->GetWorld())
	{
		UE_LOG(LogTemp, Error, TEXT("AddHoleToWall: Invalid Owner or World"));
		return;
	}

	UWorld* World = Owner->GetWorld();
	
	// PERFORMANCE OPTIMIZATION: Find and destroy existing wall actor
	AActor** ExistingWallPtr = WallActors.Find(WallSide);
	if (ExistingWallPtr && *ExistingWallPtr)
	{
		// UE_LOG(LogTemp, Warning, TEXT("🔧 AddHoleToWall: Destroying existing %s wall for replacement"), 
			// *UEnum::GetValueAsString(WallSide));
		(*ExistingWallPtr)->Destroy();
		WallActors.Remove(WallSide);
	}
	
	// Convert from corner position (main flow) to center position (test mode)
	FVector RoomCenter = Position + FVector(Width * 100.0f * 0.5f, Length * 100.0f * 0.5f, Height * 100.0f * 0.5f);
	
	// Wall dimensions in cm
	float WidthCm = Width * 100.0f;
	float LengthCm = Length * 100.0f;
	float HeightCm = Height * 100.0f;
	float WallThicknessCm = WallThickness * 100.0f;
	float HalfThickness = WallThicknessCm * 0.5f;
	float HalfWidthCm = WidthCm * 0.5f;
	float HalfLengthCm = LengthCm * 0.5f;
	float HalfHeightCm = HeightCm * 0.5f;
	
	// Calculate wall position and rotation (same logic as CreateRoomUsingIndividualActors)
	FVector WallPos;
	FRotator WallRot;
	float WallWidth, WallHeight;
	FLinearColor WallColor;
	
	switch (WallSide)
	{
		case EWallSide::North:
			WallPos = RoomCenter + FVector(0, HalfLengthCm + HalfThickness, 0);
			WallRot = FRotator(0, 0, 0);
			WallWidth = Width;
			WallHeight = Height;
			WallColor = FLinearColor::Red; // North = Red (fixed)
			break;
			
		case EWallSide::South:
			WallPos = RoomCenter + FVector(0, -HalfLengthCm - HalfThickness, 0);
			WallRot = FRotator(0, 180, 0);
			WallWidth = Width;
			WallHeight = Height;
			WallColor = FLinearColor::Green; // South = Green (fixed)
			break;
			
		case EWallSide::East:
			WallPos = RoomCenter + FVector(HalfWidthCm + HalfThickness, 0, 0);
			WallRot = FRotator(0, 90, 0);
			WallWidth = Length;
			WallHeight = Height;
			WallColor = FLinearColor::Blue; // East = Blue (fixed)
			break;
			
		case EWallSide::West:
			WallPos = RoomCenter + FVector(-HalfWidthCm - HalfThickness, 0, 0);
			WallRot = FRotator(0, 270, 0);
			WallWidth = Length;
			WallHeight = Height;
			WallColor = FLinearColor::Yellow; // West = Yellow (fixed)
			break;
			
		default:
			UE_LOG(LogTemp, Error, TEXT("AddHoleToWall: Invalid WallSide %d"), (int32)WallSide);
			return;
	}
	
	// Check if this should be a wall removal (Width >= 99.0f) or a hole
	AActor* NewWallActor = nullptr;
	
	if (DoorConfig.Width >= 99.0f)
	{
		// Complete wall removal - don't create any wall
		UE_LOG(LogTemp, Warning, TEXT("🔧 AddHoleToWall: Complete %s wall removal (Width=%.1f)"), 
			*UEnum::GetValueAsString(WallSide), DoorConfig.Width);
		// NewWallActor remains nullptr (no wall)
	}
	else
	{
		// Calculate hole position - respect OffsetFromCenter parameter for alignment
		// NOTE: Regular AddHoleToWall doesn't receive SmallerWallSize parameter, so uses full WallWidth
		float HolePositionX;
		
		// ALIGNMENT FIX: Check if OffsetFromCenter is specified (0.0f means center, not random)
		if (DoorConfig.OffsetFromCenter == 0.0f)
		{
			// Center the hole for proper alignment between rooms
			HolePositionX = WallWidth * 0.5f;
			UE_LOG(LogTemp, Warning, TEXT("🎯 CENTERED hole (OffsetFromCenter=0): %.1fm wall, hole centered at %.1fm"), 
				WallWidth, HolePositionX);
		}
		else if (WallWidth >= 5.0f)
		{
			// Random positioning on larger walls - ensure hole doesn't go outside wall bounds
			float MinPosition = DoorConfig.Width * 0.5f + 0.5f; // Half hole width + 0.5m margin
			float MaxPosition = WallWidth - (DoorConfig.Width * 0.5f + 0.5f); // Wall width - half hole width - 0.5m margin
			if (MaxPosition > MinPosition)
			{
				FRandomStream Random(FDateTime::Now().GetTicks() + (int32)WallSide); // Unique seed per wall
				HolePositionX = Random.FRandRange(MinPosition, MaxPosition);
				UE_LOG(LogTemp, Warning, TEXT("🎲 Random doorway position: %.1fm wall, hole at %.1fm (range %.1f-%.1f)"), 
					WallWidth, HolePositionX, MinPosition, MaxPosition);
			}
			else
			{
				HolePositionX = WallWidth * 0.5f; // Fallback to center if math doesn't work
			}
		}
		else
		{
			HolePositionX = WallWidth * 0.5f; // Center position for smaller walls
			UE_LOG(LogTemp, Warning, TEXT("🎯 Centered doorway: %.1fm wall, hole at center %.1fm"), WallWidth, HolePositionX);
		}

		// Create hole config from DoorConfig (same as TestGenerator approach)
		FWallHoleConfig HoleConfig = FWallHoleConfig::CreateCustom(
			DoorConfig.Width, DoorConfig.Height,
			HolePositionX, DoorConfig.Height * 0.5f, // Random/centered X, bottom-align vertically
			FString::Printf(TEXT("%sWallHole"), *UEnum::GetValueAsString(WallSide))
		);
		HoleConfig.Shape = EHoleShape::Rectangle;
		
		// UE_LOG(LogTemp, Warning, TEXT("🔧 AddHoleToWall: Creating %s wall with %.1fx%.1fm hole"), 
			// *UEnum::GetValueAsString(WallSide), DoorConfig.Width, DoorConfig.Height);
			
		// Create wall with hole using the same method as TestGenerator
		NewWallActor = UWallUnit::CreateWallWithHole(World, WallPos, WallRot,
			WallWidth, WallHeight, WallThickness, WallColor, HoleConfig);
	}
	
	// Store the new wall actor (or nullptr for removed walls)
	if (NewWallActor)
	{
		WallActors.Add(WallSide, NewWallActor);
		UE_LOG(LogTemp, Warning, TEXT("✅ AddHoleToWall: Successfully replaced %s wall with hole"), 
			*UEnum::GetValueAsString(WallSide));
	}
	else if (DoorConfig.Width >= 99.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("✅ AddHoleToWall: Successfully removed %s wall completely"), 
			*UEnum::GetValueAsString(WallSide));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("❌ AddHoleToWall: Failed to create %s wall with hole"), 
			*UEnum::GetValueAsString(WallSide));
	}
}

void UStandardRoom::AddHoleToWallWithThickness(AActor* Owner, EWallSide WallSide, const FDoorConfig& DoorConfig, float CustomThickness, float SmallerWallSize, UStandardRoom* TargetRoom)
{
	if (!Owner || !Owner->GetWorld())
	{
		UE_LOG(LogTemp, Error, TEXT("AddHoleToWallWithThickness: Invalid Owner or World"));
		return;
	}

	UWorld* World = Owner->GetWorld();
	
	// ASYMMETRIC CONNECTION: If TargetRoom is provided, create asymmetric connection
	if (TargetRoom)
	{
		UE_LOG(LogTemp, Warning, TEXT("ASYMMETRIC CONNECTION: Creating connection between two rooms"));
		
		// Calculate room areas to determine which is smaller/larger
		float ThisRoomArea = Width * Length;
		float TargetRoomArea = TargetRoom->Width * TargetRoom->Length;
		
		// Use boolean flags instead of pointer comparisons to avoid type casting issues
		bool bThisIsSmaller = (ThisRoomArea <= TargetRoomArea);
		bool bThisIsLarger = (ThisRoomArea > TargetRoomArea);
		
		UStandardRoom* SmallerRoom = bThisIsSmaller ? this : TargetRoom;
		UStandardRoom* LargerRoom = bThisIsLarger ? this : TargetRoom;
		
		// Determine wall sides for connection
		EWallSide SmallerRoomWall = bThisIsSmaller ? WallSide : GetOppositeWall(WallSide);
		EWallSide LargerRoomWall = bThisIsLarger ? WallSide : GetOppositeWall(WallSide);
		
		UE_LOG(LogTemp, Warning, TEXT("ASYMMETRIC: Room areas calculated"));
		
		// Step 1: Remove smaller room's wall completely
		FDoorConfig RemovalConfig = DoorConfig;
		RemovalConfig.Width = 999.0f; // Special value for complete wall removal
		RemovalConfig.Height = SmallerRoom->Height;
		SmallerRoom->AddHoleToWall(Owner, SmallerRoomWall, RemovalConfig);
		
		UE_LOG(LogTemp, Warning, TEXT("ASYMMETRIC: Removed smaller room wall"));
		
		// Step 2: Create thick wall with hole in larger room (only if larger room is this room)
		if (bThisIsLarger)
		{
			UE_LOG(LogTemp, Warning, TEXT("ASYMMETRIC: Creating thick wall in this room"));
			// Continue with normal thick wall creation below
		}
		else
		{
			// Target room is larger, so create thick wall there
			float ThickerWallThickness = CustomThickness * 2.0f; // Double thickness for larger room
			TargetRoom->AddHoleToWallWithThickness(Owner, LargerRoomWall, DoorConfig, ThickerWallThickness, SmallerRoom->Length);
			UE_LOG(LogTemp, Warning, TEXT("ASYMMETRIC: Created thick wall in target room"));
			return; // Done - target room handles the thick wall
		}
	}
	
	// PERFORMANCE OPTIMIZATION: Find and destroy existing wall actor
	AActor** ExistingWallPtr = WallActors.Find(WallSide);
	if (ExistingWallPtr && *ExistingWallPtr)
	{
		UE_LOG(LogTemp, Warning, TEXT("🔧 AddHoleToWallWithThickness: Destroying existing %s wall for thick replacement"), 
			*UEnum::GetValueAsString(WallSide));
		(*ExistingWallPtr)->Destroy();
		WallActors.Remove(WallSide);
	}
	
	// Convert from corner position (main flow) to center position (test mode)
	FVector RoomCenter = Position + FVector(Width * 100.0f * 0.5f, Length * 100.0f * 0.5f, Height * 100.0f * 0.5f);
	
	// Wall dimensions in cm
	float WidthCm = Width * 100.0f;
	float LengthCm = Length * 100.0f;
	float HeightCm = Height * 100.0f;
	float WallThicknessCm = CustomThickness * 100.0f; // Use custom thickness
	float HalfThickness = WallThicknessCm * 0.5f;
	float HalfWidthCm = WidthCm * 0.5f;
	float HalfLengthCm = LengthCm * 0.5f;
	float HalfHeightCm = HeightCm * 0.5f;
	
	// Calculate wall position and rotation (same logic as CreateRoomUsingIndividualActors)
	FVector WallPos;
	FRotator WallRot;
	float WallWidth, WallHeight;
	FLinearColor WallColor;
	
	switch (WallSide)
	{
		case EWallSide::North:
			WallPos = RoomCenter + FVector(0, HalfLengthCm + HalfThickness, 0);
			WallRot = FRotator(0, 0, 0);
			WallWidth = Width;
			WallHeight = Height;
			WallColor = FLinearColor::Red; // North = Red (fixed)
			break;
			
		case EWallSide::South:
			WallPos = RoomCenter + FVector(0, -HalfLengthCm - HalfThickness, 0);
			WallRot = FRotator(0, 180, 0);
			WallWidth = Width;
			WallHeight = Height;
			WallColor = FLinearColor::Green; // South = Green (fixed)
			break;
			
		case EWallSide::East:
			WallPos = RoomCenter + FVector(HalfWidthCm + HalfThickness, 0, 0);
			WallRot = FRotator(0, 90, 0);
			WallWidth = Length;
			WallHeight = Height;
			WallColor = FLinearColor::Blue; // East = Blue (fixed)
			break;
			
		case EWallSide::West:
			WallPos = RoomCenter + FVector(-HalfWidthCm - HalfThickness, 0, 0);
			WallRot = FRotator(0, 270, 0);
			WallWidth = Length;
			WallHeight = Height;
			WallColor = FLinearColor::Yellow; // West = Yellow (fixed)
			break;
			
		default:
			UE_LOG(LogTemp, Error, TEXT("AddHoleToWallWithThickness: Invalid WallSide %d"), (int32)WallSide);
			return;
	}
	
	// Calculate hole position - respect OffsetFromCenter parameter for alignment
	// BOUNDARY FIX: Use SmallerWallSize if provided to constrain random positioning within connection bounds
	float EffectiveWallWidth = (SmallerWallSize > 0) ? SmallerWallSize : WallWidth;
	UE_LOG(LogTemp, Warning, TEXT("🔧 BOUNDARY DEBUG: WallWidth=%.1fm, SmallerWallSize=%.1fm, EffectiveWallWidth=%.1fm, OffsetFromCenter=%.1fm"), 
		WallWidth, SmallerWallSize, EffectiveWallWidth, DoorConfig.OffsetFromCenter);
	float HolePositionX;
	
	// ALIGNMENT FIX: Check if OffsetFromCenter is specified (0.0f means center, not random)
	if (DoorConfig.OffsetFromCenter == 0.0f)
	{
		// Center the hole - use effective wall width for proper alignment
		HolePositionX = EffectiveWallWidth * 0.5f;
		
		// If we have boundary constraints, adjust to center within the constraint area
		if (SmallerWallSize > 0 && SmallerWallSize < WallWidth)
		{
			float ConstraintAreaStart = (WallWidth - SmallerWallSize) * 0.5f;
			HolePositionX = ConstraintAreaStart + (SmallerWallSize * 0.5f); // Center within constraint area
		}
		
		UE_LOG(LogTemp, Warning, TEXT("🎯 CENTERED hole (OffsetFromCenter=0): %.1fm wall, hole centered at %.1fm"), 
			WallWidth, HolePositionX);
	}
	else if (SmallerWallSize > 0 && SmallerWallSize < WallWidth)
	{
		// BOUNDARY CONSTRAINT: Center the smaller wall area on the larger wall
		float ConstraintAreaStart = (WallWidth - SmallerWallSize) * 0.5f; // Center the constraint area
		float MinPosition = DoorConfig.Width * 0.5f + 0.5f; // Half hole width + 0.5m margin
		float MaxPosition = SmallerWallSize - (DoorConfig.Width * 0.5f + 0.5f); // Constraint width - half hole width - 0.5m margin
		
		if (MaxPosition > MinPosition)
		{
			FRandomStream Random(FDateTime::Now().GetTicks() + (int32)WallSide * 1000); // Unique seed per wall
			float ConstraintPosition = Random.FRandRange(MinPosition, MaxPosition); // Position within constraint area
			HolePositionX = ConstraintAreaStart + ConstraintPosition; // Offset to centered constraint area
			UE_LOG(LogTemp, Warning, TEXT("🎯 BOUNDARY ALIGNED: %.1fm wall, %.1fm constraint centered at %.1f-%.1f, hole at %.1fm"), 
				WallWidth, SmallerWallSize, ConstraintAreaStart, ConstraintAreaStart + SmallerWallSize, HolePositionX);
		}
		else
		{
			HolePositionX = WallWidth * 0.5f; // Fallback to wall center
		}
	}
	else if (EffectiveWallWidth >= 5.0f)
	{
		// Random positioning on larger walls without constraint
		float MinPosition = DoorConfig.Width * 0.5f + 0.5f; // Half hole width + 0.5m margin
		float MaxPosition = EffectiveWallWidth - (DoorConfig.Width * 0.5f + 0.5f); // Wall width - half hole width - 0.5m margin
		if (MaxPosition > MinPosition)
		{
			FRandomStream Random(FDateTime::Now().GetTicks() + (int32)WallSide * 1000); // Unique seed per wall
			HolePositionX = Random.FRandRange(MinPosition, MaxPosition);
			UE_LOG(LogTemp, Warning, TEXT("🎲 Random thick doorway position: %.1fm wall, hole at %.1fm (range %.1f-%.1f)"), 
				WallWidth, HolePositionX, MinPosition, MaxPosition);
		}
		else
		{
			HolePositionX = EffectiveWallWidth * 0.5f; // Fallback to center if math doesn't work
		}
	}
	else
	{
		HolePositionX = EffectiveWallWidth * 0.5f; // Center position for smaller walls
		UE_LOG(LogTemp, Warning, TEXT("🎯 Centered thick doorway: %.1fm wall (constrained to %.1fm), hole at center %.1fm"), 
			WallWidth, EffectiveWallWidth, HolePositionX);
	}

	// Create hole config from DoorConfig (same as TestGenerator approach)
	FWallHoleConfig HoleConfig = FWallHoleConfig::CreateCustom(
		DoorConfig.Width, DoorConfig.Height,
		HolePositionX, DoorConfig.Height * 0.5f, // Random/centered X, bottom-align vertically
		FString::Printf(TEXT("%sThickWallHole"), *UEnum::GetValueAsString(WallSide))
	);
	HoleConfig.Shape = EHoleShape::Rectangle;
	
	// UE_LOG(LogTemp, Warning, TEXT("🔧 AddHoleToWallWithThickness: Creating %s wall with %.1fx%.1fm hole, %.1fm thickness"), 
		// *UEnum::GetValueAsString(WallSide), DoorConfig.Width, DoorConfig.Height, CustomThickness);
		
	// Create wall with hole using custom thickness
	AActor* NewWallActor = UWallUnit::CreateWallWithHole(World, WallPos, WallRot,
		WallWidth, WallHeight, CustomThickness, WallColor, HoleConfig);
	
	// Store the new wall actor
	if (NewWallActor)
	{
		WallActors.Add(WallSide, NewWallActor);
		UE_LOG(LogTemp, Warning, TEXT("✅ AddHoleToWallWithThickness: Successfully created thick %s wall with hole"), 
			*UEnum::GetValueAsString(WallSide));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("❌ AddHoleToWallWithThickness: Failed to create thick %s wall with hole"), 
			*UEnum::GetValueAsString(WallSide));
	}
}

void UStandardRoom::CreateRoomNumberText(int32 RoomIndex, bool bShowNumbers)
{
	// Skip creation if disabled
	if (!bShowNumbers)
	{
		UE_LOG(LogTemp, Log, TEXT("📝 Room number labels disabled - skipping room %d"), RoomIndex);
		return;
	}

	// Create room number text above the room center
	FVector RoomCenter = Position + FVector(Width * 100.0f * 0.5f, Length * 100.0f * 0.5f, Height * 100.0f * 0.5f);
	FVector NumberPosition = RoomCenter + FVector(0, 0, 150.0f); // 1.5m above room
	
	UE_LOG(LogTemp, Log, TEXT("📝 Creating billboard room number %d at %s"), RoomIndex, *NumberPosition.ToString());
	
	// Create billboard text actor
	UWorld* World = GWorld;
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("❌ No valid world for room number text"));
		return;
	}
	
	// Spawn the custom billboard text actor
	ABillboardTextActor* BillboardActor = World->SpawnActor<ABillboardTextActor>(NumberPosition, FRotator::ZeroRotator);
	if (!IsValid(BillboardActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("❌ Failed to create billboard text actor"));
		return;
	}
	
	// Configure the billboard text
	FString RoomNumberText = FString::Printf(TEXT("%d"), RoomIndex);
	BillboardActor->SetText(RoomNumberText);
	BillboardActor->SetTextSize(200.0f); // Big size for visibility
	BillboardActor->SetTextColor(FColor::White); // White for contrast
	
	UE_LOG(LogTemp, Log, TEXT("✅ Created billboard room number label: %s at %s (updates every 2s)"), *RoomNumberText, *NumberPosition.ToString());
}