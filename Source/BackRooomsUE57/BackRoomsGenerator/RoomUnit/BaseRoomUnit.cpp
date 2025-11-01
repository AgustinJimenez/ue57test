#include "BaseRoomUnit.h"
#include "Engine/Engine.h"

UBaseRoomUnit::UBaseRoomUnit()
{
	MeshComponent = nullptr;
}

void UBaseRoomUnit::CreateRoom(AActor* Owner)
{
	// Default implementation - subclasses should override this
	UE_LOG(LogTemp, Warning, TEXT("BaseRoomUnit: CreateRoom called on base class - should be overridden by subclass"));
	
	// Initialize the mesh component
	InitializeMeshComponent(Owner);
	
	// Generate mesh (pure virtual - must be implemented by subclasses)
	GenerateMesh();
}

void UBaseRoomUnit::SetMaterial(UMaterialInterface* Material)
{
	ApplyMaterialToMesh(Material);
}

void UBaseRoomUnit::InitializeMeshComponent(AActor* Owner)
{
	if (!Owner)
	{
		UE_LOG(LogTemp, Error, TEXT("BaseRoomUnit: Cannot create mesh component without valid owner"));
		return;
	}

	// Create procedural mesh component
	MeshComponent = NewObject<UProceduralMeshComponent>(Owner);
	if (!MeshComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("BaseRoomUnit: Failed to create procedural mesh component"));
		return;
	}

	// Always set as root component for room units (they should be standalone actors)
	Owner->SetRootComponent(MeshComponent);
	UE_LOG(LogTemp, Warning, TEXT("BaseRoomUnit: Set mesh component as root component for actor"));

	// Set basic properties
	MeshComponent->SetWorldLocation(Position);
	
	// Setup collision BEFORE generating mesh
	SetupCollisionSettings();
	
	// CRITICAL: Register the component with the world so it's rendered and has collision
	MeshComponent->RegisterComponent();
	UE_LOG(LogTemp, Warning, TEXT("🔧 COMPONENT REGISTRATION: Registered mesh component"));

	UE_LOG(LogTemp, Log, TEXT("BaseRoomUnit: Initialized mesh component at position %s"), 
		*Position.ToString());
}

void UBaseRoomUnit::ApplyMaterialToMesh(UMaterialInterface* Material)
{
	if (!MeshComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("BaseRoomUnit: Cannot apply material - no mesh component"));
		return;
	}

	if (Material)
	{
		MeshComponent->SetMaterial(0, Material);
		UE_LOG(LogTemp, Log, TEXT("BaseRoomUnit: Applied material to mesh"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("BaseRoomUnit: No material provided - using default"));
	}
}

void UBaseRoomUnit::SetupCollisionSettings()
{
	if (!MeshComponent)
	{
		return;
	}

	// Set collision properties for room units
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	MeshComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	MeshComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
	
	// Use complex collision for accurate walking
	MeshComponent->bUseComplexAsSimpleCollision = true;

	UE_LOG(LogTemp, Warning, TEXT("BaseRoomUnit: Configured collision settings - Complex collision enabled: %s"), 
		MeshComponent->bUseComplexAsSimpleCollision ? TEXT("TRUE") : TEXT("FALSE"));
	UE_LOG(LogTemp, Warning, TEXT("BaseRoomUnit: Collision enabled: %s"), 
		MeshComponent->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics ? TEXT("QueryAndPhysics") : TEXT("OTHER"));
}