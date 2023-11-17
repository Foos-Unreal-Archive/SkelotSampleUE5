/*
Copyright 2023 UPO33.All Rights Reserved.
*/

#pragma once

#include "Engine/SkeletalMesh.h"
#include "Components/MeshComponent.h"
#include "Containers/RingBuffer.h"
#include "Subsystems/WorldSubsystem.h"
#include "Skelot.h"
#include "SkelotBase.h"
#include "Chaos/Real.h"


#include "SkelotComponent.generated.h"

class UMaterialInterface;
class USkelotComponent;
class USkelotAnimCollection;
class UUserDefinedStruct;
struct FSkelotBlendDef;

UENUM()
enum class ESkelotInstanceFlags : uint16
{
	EIF_None = 0,

	EIF_Destroyed			= 1 << 0,
	EIF_New					= 1 << 1,	//instance is new, doesn't have prev frame data
	EIF_Hidden				= 1 << 2,	//instance is culled
	EIF_HiddenShadow		= 1 << 3,	//AKA DontCastShadow
	
	EIF_Reserved			= 1 << 4,

	EIF_AnimSkipTick		= 1 << 5, //ignore current tick
	EIF_AnimPaused			= 1 << 6, //animation is paused
	EIF_AnimLoop			= 1 << 7, //animation is looped
	EIF_AnimPlayingBlend	= 1 << 8, //animation is playing blending
	EIF_AnimNoSequence		= 1 << 9, //there is no sequence, CurrentSquence is -1 in this case
	EIF_AnimFinished		= 1 << 10, //animation is finished, CurrentSquence still has valid value

	//these  flags can be used by developer
	EIF_UserFlag0			= 1 << 11,
	EIF_UserFlag1			= 1 << 12,
	EIF_UserFlag2			= 1 << 13,
	EIF_UserFlag3			= 1 << 14,
	EIF_UserFlag4			= 1 << 15,

	EIF_AllAnimationFlags = EIF_AnimSkipTick | EIF_AnimPaused | EIF_AnimLoop | EIF_AnimPlayingBlend | EIF_AnimNoSequence | EIF_AnimFinished,

	EIF_Default = EIF_New | EIF_AnimNoSequence,
};

static const int InstaceUserFlagStart = 11;

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EInstanceUserFlags : uint8
{
	None = 0 UMETA(Hidden),

	UserFlag0 = 1 << 0,
	UserFlag1 = 1 << 1,
	UserFlag2 = 1 << 2,
	UserFlag3 = 1 << 3,
	UserFlag4 = 1 << 4,
};

ENUM_CLASS_FLAGS(EInstanceUserFlags);

UENUM(BlueprintType)
enum class ESkelotInstanceSortMode : uint8
{
	SortNothing,
	SortAnything,	//sort instances of a LOD
	SortTranslucentOnly,	//sort only if LOD has any translucent material
};

ENUM_CLASS_FLAGS(ESkelotInstanceFlags);



USTRUCT(BlueprintType)
struct SKELOT_API FSkelotInstanceAnimState
{
	GENERATED_USTRUCT_BODY()

	float Time = 0;
	float PlayScale = 1;
	//index for AnimCollection->Sequences, currently playing sequnce index
	int16 CurrentSequence = -1;	
	//
	int16 BlendBlendIndex = -1;
	//index of sequence we were playing before
	int16 SequenceFromIndex = -1;
	//animation buffer pose index that blend starts from
	uint16 BlendStartPoseIndex = 0;	

	bool IsValid() const { return CurrentSequence != -1; }

	void Tick(USkelotComponent* owner, int32 instanceIndex, float delta);

	friend FArchive& operator<<(FArchive& Ar, FSkelotInstanceAnimState& R)
	{
		Ar << R.Time << R.PlayScale << R.CurrentSequence << R.BlendBlendIndex << R.SequenceFromIndex << R.BlendStartPoseIndex;
		return Ar;
	}
	bool Serialize(FArchive& Ar)
	{
		Ar << *this;
		return true;
	}
};

/*
* SOA to keep data of instances
*/
USTRUCT()
struct SKELOT_API FSkelotInstancesData
{
	GENERATED_BODY()

	TArray<ESkelotInstanceFlags> Flags;
	TArray<uint16> FrameIndices;	//animation buffer pose index
	TArray<FSkelotInstanceAnimState> AnimationStates;
	TArray<uint16> BoundIndices;

	//3 arrays instead of FTransform3f because most of the time Scale is untouched
	TArray<FVector3f> Locations;
	TArray<FQuat4f> Rotations;
	TArray<FVector3f> Scales;

	TArray<FMatrix44f> Matrices;
	TArray<SkelotShaderMatrixT> RenderMatrices;

	TArray<float> RenderCustomData;

	void CheckValid() const
	{
		int len = Flags.Num();
		check(FrameIndices.Num() == len && AnimationStates.Num() == len && BoundIndices.Num() == len && Locations.Num() == len && Rotations.Num() == len && Scales.Num() == len);
	}
	void Reset()
	{
		Flags.Reset();
		FrameIndices.Reset();
		AnimationStates.Reset();
		BoundIndices.Reset();
		Locations.Reset();
		Rotations.Reset();
		Scales.Reset();
		RenderCustomData.Reset();
	}
	void Empty()
	{
		Flags.Empty();
		FrameIndices.Empty();
		AnimationStates.Empty();
		BoundIndices.Empty();
		Locations.Empty();
		Rotations.Empty();
		Scales.Empty();
		RenderCustomData.Empty();
	}

	friend FArchive& operator<<(FArchive& Ar, FSkelotInstancesData& R)
	{
		Ar << R.Flags << R.FrameIndices << R.AnimationStates << R.BoundIndices;
		Ar << R.Locations << R.Rotations << R.Scales;
		Ar << R.RenderCustomData;
		R.CheckValid();
		return Ar;
	}

	bool Serialize(FArchive& Ar)
	{
		Ar << *this;
		return true;
	}
};


template<> struct TStructOpsTypeTraits<FSkelotInstanceAnimState> : public TStructOpsTypeTraitsBase2<FSkelotInstanceAnimState>
{
	enum { WithSerializer = true, };
};

template<> struct TStructOpsTypeTraits<FSkelotInstancesData> : public TStructOpsTypeTraitsBase2<FSkelotInstancesData>
{
	enum { WithSerializer = true, };
};


USTRUCT(BlueprintType)
struct FSkelotAnimFinishEvent
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Skelot|AnimCollection")
	int InstanceIndex = -1;
	UPROPERTY(BlueprintReadWrite, Category = "Skelot|AnimCollection")
	UAnimSequenceBase* AnimSequence = nullptr;
};

USTRUCT(BlueprintType)
struct FSkelotAnimNotifyEvent
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Skelot|AnimCollection")
	int InstanceIndex = -1;
	UPROPERTY(BlueprintReadWrite, Category = "Skelot|AnimCollection")
	UAnimSequenceBase* AnimSequence = nullptr;
	UPROPERTY(BlueprintReadWrite, Category = "Skelot|AnimCollection")
	FName NotifyName;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSkelotAnimationFinished, USkelotComponent*, Component, const TArray<FSkelotAnimFinishEvent>&, Events);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSkelotAnimationNotify, USkelotComponent*, Component, const TArray<FSkelotAnimNotifyEvent>&, Events);

/*
* component for rendering instanced skeletal mesh.
* this component is designed to be runtime only, you can't select or edit instances in editor world, and also transform of the component doesn't matter since all instances are in world space.

this component handles frustum cull and LODing of its instances, so you can use it as a singleton for one type of skeletal mesh.
but its recommended to spawn one component for a group/flock of instances in order to utilize engine's per component frustum  and occlusion culling.

*/
UCLASS(Blueprintable, BlueprintType, editinlinenew, meta = (BlueprintSpawnableComponent),hideCategories=("Collision", "Physics", "Navigation", "HLOD"))
class SKELOT_API USkelotComponent : public UMeshComponent
{
	GENERATED_UCLASS_BODY()
public:

	//
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skelot")
	USkelotAnimCollection* AnimCollection;
	//skeletal mesh to render, should exist in AnimCollection->Meshes[]
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skelot")
	USkeletalMesh* SkeletalMesh;
	//max draw distance used for culling instances
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skelot")
	float InstanceMaxDrawDistance;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skelot")
	float InstanceMinDrawDistance;
	//how much to expand bounding box of this component.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skelot")
	FVector3f ComponentBoundExtent;
	// Defines the number of floats that will be available per instance for custom data 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skelot")
	int32 NumCustomDataFloats;
	//distance based LOD. must be sorted.
	UPROPERTY(EditAnywhere, Category = "Skelot")
	float LODDistances[SKELOT_MAX_LOD - 1];
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skelot")
	float LODDistanceScale;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skelot")
	uint8 MinLOD;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skelot")
	uint8 MaxLOD;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skelot", meta=(DisplayName="Shadow LOD Bias"))
	uint8 ShadowLODBias;
	//true if per instance custom data should be generated for instances being sent to shadow pass. 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skelot")
	uint8 bNeedCustomDataForShadowPass : 1;
	//generate inaccurate component bound from position of instances
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skelot")
	uint8 bPositionOnlyBound : 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skelot")
	uint8 bIgnoreAnimationsTick : 1;
	UPROPERTY(Transient)
	uint8 bAnyTransformChanged : 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skelot")
	ESkelotInstanceSortMode SortMode;
	UPROPERTY(Transient)
	FSkelotInstancesData InstancesData;
	//indices of destroyed instances
	UPROPERTY(Transient)
	TArray<int> FreeInstances;

	UPROPERTY(Transient)
	int MeshDataIndex;
	//
	int PrevDynamicDataInstanceCount;
	
	FBoxMinMaxFloat CachedBounds;

	//BP arrays that start with 'InstanceData_'. valid on CDO only
	TArray<FArrayProperty*> InstanceDataArrays;
	//
	const auto& GetBPInstanceDataArrays() const { return GetClass()->GetDefaultObject<USkelotComponent>()->InstanceDataArrays; }

	FPrimitiveSceneProxy* CreateSceneProxy() override;
	//FMatrix GetRenderMatrix() const override { return FMatrix::Identity; }
#if WITH_EDITOR
	void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	void PreEditChange(FProperty* PropertyThatWillChange) override;
#endif

	
	int32 GetNumMaterials() const override;
	UMaterialInterface* GetMaterial(int32 MaterialIndex) const override;
	int32 GetMaterialIndex(FName MaterialSlotName) const override;
	TArray<FName> GetMaterialSlotNames() const override;
	bool IsMaterialSlotNameValid(FName MaterialSlotName) const override;

	void OnEndOfFrameUpdateDuringTick() override;
	
	FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const;
	FBoxSphereBounds CalcLocalBounds() const override;
	void ApplyWorldOffset(const FVector& InOffset, bool bWorldShift) override;
	bool ShouldRecreateProxyOnUpdateTransform() const { return false; }

	void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	void SendRenderDynamicData_Concurrent() override;
	void CreateRenderState_Concurrent(FRegisterComponentContext* Context) override;
	bool IsPostLoadThreadSafe() const override { return false; }
	void PostLoad() override;
	void BeginDestroy() override;

	void OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport = ETeleportType::None) override;
	void DetachFromComponent(const FDetachmentTransformRules& DetachmentRules) override;
	void InitializeComponent() override;
	void UninitializeComponent() override;
	void PostDuplicate(bool bDuplicateForPIE) override;
	void PreDuplicate(FObjectDuplicationParameters& DupParams) override;
	void OnRegister() override;
	void OnUnregister() override;

	TStructOnScope<FActorComponentInstanceData> GetComponentInstanceData() const override;
	void PostInitProperties() override;
	void PostCDOContruct() override;
	
	void TickAnimations(float DeltaTime);

	FBoxMinMaxFloat CalcInstancesBound() const;


	UFUNCTION(BlueprintCallable, meta=(DisplayName="MarkRenderStateDirty"), Category = "Skelot|Rendering")
	void K2_MarkRenderStateDirty() { this->MarkRenderStateDirty(); }

	//
	UFUNCTION(BlueprintCallable, Category = "Skelot")
	void SetAnimCollection(USkelotAnimCollection* asset);
	//
	UFUNCTION(BlueprintCallable, Category = "Skelot")
	void SetMesh(USkeletalMesh* mesh);

	void Internal_SetAnimCollection(USkelotAnimCollection* asset, bool bForce);
	void Internal_SetMesh(USkeletalMesh* mesh, bool bFroce);
	void Internal_CheckAsssets();
	
	UFUNCTION(BlueprintCallable, Category = "Skelot|Animation")
	void ResetInstanceAnimationState(int InstanceIndex);

	//return total number of instances (including destroyed instances)
	UFUNCTION(BlueprintPure, meta=(Keywords="Num Instance,"), Category="Skelot")
	int32 GetInstanceCount() const { return InstancesData.Flags.Num(); }
	//return number of alive instances
	UFUNCTION(BlueprintPure, Category = "Skelot")
	int32 GetAliveInstanceCount() const { return InstancesData.Flags.Num() - FreeInstances.Num(); }
	//return number of destroyed instances
	UFUNCTION(BlueprintPure, Category = "Skelot")
	int32 GetDestroyedInstanceCount() const { return FreeInstances.Num(); }
	
	//return True if instance is not destroyed
	bool IsInstanceAlive(int32 InstanceIndex) const { return !EnumHasAnyFlags(InstancesData.Flags[InstanceIndex], ESkelotInstanceFlags::EIF_Destroyed); }

	//return true if index and flags of the specified instance are valid
	UFUNCTION(BlueprintPure, Category="Skelot")
	bool IsInstanceValid(int32 InstanceIndex) const;
	//add an instance to this component
	//@return	instance index
	UFUNCTION(BlueprintCallable, Category="Skelot")
	int AddInstance(const FTransform3f& WorldTransform);
	//
	UFUNCTION(BlueprintCallable, Category = "Skelot")
	int AddInstance_CopyFrom(const USkelotComponent* Src, int SrcInstanceIndex);
	//Destroy the instance at specified index, Note that this will not remove anything from the arrays but mark the index as destroyed.
	//Returns True on success
	UFUNCTION(BlueprintCallable, Category="Skelot")
	bool DestroyInstance(int InstanceIndex);
	//
	UFUNCTION(BlueprintCallable, Category = "Skelot")
	void DestroyInstances(const TArray<int>& InstanceIndices);
	//
	UFUNCTION(BlueprintCallable, Category = "Skelot")
	void DestroyInstancesByRange(int StartIndex, int Count);
	//
	bool Internal_DestroyAt(int InstanceIndex);
	/*
	remove all destroyed instances from the arrays and shrink the memory, it will invalidate indices.
	typically you need to call it if you have lots of destroyed instances that are not going to be reused. 
	for example you add 100k instances then remove 90k, so 90% of your instances are unused, they are taking memory and iteration takes more time.
	
	@param RemapArray	can be used to map old indices to new indices. NewInstanceIndex = RemapArray[OldInstanceIndex];
	@return				number of freed instances
	*/
	UFUNCTION(BlueprintCallable, meta=(DisplayName="FlushInstances"), Category = "Skelot")
	int K2_FlushInstances(TArray<int>& RemapArray);
	int FlushInstances(TArray<int>* OutRemapArray = nullptr);
	/*
	clear all the instances being rendered by this component.
	@param bEmpty	true if memory should be freed
	*/
	UFUNCTION(BlueprintCallable, Category = "Skelot")
	void ClearInstances(bool bEmpty = true);

	/*
	try to copy instance data (transform, animation state, ...) from another component to the specified instance in this component
	*/
	UFUNCTION(BlueprintCallable, Category = "Skelot")
	void InstanceCopyFrom(const USkelotComponent* SrcComponent, int SrcInstanceIndex, int InstanceIndex);
	
	//subclass can override the following functions to register its own per instance data
	//subclass should set the initial value of an element
	virtual void CustomInstanceData_Initialize(int InstanceIndex) { /*e.g: AgentBodies[InstanceIndex] = new FMyAgentBody();*/ }
	//subclass should destroy the value of an element 
	virtual void CustomInstanceData_Destroy(int InstanceIndex) { /*e.g:  delete AgentBodies[InstanceIndex]; */ }
	//subclass should move the value of an element to another index
	virtual void CustomInstanceData_Move(int DstIndex, int SrcIndex) { /* e.g: AgendBodies[DstIndex] = AgendBodies[SrcIndex]; */ }
	//subclass should change the length of array using SetNum
	virtual void CustomInstanceData_SetNum(int NewNum) { /* e.g: AgendBodies.SetNum(NewNum); */ }


	UFUNCTION(BlueprintCallable, Category = "Skelot|Transform")
	void BatchUpdateTransforms(int StartInstanceIndex, const TArray<FTransform3f>& NewTransforms);

	UFUNCTION(BlueprintCallable, Category = "Skelot|Transform")
	void SetInstanceTransform(int InstanceIndex, const FTransform3f& NewTransform);
	UFUNCTION(BlueprintCallable, Category = "Skelot|Transform")
	void SetInstanceLocation(int InstanceIndex, const FVector3f& NewLocation);
	UFUNCTION(BlueprintCallable, Category = "Skelot|Transform", meta = (DisplayName = "Set Instance Rotation (Rotator)"))
	void SetInstanceRotator(int InstanceIndex, const FRotator3f& NewRotator);
	UFUNCTION(BlueprintCallable, Category = "Skelot|Transform", meta=(DisplayName="Set Instance Rotation (Quat)"))
	void SetInstanceRotation(int InstanceIndex, const FQuat4f& NewRotation);
	UFUNCTION(BlueprintCallable, Category = "Skelot|Transform")
	void SetInstanceScale(int InstanceIndex, const FVector3f& NewScale);
	UFUNCTION(BlueprintCallable, Category = "Skelot|Transform")
	void AddInstanceLocation(int InstanceIndex, const FVector3f& Offset);

	//add offset to the location of all valid instances, mostly used by Blueprint since looping through thousands of instances takes too much time there
	UFUNCTION(BlueprintCallable, Category="Skelot|Transform")
	void MoveAllInstances(const FVector3f& Offset);
	

	UFUNCTION(BlueprintCallable, Category = "Skelot")
	void InstanceAddUserFlags(int InstanceIndex, UPARAM(meta = (Bitmask, BitmaskEnum=EInstanceUserFlags)) int32 Flags)
	{
		InstanceAddFlags(InstanceIndex, static_cast<uint16>(Flags << InstaceUserFlagStart));
	}
	UFUNCTION(BlueprintCallable, Category = "Skelot")
	void InstanceRemoveUserFlags(int InstanceIndex, UPARAM(meta = (Bitmask, BitmaskEnum = EInstanceUserFlags)) int32 Flags)
	{
		InstanceRemoveFlags(InstanceIndex, static_cast<uint16>(Flags << InstaceUserFlagStart));
	}
	UFUNCTION(BlueprintCallable, Category = "Skelot")
	bool InstanceHasAnyUserFlags(int InstanceIndex, UPARAM(meta = (Bitmask, BitmaskEnum = EInstanceUserFlags)) int32 Flags) const
	{
		return InstanceHasAnyFlag(InstanceIndex, static_cast<uint16>(Flags << InstaceUserFlagStart));
	}

	void InstanceAddFlags(int InstanceIndex, uint16 FlagsToAdd)
	{
		check(IsInstanceValid(InstanceIndex));
		EnumAddFlags(InstancesData.Flags[InstanceIndex], (ESkelotInstanceFlags)FlagsToAdd);
	}
	void InstanceRemoveFlags(int InstanceIndex, uint16 FlagsToRemove)
	{
		check(IsInstanceValid(InstanceIndex));
		EnumRemoveFlags(InstancesData.Flags[InstanceIndex], (ESkelotInstanceFlags)FlagsToRemove);
	}
	bool InstanceHasAnyFlag(int InstanceIndex, uint16 FlagsToTest) const
	{
		check(IsInstanceValid(InstanceIndex));
		return EnumHasAnyFlags(InstancesData.Flags[InstanceIndex], (ESkelotInstanceFlags)FlagsToTest);
	}
	

	UFUNCTION(BlueprintPure, Category = "Skelot|Transform")
	FTransform3f GetInstanceTransform(int InstanceIndex) const;
	UFUNCTION(BlueprintPure, Category = "Skelot|Transform")
	const FVector3f& GetInstanceLocation(int InstanceIndex) const;
	UFUNCTION(BlueprintPure, Category = "Skelot|Transform", meta=(Keywords="Get Rotation Quat"))
	const FQuat4f& GetInstanceRotation(int InstanceIndex) const;
	UFUNCTION(BlueprintPure, Category = "Skelot|Transform", meta=(Keywords="Get Rotation Rotator"))
	FRotator3f GetInstanceRotator(int InstanceIndex) const;

	//return current local bounding box of an instance
	const FBoxCenterExtentFloat& GetInstanceLocalBound(int InstanceIndex) const;
	//calculate and return bounding box of an instance in world space
	FBoxCenterExtentFloat CalculateInstanceBound(int InstanceIndex) const;

	void OnInstanceTransformChange(int InstanceIndex);

	UFUNCTION(BlueprintCallable, Category = "Skelot|Rendering")
	bool IsInstanceHidden(int InstanceIndex) const;
	UFUNCTION(BlueprintCallable,Category = "Skelot|Rendering")
	void SetInstanceHidden(int InstanceIndex, bool bHidden);
	UFUNCTION(BlueprintCallable, Category = "Skelot|Rendering")
	void ToggleInstanceVisibility(int InstanceIndex);


	UFUNCTION(BlueprintCallable, Category = "Skelot|Rendering")
	bool IsInstanceCastShadow(int InstanceIndex) const;
	UFUNCTION(BlueprintCallable, Category = "Skelot|Rendering")
	void SetInstanceCastShadow(int InstanceIndex, bool bCastShadow);
	UFUNCTION(BlueprintCallable, Category = "Skelot|Rendering")
	void ToggleInstanceCastShadow(int InstanceIndex);


	/*
	play animation on an instance.
	@param InstanceIndex	instance to play animation on
	@param Animation		animation sequence to be played, should exist in AnimSet->Sequences
	@param bLoop			true if animation should loop, false if needs to be played once (when animation is finished the last pose index is kept and OnAnimationFinished is called so you can play animation inside it again).
	@param bBlendIn			if true then attempt to blend to the specified animation, otherwise play without blending (jumps to the target pose index)
	@param StartAt			time to start playback at, is wrapped to sequence length. only used if @bBlend is False since blending must start from 0.
	@param PlayScale		speed of playback, negative is not supported
	@return					sequence length if playing started, otherwise -1
	*/
	UFUNCTION(BlueprintCallable, Category = "Skelot|Animation")
	UPARAM(DisplayName = "SequenceLength") float InstancePlayAnimation(int InstanceIndex, UAnimSequenceBase* Animation, bool bLoop = true, bool bBlendIn = false, float StartAt = 0, float PlayScale = 1);

	float Internal_InstancePlayAnimation(int InstanceIndex, UAnimSequenceBase * Animation, bool bLoop = true, float StartAt = 0, float PlayScale = 1);
	float Internal_InstancePlayAnimationWithBlend(int InstanceIndex, UAnimSequenceBase* Animation, bool bLoop = true, float PlayScale = 1);
	

	
	/*
	stop playing animation, 
	*/
	UFUNCTION(BlueprintCallable, Category = "Skelot|Animation")
	void StopInstanceAnimation(int InstanceIndex, bool bResetPose);

	UFUNCTION(BlueprintCallable, Category = "Skelot|Animation")
	void PauseInstanceAnimation(int InstanceIndex, bool bPause);
	UFUNCTION(BlueprintCallable, Category = "Skelot|Animation")
	bool IsInstanceAnimationPaused(int InstanceIndex) const;
	UFUNCTION(BlueprintCallable, Category = "Skelot|Animation")
	void ToggleInstanceAnimationPause(int InstanceIndex);

	UFUNCTION(BlueprintCallable, Category = "Skelot|Animation")
	void SetInstanceAnimationLooped(int InstanceIndex, bool bLoop);
	UFUNCTION(BlueprintCallable, Category = "Skelot|Animation")
	bool IsInstanceAnimationLooped(int InstanceIndex) const;
	UFUNCTION(BlueprintCallable, Category = "Skelot|Animation")
	void ToggleInstanceAnimationLoop(int InstanceIndex);

	//returns true if the specified animation is being played by the the instance
	UFUNCTION(BlueprintCallable, Category = "Skelot|Animation")
	bool IsInstancePlayingAnimation(int InstanceIndex, const UAnimSequenceBase* Animation) const;
	//returns true if the specified instance is playing any animation
	UFUNCTION(BlueprintCallable, Category = "Skelot|Animation")
	bool IsInstancePlayingAnyAnimation(int InstanceIndex) const;
	
	UFUNCTION(BlueprintCallable, Category = "Skelot|Animation")
	UAnimSequenceBase* GetInstanceCurrentAnimSequence(int InstanceIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Skelot|Animation")
	void SetInstancePlayScale(int InstanceIndex, float NewPlayScale);
	
	float GetInstancePlayLength(int InstanceIndex) const;
	float GetInstancePlayTime(int InstanceIndex) const;
	float GetInstancePlayTimeRemaining(int InstanceIndex) const;
	float GetInstancePlayTimeFraction(int InstanceIndex) const;
	float GetInstancePlayTimeRemainingFraction(int InstanceIndex) const;

	
	//
	virtual void OnAnimationFinished(const TArray<FSkelotAnimFinishEvent>& Events) {}
	virtual void OnAnimationNotify(const TArray<FSkelotAnimNotifyEvent>& Events) {}

	//is called when animation is finished (only non-looped animations)
	UPROPERTY(BlueprintAssignable, meta=(DisplayName = "OnAnimationFinished"))
	FSkelotAnimationFinished OnAnimationFinishedDelegate;

	UPROPERTY(BlueprintAssignable, meta = (DisplayName = "OnAnimationNotify"))
	FSkelotAnimationNotify OnAnimationNotifyDelegate;

	UFUNCTION(BlueprintCallable, Category = "Skelot|Animation")
	void PlayAnimationOnAll(UAnimSequenceBase* Animation, bool bLoop = true, bool bBlendIn = false, float StartAt = 0, float PlayScale = 1);

	FTransform3f GetInstanceBoneTransform(int instanceIndex, int boneIndex, bool bWorldSpace) const
	{
		if(bWorldSpace)
			return GetInstanceBoneTransformWS(instanceIndex, boneIndex);

		return GetInstanceBoneTransformLS(instanceIndex, boneIndex);
	}
	//returns the cached bone transform in local space
	FTransform3f GetInstanceBoneTransformLS(int instanceIndex, int boneIndex) const;
	//returns the cached bone transform in world space
	FTransform3f GetInstanceBoneTransformWS(int instanceIndex, int boneIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Skelot")
	int32 GetBoneIndex(FName BoneName) const;
	UFUNCTION(BlueprintCallable, Category = "Skelot")
	FName GetBoneName(int32 BoneIndex) const;

	//return the transform of a socket/bone on success, identity otherwise.
	UFUNCTION(BlueprintCallable, Category = "Skelot|Transform")
	FTransform3f GetInstanceSocketTransform(int32 InstanceIndex, FName BoneName, bool bWorldSpace) const;

	/*
	* get socket/bone transform of all valid instances (destroyed instances are filled with zero)
	*/
	UFUNCTION(BlueprintCallable, Category = "Skelot|Transform")
	void GetInstancesSocketTransform(TArray<FTransform3f>& OutTransforms, FName SocketName, bool bWorldSpace = true) const;





	//float GetLocalBoundCoveringRadius() const;
	void ResetAnimationStates();
	
	template <typename TProc> void ForEachValidInstance(TProc Proc)
	{
		for (int i = 0; i < InstancesData.Flags.Num(); i++)
			if(IsInstanceAlive(i))
				Proc(i);
	}


	//return indices of instances whose location are inside the specified sphere
	UFUNCTION(BlueprintCallable, Category = "Skelot|Utility")
	void QueryLocationOverlapingSphere(const FVector3f& Center, float Radius, TArray<int>& OutIndices) const;
	//return indices of instances whose location are inside the specified box
	UFUNCTION(BlueprintCallable, Category = "Skelot|Utility")
	void QueryLocationOverlapingBox(const FBox3f& Box, TArray<int>& OutIndices) const;

	
	//trace a ray against the specified instance using CompactPhysicsAsset 
	//return -1 if no hit found, other wise reference skeleton bone index of the hit
	UFUNCTION(BlueprintCallable, Category="Skelot|Utility")
	int LineTraceInstanceAny(int InstanceIndex, const FVector& Start, const FVector& End) const;

	
	UFUNCTION(BlueprintCallable, Category = "Skelot|Utility")
	int OverlapTestInstance(int InstanceIndex, const FVector& Point, float Thickness) const;

	//line trace the specified instance and return bone index of the closest shape hit
	UFUNCTION(BlueprintCallable, Category = "Skelot|Utility")
	int LineTraceInstanceSingle(int InstanceIndex, const FVector& Start, const FVector& End, float Thickness, double& OutTime, FVector& OutPosition, FVector& OutNormal) const;

	//line trace over the specified instances and return the instance index of the closest hit
	int LineTraceInstancesSingle(const TArrayView<int> InstanceIndices, const FVector& Start, const FVector& End, double Thickness, double& OutTime, FVector& OutPosition, FVector& OutNormal, int& OutBoneIndex) const;


	UFUNCTION(BlueprintCallable, Category = "Skelot|Rendering")
	void SetInstanceCustomData(int InstanceIndex, int FloatIndex, float InValue)
	{
		check(IsInstanceValid(InstanceIndex) && FloatIndex < NumCustomDataFloats);
		InstancesData.RenderCustomData[InstanceIndex * NumCustomDataFloats + FloatIndex] = InValue;
	}
	UFUNCTION(BlueprintCallable, Category = "Skelot|Rendering")
	float GetInstanceCustomData(int InstanceIndex, int FloatIndex) const
	{
		check(IsInstanceValid(InstanceIndex) && FloatIndex < NumCustomDataFloats);
		return InstancesData.RenderCustomData[InstanceIndex * NumCustomDataFloats + FloatIndex];
	}


	UFUNCTION(BlueprintCallable, Category = "Skelot|Rendering")
	void SetInstanceCustomDataFloat1(int InstanceIndex, float Value)			{ SetInstanceCustomData<float>(InstanceIndex, Value);		}
	UFUNCTION(BlueprintCallable, Category = "Skelot|Rendering")
	void SetInstanceCustomDataFloat2(int InstanceIndex, const FVector2f& Value) { SetInstanceCustomData<FVector2f>(InstanceIndex, Value);	}
	UFUNCTION(BlueprintCallable, Category = "Skelot|Rendering")
	void SetInstanceCustomDataFloat3(int InstanceIndex, const FVector3f& Value) { SetInstanceCustomData<FVector3f>(InstanceIndex, Value);	}
	UFUNCTION(BlueprintCallable, Category = "Skelot|Rendering")
	void SetInstanceCustomDataFloat4(int InstanceIndex, const FVector4f& Value) { SetInstanceCustomData<FVector4f>(InstanceIndex, Value);	}


	UFUNCTION(BlueprintCallable, Category = "Skelot|Rendering")
	float GetInstanceCustomDataFloat1(int InstanceIndex) const		{ return GetInstanceCustomData<float>(InstanceIndex);		}
	UFUNCTION(BlueprintCallable, Category = "Skelot|Rendering")
	FVector2f GetInstanceCustomDataFloat2(int InstanceIndex) const	{ return GetInstanceCustomData<FVector2f>(InstanceIndex);	}
	UFUNCTION(BlueprintCallable, Category = "Skelot|Rendering")
	FVector3f GetInstanceCustomDataFloat3(int InstanceIndex) const	{ return GetInstanceCustomData<FVector3f>(InstanceIndex);	}
	UFUNCTION(BlueprintCallable, Category = "Skelot|Rendering")
	FVector4f GetInstanceCustomDataFloat4(int InstanceIndex) const	{ return GetInstanceCustomData<FVector4f>(InstanceIndex);	}

	void ZeroInstanceCustomData(int InstanceIndex)
	{
		check(IsInstanceValid(InstanceIndex));
		for (int i = 0; i < NumCustomDataFloats; i++)
			InstancesData.RenderCustomData[InstanceIndex * NumCustomDataFloats + i] = 0;
	}
	//
	template<typename TData /*float, FVector2f, ... */> void SetInstanceCustomData(int InstanceIndex, const TData& InValue)
	{
		check(IsInstanceValid(InstanceIndex) && NumCustomDataFloats > 0 && sizeof(TData) == (sizeof(float) * NumCustomDataFloats));
		float* Base = &InstancesData.RenderCustomData[InstanceIndex * NumCustomDataFloats];
		*reinterpret_cast<TData*>(Base) = InValue;
	}
	//
	template<typename TData /*float, FVector2f, ... */> const TData& GetInstanceCustomData(int InstanceIndex) const
	{
		check(IsInstanceValid(InstanceIndex) && NumCustomDataFloats > 0 && sizeof(TData) == (sizeof(float) * NumCustomDataFloats));
		const float* Base = &InstancesData.RenderCustomData[InstanceIndex * NumCustomDataFloats];
		return *reinterpret_cast<const TData*>(Base);
	}

};


USTRUCT()
struct FSkelotComponentInstanceData : public FPrimitiveComponentInstanceData
{
	GENERATED_USTRUCT_BODY()

	FSkelotComponentInstanceData() = default;
	FSkelotComponentInstanceData(const USkelotComponent* InComponent);
	
	bool ContainsData() const override { return true; }
	void ApplyToComponent(UActorComponent* Component, const ECacheApplyPhase CacheApplyPhase) override;
	void AddReferencedObjects(FReferenceCollector& Collector) override;

	UPROPERTY()
	FSkelotInstancesData InstanceData;
	UPROPERTY()
	TArray<int> FreeInstances;

};



