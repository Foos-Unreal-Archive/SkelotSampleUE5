/*
Copyright 2023 UPO33.All Rights Reserved.
*/


#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Stats/Stats.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSkelot, Log, All);

class FSkelotModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

static constexpr int SKELOT_MAX_LOD = 8;


DECLARE_STATS_GROUP(TEXT("Skelot"), STATGROUP_SKELOT, STATCAT_Advanced);

DECLARE_CYCLE_STAT_EXTERN(TEXT("GetDynamicMeshElements"), STAT_SKELOT_GetDynamicMeshElements, STATGROUP_SKELOT, SKELOT_API);
DECLARE_CYCLE_STAT_EXTERN(TEXT("SendRenderDynamicData"), STAT_SKELOT_SendRenderDynamicData, STATGROUP_SKELOT, SKELOT_API );
DECLARE_CYCLE_STAT_EXTERN(TEXT("FrustumCull"), STAT_SKELOT_CullTime, STATGROUP_SKELOT, SKELOT_API);
DECLARE_CYCLE_STAT_EXTERN(TEXT("ShadowFrustmCull"), STAT_SKELOT_ShadowCullTime, STATGROUP_SKELOT, SKELOT_API );
DECLARE_CYCLE_STAT_EXTERN(TEXT("AnimationTick"), STAT_SKELOT_AnimationTickTime, STATGROUP_SKELOT, SKELOT_API );
DECLARE_CYCLE_STAT_EXTERN(TEXT("BufferFilling"), STAT_SKELOT_BufferFilling, STATGROUP_SKELOT, SKELOT_API );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Sorting"), STAT_SKELOT_Sorting, STATGROUP_SKELOT, SKELOT_API );


DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("ViewNumCulledInstance"), STAT_SKELOT_ViewNumCulled, STATGROUP_SKELOT, SKELOT_API);
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("ViewNumVisibleInstance"), STAT_SKELOT_ViewNumVisible, STATGROUP_SKELOT, SKELOT_API);

DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("ShadowNumCulledInstance"), STAT_SKELOT_ShadowNumCulled, STATGROUP_SKELOT, SKELOT_API );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("ShadowNumVisibleInstance"), STAT_SKELOT_ShadowNumVisible, STATGROUP_SKELOT, SKELOT_API );


#define SKELOT_UE_VERSION 4.27	//the version of engine this plugin is for
