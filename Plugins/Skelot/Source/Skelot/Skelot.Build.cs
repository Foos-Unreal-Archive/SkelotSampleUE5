// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Skelot : ModuleRules
{
	public Skelot(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		bUseUnity = false;
		bUsePrecompiled=true;
		
        PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core", "RHI", "RenderCore", "Engine", "DeveloperSettings"
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject", "Engine", "Projects", "Slate", "SlateCore", "Chaos"
				// ... add private dependencies that you statically link with here ...	
			}
			);

        if (Target.bBuildEditor == true)
        {
            PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "EditorStyle",  });
        }

        DynamicallyLoadedModuleNames.AddRange(new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			});
	}
}
