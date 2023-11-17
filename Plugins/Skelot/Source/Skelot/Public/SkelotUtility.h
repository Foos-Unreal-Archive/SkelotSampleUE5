#pragma once

#include "SkelotComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "SkelotUtility.generated.h"


UCLASS()
class USkelotBPUtility : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/*
	* iterate over all the instances and find the closest hit
	*/
	UFUNCTION(BlueprintCallable, Category = "Skelot|Utility")
	static UPARAM(DisplayName="OutInstanceIndex") int LineTraceInstancesSingle(const USkelotComponent* Component, const FVector& Start, const FVector& End, float Thickness, float DebugDrawTime, double& OutTime, FVector& OutPosition, FVector& OutNormal, int& OutBoneIndex);

};

struct FSkelotUtility
{

	

};