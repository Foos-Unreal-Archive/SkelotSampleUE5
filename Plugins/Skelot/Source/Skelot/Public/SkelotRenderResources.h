/*
Copyright 2023 UPO33.All Rights Reserved.
*/


#pragma once


#include "Engine/SkeletalMesh.h"
#include "VertexFactory.h"
#include "SkelotBase.h"
#include "Rendering/SkinWeightVertexBuffer.h"
#include "RenderResource.h"
#include "SceneManagement.h"
#include "Skelot.h"



class FSkelotSkinWeightVertexBuffer;
class USkelotAnimCollection;
class FSkeletalMeshLODRenderData;

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FSkelotVertexFactoryParameters, )
SHADER_PARAMETER(uint32, BoneCount)
SHADER_PARAMETER(uint32, InstanceOffset)
SHADER_PARAMETER(uint32, MaxInstance /* NumInstance - 1 */)
SHADER_PARAMETER(uint32, NumCustomDataFloats)
SHADER_PARAMETER(uint32, CustomDataInstanceOffset)
SHADER_PARAMETER_SRV(Buffer<float4>, AnimationBuffer)
SHADER_PARAMETER_SRV(Buffer<float4>, Instance_Transforms)
SHADER_PARAMETER_SRV(Buffer<uint>, Instance_AnimationFrameIndices)
SHADER_PARAMETER_SRV(Buffer<float>, Instance_CustomData)
END_GLOBAL_SHADER_PARAMETER_STRUCT()

typedef TUniformBufferRef<FSkelotVertexFactoryParameters> FSkelotVertexFactoryBufferRef;


/*
*/
class FSkelotBaseVertexFactory : public FVertexFactory
{
public:
	typedef FVertexFactory Super;

	//DECLARE_VERTEX_FACTORY_TYPE(FSkelotBaseVertexFactory);

	struct FDataType : FStaticMeshDataType
	{
		/** The stream to read the bone indices from */
		FVertexStreamComponent BoneIndices;
		/** The stream to read the bone weights from */
		FVertexStreamComponent BoneWeights;
	};

	FSkelotBaseVertexFactory(ERHIFeatureLevel::Type InFeatureLevel) : Super(InFeatureLevel)
	{
	}
	~FSkelotBaseVertexFactory()
	{
	}
	void FillData(FDataType& data, const FSkelotSkinWeightVertexBuffer* weightBuffer, const FSkeletalMeshLODRenderData* LODData) const;
	void SetData(const FDataType& data);

	FString GetFriendlyName() const override { return TEXT("FSkelotBaseVertexFactory"); }

	int MaxBoneInfluence;



};

enum ESkelotVerteFactoryMode
{
	EVF_BoneInfluence1,
	EVF_BoneInfluence2,
	EVF_BoneInfluence3,
	EVF_BoneInfluence4,

	EVF_Max,
};


template<ESkelotVerteFactoryMode FactoryMode> class TSkelotVertexFactory : public FSkelotBaseVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(TSkelotVertexFactory<FactoryMode>);
	
	typedef FSkelotBaseVertexFactory Super;

	TSkelotVertexFactory(ERHIFeatureLevel::Type InFeatureLevel) : Super(InFeatureLevel)
	{
		MaxBoneInfluence = StaticMaxBoneInfluence();
	}
	static int StaticMaxBoneInfluence()
	{
		switch (FactoryMode)
		{
		case EVF_BoneInfluence1: return 1; 
		case EVF_BoneInfluence2: return 2;
		case EVF_BoneInfluence3: return 3;
		case EVF_BoneInfluence4: return 4;
		}
		return 4;
	}

	static bool ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters);
	static void ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);
	static bool SupportsTessellationShaders() { return false; }
	static void ValidateCompiledResult(const FVertexFactoryType* Type, EShaderPlatform Platform, const FShaderParameterMap& ParameterMap, TArray<FString>& OutErrors);
};

/** A vertex buffer storing bone index/weight data. */
class FSkelotSkinWeightVertexBuffer : public FVertexBuffer
{
public:
	static const int MAX_INFLUENCE = 4;

	struct FBoneWeight
	{
		uint8 BoneIndex[MAX_INFLUENCE];
		uint8 BoneWeight[MAX_INFLUENCE];

		friend inline FArchive& operator <<(FArchive& Ar, FBoneWeight& data)
		{
			Ar.Serialize(&data, sizeof(FBoneWeight));
			return Ar;
		}
	};

	FStaticMeshVertexDataInterface* WeightData;

	FSkelotSkinWeightVertexBuffer() : WeightData(nullptr)
	{
	}
	~FSkelotSkinWeightVertexBuffer();

	void Serialize(FArchive& Ar);

	void InitRHI() override;
	void ReleaseRHI() override;
	void AllocBuffer();
	void InitBuffer(const TArray<FSkinWeightInfo>& weightsArray);
};

//vertex buffer containing bone transforms of all baked animations
struct FSkelotAnimationBuffer : FRenderResource
{
	FStaticMeshVertexDataInterface* Tranforms = nullptr;

	FBufferRHIRef Buffer;
	FShaderResourceViewRHIRef ShaderResourceViewRHI;

	bool bHighPrecision = false;

	~FSkelotAnimationBuffer();
	void InitRHI() override;
	void ReleaseRHI() override;
	void Serialize(FArchive& Ar);

	void AllocateBuffer();
	void InitBuffer(const TArrayView<FTransform> inTransforms, bool inHightPercision);
	void DestroyBuffer();
};


/*
*/
class FSkelotMeshDataEx
{
public:
	struct FLODData
	{
		FSkelotSkinWeightVertexBuffer SkinWeight;
		TSkelotVertexFactory<EVF_BoneInfluence1> VertexFactory1;
		TSkelotVertexFactory<EVF_BoneInfluence2> VertexFactory2;
		TSkelotVertexFactory<EVF_BoneInfluence3> VertexFactory3;
		TSkelotVertexFactory<EVF_BoneInfluence4> VertexFactory4;

		FLODData(ERHIFeatureLevel::Type InFeatureLevel) : VertexFactory1(InFeatureLevel), VertexFactory2(InFeatureLevel), VertexFactory3(InFeatureLevel), VertexFactory4(InFeatureLevel)
		{

		}

		FSkelotBaseVertexFactory* GetVertexFactory(int MaxBoneInfluence)
		{
			check(MaxBoneInfluence > 0 && MaxBoneInfluence <= 4);
			FSkelotBaseVertexFactory* LUT[] = { nullptr, &VertexFactory1, &VertexFactory2, &VertexFactory3, &VertexFactory4 };
			return LUT[MaxBoneInfluence];
		}
		void InitResources();
		void ReleaseResouces();

		void FillVertexFactories(const FSkeletalMeshLODRenderData* LODData);
		void Serialize(FArchive& Ar);
	};

	TArray<FLODData, TFixedAllocator<SKELOT_MAX_LOD>> LODs;

	#if WITH_EDITOR
	void InitFromMesh(USkeletalMesh* SKMesh, const USkelotAnimCollection* AnimSet);
	#endif
	void InitResources();
	void ReleaseResouces();
	void Serialize(FArchive& Ar);

	void FillVertexFactories(const FSkeletalMeshRenderData* SKRenderData);
};

typedef TSharedPtr<FSkelotMeshDataEx, ESPMode::ThreadSafe> FSkelotMeshDataExPtr;

//with prev frame data
struct FSkelotInstanceBuffer : TSharedFromThis<FSkelotInstanceBuffer>
{
	static const uint32 SizeAlign = 8192;	//must be pow2	InstanceCount is aligned to this 

	FBufferRHIRef TransformVB;
	FShaderResourceViewRHIRef TransformSRV;
	FBufferRHIRef FrameIndexVB;
	FShaderResourceViewRHIRef FrameIndexSRV;
	uint32 CreationFrameNumber = 0;
	uint32 InstanceCount = 0;

	SkelotShaderMatrixT* MappedTransforms = nullptr;
	uint32* MappedFrameIndices = nullptr;

	void LockBuffers();
	void UnlockBuffers();
	bool IsLocked() const { return MappedTransforms != nullptr; }
	uint32 GetSize() const { return InstanceCount; }

	static TSharedPtr<FSkelotInstanceBuffer> Create(uint32 InstanceCount);
};

//without prev frame data
struct FSkelotInstanceShadowBuffer : TSharedFromThis<FSkelotInstanceShadowBuffer>
{
	static const uint32 SizeAlign = 8192;	//must be pow2	InstanceCount is aligned to this 

	FBufferRHIRef TransformVB;
	FShaderResourceViewRHIRef TransformSRV;
	FBufferRHIRef FrameIndexVB;
	FShaderResourceViewRHIRef FrameIndexSRV;
	uint32 CreationFrameNumber = 0;
	uint32 InstanceCount = 0;

	SkelotShaderMatrixT* MappedTransforms = nullptr;
	uint16* MappedFrameIndices = nullptr;

	void LockBuffers();
	void UnlockBuffers();
	bool IsLocked() const { return MappedTransforms != nullptr; }
	uint32 GetSize() const { return InstanceCount; }

	static TSharedPtr<FSkelotInstanceShadowBuffer> Create(uint32 InstanceCount);
};


typedef TSharedPtr<FSkelotInstanceBuffer> FSkelotInstanceBufferPtr;
typedef TSharedPtr<FSkelotInstanceShadowBuffer> FSkelotInstanceShadowBufferPtr;



template<typename TResoruce> struct TBufferAllocator
{
	struct FAllocation
	{
		TResoruce Resource;	//must be TSharedPtr
		uint32 Offset = 0;

		uint32 Avail() const { return Resource->GetSize() - Offset; }
	};

	struct FPool : FAllocation
	{
		uint32 UnusedCounter = 0;
	};
	
	TArray<FPool> Pools;
	FPool* Current = nullptr;
	uint32 Counter = 0;

	FAllocation Alloc(uint32 Size )
	{
		check(IsInRenderingThread());

		if(Current && Size > Current->Avail())	//current buffer is not big enough ?
		{
			Current = nullptr;
			for (FPool& Pool : Pools)	//look for compatible pool
			{
				if (Pool.Avail() >= Size)
				{
					Current = &Pool;
					break;
				}
			}
		}
		

		if (Current == nullptr)	//nothing exist so should allocate a new one
		{
			Current = &Pools.AddDefaulted_GetRef();
			Current->Resource = TResoruce::ElementType::Create(Align(Size, TResoruce::ElementType::SizeAlign));
		}

		if(!Current->Resource->IsLocked())	//try map if its not already
			Current->Resource->LockBuffers();

		FAllocation Alc;
		Alc.Resource = Current->Resource;
		Alc.Offset = Current->Offset;
		Current->Offset += Size;
		return Alc;
	}
	void Commit()
	{
		check(IsInRenderingThread());

		Current = nullptr;
		
		Pools.RemoveAllSwap([&](FPool& Pool){
			if (Pool.Resource->IsLocked())
			{
				Pool.Offset = 0;
				Pool.UnusedCounter = 0;
				Pool.Resource->UnlockBuffers();
			}
			else
			{
				constexpr uint32 ReleaseThreshold = 120;
				if (Pool.UnusedCounter++ >= ReleaseThreshold)
				{
					Pool.Resource = nullptr;
					return true;
				}
			}
			return false;
		});

		Current = Pools.Num() ? &Pools[0] : nullptr;
	}

};

typedef TBufferAllocator<FSkelotInstanceBufferPtr> FSkelotInstanceBufferAllocator;
typedef TBufferAllocator<FSkelotInstanceShadowBufferPtr> FSkelotInstanceShadowBufferAllocator;

extern FSkelotInstanceBufferAllocator GSkelotInstanceBufferAllocatorForInitViews;
extern FSkelotInstanceShadowBufferAllocator GSkelotInstanceBufferAllocatorForInitShadows;



struct FSkelotCIDBuffer : TSharedFromThis<FSkelotCIDBuffer>
{
	static const uint32 SizeAlign = 4096;	//must be pow2	InstanceCount is aligned to this 

	FBufferRHIRef CustomDataBuffer;
	FShaderResourceViewRHIRef CustomDataSRV;
	
	uint32 CreationFrameNumber = 0;
	uint32 NumberOfFloat = 0;

	float* MappedData = nullptr;

	void LockBuffers();
	void UnlockBuffers();
	bool IsLocked() const { return MappedData != nullptr; }
	uint32 GetSize() const { return NumberOfFloat; }

	static TSharedPtr<FSkelotCIDBuffer> Create(uint32 InNumberOfFloat);
};

typedef TSharedPtr<FSkelotCIDBuffer> FSkelotCIDBufferPtr;

typedef TBufferAllocator<FSkelotCIDBufferPtr> FSkelotCIDBufferAllocator;
//#TODO do we need two allocators ?
extern FSkelotCIDBufferAllocator GSkelotCIDBufferAllocatorForInitViews;
extern FSkelotCIDBufferAllocator GSkelotCIDBufferAllocatorForInitShadows;

struct FSkelotBatchElementOFR : FOneFrameResource
{
	uint32 MaxBoneInfluences;
	FSkelotBaseVertexFactory* VertexFactory;
	TUniformBufferRef<FSkelotVertexFactoryParameters> UniformBuffer;	//uniform buffer holding data for drawing a LOD
};