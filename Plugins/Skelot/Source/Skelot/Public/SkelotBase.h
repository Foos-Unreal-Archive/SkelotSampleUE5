/*
Copyright 2023 UPO33.All Rights Reserved.
*/


#pragma once

#include "CoreMinimal.h"
#include "Matrix3x4.h"


class UMaterialInterface;
class USkelotComponent;
class USkelotAnimCollection;
struct FSkelotRenderAsset;
class FSkelotSkinWeightVertexBuffer;
class FSkeletalMeshLODRenderData;
class FStaticMeshVertexDataInterface;
class FSkelotProxy;
struct FSkelotInstanceDynamicVB;


//same as FMatrix3x4 but uses FFloa16
struct alignas(8) FMatrix3x4Half
{
	FFloat16 M[3][4];

	void SetMatrix(const FMatrix44f& Mat);
	void SetMatrixTranspose(const FMatrix44f& Mat);
};

inline FArchive& operator <<(FArchive& Ar, FMatrix3x4Half& data)
{
	Ar.Serialize(&data, sizeof(FMatrix3x4Half));
	return Ar;
}
inline FArchive& operator <<(FArchive& Ar, FMatrix3x4& data)
{
	Ar.Serialize(&data, sizeof(FMatrix3x4));
	return Ar;
}


namespace UE {
namespace Math {

template<typename T> struct TBoxCenterExtent
{
	TVector<T> Center, Extent;

	TBoxCenterExtent() {}
	TBoxCenterExtent(EForceInit In) : Center(In), Extent(In) {}
	TBoxCenterExtent(TVector<T> C, TVector<T> E) : Center(C), Extent(E) {}
	TBoxCenterExtent(const TBox<T>& Box) { Box.GetCenterAndExtents(Center, Extent); }

	TBox<T> GetFBox() const { return TBox<T>(Center - Extent, Center + Extent); }
	void Shift(const FVector& Offset) { Center += Offset; }

	TBoxCenterExtent TransformBy(const TMatrix<T>& M) const
	{
		const auto m0 = VectorLoadAligned(M.M[0]);
		const auto m1 = VectorLoadAligned(M.M[1]);
		const auto m2 = VectorLoadAligned(M.M[2]);
		const auto m3 = VectorLoadAligned(M.M[3]);

		const auto CurOrigin = VectorLoadFloat3(&Center);
		const auto CurExtent = VectorLoadFloat3(&Extent);

		auto NewOrigin = VectorMultiply(VectorReplicate(CurOrigin, 0), m0);
		NewOrigin = VectorMultiplyAdd(VectorReplicate(CurOrigin, 1), m1, NewOrigin);
		NewOrigin = VectorMultiplyAdd(VectorReplicate(CurOrigin, 2), m2, NewOrigin);
		NewOrigin = VectorAdd(NewOrigin, m3);

		auto NewExtent = VectorAbs(VectorMultiply(VectorReplicate(CurExtent, 0), m0));
		NewExtent = VectorAdd(NewExtent, VectorAbs(VectorMultiply(VectorReplicate(CurExtent, 1), m1)));
		NewExtent = VectorAdd(NewExtent, VectorAbs(VectorMultiply(VectorReplicate(CurExtent, 2), m2)));

		TBoxCenterExtent Ret;
		VectorStoreFloat3(NewOrigin, &Ret.Center);
		VectorStoreFloat3(NewExtent, &Ret.Extent);
		return Ret;
	}
	
};


//like FBox but aligned and without bool bIsValid, used for fast bound generation
template<typename T> struct TBoxMinMax
{
	TVectorRegisterType<T> Min, Max;

	TBoxMinMax() {}

	TBoxMinMax(EForceInit In)
	{
		Min = VectorSetFloat1(TNumericLimits<T>::Max());
		Max = VectorSetFloat1(-TNumericLimits<T>::Max());
	}
	TBoxMinMax(const TVector<T>& InMin, const TVector<T>& InMax)
	{
		Min = VectorLoadFloat3(&InMin);
		Max = VectorLoadFloat3(&InMax);
	}
	//Adds to this bounding box to include a new bounding volume.
	inline void Add(const TBoxCenterExtent<T>& R)
	{
		auto RC = VectorLoadFloat3(&R.Center);
		auto RE = VectorLoadFloat3(&R.Extent);
		Min = VectorMin(Min, VectorSubtract(RC, RE));
		Max = VectorMax(Max, VectorAdd(RC, RE));
	}
	inline void Add(const TBoxMinMax& In)
	{
		Min = VectorMin(Min, In.Min);
		Max = VectorMax(Max, In.Max);
	}
	//transform @OtherBox and add it to this bound 
	void AddTransformed(const TBoxCenterExtent<T>& OtherBox, const TMatrix<T>& M)
	{
		const auto m0 = VectorLoadAligned(M.M[0]);
		const auto m1 = VectorLoadAligned(M.M[1]);
		const auto m2 = VectorLoadAligned(M.M[2]);
		const auto m3 = VectorLoadAligned(M.M[3]);

		const auto CurOrigin = VectorLoadFloat3(&OtherBox.Center);
		const auto CurExtent = VectorLoadFloat3(&OtherBox.Extent);

		auto NewOrigin = VectorMultiply(VectorReplicate(CurOrigin, 0), m0);
		NewOrigin = VectorMultiplyAdd(VectorReplicate(CurOrigin, 1), m1, NewOrigin);
		NewOrigin = VectorMultiplyAdd(VectorReplicate(CurOrigin, 2), m2, NewOrigin);
		NewOrigin = VectorAdd(NewOrigin, m3);

		auto NewExtent = VectorAbs(VectorMultiply(VectorReplicate(CurExtent, 0), m0));
		NewExtent = VectorAdd(NewExtent, VectorAbs(VectorMultiply(VectorReplicate(CurExtent, 1), m1)));
		NewExtent = VectorAdd(NewExtent, VectorAbs(VectorMultiply(VectorReplicate(CurExtent, 2), m2)));

		Min = VectorMin(Min, VectorSubtract(NewOrigin, NewExtent));
		Max = VectorMax(Max, VectorAdd(NewOrigin, NewExtent));
	}
	inline void Add(const TVectorRegisterType<T>& InVector)
	{
		Min = VectorMin(Min, InVector);
		Max = VectorMax(Max, InVector);
	}
	inline void Add(const TVector<T>& InVector)
	{
		Add(VectorLoadFloat3(&InVector));
	}
	void Shift(const TVector<T>& Offset)
	{
		auto VOffset = VectorLoadFloat3(&Offset);
		Min = VectorAdd(Min, VOffset);
		Max = VectorAdd(Max, VOffset);
	}
	bool IsForceInitValue() const
	{
		return VectorMaskBits(VectorBitwiseAnd(VectorCompareEQ(Min, VectorSetFloat1(TNumericLimits<T>::Max())), VectorCompareEQ(Max, VectorSetFloat1(-TNumericLimits<T>::Max())))) == 0xF;
	}
	void Expand(const TVector<T>& Value)
	{
		auto V = VectorLoadFloat3(&Value);
		Min = VectorSubtract(Min, V);
		Max = VectorAdd(Max, V);
	}
	TBox<T> ToBox() const { return TBox<T>(*reinterpret_cast<const TVector<T>*>(&Min), *reinterpret_cast<const TVector<T>*>(&Max)); }
	TBoxSphereBounds<T> ToBoxSphereBounds() const { return TBoxSphereBounds<T>(ToBox()); }

	bool IsNearlyEqual(const TBoxMinMax& Other, T ErrorTolerance = UE_SMALL_NUMBER) const
	{
		auto VT = VectorSetFloat1(ErrorTolerance);
		return VectorMaskBits(VectorBitwiseAnd(
			VectorCompareLE(VectorAbs(VectorSubtract(Min, Other.Min)), VT), 
			VectorCompareLE(VectorAbs(VectorSubtract(Max, Other.Max)), VT))) == 0b111;
	}
};

/*
@InBox					box to be transformed
@M						Matrix to transform with
@OutTransformedBound	the transformed box
@IncreasingBound		box that OutTransformedBound must be added to
*/
void TransformCenteredBoxEx(const TBoxCenterExtent<float>& InBox, const FMatrix44f& M, TBoxCenterExtent<float>& OutTransformedBound, TBoxMinMax<float>& IncreasingBound);


};
};

using FBoxCenterExtentFloat = UE::Math::TBoxCenterExtent<float>;
using FBoxCenterExtentDouble = UE::Math::TBoxCenterExtent<double>;

using FBoxMinMaxFloat = UE::Math::TBoxMinMax<float>;
using FBoxMinMaxDouble = UE::Math::TBoxMinMax<double>;

template<typename T> inline FArchive& operator <<(FArchive& Ar, UE::Math::TBoxCenterExtent<T>& Bound)
{
	Ar.Serialize(&Bound.Center, sizeof(T[3]));
	Ar.Serialize(&Bound.Extent, sizeof(T[3]));
	return Ar;
}

template<typename T> inline FArchive& operator <<(FArchive& Ar, UE::Math::TBoxMinMax<T>& Bound)
{
	Ar.Serialize(&Bound.Min, sizeof(T[3]));
	Ar.Serialize(&Bound.Max, sizeof(T[3]));
	return Ar;
}

inline float GetBoxCoveringRadius(const FBox3f& Box)
{
	return FMath::Max(Box.Max.Size(), Box.Min.Size());
}

namespace UE {
namespace Math {
template<typename T> struct alignas(16) TMatrix3x4NotTranspose
{
	T M[12];

	TMatrix3x4NotTranspose()
	{
	}
	TMatrix3x4NotTranspose(const TMatrix<T>& RESTRICT Src)
	{
		M[0] = Src.M[0][0];
		M[1] = Src.M[0][1];
		M[2] = Src.M[0][2];

		M[3] = Src.M[1][0];
		M[4] = Src.M[1][1];
		M[5] = Src.M[1][2];

		M[6] = Src.M[2][0];
		M[7] = Src.M[2][1];
		M[8] = Src.M[2][2];

		M[9]  = Src.M[3][0]; //Origin.X
		M[10] = Src.M[3][1]; //Origin.Y
		M[11] = Src.M[3][2]; //Origin.Z
	}
	inline TMatrix3x4NotTranspose& operator = (const TMatrix3x4NotTranspose& R)
	{
		VectorStoreAligned(VectorLoadAligned(&R.M[0]), &M[0]);
		VectorStoreAligned(VectorLoadAligned(&R.M[4]), &M[4]);
		VectorStoreAligned(VectorLoadAligned(&R.M[8]), &M[8]);
		return *this;
	}
	TVector<T> GetOrigin() const 
	{
		//FMatrix::GetOrigin() ===> return FVector(M[3][0], M[3][1], M[3][2]);
		return TVector<T>(M[9], M[10], M[11]);
	}
	void SetOrigin(const TVector<T>& O) 
	{
		M[9] = O.X; M[10] = O.Y; M[11] = O.Z;
	}

	TMatrix<T> ToMatrix4x4() const
	{
		Matrix<T> Ret = TMatrix<T>::Identity;
		Ret.SetAxis(0, TVector<T>(M[0], M[1], M[2]));
		Ret.SetAxis(1, TVector<T>(M[3], M[4], M[5]));
		Ret.SetAxis(2, TVector<T>(M[6], M[7], M[8]));
		Ret.SetAxis(3, TVector<T>(M[9], M[10], M[11]));
		return Ret;
	}
	explicit operator TMatrix<T>() const
	{
		return ToMatrix4x4();
	}
};


template<typename T> struct alignas(16) TMatrix3x4Transpose
{
	T M[12];

	TMatrix3x4Transpose()
	{
	}
	TMatrix3x4Transpose(const TMatrix<T>& RESTRICT Mat)
	{
		const T* RESTRICT Src = &(Mat.M[0][0]);
		T* RESTRICT Dest = &(M[0]);

		Dest[0] = Src[0];   // [0][0]
		Dest[1] = Src[4];   // [1][0]
		Dest[2] = Src[8];   // [2][0]
		Dest[3] = Src[12];  // [3][0]

		Dest[4] = Src[1];   // [0][1]
		Dest[5] = Src[5];   // [1][1]
		Dest[6] = Src[9];   // [2][1]
		Dest[7] = Src[13];  // [3][1]

		Dest[8] = Src[2];   // [0][2]
		Dest[9] = Src[6];   // [1][2]
		Dest[10] = Src[10]; // [2][2]
		Dest[11] = Src[14]; // [3][2]
	}
	inline TMatrix3x4Transpose& operator = (const TMatrix3x4Transpose& R)
	{
		VectorStoreAligned(VectorLoadAligned(&R.M[0]), &M[0]);
		VectorStoreAligned(VectorLoadAligned(&R.M[4]), &M[4]);
		VectorStoreAligned(VectorLoadAligned(&R.M[8]), &M[8]);
		return *this;
	}
	TVector<T> GetOrigin() const 
	{
		//FMatrix::GetOrigin() ===> return FVector(M[3][0], M[3][1], M[3][2]);
		return TVector(M[3], M[7], M[11]); 
	}
	void SetOrigin(const TVector<T>& O) 
	{
		M[3] = O.X; M[7] = O.Y; M[11] = O.Z; 
	}
	TMatrix<T> ToMatrix4x4() const
	{
		TMatrix<T> Ret;
		Ret.SetAxis(0, TVector<T>(M[0], M[4], M[8]));
		Ret.SetAxis(1, TVector<T>(M[1], M[5], M[9]));
		Ret.SetAxis(2, TVector<T>(M[2], M[6], M[10]));
		Ret.SetAxis(3, TVector<T>(M[3], M[7], M[11]));
		return Ret;
	}
	explicit operator TMatrix<T>() const
	{
		return ToMatrix4x4();
	}
};

};

};

typedef UE::Math::TMatrix3x4Transpose<float> SkelotShaderMatrixT;	//this is the matrix we send to shader :|