/*
Copyright 2023 UPO33.All Rights Reserved.
*/


#pragma once

#include "Engine/DeveloperSettings.h"

#include "SkelotSettings.generated.h"

UCLASS(config = Engine, defaultconfig, meta=(DisplayName="Skelot"))
class SKELOT_API USkelotSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(config, Category = "Skelot", EditAnywhere, meta = (ConsoleVariable = "skelot.ShaderMaxBoneInfluence", ConfigRestartRequired = true, ClampMin=-1,ClampMax=4))
	int ShaderMaxBoneInfluence;

	UPROPERTY(config, Category = "Skelot", EditAnywhere, meta = (ConsoleVariable = "skelot.ShadowForceLOD", ConfigRestartRequired=false))
	int ShadowForceLOD;

	FName GetCategoryName() const override { return FName(TEXT("Plugins")); }
};