#include "TestGenerator.h"
#include "RoomUnit/BaseRoom.h"
#include "RoomUnit/StandardRoom.h"
#include "WallUnit/WallUnit.h"
#include "WallUnit/HoleGenerator.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "Components/TextRenderComponent.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "DrawDebugHelpers.h"

DEFINE_LOG_CATEGORY(LogTestGenerator);

ATestGenerator::ATestGenerator()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ATestGenerator::BeginPlay()
{
	Super::BeginPlay();
	
	// Delay generation to avoid resource conflicts
	if (UWorld* World = GetWorld())
	{
		FTimerHandle TimerHandle;
		World->GetTimerManager().SetTimer(TimerHandle, [this]()
		{
			if (IsValid(this))
			{
				GenerateBackroomsInTestMode(); // ENABLED - boundary test mode
			}
		}, 0.1f, false);
	}
}

void ATestGenerator::GenerateBackroomsInTestMode()
{
	// Safety check
	if (!IsValid(this) || !GetWorld())
	{
		DebugLog(TEXT("❌ TestGenerator not valid or world not found"));
		return;
	}
	
	// Clear any existing rooms
	RoomUnits.Empty();
	GeneratedRooms.Empty();
	
	DebugLog(TEXT("=== CONNECTION TEST MODE: Room to Hallway Connection ==="));
	
	// Generate the room to hallway connection test
	TestRoomToHallwayConnection();
	
	// Keep character at current location to view the boundary test rooms
	DebugLog(TEXT("✅ Character kept at original location to view boundary test rooms"));
	
	DebugLog(TEXT("✅ Wall test grid generation complete"));
}

void ATestGenerator::GenerateWallTestGrid()
{
	// Generate selected demonstration rows - ROOM DEMO MODE (1, 9, 10, 11) + WALL DEMO (2, 3, 4)
	CreateRowCompleteRoomAssembly();     // ROW 1: Complete rooms - most complex
	CreateRowPitchRotationWalls();       // ROW 2: Pitch rotation tests  
	CreateRowYawRotationWalls();         // ROW 3: Yaw rotation tests
	CreateRowRollRotationWalls();        // ROW 4: Roll rotation tests
	CreateRowDefaultSizingWalls();       // ROW 5: Default sizing tests
	CreateRowCustomPositioningWalls();   // ROW 6: Custom positioning tests
	CreateRowIrregularHolesWalls();      // ROW 7: Irregular holes tests
	CreateRowSpecificShapesWalls();      // ROW 8: Specific shapes tests
	CreateRowStandardRoomUnits();        // ROW 9: Multiple StandardRoom units
	CreateRowModifiableUnits();          // ROW 11: New modifiable units row
	
	DebugLog(TEXT("✅ FULL DEMO MODE: Created all 11 demonstration rows (1-11) - complete rooms + walls with unified positioning"));
}

void ATestGenerator::DebugLog(const FString& Message) const
{
	// Generate timestamp with milliseconds
	FDateTime Now = FDateTime::Now();
	FString TimeStamp = FString::Printf(TEXT("[%02d:%02d:%02d.%03d]"),
		Now.GetHour(), Now.GetMinute(), Now.GetSecond(), Now.GetMillisecond());
	
	UE_LOG(LogTestGenerator, Log, TEXT("%s %s"), *TimeStamp, *Message);
}

void ATestGenerator::CreateRowLabel(const FVector& Position, const FString& LabelText)
{
	// Safety checks
	if (!IsValid(this) || !GetWorld())
	{
		return;
	}
	
	// Create a text render component actor for the label
	AActor* LabelActor = GetWorld()->SpawnActor<AActor>();
	if (!IsValid(LabelActor))
	{
		DebugLog(TEXT("❌ Failed to create label actor"));
		return;
	}
	UTextRenderComponent* TextRender = NewObject<UTextRenderComponent>(LabelActor);
	LabelActor->SetRootComponent(TextRender);
	
	// Configure the text
	TextRender->SetText(FText::FromString(LabelText));
	TextRender->SetWorldSize(60.0f); // Bigger text for better visibility
	TextRender->SetTextRenderColor(FColor::Yellow); // More visible color
	TextRender->SetHorizontalAlignment(EHTA_Left);
	TextRender->SetVerticalAlignment(EVRTA_TextCenter);
	// Use default font (no need to set specific font)
	
	// Position and orient the text
	LabelActor->SetActorLocation(Position);
	LabelActor->SetActorRotation(FRotator(0.0f, 180.0f, 0.0f)); // Turn 180° to face character
	
	// Register the component
	TextRender->RegisterComponent();
	
	DebugLog(FString::Printf(TEXT("Created label: %s at %s"), *LabelText, *Position.ToString()));
}

void ATestGenerator::CreateRowLabel(const FVector& Position, const FString& LabelText, float YawRotation)
{
	// Safety checks
	if (!IsValid(this) || !GetWorld())
	{
		return;
	}
	
	// Create a text render component actor for the label
	AActor* LabelActor = GetWorld()->SpawnActor<AActor>();
	if (!IsValid(LabelActor))
	{
		DebugLog(TEXT("❌ Failed to create label actor"));
		return;
	}
	UTextRenderComponent* TextRender = NewObject<UTextRenderComponent>(LabelActor);
	LabelActor->SetRootComponent(TextRender);
	
	// Configure the text
	TextRender->SetText(FText::FromString(LabelText));
	TextRender->SetWorldSize(60.0f); // Bigger text for better visibility
	TextRender->SetTextRenderColor(FColor::Yellow); // More visible color
	TextRender->SetHorizontalAlignment(EHTA_Left);
	TextRender->SetVerticalAlignment(EVRTA_TextCenter);
	// Use default font (no need to set specific font)
	
	// Position and orient the text with custom rotation
	LabelActor->SetActorLocation(Position);
	LabelActor->SetActorRotation(FRotator(0.0f, 180.0f + YawRotation, 0.0f)); // Turn 180° + custom rotation to face character
	
	// Register the component
	TextRender->RegisterComponent();
	
	DebugLog(FString::Printf(TEXT("Created rotated label (%.1f°): %s at %s"), YawRotation, *LabelText, *Position.ToString()));
}

FVector ATestGenerator::CalculateLabelPosition(float RowX, float RowY) const
{
	// === UNIFIED LABEL POSITIONING SYSTEM ===
	// Character spawn: X = -4000.0f, Y = 2000.0f
	// FRONT = toward character (negative X direction)
	// LEFT = negative Y direction
	
	float CharacterX = -4000.0f;
	float LabelOffsetTowardCharacter = 500.0f; // Distance label moves toward character from row
	float LabelOffsetLeft = 500.0f; // Distance label moves to the left of row center
	
	// Calculate label position relative to row position
	float LabelX = RowX - LabelOffsetTowardCharacter; // Move toward character (FRONT)
	float LabelY = RowY - LabelOffsetLeft; // Move to the left of row
	
	return FVector(LabelX, LabelY, -150.0f); // Standard Z height for all labels
}

void ATestGenerator::CreateMultipleHolesTestWalls()
{
	// Create comprehensive demonstration using NEW CARTESIAN POSITIONING SYSTEM
	// Wall dimensions: 4m × 3m (easier for cartesian coordinates)
	
	// Array of wall colors for variety
	TArray<FLinearColor> HoleWallColors = {
		FLinearColor::Red,
		FLinearColor::Green,
		FLinearColor::Blue,
		FLinearColor::Yellow,
		FLinearColor(1.0f, 0.0f, 1.0f, 1.0f), // Magenta
		FLinearColor(0.0f, 1.0f, 1.0f, 1.0f), // Cyan
		FLinearColor(1.0f, 0.5f, 0.0f, 1.0f), // Orange
		FLinearColor::White,
		FLinearColor(0.5f, 0.5f, 0.5f, 1.0f), // Gray
		FLinearColor(1.0f, 0.75f, 0.8f, 1.0f)  // Pink
	};
	
	// Wall 1: DEFAULT DOORWAY using "custom" type (centered on 4m wall)
	FWallHoleConfig DefaultDoor = FWallHoleConfig::CreateCustom(0.8f, 2.0f, 2.0f, 1.0f, TEXT("DefaultDoorway"));
	UWallUnit::CreateWallWithHole(GetWorld(), FVector(0, 1800.0f, -220), FRotator::ZeroRotator, 
		4.0f, 3.0f, 0.2f, HoleWallColors[0], DefaultDoor);
	DebugLog(TEXT("✅ Default doorway (custom type: center at 2.0m, 1.0m)"));
	
	// Wall 2: CUSTOM - Bottom-left area (X=0.8m, Y=0.5m) - center of hole 
	FWallHoleConfig BottomLeft = FWallHoleConfig::CreateCustom(0.6f, 1.0f, 0.8f, 0.5f, TEXT("BottomLeft"));
	UWallUnit::CreateWallWithHole(GetWorld(), FVector(500.0f, 1800.0f, -220), FRotator::ZeroRotator, 
		4.0f, 3.0f, 0.2f, HoleWallColors[1], BottomLeft);
	DebugLog(TEXT("✅ Custom: Bottom-left area (center at 0.8m, 0.5m)"));
	
	// Wall 3: CUSTOM - Top-right area (X=3.0m, Y=2.2m) - center of hole
	FWallHoleConfig TopRight = FWallHoleConfig::CreateCustom(0.8f, 0.8f, 3.0f, 2.2f, TEXT("TopRight"));
	UWallUnit::CreateWallWithHole(GetWorld(), FVector(1000.0f, 1800.0f, -220), FRotator::ZeroRotator, 
		4.0f, 3.0f, 0.2f, HoleWallColors[2], TopRight);
	DebugLog(TEXT("✅ Custom: Top-right area (center at 3.0m, 2.2m)"));
	
	// Wall 4: CUSTOM - Left middle (X=0.5m, Y=1.5m) - center of hole
	FWallHoleConfig LeftMiddle = FWallHoleConfig::CreateCustom(0.4f, 1.0f, 0.5f, 1.5f, TEXT("LeftMiddle"));
	UWallUnit::CreateWallWithHole(GetWorld(), FVector(1500.0f, 1800.0f, -220), FRotator::ZeroRotator, 
		4.0f, 3.0f, 0.2f, HoleWallColors[3], LeftMiddle);
	DebugLog(TEXT("✅ Custom: Left middle (center at 0.5m, 1.5m)"));
	
	// Wall 5: CUSTOM - Exact center (X=2.0m, Y=1.5m) on 4×3 wall
	FWallHoleConfig ExactCenter = FWallHoleConfig::CreateCustom(1.0f, 1.0f, 2.0f, 1.5f, TEXT("ExactCenter"));
	UWallUnit::CreateWallWithHole(GetWorld(), FVector(2000.0f, 1800.0f, -220), FRotator::ZeroRotator, 
		4.0f, 3.0f, 0.2f, HoleWallColors[4], ExactCenter);
	DebugLog(TEXT("✅ Custom: Exact center (center at 2.0m, 1.5m)"));
	
	// Wall 6: CUSTOM - Right middle (X=3.5m, Y=1.5m) - center of hole
	FWallHoleConfig RightMiddle = FWallHoleConfig::CreateCustom(0.6f, 0.8f, 3.5f, 1.5f, TEXT("RightMiddle"));
	UWallUnit::CreateWallWithHole(GetWorld(), FVector(2500.0f, 1800.0f, -220), FRotator::ZeroRotator, 
		4.0f, 3.0f, 0.2f, HoleWallColors[5], RightMiddle);
	DebugLog(TEXT("✅ Custom: Right middle (center at 3.5m, 1.5m)"));
	
	// Wall 7: CUSTOM - Top horizontal slot (X=2.0m, Y=2.5m) - center of hole
	FWallHoleConfig TopSlot = FWallHoleConfig::CreateCustom(2.0f, 0.3f, 2.0f, 2.5f, TEXT("TopSlot"));
	UWallUnit::CreateWallWithHole(GetWorld(), FVector(3000.0f, 1800.0f, -220), FRotator::ZeroRotator, 
		4.0f, 3.0f, 0.2f, HoleWallColors[6], TopSlot);
	DebugLog(TEXT("✅ Custom: Top horizontal slot (center at 2.0m, 2.5m)"));
	
	// Wall 8: CUSTOM - Right vertical slot (X=3.5m, Y=1.0m) - center of hole  
	FWallHoleConfig RightSlot = FWallHoleConfig::CreateCustom(0.2f, 2.0f, 3.5f, 1.0f, TEXT("RightSlot"));
	UWallUnit::CreateWallWithHole(GetWorld(), FVector(3500.0f, 1800.0f, -220), FRotator::ZeroRotator, 
		4.0f, 3.0f, 0.2f, HoleWallColors[7], RightSlot);
	DebugLog(TEXT("✅ Custom: Right vertical slot (center at 3.5m, 1.0m)"));
	
	// Wall 9: CUSTOM - Top-left peephole (X=0.5m, Y=2.5m) - center of hole
	FWallHoleConfig TopLeftPeephole = FWallHoleConfig::CreateCustom(0.2f, 0.2f, 0.5f, 2.5f, TEXT("TopLeftPeephole"));
	UWallUnit::CreateWallWithHole(GetWorld(), FVector(4000.0f, 1800.0f, -220), FRotator::ZeroRotator, 
		4.0f, 3.0f, 0.2f, HoleWallColors[8], TopLeftPeephole);
	DebugLog(TEXT("✅ Custom: Top-left peephole (center at 0.5m, 2.5m)"));
	
	// Wall 10: CUSTOM - Off-center large opening (X=2.5m, Y=1.0m) - center of hole
	FWallHoleConfig OffCenterLarge = FWallHoleConfig::CreateCustom(1.5f, 1.5f, 2.5f, 1.0f, TEXT("OffCenterLarge"));
	UWallUnit::CreateWallWithHole(GetWorld(), FVector(4500.0f, 1800.0f, -220), FRotator::ZeroRotator, 
		4.0f, 3.0f, 0.2f, HoleWallColors[9], OffCenterLarge);
	DebugLog(TEXT("✅ Custom: Off-center large opening (center at 2.5m, 1.0m)"));
	
	DebugLog(TEXT("✅ NEW CUSTOM POSITIONING SYSTEM demonstrated (10 walls: all using 'custom' type with precise coordinates)"));
}

// === INDIVIDUAL ROW GENERATION FUNCTIONS ===

void ATestGenerator::CreateRowPitchRotationWalls()
{
	// === SHARED ROW POSITIONING (same as other rows) ===
	float SharedRowStartX = -5000.0f; // Both rows start at same X position for alignment
	float RowSeparationDistance = 1700.0f; // Distance between room rows (Y-axis)
	float WallRowSeparationDistance = 800.0f; // Distance between wall rows (smaller, closer together)
	
	float BaseY = -2.0f * RowSeparationDistance; // Row 2 position - after Row 1, closer spacing (-3400.0f)
	
	// Create row label using unified positioning system (same as other rows)
	FVector LabelPosition2 = CalculateLabelPosition(SharedRowStartX, BaseY);
	CreateRowLabel(LabelPosition2, TEXT("Row 2: Solid Walls + Pitch Rotations (0° to 315°)"), -30.0f); // -30° rotation for wall row
	
	// Array of wall colors for variety
	TArray<FLinearColor> WallColors = {
		FLinearColor::Red,
		FLinearColor::Green,
		FLinearColor::Blue,
		FLinearColor::Yellow,
		FLinearColor(1.0f, 0.0f, 1.0f, 1.0f), // Magenta
		FLinearColor(0.0f, 1.0f, 1.0f, 1.0f), // Cyan
		FLinearColor(1.0f, 0.5f, 0.0f, 1.0f), // Orange
		FLinearColor::White
	};
	
	// Row 2: Create walls with doorways and progressive pitch rotations
	for (int32 i = 0; i < 8; i++)
	{
		float XOffset = i * 800.0f; // 8m spacing along X-axis (same as other rows)
		FVector Position = FVector(SharedRowStartX + XOffset, BaseY, -220); // Use shared positioning system
		float PitchAngle = i * 45.0f; // 0°, 45°, 90°, 135°, 180°, 225°, 270°, 315°
		FRotator Rotation = FRotator(PitchAngle, 0, 0); // Pitch rotation only
		
		// Create solid walls to demonstrate pitch rotations
		UWallUnit::CreateSolidWallActor(GetWorld(), Position, Rotation, 3.0f, 3.0f, 0.2f, WallColors[i]);
		
		DebugLog(FString::Printf(TEXT("✅ Row 2: Created solid wall with %.1f° pitch rotation at %s"), 
			PitchAngle, *Position.ToString()));
	}
}

void ATestGenerator::CreateRowYawRotationWalls()
{
	// === SHARED ROW POSITIONING (same as other rows) ===
	float SharedRowStartX = -5000.0f; // Both rows start at same X position for alignment
	float RowSeparationDistance = 1700.0f; // Distance between room rows (Y-axis)
	float WallRowSeparationDistance = 800.0f; // Distance between wall rows (smaller, closer together)
	
	float BaseY = -2.0f * RowSeparationDistance - 1.0f * WallRowSeparationDistance; // Row 3 position - after Row 2, closer spacing (-4200.0f)
	
	// Create row label using unified positioning system (same as other rows)
	FVector LabelPosition3 = CalculateLabelPosition(SharedRowStartX, BaseY);
	CreateRowLabel(LabelPosition3, TEXT("Row 3: Solid Walls + Yaw Rotations (0° to 315°)"), -30.0f); // -30° rotation for wall row
	
	// Array of wall colors for variety
	TArray<FLinearColor> WallColors = {
		FLinearColor::Red,
		FLinearColor::Green,
		FLinearColor::Blue,
		FLinearColor::Yellow,
		FLinearColor(1.0f, 0.0f, 1.0f, 1.0f), // Magenta
		FLinearColor(0.0f, 1.0f, 1.0f, 1.0f), // Cyan
		FLinearColor(1.0f, 0.5f, 0.0f, 1.0f), // Orange
		FLinearColor::White
	};
	
	// Row 3: Create solid walls with yaw rotations
	for (int32 i = 0; i < 8; i++)
	{
		float XOffset = i * 800.0f; // 8m spacing along X-axis (same as other rows)
		FVector Position = FVector(SharedRowStartX + XOffset, BaseY, -220); // Use shared positioning system
		float YawAngle = i * 45.0f; // 0°, 45°, 90°, 135°, 180°, 225°, 270°, 315°
		FRotator Rotation = FRotator(0, YawAngle, 0); // Yaw rotation only
		
		UWallUnit::CreateSolidWallActor(GetWorld(), Position, Rotation, 3.0f, 3.0f, 0.2f, WallColors[i]);
		
		DebugLog(FString::Printf(TEXT("✅ Row 3: Created solid wall with %.1f° yaw rotation at %s"), 
			YawAngle, *Position.ToString()));
	}
}

void ATestGenerator::CreateRowRollRotationWalls()
{
	// === SHARED ROW POSITIONING (same as other rows) ===
	float SharedRowStartX = -5000.0f; // Both rows start at same X position for alignment
	float RowSeparationDistance = 1700.0f; // Distance between room rows (Y-axis)
	float WallRowSeparationDistance = 800.0f; // Distance between wall rows (smaller, closer together)
	
	float BaseY = -2.0f * RowSeparationDistance - 2.0f * WallRowSeparationDistance; // Row 4 position - after Row 3, closer spacing (-5000.0f)
	
	// Create row label using unified positioning system (same as other rows)
	FVector LabelPosition4 = CalculateLabelPosition(SharedRowStartX, BaseY);
	CreateRowLabel(LabelPosition4, TEXT("Row 4: Solid Walls + Roll Rotations (0° to 315°)"), -30.0f); // -30° rotation for wall row
	
	// Array of wall colors for variety
	TArray<FLinearColor> WallColors = {
		FLinearColor::Red,
		FLinearColor::Green,
		FLinearColor::Blue,
		FLinearColor::Yellow,
		FLinearColor(1.0f, 0.0f, 1.0f, 1.0f), // Magenta
		FLinearColor(0.0f, 1.0f, 1.0f, 1.0f), // Cyan
		FLinearColor(1.0f, 0.5f, 0.0f, 1.0f), // Orange
		FLinearColor::White
	};
	
	// Row 4: Create solid walls with roll rotations
	for (int32 i = 0; i < 8; i++)
	{
		float XOffset = i * 800.0f; // 8m spacing along X-axis (same as other rows)
		FVector Position = FVector(SharedRowStartX + XOffset, BaseY, -220); // Use shared positioning system
		float RollAngle = i * 45.0f; // 0°, 45°, 90°, 135°, 180°, 225°, 270°, 315°
		FRotator Rotation = FRotator(0, 0, RollAngle); // Roll rotation only (Z-axis)
		
		UWallUnit::CreateSolidWallActor(GetWorld(), Position, Rotation, 3.0f, 3.0f, 0.2f, WallColors[i]);
		
		DebugLog(FString::Printf(TEXT("✅ Row 4: Created solid wall with %.1f° roll rotation at %s"), 
			RollAngle, *Position.ToString()));
	}
}

void ATestGenerator::CreateRowDefaultSizingWalls()
{
	// === SHARED ROW POSITIONING (same as other rows) ===
	float SharedRowStartX = -5000.0f; // Both rows start at same X position for alignment
	float RowSeparationDistance = 1700.0f; // Distance between room rows (Y-axis)
	float WallRowSeparationDistance = 800.0f; // Distance between wall rows (smaller, closer together)
	
	float BaseY = -2.0f * RowSeparationDistance - 3.0f * WallRowSeparationDistance; // Row 5 position - after Row 4, closer spacing (-5800.0f)
	
	// Create row label using unified positioning system (same as other rows)
	FVector LabelPosition5 = CalculateLabelPosition(SharedRowStartX, BaseY);
	CreateRowLabel(LabelPosition5, TEXT("Row 5: Default Positioning + Different Sizes"), -30.0f); // -30° rotation for wall row
	
	// Row 4: Default positioning system - same hole position, different sizes
	
	// Wall 1: Small doorway - rectangle shape for proper rectangular holes
	FWallHoleConfig SmallDoor(0.6f, 1.8f, TEXT("SmallDoor"));
	SmallDoor.Shape = EHoleShape::Rectangle; // Explicitly set to Rectangle
			float XOffset = 0 * 800.0f; // 8m spacing along X-axis (same as other rows)
		FVector Position1 = FVector(SharedRowStartX + XOffset, BaseY, -220); // Use shared positioning system
	UWallUnit::CreateWallWithHole(GetWorld(), Position1, FRotator::ZeroRotator, 
		3.0f, 3.0f, 0.2f, FLinearColor::Red, SmallDoor);
	DebugLog(TEXT("✅ Row 4: Created small doorway (default type, custom size)"));
	
	// Wall 2: Standard doorway - rectangle shape
	FWallHoleConfig StandardDoor(0.8f, 2.0f, TEXT("StandardDoor"));
	StandardDoor.Shape = EHoleShape::Rectangle;
		XOffset = 1 * 800.0f;
		FVector Position2 = FVector(SharedRowStartX + XOffset, BaseY, -220);
	UWallUnit::CreateWallWithHole(GetWorld(), Position2, FRotator::ZeroRotator, 
		3.0f, 3.0f, 0.2f, FLinearColor::Green, StandardDoor);
	DebugLog(TEXT("✅ Row 4: Created standard doorway (default type, standard size)"));
	
	// Wall 3: Wide doorway - rectangle shape
	FWallHoleConfig WideDoor(1.2f, 2.2f, TEXT("WideDoor"));
	WideDoor.Shape = EHoleShape::Rectangle;
		XOffset = 2 * 800.0f;
		FVector Position3 = FVector(SharedRowStartX + XOffset, BaseY, -220);
	UWallUnit::CreateWallWithHole(GetWorld(), Position3, FRotator::ZeroRotator, 
		3.0f, 3.0f, 0.2f, FLinearColor::Blue, WideDoor);
	DebugLog(TEXT("✅ Row 4: Created wide doorway (default type, custom size)"));
	
	// Wall 4: High window - rectangle shape
	FWallHoleConfig HighWindow(1.0f, 0.8f, TEXT("HighWindow"));
	HighWindow.Shape = EHoleShape::Rectangle;
		XOffset = 3 * 800.0f;
		FVector Position4 = FVector(SharedRowStartX + XOffset, BaseY, -220);
	UWallUnit::CreateWallWithHole(GetWorld(), Position4, FRotator::ZeroRotator, 
		3.0f, 3.0f, 0.2f, FLinearColor::Yellow, HighWindow);
	DebugLog(TEXT("✅ Row 4: Created high window (default type, same position)"));
	
	// Wall 5: Mid-level window - rectangle shape
	FWallHoleConfig CenterWindow(1.0f, 1.0f, TEXT("CenterWindow"));
	CenterWindow.Shape = EHoleShape::Rectangle;
		XOffset = 4 * 800.0f;
		FVector Position5 = FVector(SharedRowStartX + XOffset, BaseY, -220);
	UWallUnit::CreateWallWithHole(GetWorld(), Position5, FRotator::ZeroRotator, 
		3.0f, 3.0f, 0.2f, FLinearColor(1.0f, 0.0f, 1.0f, 1.0f), CenterWindow);
	DebugLog(TEXT("✅ Row 4: Created center window (default type, same position)"));
	
	// Wall 6: Extra wide doorway - rectangle shape
	FWallHoleConfig ExtraWide(1.4f, 2.4f, TEXT("ExtraWide"));
	ExtraWide.Shape = EHoleShape::Rectangle;
		XOffset = 5 * 800.0f;
		FVector Position6 = FVector(SharedRowStartX + XOffset, BaseY, -220);
	UWallUnit::CreateWallWithHole(GetWorld(), Position6, FRotator::ZeroRotator, 
		3.0f, 3.0f, 0.2f, FLinearColor(0.0f, 1.0f, 1.0f, 1.0f), ExtraWide);
	DebugLog(TEXT("✅ Row 4: Created extra wide doorway (default type, same position)"));
}

void ATestGenerator::CreateRowCustomPositioningWalls()
{
	// === SHARED ROW POSITIONING (same as other rows) ===
	float SharedRowStartX = -5000.0f; // Both rows start at same X position for alignment
	float RowSeparationDistance = 1700.0f; // Distance between room rows (Y-axis)
	float WallRowSeparationDistance = 800.0f; // Distance between wall rows (smaller, closer together)
	
	float BaseY = -2.0f * RowSeparationDistance - 4.0f * WallRowSeparationDistance; // Row 6 position - after Row 5, closer spacing (-6600.0f)
	
	// Create row label using unified positioning system (same as other rows)
	FVector LabelPosition6 = CalculateLabelPosition(SharedRowStartX, BaseY);
	CreateRowLabel(LabelPosition6, TEXT("Row 6: Custom Cartesian Positioning"), -30.0f); // -30° rotation for wall row
	
	// Row 6: Custom positioning system - different positions, various sizes
	// Array of wall colors for variety
	TArray<FLinearColor> HoleWallColors = {
		FLinearColor::Red,
		FLinearColor::Green,
		FLinearColor::Blue,
		FLinearColor::Yellow,
		FLinearColor(1.0f, 0.0f, 1.0f, 1.0f), // Magenta
		FLinearColor(0.0f, 1.0f, 1.0f, 1.0f), // Cyan
		FLinearColor(1.0f, 0.5f, 0.0f, 1.0f), // Orange
		FLinearColor::White
	};
	
	// Wall 1: Default doorway using custom positioning
	FWallHoleConfig DefaultDoor = FWallHoleConfig::CreateCustom(0.8f, 2.0f, 2.0f, 1.0f, TEXT("DefaultDoorway"));
	float XOffset = 0 * 800.0f;
	FVector Position1 = FVector(SharedRowStartX + XOffset, BaseY, -220);
	UWallUnit::CreateWallWithHole(GetWorld(), Position1, FRotator::ZeroRotator, 
		4.0f, 3.0f, 0.2f, HoleWallColors[0], DefaultDoor);
	
	// Wall 2: Bottom-left area hole
	FWallHoleConfig BottomLeft = FWallHoleConfig::CreateCustom(0.6f, 1.0f, 0.8f, 0.5f, TEXT("BottomLeft"));
	XOffset = 1 * 800.0f;
	FVector Position2 = FVector(SharedRowStartX + XOffset, BaseY, -220);
	UWallUnit::CreateWallWithHole(GetWorld(), Position2, FRotator::ZeroRotator, 
		4.0f, 3.0f, 0.2f, HoleWallColors[1], BottomLeft);
	
	// Wall 3: Top-right area hole
	FWallHoleConfig TopRight = FWallHoleConfig::CreateCustom(0.8f, 0.8f, 3.0f, 2.2f, TEXT("TopRight"));
	XOffset = 2 * 800.0f;
	FVector Position3 = FVector(SharedRowStartX + XOffset, BaseY, -220);
	UWallUnit::CreateWallWithHole(GetWorld(), Position3, FRotator::ZeroRotator, 
		4.0f, 3.0f, 0.2f, HoleWallColors[2], TopRight);
	
	// Wall 4: Left middle hole
	FWallHoleConfig LeftMiddle = FWallHoleConfig::CreateCustom(0.4f, 1.0f, 0.5f, 1.5f, TEXT("LeftMiddle"));
	XOffset = 3 * 800.0f;
	FVector Position4 = FVector(SharedRowStartX + XOffset, BaseY, -220);
	UWallUnit::CreateWallWithHole(GetWorld(), Position4, FRotator::ZeroRotator, 
		4.0f, 3.0f, 0.2f, HoleWallColors[3], LeftMiddle);
	
	// Wall 5: Exact center hole
	FWallHoleConfig ExactCenter = FWallHoleConfig::CreateCustom(1.0f, 1.0f, 2.0f, 1.5f, TEXT("ExactCenter"));
	XOffset = 4 * 800.0f;
	FVector Position5 = FVector(SharedRowStartX + XOffset, BaseY, -220);
	UWallUnit::CreateWallWithHole(GetWorld(), Position5, FRotator::ZeroRotator, 
		4.0f, 3.0f, 0.2f, HoleWallColors[4], ExactCenter);
	
	// Wall 6: Right middle hole
	FWallHoleConfig RightMiddle = FWallHoleConfig::CreateCustom(0.6f, 0.8f, 3.5f, 1.5f, TEXT("RightMiddle"));
	XOffset = 5 * 800.0f;
	FVector Position6 = FVector(SharedRowStartX + XOffset, BaseY, -220);
	UWallUnit::CreateWallWithHole(GetWorld(), Position6, FRotator::ZeroRotator, 
		4.0f, 3.0f, 0.2f, HoleWallColors[5], RightMiddle);
	
	DebugLog(TEXT("✅ Row 6: Created 6 custom positioning demonstration walls with unified positioning"));
}

void ATestGenerator::CreateRowIrregularHolesWalls()
{
	// === SHARED ROW POSITIONING (same as other rows) ===
	float SharedRowStartX = -5000.0f; // Both rows start at same X position for alignment
	float RowSeparationDistance = 1700.0f; // Distance between room rows (Y-axis)
	float WallRowSeparationDistance = 800.0f; // Distance between wall rows (smaller, closer together)
	
	float BaseY = -2.0f * RowSeparationDistance - 5.0f * WallRowSeparationDistance; // Row 7 position - after Row 6, closer spacing (-7400.0f)
	
	// Create row label using unified positioning system (same as other rows)
	FVector LabelPosition7 = CalculateLabelPosition(SharedRowStartX, BaseY);
	CreateRowLabel(LabelPosition7, TEXT("Row 7: Random Irregular Shapes"), -30.0f); // -30° rotation for wall row
	
	// Row 6: Irregular holes with random shapes - demonstrate size and position control only
	// Wall dimensions: 4m × 3m for consistent testing
	
	// Array of wall colors for variety (bright colors matching other rows)
	TArray<FLinearColor> IrregularWallColors = {
		FLinearColor::Red,
		FLinearColor::Green,
		FLinearColor::Blue,
		FLinearColor::Yellow,
		FLinearColor(1.0f, 0.0f, 1.0f, 1.0f), // Magenta
		FLinearColor(0.0f, 1.0f, 1.0f, 1.0f), // Cyan
		FLinearColor(1.0f, 0.5f, 0.0f, 1.0f), // Orange
		FLinearColor::White
	};
	
	// Wall 1: Small irregular hole - center left
	FWallHoleConfig SmallIrregular = FWallHoleConfig::CreateCustom(0.8f, 0.8f, 1.0f, 1.5f, TEXT("SmallIrregular"));
	SmallIrregular.Shape = EHoleShape::Irregular;
	// Row 7: Create walls with irregular holes and progressive positions
	for (int32 i = 0; i < 8; i++)
	{
		float XOffset = i * 800.0f; // 8m spacing along X-axis (same as other rows)
		FVector Position = FVector(SharedRowStartX + XOffset, BaseY, -220); // Use shared positioning system
		
		// Wall 1: Small irregular hole - center left
		if (i == 0) {
			FWallHoleConfig SmallIrregular = FWallHoleConfig::CreateCustom(0.8f, 0.8f, 1.0f, 1.5f, TEXT("SmallIrregular"));
			SmallIrregular.Shape = EHoleShape::Irregular;
			UWallUnit::CreateWallWithHole(GetWorld(), Position, FRotator::ZeroRotator, 
		4.0f, 3.0f, 0.2f, IrregularWallColors[0], SmallIrregular);
	DebugLog(TEXT("✅ Row 7: Small irregular hole (0.8m, center-left at 1.0m, 1.5m)"));
	
	// Wall 2: Medium irregular hole - bottom center
	FWallHoleConfig MediumIrregular = FWallHoleConfig::CreateCustom(1.2f, 1.2f, 2.0f, 0.8f, TEXT("MediumIrregular"));
	MediumIrregular.Shape = EHoleShape::Irregular;
		} else if (i == 1) {
			FWallHoleConfig MediumIrregular = FWallHoleConfig::CreateCustom(1.2f, 1.2f, 2.0f, 0.8f, TEXT("MediumIrregular"));
			MediumIrregular.Shape = EHoleShape::Irregular;
			UWallUnit::CreateWallWithHole(GetWorld(), Position, FRotator::ZeroRotator, 
		4.0f, 3.0f, 0.2f, IrregularWallColors[1], MediumIrregular);
	DebugLog(TEXT("✅ Row 7: Medium irregular hole (1.2m, bottom-center at 2.0m, 0.8m)"));
	
	// Wall 3: Large irregular hole - top right
	FWallHoleConfig LargeIrregular = FWallHoleConfig::CreateCustom(1.5f, 1.5f, 3.2f, 2.2f, TEXT("LargeIrregular"));
	LargeIrregular.Shape = EHoleShape::Irregular;
	UWallUnit::CreateWallWithHole(GetWorld(), FVector(5200.0f, 1000.0f, -220), FRotator::ZeroRotator, // Moved far from room rows 
		4.0f, 3.0f, 0.2f, IrregularWallColors[2], LargeIrregular);
	DebugLog(TEXT("✅ Row 7: Large irregular hole (1.5m, top-right at 3.2m, 2.2m)"));
	
	// Wall 4: Elongated irregular hole - center horizontal
	FWallHoleConfig ElongatedIrregular = FWallHoleConfig::CreateCustom(2.5f, 0.8f, 2.0f, 1.5f, TEXT("ElongatedIrregular"));
	ElongatedIrregular.Shape = EHoleShape::Irregular;
		} else if (i == 2) {
			FWallHoleConfig LargeIrregular = FWallHoleConfig::CreateCustom(1.5f, 1.5f, 3.2f, 2.2f, TEXT("LargeIrregular"));
			LargeIrregular.Shape = EHoleShape::Irregular;
			UWallUnit::CreateWallWithHole(GetWorld(), Position, FRotator::ZeroRotator, 
		4.0f, 3.0f, 0.2f, IrregularWallColors[2], LargeIrregular);
		DebugLog(TEXT("✅ Row 7: Large irregular hole (1.5m, top-right at 3.2m, 2.2m)"));
		}
	}
	
	DebugLog(TEXT("✅ Row 7: Created 3 irregular holes with unified positioning"));
}

void ATestGenerator::CreateRowSpecificShapesWalls()
{
	// === SHARED ROW POSITIONING (same as other rows) ===
	float SharedRowStartX = -5000.0f; // Both rows start at same X position for alignment
	float RowSeparationDistance = 1700.0f; // Distance between room rows (Y-axis)
	float WallRowSeparationDistance = 800.0f; // Distance between wall rows (smaller, closer together)
	
	float BaseY = -2.0f * RowSeparationDistance - 6.0f * WallRowSeparationDistance; // Row 8 position - after Row 7, closer spacing (-8200.0f)
	
	// Create row label using unified positioning system (same as other rows)
	FVector LabelPosition8 = CalculateLabelPosition(SharedRowStartX, BaseY);
	CreateRowLabel(LabelPosition8, TEXT("Row 8: Specific Shapes (Circle, Triangle, Star, etc.)"), -30.0f); // -30° rotation for wall row
	
	// Row 7: Specific geometric and organic shapes - demonstrate controlled shape generation
	// Wall dimensions: 4m × 3m for consistent testing
	
	// Array of wall colors for variety (bright colors matching other rows)
	TArray<FLinearColor> ShapeWallColors = {
		FLinearColor::Red,
		FLinearColor::Green,
		FLinearColor::Blue,
		FLinearColor::Yellow,
		FLinearColor(1.0f, 0.0f, 1.0f, 1.0f), // Magenta
		FLinearColor(0.0f, 1.0f, 1.0f, 1.0f), // Cyan
		FLinearColor(1.0f, 0.5f, 0.0f, 1.0f), // Orange
		FLinearColor::White
	};
	
	// Wall 1: Perfect Circle - many points, no irregularity, max smoothness
	FWallHoleConfig CircleHole = FWallHoleConfig::CreateCustom(1.5f, 1.5f, 2.0f, 1.5f, TEXT("Circle"));
	CircleHole.Shape = EHoleShape::Irregular;
	// Circle parameters will be set in WallUnit.cpp: 24 points, 0 irregularity, 1.0 smoothness
	float XOffset = 0 * 800.0f;
	FVector Position1 = FVector(SharedRowStartX + XOffset, BaseY, -220);
	UWallUnit::CreateWallWithHole(GetWorld(), Position1, FRotator::ZeroRotator, 
		4.0f, 3.0f, 0.2f, ShapeWallColors[0], CircleHole);
	DebugLog(TEXT("✅ Row 8: Perfect circle (24 points, 0 irregularity, max smoothness)"));
	
	// Wall 2: Triangle - 3 points, low irregularity, low smoothness
	FWallHoleConfig TriangleHole = FWallHoleConfig::CreateCustom(1.4f, 1.4f, 2.0f, 1.5f, TEXT("Triangle"));
	TriangleHole.Shape = EHoleShape::Irregular;
	// Will be configured as: 3 points, 0.1 irregularity, 0.1 smoothness
	XOffset = 1 * 800.0f;
	FVector Position2 = FVector(SharedRowStartX + XOffset, BaseY, -220);
	UWallUnit::CreateWallWithHole(GetWorld(), Position2, FRotator::ZeroRotator, 
		4.0f, 3.0f, 0.2f, ShapeWallColors[1], TriangleHole);
	DebugLog(TEXT("✅ Row 8: Triangle (3 points, sharp edges)"));
	
	// Wall 3: Square/Diamond - 4 points, no irregularity, low smoothness, 45° rotation
	FWallHoleConfig SquareHole = FWallHoleConfig::CreateCustom(1.3f, 1.3f, 2.0f, 1.5f, TEXT("Square"));
	SquareHole.Shape = EHoleShape::Irregular;
	// Will be configured as: 4 points, 0.0 irregularity, 0.2 smoothness, 45° rotation
	XOffset = 2 * 800.0f;
	FVector Position3 = FVector(SharedRowStartX + XOffset, BaseY, -220);
	UWallUnit::CreateWallWithHole(GetWorld(), Position3, FRotator::ZeroRotator, 
		4.0f, 3.0f, 0.2f, ShapeWallColors[2], SquareHole);
	DebugLog(TEXT("✅ Row 8: Diamond/Square (4 points, 45° rotation)"));
	
	// Wall 4: Hexagon - 6 points, no irregularity, medium smoothness
	FWallHoleConfig HexagonHole = FWallHoleConfig::CreateCustom(1.4f, 1.4f, 2.0f, 1.5f, TEXT("Hexagon"));
	HexagonHole.Shape = EHoleShape::Irregular;
	// Will be configured as: 6 points, 0.0 irregularity, 0.5 smoothness
	XOffset = 3 * 800.0f;
	FVector Position4 = FVector(SharedRowStartX + XOffset, BaseY, -220);
	UWallUnit::CreateWallWithHole(GetWorld(), Position4, FRotator::ZeroRotator, 
		4.0f, 3.0f, 0.2f, ShapeWallColors[3], HexagonHole);
	DebugLog(TEXT("✅ Row 8: Hexagon (6 points, geometric precision)"));
	
	// Wall 5: Star - 8 points, medium irregularity, low smoothness
	FWallHoleConfig StarHole = FWallHoleConfig::CreateCustom(1.5f, 1.5f, 2.0f, 1.5f, TEXT("Star"));
	StarHole.Shape = EHoleShape::Irregular;
	// Will be configured as: 8 points, 0.5 irregularity, 0.1 smoothness
	XOffset = 4 * 800.0f;
	FVector Position5 = FVector(SharedRowStartX + XOffset, BaseY, -220);
	UWallUnit::CreateWallWithHole(GetWorld(), Position5, FRotator::ZeroRotator, 
		4.0f, 3.0f, 0.2f, ShapeWallColors[4], StarHole);
	DebugLog(TEXT("✅ Row 8: Star shape (8 points, spiky edges)"));
	
	// Wall 6: Flower - 12 points, medium irregularity, high smoothness
	FWallHoleConfig FlowerHole = FWallHoleConfig::CreateCustom(1.4f, 1.4f, 2.0f, 1.5f, TEXT("Flower"));
	FlowerHole.Shape = EHoleShape::Irregular;
	// Will be configured as: 12 points, 0.4 irregularity, 0.8 smoothness
	XOffset = 5 * 800.0f;
	FVector Position6 = FVector(SharedRowStartX + XOffset, BaseY, -220);
	UWallUnit::CreateWallWithHole(GetWorld(), Position6, FRotator::ZeroRotator, 
		4.0f, 3.0f, 0.2f, ShapeWallColors[5], FlowerHole);
	DebugLog(TEXT("✅ Row 8: Flower shape (12 points, smooth organic curves)"));
	
	// Wall 7: Blob - 10 points, high irregularity, high smoothness
	FWallHoleConfig BlobHole = FWallHoleConfig::CreateCustom(1.5f, 1.5f, 2.0f, 1.5f, TEXT("Blob"));
	BlobHole.Shape = EHoleShape::Irregular;
	// Will be configured as: 10 points, 0.8 irregularity, 0.9 smoothness
	XOffset = 6 * 800.0f;
	FVector Position7 = FVector(SharedRowStartX + XOffset, BaseY, -220);
	UWallUnit::CreateWallWithHole(GetWorld(), Position7, FRotator::ZeroRotator, 
		4.0f, 3.0f, 0.2f, ShapeWallColors[6], BlobHole);
	DebugLog(TEXT("✅ Row 8: Organic blob (high chaos, very smooth)"));
	
	// Wall 8: Crystal/Jagged - 6 points, medium irregularity, no smoothness
	FWallHoleConfig CrystalHole = FWallHoleConfig::CreateCustom(1.3f, 1.3f, 2.0f, 1.5f, TEXT("Crystal"));
	CrystalHole.Shape = EHoleShape::Irregular;
	// Will be configured as: 6 points, 0.6 irregularity, 0.0 smoothness
	XOffset = 7 * 800.0f;
	FVector Position8 = FVector(SharedRowStartX + XOffset, BaseY, -220);
	UWallUnit::CreateWallWithHole(GetWorld(), Position8, FRotator::ZeroRotator, 
		4.0f, 3.0f, 0.2f, ShapeWallColors[7], CrystalHole);
	DebugLog(TEXT("✅ Row 8: Crystal/Jagged (sharp angular edges)"));
	
	DebugLog(TEXT("✅ Row 8: Created 8 specific shapes: Circle, Triangle, Diamond, Hexagon, Star, Flower, Blob, Crystal"));
}

void ATestGenerator::CreateRowCompleteRoomAssembly()
{
	// === COORDINATE SYSTEM REFERENCE ===
	// Character spawn: X = -4000.0f, Y = 2000.0f
	// Character looks in POSITIVE X direction (toward rooms)
	// FRONT = toward character (negative X direction, toward -4000.0f)
	// BACK = away from character (positive X direction, away from -4000.0f)
	// RIGHT = positive Y direction (when facing positive X)
	// LEFT = negative Y direction (when facing positive X)
	
	float CharacterX = -4000.0f;
	float CharacterY = 2000.0f;
	
	// === SHARED ROW POSITIONING (both Row 1 and Row 9 use same X position) ===
	float SharedRowStartX = -5000.0f; // Both rows start at same X position for alignment
	float RowSeparationDistance = 1700.0f; // Distance between each row (Y-axis)
	
	// Room row positioning
	float BaseY = RowSeparationDistance; // Row 1 position - rotated 90° so rooms are along X-axis
	
	// Create row label using unified positioning system
	FVector LabelPosition = CalculateLabelPosition(SharedRowStartX, BaseY); // Position relative to actual first room location
	CreateRowLabel(LabelPosition, TEXT("Row 1: Multiple Rooms with Different Sizes + Doorways"));
	
	// Row 1: Multiple complete rooms with different sizes and connecting doorways  
	// Position rooms along Y=400.0f at the front of all demonstrations
	
	float WallThickness = 0.2f; // 20cm thick walls
	// BaseY now defined above in coordinate system section
	
	// FIXED FLOOR LEVEL: All rooms will have floors at the same Z level
	float FixedFloorZ = -280.0f; // Consistent floor level for all rooms (lowered 60cm)
	
	// Calculate room centers based on floor level + half height
	
	// === ROOM 1: SMALL SQUARE ROOM (4m × 4m × 3m) - NO ROTATION ===
	float Room1Height = 3.0f;
	float Room1CenterZ = FixedFloorZ + (Room1Height * 100.0f * 0.5f);
	CreateSingleRoom(FVector(SharedRowStartX, BaseY, Room1CenterZ), 4.0f, 4.0f, Room1Height, WallThickness, 
		FLinearColor::White, TEXT("Square"), true, false, false, false, 0.0f, 0.0f);
	
	// === ROOM 2: LONG CORRIDOR (3m × 12m × 4m) - NO ROTATION ===
	float Room2Height = 4.0f;
	float Room2CenterZ = FixedFloorZ + (Room2Height * 100.0f * 0.5f);
	CreateSingleRoom(FVector(-3800.0f, BaseY, Room2CenterZ), 3.0f, 12.0f, Room2Height, WallThickness, 
		FLinearColor::White, TEXT("Corridor"), false, true, true, false, 0.0f, 0.0f);
	
	// === ROOM 3: RECTANGULAR ROOM (6m × 10m × 5m) - VERTICAL 45° ===
	float Room3Height = 5.0f;
	float Room3CenterZ = FixedFloorZ + (Room3Height * 100.0f * 0.5f);
	CreateSingleRoom(FVector(-2600.0f, BaseY, Room3CenterZ), 6.0f, 10.0f, Room3Height, WallThickness, 
		FLinearColor::White, TEXT("Rectangle"), false, false, true, true, 0.0f, 45.0f);
	
	// === ROOM 4: NARROW HALLWAY (2.5m × 16m × 6m) - NO ROTATION ===
	float Room4Height = 6.0f;
	float Room4CenterZ = FixedFloorZ + (Room4Height * 100.0f * 0.5f);
	CreateSingleRoom(FVector(-800.0f, BaseY, Room4CenterZ), 2.5f, 16.0f, Room4Height, WallThickness, 
		FLinearColor::White, TEXT("Narrow"), true, true, false, false, 0.0f, 0.0f);
	
	// === ROOM 5: WIDE HALL (18m × 6m × 4.5m) - VERTICAL 45° ===
	float Room5Height = 4.5f;
	float Room5CenterZ = FixedFloorZ + (Room5Height * 100.0f * 0.5f);
	CreateSingleRoom(FVector(1600.0f, BaseY, Room5CenterZ), 18.0f, 6.0f, Room5Height, WallThickness, 
		FLinearColor::White, TEXT("Wide"), false, false, true, true, 0.0f, 45.0f);
	
	// === ROOM 6: LONG CHAMBER (8m × 14m × 5.5m) - VERTICAL 45° ===
	float Room6Height = 5.5f;
	float Room6CenterZ = FixedFloorZ + (Room6Height * 100.0f * 0.5f);
	CreateSingleRoom(FVector(4600.0f, BaseY, Room6CenterZ), 8.0f, 14.0f, Room6Height, WallThickness, 
		FLinearColor::White, TEXT("Chamber"), true, true, true, true, 0.0f, 45.0f);
	
	DebugLog(TEXT("✅ Row 1: Created 6 rooms with different sizes and connecting doorways"));
	DebugLog(TEXT("✅ Room sizes: 5×5×4m, 8×10×5m, 12×8×6m, 5×15×7m, 15×5×4.5m, 10×10×5.5m"));
}

void ATestGenerator::CreateSingleRoom(FVector RoomCenter, float Width, float Depth, float Height, 
	float WallThickness, FLinearColor WallColor, const FString& RoomName, 
	bool bSouthDoor, bool bNorthDoor, bool bEastDoor, bool bWestDoor, 
	float YawRotation, float PitchRotation)
{
	// Safety checks
	if (!IsValid(this) || !GetWorld())
	{
		DebugLog(TEXT("❌ CreateSingleRoom: Invalid object or world"));
		return;
	}
	
	// Convert to Unreal units (cm)
	float HalfWidth = Width * 0.5f * 100.0f;
	float HalfDepth = Depth * 0.5f * 100.0f;
	float HeightCm = Height * 100.0f;
	
	// Color variations for different elements
	FLinearColor FloorColor = WallColor * 0.6f + FLinearColor(0.3f, 0.3f, 0.3f, 1.0f); // Darker floor
	FLinearColor CeilingColor = FLinearColor::White * 0.9f; // Light gray ceiling
	
	// Individual wall colors: North=Red, South=Green, East=Blue, West=Yellow
	FLinearColor SouthWallColor = FLinearColor::Green;   // South wall = Green
	FLinearColor NorthWallColor = FLinearColor::Red;     // North wall = Red  
	FLinearColor EastWallColor = FLinearColor::Blue;     // East wall = Blue
	FLinearColor WestWallColor = FLinearColor::Yellow;   // West wall = Yellow
	
	// Create room rotation transform
	FRotator RoomRotation = FRotator(PitchRotation, YawRotation, 0);
	FTransform RoomTransform = FTransform(RoomRotation, RoomCenter);
	
	// === WALLS ===
	// Create walls in local space, then transform to world space with room rotation
	// ALL WALLS HAVE DOORWAYS - ignoring boolean parameters for full connectivity
	
	// South Wall (front wall) - GREEN - ALWAYS has doorway
	FVector SouthWallLocalPos = FVector(0, -HalfDepth, 0);
	FVector SouthWallWorldPos = RoomTransform.TransformPosition(SouthWallLocalPos);
	FRotator SouthWallWorldRot = RoomTransform.TransformRotation(FRotator(0, 0, 0).Quaternion()).Rotator();
	FWallHoleConfig SouthDoorConfig(1.0f, 2.3f, TEXT("SouthDoor"));
	SouthDoorConfig.Shape = EHoleShape::Rectangle;
	UWallUnit::CreateWallWithHole(GetWorld(), SouthWallWorldPos, SouthWallWorldRot, 
		Width, Height, WallThickness, SouthWallColor, SouthDoorConfig);
	DebugLog(FString::Printf(TEXT("✅ %s Room: South wall (GREEN) with doorway"), *RoomName));
	
	// North Wall (back wall) - RED - ALWAYS has doorway
	FVector NorthWallLocalPos = FVector(0, HalfDepth, 0);
	FVector NorthWallWorldPos = RoomTransform.TransformPosition(NorthWallLocalPos);
	FRotator NorthWallWorldRot = RoomTransform.TransformRotation(FRotator(0, 180, 0).Quaternion()).Rotator();
	FWallHoleConfig NorthDoorConfig(1.0f, 2.3f, TEXT("NorthDoor"));
	NorthDoorConfig.Shape = EHoleShape::Rectangle;
	UWallUnit::CreateWallWithHole(GetWorld(), NorthWallWorldPos, NorthWallWorldRot, 
		Width, Height, WallThickness, NorthWallColor, NorthDoorConfig);
	DebugLog(FString::Printf(TEXT("✅ %s Room: North wall (RED) with doorway"), *RoomName));
	
	// East Wall (right wall) - BLUE - ALWAYS has doorway
	FVector EastWallLocalPos = FVector(HalfWidth, 0, 0);
	FVector EastWallWorldPos = RoomTransform.TransformPosition(EastWallLocalPos);
	FRotator EastWallWorldRot = RoomTransform.TransformRotation(FRotator(0, 90, 0).Quaternion()).Rotator();
	FWallHoleConfig EastDoorConfig(1.0f, 2.3f, TEXT("EastDoor"));
	EastDoorConfig.Shape = EHoleShape::Rectangle;
	UWallUnit::CreateWallWithHole(GetWorld(), EastWallWorldPos, EastWallWorldRot, 
		Depth, Height, WallThickness, EastWallColor, EastDoorConfig);
	DebugLog(FString::Printf(TEXT("✅ %s Room: East wall (BLUE) with doorway"), *RoomName));
	
	// West Wall (left wall) - YELLOW - ALWAYS has doorway
	FVector WestWallLocalPos = FVector(-HalfWidth, 0, 0);
	FVector WestWallWorldPos = RoomTransform.TransformPosition(WestWallLocalPos);
	FRotator WestWallWorldRot = RoomTransform.TransformRotation(FRotator(0, 270, 0).Quaternion()).Rotator();
	FWallHoleConfig WestDoorConfig(1.0f, 2.3f, TEXT("WestDoor"));
	WestDoorConfig.Shape = EHoleShape::Rectangle;
	UWallUnit::CreateWallWithHole(GetWorld(), WestWallWorldPos, WestWallWorldRot, 
		Depth, Height, WallThickness, WestWallColor, WestDoorConfig);
	DebugLog(FString::Printf(TEXT("✅ %s Room: West wall with doorway"), *RoomName));
	
	// === FLOOR ===
	// Position floor at bottom of room in local space, then transform
	FVector FloorLocalPos = FVector(0, 0, -(HeightCm * 0.5f + WallThickness * 100.0f * 0.5f + 2.0f));
	FVector FloorWorldPos = RoomTransform.TransformPosition(FloorLocalPos);
	FRotator FloorWorldRot = RoomTransform.TransformRotation(FRotator(0, 0, 90).Quaternion()).Rotator();
	UWallUnit::CreateSolidWallActor(GetWorld(), FloorWorldPos, FloorWorldRot, 
		Width, Depth, WallThickness, FloorColor);
	
	// === CEILING ===
	// Position ceiling at top of room in local space, then transform
	FVector CeilingLocalPos = FVector(0, 0, HeightCm * 0.5f + (WallThickness * 100.0f * 0.5f + 2.0f));
	FVector CeilingWorldPos = RoomTransform.TransformPosition(CeilingLocalPos);
	FRotator CeilingWorldRot = RoomTransform.TransformRotation(FRotator(0, 0, 270).Quaternion()).Rotator();
	UWallUnit::CreateSolidWallActor(GetWorld(), CeilingWorldPos, CeilingWorldRot, 
		Width, Depth, WallThickness, CeilingColor);
	
	// === DECORATIVE ELEMENTS REMOVED ===
	// Removed floating decorative walls that were appearing above ceilings
	
	DebugLog(FString::Printf(TEXT("✅ %s Room complete: %.1fm × %.1fm × %.1fm at %s"), 
		*RoomName, Width, Depth, Height, *RoomCenter.ToString()));
}


void ATestGenerator::CreateRowStandardRoomUnits()
{
	// === SHARED ROW POSITIONING (same as Row 1) ===
	float SharedRowStartX = -5000.0f; // Both rows start at same X position for alignment
	float RowSeparationDistance = 1700.0f; // Distance between each row (Y-axis)
	
	float BaseY = -RowSeparationDistance; // Row 9 position - using shared separation distance
	
	// Create row label using unified positioning system (same as Row 1)
	FVector LabelPosition9 = CalculateLabelPosition(SharedRowStartX, BaseY); // Position relative to actual first room location
	CreateRowLabel(LabelPosition9, TEXT("Row 9: StandardRoom-style Units using WallUnit actors"));
	float FixedFloorZ = -350.0f; // Adjusted to better height level
	float WallThickness = 0.2f; // 20cm thick walls
	
	// Room configurations: {Width, Length, Height, X_Position, DoorWalls}
	struct RoomConfig
	{
		float Width;
		float Length; 
		float Height;
		float XPosition; // X position along the row (same as Row 1)
		TArray<EWallSide> DoorWalls;
		FString RoomName;
	};
	
	TArray<RoomConfig> RoomConfigs = {
		// Room 1: Small square room with single door
		{3.0f, 3.0f, 3.0f, 0.0f, {EWallSide::South}, TEXT("Small")},
		
		// Room 2: Medium room with two doors
		{5.0f, 4.0f, 3.5f, 800.0f, {EWallSide::South, EWallSide::North}, TEXT("Medium")},
		
		// Room 3: Long corridor with multiple doors
		{2.5f, 10.0f, 4.0f, 1600.0f, {EWallSide::South, EWallSide::North, EWallSide::East}, TEXT("Corridor")},
		
		// Room 4: Large room with doors on all sides
		{8.0f, 6.0f, 5.0f, 2800.0f, {EWallSide::South, EWallSide::North, EWallSide::East, EWallSide::West}, TEXT("Large")},
		
		// Room 5: Tall narrow room with opposing doors
		{3.0f, 3.0f, 7.0f, 4200.0f, {EWallSide::East, EWallSide::West}, TEXT("Tall")}
	};
	
	for (int32 i = 0; i < RoomConfigs.Num(); i++)
	{
		const RoomConfig& Config = RoomConfigs[i];
		
		// Calculate room center position
		float RoomCenterZ = FixedFloorZ + (Config.Height * 100.0f * 0.5f);
		FVector RoomCenter = FVector(SharedRowStartX + Config.XPosition, BaseY, RoomCenterZ); // Use shared X start position + offset
		
		// Use the same approach as the old row - create individual WallUnit actors
		CreateSingleRoom(RoomCenter, Config.Width, Config.Length, Config.Height, WallThickness,
			FLinearColor::White, Config.RoomName, 
			Config.DoorWalls.Contains(EWallSide::South),  // bSouthDoor
			Config.DoorWalls.Contains(EWallSide::North),  // bNorthDoor
			Config.DoorWalls.Contains(EWallSide::East),   // bEastDoor
			Config.DoorWalls.Contains(EWallSide::West),   // bWestDoor
			0.0f, 0.0f); // No rotation
		
		DebugLog(FString::Printf(TEXT("✅ WallUnit Room %d (%s): %.1fx%.1fx%.1fm with doors on %d walls at %s"), 
			i+1, *Config.RoomName, Config.Width, Config.Length, Config.Height, 
			Config.DoorWalls.Num(), *RoomCenter.ToString()));
	}
	
	DebugLog(TEXT("✅ Row 9: Created 5 WallUnit-based rooms with same configurations as StandardRoom units"));
}


void ATestGenerator::CreateRowModifiableUnits()
{
	// === SHARED ROW POSITIONING (same as other rows) ===
	float SharedRowStartX = -5000.0f; // Both rows start at same X position for alignment
	float RowSeparationDistance = 1700.0f; // Distance between each row (Y-axis)
	
	float BaseY = 2.0f * RowSeparationDistance; // Row 11 position - 2x separation distance (3400.0f)
	
	// Create row label using unified positioning system (same as other rows)
	FVector LabelPosition11 = CalculateLabelPosition(SharedRowStartX, BaseY);
	CreateRowLabel(LabelPosition11, TEXT("Row 11: Modifiable Units - Ready for Customization"));
	
	float FixedFloorZ = -350.0f; // Same level as other rows
	float WallThickness = 0.2f; // 20cm thick walls
	
	// Modifiable unit configurations - starting template for your modifications
	struct ModifiableConfig
	{
		float Width;
		float Length; 
		float Height;
		float XPosition; // X position along the row (same as other rows)
		TArray<EWallSide> DoorWalls;
		FString UnitName;
		FLinearColor UnitColor;
	};
	
	TArray<ModifiableConfig> ModifiableConfigs = {
		// Unit 1: Test unit for modifications
		{4.0f, 4.0f, 3.0f, 0.0f, {EWallSide::South}, TEXT("TestUnit1"), FLinearColor(0.0f, 1.0f, 1.0f, 1.0f)}, // Cyan
		
		// Unit 2: Another test unit
		{5.0f, 6.0f, 4.0f, 800.0f, {EWallSide::South, EWallSide::North}, TEXT("TestUnit2"), FLinearColor(1.0f, 0.0f, 1.0f, 1.0f)}, // Magenta
		
		// Unit 3: Wide test unit
		{8.0f, 4.0f, 3.5f, 1600.0f, {EWallSide::East, EWallSide::West}, TEXT("TestUnit3"), FLinearColor(1.0f, 0.5f, 0.0f, 1.0f)}, // Orange
		
		// Unit 4: Tall test unit
		{3.0f, 3.0f, 6.0f, 2800.0f, {EWallSide::North}, TEXT("TestUnit4"), FLinearColor(0.5f, 0.0f, 1.0f, 1.0f)}, // Purple
		
		// Unit 5: Complex test unit
		{6.0f, 8.0f, 5.0f, 4200.0f, {EWallSide::South, EWallSide::North, EWallSide::East, EWallSide::West}, TEXT("TestUnit5"), FLinearColor::Yellow}
	};
	
	for (int32 i = 0; i < ModifiableConfigs.Num(); i++)
	{
		const ModifiableConfig& Config = ModifiableConfigs[i];
		
		// Calculate unit center position
		float UnitCenterZ = FixedFloorZ + (Config.Height * 100.0f * 0.5f);
		FVector UnitCenter = FVector(SharedRowStartX + Config.XPosition, BaseY, UnitCenterZ); // Use shared X start position + offset
		
		// Create the unit using the same approach - ready for your modifications
		CreateSingleRoom(UnitCenter, Config.Width, Config.Length, Config.Height, WallThickness,
			Config.UnitColor, Config.UnitName, 
			Config.DoorWalls.Contains(EWallSide::South),  // bSouthDoor
			Config.DoorWalls.Contains(EWallSide::North),  // bNorthDoor
			Config.DoorWalls.Contains(EWallSide::East),   // bEastDoor
			Config.DoorWalls.Contains(EWallSide::West),   // bWestDoor
			0.0f, 0.0f); // No rotation
		
		DebugLog(FString::Printf(TEXT("✅ ModifiableUnit %d (%s): %.1fx%.1fx%.1fm with doors on %d walls at %s"), 
			i+1, *Config.UnitName, Config.Width, Config.Length, Config.Height, 
			Config.DoorWalls.Num(), *UnitCenter.ToString()));
	}
	
	DebugLog(TEXT("✅ Row 11: Created 5 modifiable units - ready for your customizations!"));
}

void ATestGenerator::TestSmallToLargeRoomConnection()
{
	// DebugLog(TEXT("================================================================================"));
	// DebugLog(TEXT("🧪 STARTING BOUNDARY TEST: Small Room (5m) to Large Room (20m) Connection"));
	// DebugLog(TEXT("================================================================================"));
	
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
	
	// DebugLog(FString::Printf(TEXT("🎯 CHARACTER LOCATION: %s"), *CharacterLocation.ToString()));
	
	// Create first room with random size - positioned so character is in center
	FRandomStream Random(FDateTime::Now().GetTicks());
	
	// Create first room with auto-generated properties
	FRoomData SmallRoom;
	UBaseRoom::InitializeRandomRoom(SmallRoom, ERoomCategory::Room, 0, Random);
	
	// Room Position is corner - offset by half room size to center character
	float HalfWidthCm = MetersToUnrealUnits(SmallRoom.Width) * 0.5f;
	float HalfLengthCm = MetersToUnrealUnits(SmallRoom.Length) * 0.5f;
	SmallRoom.Position = CharacterLocation + FVector(-HalfWidthCm, -HalfLengthCm, -150.0f);
	
	// Create the room unit
	SmallRoom.RoomUnit = NewObject<UStandardRoom>(this);
	SmallRoom.RoomUnit->Position = SmallRoom.Position;
	SmallRoom.RoomUnit->Width = SmallRoom.Width;
	SmallRoom.RoomUnit->Length = SmallRoom.Length;
	SmallRoom.RoomUnit->Height = SmallRoom.Height;
	SmallRoom.RoomUnit->RoomCategory = SmallRoom.Category;
	SmallRoom.RoomUnit->Elevation = SmallRoom.Elevation;
	
	// Create the small room geometry
	SmallRoom.RoomUnit->CreateRoomUsingIndividualActors(this);
	
	// Add to arrays
	GeneratedRooms.Add(SmallRoom);
	RoomUnits.Add(SmallRoom.RoomUnit);
	
	DebugLog(FString::Printf(TEXT("🎲 Random Room 1: %.1fx%.1fm"), SmallRoom.Width, SmallRoom.Length));
	
	// Create second room with auto-generated properties (larger room)
	FRoomData LargeRoom;
	UBaseRoom::InitializeRandomRoom(LargeRoom, ERoomCategory::Room, 1, Random);
	
	// ROTATE ROOM: Ensure smallest face becomes the connection wall (south wall)
	// If Length > Width, swap them so Width becomes Length (making south wall shorter)
	if (LargeRoom.Length > LargeRoom.Width)
	{
		float TempDimension = LargeRoom.Width;
		LargeRoom.Width = LargeRoom.Length;
		LargeRoom.Length = TempDimension;
		DebugLog(FString::Printf(TEXT("🔄 ROTATED Room 2: Swapped dimensions %.1fx%.1fm → %.1fx%.1fm (smallest face now on south)"), 
			TempDimension, LargeRoom.Length, LargeRoom.Width, LargeRoom.Length));
	}
	
	// Position large room so its SOUTH wall connects to small room's EAST wall (90-degree rotation)
	float SmallRoomEastX = CharacterLocation.X + HalfWidthCm;
	float LargeRoomStartX = SmallRoomEastX + MetersToUnrealUnits(0.02f); // Small gap for wall thickness
	float LargeHalfWidthCm = MetersToUnrealUnits(LargeRoom.Width) * 0.5f; // Now using Width for Y positioning
	// Position room so its south wall (Y=0) aligns with the connection point
	LargeRoom.Position = FVector(LargeRoomStartX, CharacterLocation.Y - LargeHalfWidthCm, CharacterLocation.Z - 150.0f);
	
	// Create the room unit
	LargeRoom.RoomUnit = NewObject<UStandardRoom>(this);
	LargeRoom.RoomUnit->Position = LargeRoom.Position;
	LargeRoom.RoomUnit->Width = LargeRoom.Width;
	LargeRoom.RoomUnit->Length = LargeRoom.Length;
	LargeRoom.RoomUnit->Height = LargeRoom.Height;
	LargeRoom.RoomUnit->RoomCategory = LargeRoom.Category;
	LargeRoom.RoomUnit->Elevation = LargeRoom.Elevation;
	
	// Create the large room geometry
	LargeRoom.RoomUnit->CreateRoomUsingIndividualActors(this);
	
	// Add to arrays
	GeneratedRooms.Add(LargeRoom);
	RoomUnits.Add(LargeRoom.RoomUnit);
	
	DebugLog(FString::Printf(TEXT("🎲 Random Room 2: %.1fx%.1fm"), LargeRoom.Width, LargeRoom.Length));
	
	// Test the boundary constraint by creating a connection (90-degree rotation)
	// Use actual wall sizes from the random rooms
	float SmallerWallSize = SmallRoom.Length; // Small room's east wall (north-south length)
	float LargerWallSize = LargeRoom.Width;   // Large room's SOUTH wall (east-west width) - ROTATED!
	
	DebugLog(FString::Printf(TEXT("🔗 ROTATED CONNECTION: %.1fx%.1fm room → %.1fx%.1fm room (90° rotated), hole constrained to %.1fm"), 
		SmallRoom.Width, SmallRoom.Length, LargeRoom.Width, LargeRoom.Length, SmallerWallSize));
	
	// Create a test hole in the large room's SOUTH wall with boundary constraint (90-degree rotation)
	FDoorConfig TestDoorConfig;
	TestDoorConfig.bHasDoor = true;
	TestDoorConfig.WallSide = EWallSide::South; // Changed from West to South for rotation
	TestDoorConfig.HoleShape = EHoleShape::Rectangle;
	TestDoorConfig.Width = 1.4f;   // Opening width
	TestDoorConfig.Height = 2.5f;  // Opening height
	TestDoorConfig.OffsetFromCenter = 0.0f; // Will be randomized with boundary constraint
	
	// Create asymmetric connection using new unified method (east wall to south wall connection)
	SmallRoom.RoomUnit->AddHoleToWallWithThickness(this, EWallSide::East, TestDoorConfig, 0.4f, SmallerWallSize, LargeRoom.RoomUnit);
	
	DebugLog(TEXT("🧪 BOUNDARY TEST COMPLETED - Check hole position"));
}

void ATestGenerator::TestRoomToHallwayConnection()
{
	DebugLog(TEXT("================================================================================"));
	DebugLog(TEXT("🎲 STARTING RANDOM ROOM TO HALLWAY TEST: Random Room → Random Hallway"));
	DebugLog(TEXT("================================================================================"));
	
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
	
	DebugLog(FString::Printf(TEXT("🎯 CHARACTER LOCATION: %s"), *CharacterLocation.ToString()));
	
	// Create random seed for consistent randomization
	FRandomStream Random(FDateTime::Now().GetTicks());
	
	// Create first standard room (where character stands) - auto-generated properties
	FRoomData Room1;
	UBaseRoom::InitializeRandomRoom(Room1, ERoomCategory::Room, 0, Random);
	
	// Position room so character is in center
	float HalfWidthCm = MetersToUnrealUnits(Room1.Width) * 0.5f;
	float HalfLengthCm = MetersToUnrealUnits(Room1.Length) * 0.5f;
	Room1.Position = CharacterLocation + FVector(-HalfWidthCm, -HalfLengthCm, -150.0f);
	
	// Create the room unit
	Room1.RoomUnit = NewObject<UStandardRoom>(this);
	Room1.RoomUnit->Position = Room1.Position;
	Room1.RoomUnit->Width = Room1.Width;
	Room1.RoomUnit->Length = Room1.Length;
	Room1.RoomUnit->Height = Room1.Height;
	Room1.RoomUnit->RoomCategory = Room1.Category;
	Room1.RoomUnit->Elevation = Room1.Elevation;
	
	// Create the first room geometry
	Room1.RoomUnit->CreateRoomUsingIndividualActors(this);
	
	// Add to arrays
	GeneratedRooms.Add(Room1);
	RoomUnits.Add(Room1.RoomUnit);
	
	DebugLog(FString::Printf(TEXT("✅ Created Random Room 1 (%.1fx%.1fm)"), 
		Room1.Width, Room1.Length));
	
	// Create second room as hallway - auto-generated properties
	FRoomData Room2;
	UBaseRoom::InitializeRandomRoom(Room2, ERoomCategory::Hallway, 1, Random);
	
	// Position second room adjacent to first room's east wall
	float Room1EastX = CharacterLocation.X + HalfWidthCm;
	float Room2StartX = Room1EastX + MetersToUnrealUnits(0.02f); // Small gap for wall thickness
	float Room2HalfLengthCm = MetersToUnrealUnits(Room2.Length) * 0.5f;
	Room2.Position = FVector(Room2StartX, CharacterLocation.Y - Room2HalfLengthCm, CharacterLocation.Z - 150.0f);
	
	// Create the second room unit
	Room2.RoomUnit = NewObject<UStandardRoom>(this);
	Room2.RoomUnit->Position = Room2.Position;
	Room2.RoomUnit->Width = Room2.Width;
	Room2.RoomUnit->Length = Room2.Length;
	Room2.RoomUnit->Height = Room2.Height;
	Room2.RoomUnit->RoomCategory = Room2.Category;
	Room2.RoomUnit->Elevation = Room2.Elevation;
	
	// Create the second room geometry
	Room2.RoomUnit->CreateRoomUsingIndividualActors(this);
	
	// Add to arrays
	GeneratedRooms.Add(Room2);
	RoomUnits.Add(Room2.RoomUnit);
	
	DebugLog(FString::Printf(TEXT("✅ Created Random Hallway (%.1fx%.1fm)"), 
		Room2.Width, Room2.Length));
	
	// Create connections between the rooms
	// Room 1 East wall → Room 2 West wall connection
	
	// Determine connection type and hole size based on room sizes
	float SmallerWallHeight = FMath::Min(Room1.Length, Room2.Length); // Smaller wall determines hole constraint
	float ConnectionWidth = 1.4f;  // Opening width
	float ConnectionHeight = 2.5f; // Opening height
	
	// Create asymmetric connection using new unified method
	FDoorConfig ConnectionConfig;
	ConnectionConfig.bHasDoor = true;
	ConnectionConfig.WallSide = EWallSide::East;
	ConnectionConfig.HoleShape = EHoleShape::Rectangle;
	ConnectionConfig.Width = ConnectionWidth;
	ConnectionConfig.Height = ConnectionHeight;
	ConnectionConfig.OffsetFromCenter = 0.0f; // Centered
	
	// Single call creates the entire asymmetric connection (removes smaller wall, adds thick wall with hole)
	Room1.RoomUnit->AddHoleToWallWithThickness(this, EWallSide::East, ConnectionConfig, 0.2f, SmallerWallHeight, Room2.RoomUnit);
	
	DebugLog(TEXT("🔗 ROOM CONNECTION: Created opening between Room 1 (East) and Hallway (West)"));
	DebugLog(FString::Printf(TEXT("🎯 Connection: %.1fm × %.1fm opening, constrained to %.1fm wall"), 
		ConnectionWidth, ConnectionHeight, SmallerWallHeight));
		
	DebugLog(TEXT("================================================================================"));
	DebugLog(TEXT("🎲 RANDOM ROOM TO HALLWAY TEST COMPLETED - Check connected random-sized spaces"));
	DebugLog(TEXT("================================================================================"));
}

