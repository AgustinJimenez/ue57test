// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BackRooomsUE57 : ModuleRules
{
	public BackRooomsUE57(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"ProceduralMeshComponent"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"BackRooomsUE57",
			"BackRooomsUE57/BackRoomsGenerator",
			"BackRooomsUE57/BackRoomsGenerator/RoomUnit",
			"BackRooomsUE57/BackRoomsGenerator/WallUnit",
			"BackRooomsUE57/Variant_Platforming",
			"BackRooomsUE57/Variant_Platforming/Animation",
			"BackRooomsUE57/Variant_Combat",
			"BackRooomsUE57/Variant_Combat/AI",
			"BackRooomsUE57/Variant_Combat/Animation",
			"BackRooomsUE57/Variant_Combat/Gameplay",
			"BackRooomsUE57/Variant_Combat/Interfaces",
			"BackRooomsUE57/Variant_Combat/UI",
			"BackRooomsUE57/Variant_SideScrolling",
			"BackRooomsUE57/Variant_SideScrolling/AI",
			"BackRooomsUE57/Variant_SideScrolling/Gameplay",
			"BackRooomsUE57/Variant_SideScrolling/Interfaces",
			"BackRooomsUE57/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
