/*
Copyright 2023 UPO33.All Rights Reserved.
*/


#pragma once


#include "SkelotBase.h"
#include "Engine/DataAsset.h"
#include "SkelotRenderResources.h"
#include "Chaos/Real.h"

#include "SkelotAnimCollection.generated.h"

class UAnimSequenceBase;
class USkelotAnimCollection;
class USkeletalMesh;




USTRUCT(BlueprintType)
struct SKELOT_API FSkelotBlendDef
{
	GENERATED_USTRUCT_BODY()

	//the animation we blend to, it's generated form 0 to @Duration
	UPROPERTY(EditAnywhere, meta=(NoResetToDefault), Category = "Skelot|AnimCollection")
	UAnimSequenceBase* BlendTo = nullptr;
	//duration of blend in seconds
	UPROPERTY(EditAnywhere, Category = "Skelot|AnimCollection", meta=(UIMin=0.1f, UIMax=0.3f, NoResetToDefault))
	float Duration = 0.1;
	//index of the pose in animation buffer
	UPROPERTY(VisibleAnywhere, Category = "Skelot|AnimCollection")
	uint16 BasePoseIndex = 0;
	//number of pose generated for this blend
	UPROPERTY(VisibleAnywhere, Category = "Skelot|AnimCollection")
	uint16 PoseCount = 0;

};

USTRUCT()
struct FSkelotSimpleAnimNotifyEvent
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	float Time;
	UPROPERTY()
	FName Name;

};

USTRUCT(BlueprintType)
struct SKELOT_API FSkelotSequenceDef
{
	GENERATED_USTRUCT_BODY()

	//animation sequence 
	UPROPERTY(EditAnywhere, Category = "Skelot|AnimCollection", meta=(NoResetToDefault))
	UAnimSequenceBase* Sequence;
	//number of samples in a second
	UPROPERTY(EditAnywhere, Category = "Skelot|AnimCollection", meta=(UIMin=10, UIMax=60, NoResetToDefault))
	uint16 SampleFrequency;
	UPROPERTY(VisibleAnywhere, Category = "Skelot|AnimCollection")
	uint16 BasePoseIndex;
	//number of pose generated, AKA FramCount
	UPROPERTY(VisibleAnywhere, Category = "Skelot|AnimCollection")
	uint16 PoseCount;
	//copied from UAnimSequence, we don't need to touch @Sequence
	UPROPERTY()
	float SequenceLength;
	//copied from SampleFrequency, to avoid int to float cast
	UPROPERTY()
	float SampleFrequencyFloat;
	//
	UPROPERTY(EditAnywhere, Category = "Skelot|AnimCollection")
	TArray<FSkelotBlendDef> Blends;
	//we save name only notifications of animation sequence for faster access
	UPROPERTY()
	TArray<FSkelotSimpleAnimNotifyEvent> Notifies;


	FSkelotSequenceDef();

	int CalcFrameIndex(float time) const;

	float GetSequenceLength() const { return SequenceLength; }

	const FSkelotBlendDef* FindBlendDef(const UAnimSequenceBase* Anim) const;
	int IndexOfBlendDef(const UAnimSequenceBase* Anim) const;
	int IndexOfBlendDefByPath(const FSoftObjectPath& AnimationPath) const;
};



/*
is made from UPhysicsAsset for raycast support
#TODO maybe we can replace it with Chaos objects ?  ChaosInterface::CreateGeometry  ?
*/
struct SKELOT_API FSkelotCompactPhysicsAsset
{
	struct FShapeSphere
	{
		FVector Center;
		float Radius;
		int BoneIndex;
	};

	struct FShapeBox
	{
		FQuat Rotation;
		FVector Center;
		FVector BoxMin, BoxMax;
		int BoneIndex;
		bool bHasTransform;
	};

	struct FShapeCapsule
	{
		FVector A;
		FVector B;
		float Radius;
		int BoneIndex;
	};

	//most of the time physics asset are made of capsules and there is only 1 capsule per bone
	TArray<FShapeCapsule> Capsules;
	TArray<FShapeSphere> Spheres;
	TArray<FShapeBox> Boxes;


	void Init(const USkeleton* Skeleton, const UPhysicsAsset* PhysAsset);
	//trace a ray and return reference skeleton bone index of the first shape hit (not the closest necessarily), -1 otherwise
	int RayCastAny(const USkelotAnimCollection* AnimCollection, int AnimFrameIndex, const FVector& Start, const FVector& Dir, Chaos::FReal Len) const;
	//test if any shape overlaps the specified point, returns the 
	//return reference skeleton bone index of the first overlap (not the closest necessarily)
	int Overlap(const USkelotAnimCollection* AnimCollection, int AnimFrameIndex, const FVector& Point, Chaos::FReal Thickness) const;
	//trace a ray and return reference skeleton bone index of the closest shape hit, -1 otherwise
	int Raycast(const USkelotAnimCollection* AnimCollection, int AnimFrameIndex, const FVector& StartPoint, const FVector& Dir, Chaos::FReal Length, Chaos::FReal Thickness, Chaos::FReal& OutTime, FVector& OutPosition, FVector& OutNormal) const;

};

extern FArchive& operator <<(FArchive& Ar, FSkelotCompactPhysicsAsset& PA);
extern FArchive& operator <<(FArchive& Ar, FSkelotCompactPhysicsAsset::FShapeSphere& Shape);
extern FArchive& operator <<(FArchive& Ar, FSkelotCompactPhysicsAsset::FShapeBox& Shape);
extern FArchive& operator <<(FArchive& Ar, FSkelotCompactPhysicsAsset::FShapeCapsule& Shape);

USTRUCT(BlueprintType)
struct SKELOT_API FSkelotMeshDef
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = "Skelot|AnimCollection")
	USkeletalMesh* Mesh;
	//which LOD should be used for generating bounding box 
	UPROPERTY(EditAnywhere, Category = "Skelot|AnimCollection")
	int BoundigBoxLOD;
	//if true bounding box is generated from PhysicAsset instead of SkeletalMesh
	UPROPERTY(EditAnywhere, Category = "Skelot|AnimCollection")
	bool bBoundigBoxUsePhysicsAsset;
	//set -1 if this mesh should generate its own bounds, otherwise a valid index in Meshes array.
	//most of the time meshes are nearly identical in size so bounds can be shared.
	UPROPERTY(EditAnywhere, Category = "Skelot|AnimCollection")
	int8 OwningBoundMeshIndex;
	//
	UPROPERTY(EditAnywhere, Category = "Skelot|AnimCollection")
	FVector BoundExpand;
	
	//mesh data contacting our own SkinWeight and VertexFactory
	FSkelotMeshDataExPtr MeshData;
	//bounds generated from sequences, we don't generate for Blends because that would be a lot and they don't differ much, so Bounds.Num() == AnimSet->PoseCountSequences
	TArray<FBoxCenterExtentFloat> Bounds;
	//
	FBox MaxBBox;
	//
	float MaxCoveringRadius;

	FSkelotCompactPhysicsAsset CompactPhysicsAsset;

	FSkelotMeshDef();
};

struct FSkelotTransformArray
{
	TArray<FVector3f> Positions;
	//same length as Positions
	TArray<FQuat4f> Rotations;
	//zero length if all scales are 1, otherwise same length as above
	TArray<FVector3f> Scales;
};

inline FArchive& operator <<(FArchive& Ar, FSkelotTransformArray& TA)
{
	Ar << TA.Positions << TA.Rotations << TA.Scales;
	return Ar;
}





/*
* data asset that generates and keeps animation data. typically you need one per skeleton.
*/
UCLASS(BlueprintType)
class SKELOT_API USkelotAnimCollection : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimCollection")
	USkeleton* Skeleton;
	//
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimCollection")
	TArray<FSkelotSequenceDef> Sequences;

	//bones that we need their transform be available on memory. for attaching, sockets, ...
	UPROPERTY(EditAnywhere, Category = "AnimCollection", meta = (GetOptions = "GetValidBones"))
	TSet<FName> BonesToCache;
	//meshes that need to be rendered must be added here, we generate our own SkinWieght and bounds for them.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimCollection")
	TArray<FSkelotMeshDef> Meshes;

	UPROPERTY(VisibleAnywhere, Category = "AnimCollection")
	int BoneCount;
	UPROPERTY(VisibleAnywhere, Category = "AnimCollection")
	int PoseCount;
	//number of pose generated for sequences
	UPROPERTY(VisibleAnywhere, Category = "AnimCollection")
	int PoseCountSequences;
	//number of pose generated for blends
	UPROPERTY(VisibleAnywhere, Category = "AnimCollection")
	int PoseCountBlends;
	UPROPERTY(EditAnywhere, Category = "AnimCollection")
	bool bExtractRootMotion;
	//
	UPROPERTY(EditAnywhere, Category = "AnimCollection", meta=(DisplayAfter="BonesToCache"))
	bool bCachePhysicsAssetBones;
	//true if animation data should be kept as float32 instead of float16, with low precision you may see jitter in places like fingers
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimCollection")
	bool bHighPrecision;
	//
	bool bNeedRebuild;
	//
	FRenderCommandFence ReleaseResourcesFence;
	//cached bone transforms used for attaching, accessed by [boneIndex][frameIndex]
	TArray<FSkelotTransformArray> BonesTransform;
	//
	FSkelotAnimationBuffer AnimationBuffer;
	

	USkelotAnimCollection();
	virtual ~USkelotAnimCollection();


	UFUNCTION()
	TArray<FName> GetValidBones() const;


	FTransform3f GetBoneTransformFast(int BoneIndex, int FrameIndex) const
	{
		const FSkelotTransformArray& TA = BonesTransform[BoneIndex];
		return FTransform3f(TA.Rotations[FrameIndex], TA.Positions[FrameIndex], TA.Scales.Num() ? TA.Scales[FrameIndex] : FVector3f::OneVector);
	}
	FTransform3f GetBoneTransform(int BoneIndex, int FrameIndex) const
	{
		if (BonesTransform.IsValidIndex(BoneIndex) && BonesTransform[BoneIndex].Positions.IsValidIndex(FrameIndex))
			return GetBoneTransformFast(BoneIndex, FrameIndex);

		return FTransform3f::Identity;
	}


	void PostInitProperties() override;
	void PostLoad() override;
#if WITH_EDITOR
	void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	void PreEditChange(FProperty* PropertyThatWillChange) override;
#endif

	bool IsPostLoadThreadSafe() const override { return false; }
	void BeginDestroy() override;
	bool IsReadyForFinishDestroy() override;
	void FinishDestroy() override;
	void GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) override;
	bool CanBeClusterRoot() const override { return false; }
	void Serialize(FArchive& Ar) override;

#if WITH_EDITOR
	void BeginCacheForCookedPlatformData(const ITargetPlatform* TargetPlatform);
	//void ClearCachedCookedPlatformData(const ITargetPlatform* TargetPlatform) {  }
	void ClearAllCachedCookedPlatformData();
#endif

	int GetBoneCount() const;
	int GetPoseCount() const;

	UFUNCTION(CallInEditor, Category = "AnimCollection")
	void Rebuild();
	UFUNCTION(CallInEditor, Category = "AnimCollection")
	void RemoveBlends();
	UFUNCTION(CallInEditor, Category = "AnimCollection")
	void AddAllBlends();

	void BuildAnimations();
	void DestroyBuildData();
	void TryBuildAll();

	int FindSequenceDef(const UAnimSequenceBase* animation) const;
	int FindSequenceDefByPath(const FSoftObjectPath& AnimationPath) const;
	int FindMeshDef(const USkeletalMesh* Mesh) const;
	int FindMeshDefByPath(const FSoftObjectPath& MeshPath) const;

	FSkelotMeshDataExPtr FindMeshData(const USkeletalMesh* mesh) const;
	
	UFUNCTION(BlueprintCallable, Category = "Skelot|AnimCollection")
	UAnimSequenceBase* GetRandomAnimSequenceFromSteam(const FRandomStream& RandomSteam) const;
	UFUNCTION(BlueprintCallable, Category = "Skelot|AnimCollection")
	UAnimSequenceBase* GetRandomAnimSequence() const;
	UFUNCTION(BlueprintCallable, Category = "Skelot|AnimCollection")
	UAnimSequenceBase* FindAnimByName(const FString& ContainingName);
	UFUNCTION(BlueprintCallable, Category = "Skelot|AnimCollection")
	USkeletalMesh* FindMeshByName(const FString& ContainingName);
	UFUNCTION(BlueprintCallable, Category = "Skelot|AnimCollection")
	USkeletalMesh* GetRandomMesh() const;
	UFUNCTION(BlueprintCallable, Category = "Skelot|AnimCollection")
	USkeletalMesh* GetRandomMeshFromSteam(const FRandomStream& RandomSteam) const;

	void InitMeshDataResoruces();
	void ReleaseMeshDataResources();


	void PostDuplicate(bool bDuplicateForPIE) override;

};

