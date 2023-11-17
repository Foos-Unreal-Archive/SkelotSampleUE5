/*
Copyright 2023 UPO33.All Rights Reserved.
*/

#pragma once

#include "PrimitiveSceneProxy.h"
#include "VertexFactory.h"
#include "Components.h"
#include "SkelotComponent.h"
#include "SkelotRenderResources.h"
#include "Containers/TripleBuffer.h"
#include "Containers/CircularQueue.h"

extern bool GSkelot_DrawInstancesBounds;
extern int GSkelot_ForcedAnimFrameIndex;
extern int GSkelot_ForceLOD;
extern int GSkelot_MaxTrianglePerInstance;
extern int GSkelot_FroceMaxBoneInfluence;
extern int GSkelot_ShaderMaxBoneInfluence;
extern bool GSkelot_DebugDraw;

struct alignas(16) FSkelotDynamicData
{
	ESkelotInstanceFlags* Flags;
	SkelotShaderMatrixT* Transforms;
	FBoxCenterExtentFloat* Bounds;
	uint16* FrameIndices;
	float* CustomData;
	uint32 InstanceCount;
	uint32 AliveInstanceCount;
	uint32 CreationNumber;

	static FSkelotDynamicData* Allocate(uint32 InstanceCount, int NumCustomDataFloats);

	void operator delete(void* ptr) { return FMemory::Free(ptr); }
};

#if 0
struct FSkelotInstanceBufferOFR : FOneFrameResource
{
	FSkelotInstanceBufferPtr InstanceBuffer;
	uint32 CreationFrameNumber;

	FSkelotInstanceBufferOFR()
	{
		CreationFrameNumber = GFrameNumberRenderThread;
	}
	virtual ~FSkelotInstanceBufferOFR();
};
#endif



struct FProxyLODData
{
	uint8 bAllMaterialsSame : 1;
	uint8 bUseUnifiedMesh : 1;
	uint8 bAllSectionsCastShadow : 1;
	uint8 bAnySectionCastShadow : 1;
	uint8 bHasAnyTranslucentMaterial : 1;	//true if any section of this LOD has trans material
	uint8 bSameMaxBoneInfluence : 1;	//true if all sections have the same MBI

	int SectionsMaxBoneInfluence;
	int SectionsNumTriangle;
	int SectionsNumVertices;
};



class FSkelotProxy : public FPrimitiveSceneProxy
{
public:
	typedef FPrimitiveSceneProxy Super;

	FSkelotProxy(const USkelotComponent* Component, FName ResourceName);
	~FSkelotProxy();

	SIZE_T GetTypeHash() const override;
	bool CanBeOccluded() const override;
	void CreateRenderThreadResources() override;
	void DestroyRenderThreadResources() override;
	uint32 GetMemoryFootprint(void) const override;
	void ApplyWorldOffset(FVector InOffset) override;

	void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;
	const TArray<FBoxSphereBounds>* GetOcclusionQueries(const FSceneView* View) const override;
	void AcceptOcclusionResults(const FSceneView* View, TArray<bool>* Results, int32 ResultsStart, int32 NumResults) override;
	virtual bool HasSubprimitiveOcclusionQueries() const override;

	FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;
	uint32 GetAllocatedSize(void) const { return (FPrimitiveSceneProxy::GetAllocatedSize()); }
	
	void SetDynamicDataRT(FSkelotDynamicData* pData);

	void GetShadowShapes(FVector PreViewTranslation, TArray<FCapsuleShape3f>& CapsuleShapes) const override;
	void GetLightRelevance(const FLightSceneProxy* LightSceneProxy, bool& bDynamic, bool& bRelevant, bool& bLightMapped, bool& bShadowMapped) const override
	{
		bDynamic = true;
		bRelevant = true;
		bLightMapped = false;
		bShadowMapped = true;
	}

	

	float InstanceMaxDrawDistance;
	float InstanceMinDrawDistance;
	float LocalBoxCoveringRadius;
	float InstanceCoveringRadius;
	FMaterialRelevance MaterialRelevance;
	TArray<FMaterialRenderProxy*, TInlineAllocator<16>> MaterialsProxy;
	FProxyLODData LODData[SKELOT_MAX_LOD];
	
	const FSkeletalMeshRenderData* SkeletalRenderData;
	USkelotAnimCollection* AnimSet;
	USkeletalMesh* SkeletalMesh;
	FSkelotMeshDataExPtr MeshDataEx;
	int MeshDefIndex;
	uint8 MinLODIndex;
	uint8 MaxLODIndex;
	uint8 ShadowLODBias;
	ESkelotInstanceSortMode SortMode;
	bool bNeedCustomDataForShadowPass;
	bool bHasAnyTranslucentMaterial;	//true if we any of the LODS have any translucent section
	float LODDistances[SKELOT_MAX_LOD - 1];
	float DistanceScale;
	int NumCustomDataFloats;

	FSkelotDynamicData* DynamicData;
	FSkelotDynamicData* OldDynamicData;


};


