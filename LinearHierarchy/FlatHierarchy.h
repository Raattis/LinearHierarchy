#ifndef FLAT_FLATHIERARCHY_H
#define FLAT_FLATHIERARCHY_H

#include "FlatAssert.h"

#include <inttypes.h>
#define FLAT_SIZETYPE uint32_t
#define FLAT_DEPTHTYPE uint16_t

// Exclude most significant bit to catch roll over errors
#define FLAT_MAXDEPTH ((FLAT_DEPTHTYPE)(~0) >> 1)

#define FLAT_ERROR(string) FLAT_ASSERT(!(string))

#include <string.h> /* memset, memcpy, memmoce*/
#define FLAT_MEMSET memset
#define FLAT_MEMCPY memcpy
#define FLAT_MEMMOVE memmove

#include <emmintrin.h>
#define FLAT_USE_SIMD true

#ifndef FLAT_VECTOR
	#include <vector>
	template<typename ValueType>
	class FLAT_Vector_Type : public std::vector<ValueType alignas(16)>
	{
	public:
		void pushBack(const ValueType& v) { push_back(v); }
		size_t getSize() const { return size(); }
		void insert(size_t index, const ValueType& v) { std::vector<ValueType>::insert(begin() + index, v); }
		const ValueType& getBack() const { return back(); }
		ValueType& getBack() { return back(); }
		size_t getCapacity() const { return capacity(); }
		void zero(size_t start, size_t end) { return std::fill(begin() + start, begin() + end, 0); }
		ValueType* getPointer() { return &*begin(); }
		const ValueType* getPointer() const { return &*begin(); }
		void erase(size_t index) { std::vector<ValueType>::erase(begin() + index); }
	};
	
	#define FLAT_VECTOR FLAT_Vector_Type
#endif

/////////////////////////////////////////////////////////////////
//
// Base implementation containing only a depths-vector
// Mostly used by cache structures to determine node relations
//
/////////////////////////////////////////////////////////////////
class FlatHierarchyBase
{
public:
	typedef FLAT_SIZETYPE SizeType;
	typedef FLAT_DEPTHTYPE DepthValue;
	typedef SizeType HierarchyIndex;

	FLAT_VECTOR<DepthValue> depths;



	SizeType getCount() const
	{
		return depths.getSize();
	}



	__declspec(noinline)
		DepthValue findMaxDepth() const
	{
		DepthValue result = 0;
#if FLAT_USE_SIMD == false
		for (HierarchyIndex i = 0; i < getCount(); i++)
		{
			if (depths[i] > result)
				result = depths[i];
		}
		return result;
#else
		enum { Bytes = sizeof(DepthValue) }; // Make Bytes an enum to ensure it is used as compile-time constant
		static_assert(Bytes == 1 || Bytes == 2 || Bytes == 4 || Bytes > 4, "Byte count mismatch");

		if (Bytes <= 4) // DepthValue == uint8_t / uint16_t / uint32_t
		{
			FLAT_ASSERT(Bytes == 1 || Bytes == 2 || Bytes == 4);

			SizeType bytesLeft = getCount() * Bytes;
			const uint8_t* data = (const uint8_t*)depths.getPointer();

			FLAT_ASSERT((((uintptr_t)data) & 0xF) == 0);

			while (bytesLeft > 0 && (((uintptr_t)data) & 0xF) != 0)
			{
				if (result < *data)
					result = *data;
				data += Bytes;
				bytesLeft -= Bytes;
			}

			FLAT_ASSERT((((uintptr_t)data) & 0xF) == 0);

			__m128i res = _mm_setzero_si128();
			__m128i max0 = _mm_setzero_si128();
			__m128i max1 = _mm_setzero_si128();
			__m128i max2 = _mm_setzero_si128();
			__m128i max3 = _mm_setzero_si128();

			while (bytesLeft >= 64)
			{
				__m128i in0 = _mm_load_si128((const __m128i*)data + 0);
				__m128i in1 = _mm_load_si128((const __m128i*)data + 1);
				__m128i in2 = _mm_load_si128((const __m128i*)data + 2);
				__m128i in3 = _mm_load_si128((const __m128i*)data + 3);

				if (Bytes == 1) max0 = _mm_max_epu8(max0, in0);
				if (Bytes == 1) max1 = _mm_max_epu8(max1, in1);
				if (Bytes == 1) max2 = _mm_max_epu8(max2, in2);
				if (Bytes == 1) max3 = _mm_max_epu8(max3, in3);

				if (Bytes == 2) max0 = _mm_max_epu16(max0, in0);
				if (Bytes == 2) max1 = _mm_max_epu16(max1, in1);
				if (Bytes == 2) max2 = _mm_max_epu16(max2, in2);
				if (Bytes == 2) max3 = _mm_max_epu16(max3, in3);

				if (Bytes == 4) max0 = _mm_max_epu32(max0, in0);
				if (Bytes == 4) max1 = _mm_max_epu32(max1, in1);
				if (Bytes == 4) max2 = _mm_max_epu32(max2, in2);
				if (Bytes == 4) max3 = _mm_max_epu32(max3, in3);

				data += 64;
				bytesLeft -= 64;
			}

			if (bytesLeft >= 48)
			{
				__m128i in0 = _mm_load_si128((const __m128i*)data + 0);
				__m128i in1 = _mm_load_si128((const __m128i*)data + 1);
				__m128i in2 = _mm_load_si128((const __m128i*)data + 2);

				if (Bytes == 1) max0 = _mm_max_epu8(max0, in0);
				if (Bytes == 1) max1 = _mm_max_epu8(max1, in1);
				if (Bytes == 1) max2 = _mm_max_epu8(max2, in2);

				if (Bytes == 2) max0 = _mm_max_epu16(max0, in0);
				if (Bytes == 2) max1 = _mm_max_epu16(max1, in1);
				if (Bytes == 2) max2 = _mm_max_epu16(max2, in2);

				if (Bytes == 4) max0 = _mm_max_epu32(max0, in0);
				if (Bytes == 4) max1 = _mm_max_epu32(max1, in1);
				if (Bytes == 4) max2 = _mm_max_epu32(max2, in2);

				data += 48;
				bytesLeft -= 48;
			}
			else if (bytesLeft >= 32)
			{
				__m128i in0 = _mm_load_si128((const __m128i*)data + 0);
				__m128i in1 = _mm_load_si128((const __m128i*)data + 1);

				if (Bytes == 1) max0 = _mm_max_epu8(max0, in0);
				if (Bytes == 1) max1 = _mm_max_epu8(max1, in1);

				if (Bytes == 2) max0 = _mm_max_epu16(max0, in0);
				if (Bytes == 2) max1 = _mm_max_epu16(max1, in1);

				if (Bytes == 4) max0 = _mm_max_epu32(max0, in0);
				if (Bytes == 4) max1 = _mm_max_epu32(max1, in1);

				data += 32;
				bytesLeft -= 32;
			}
			else if (bytesLeft >= 16)
			{
				__m128i in0 = _mm_load_si128((const __m128i*)data + 0);

				if (Bytes == 1) max0 = _mm_max_epu8(max0, in0);
				if (Bytes == 2) max0 = _mm_max_epu16(max0, in0);
				if (Bytes == 4) max0 = _mm_max_epu32(max0, in0);

				data += 16;
				bytesLeft -= 16;
			}

			// Collect all max values
			{
				if (Bytes == 1) res = _mm_max_epu8(_mm_max_epu8(max0, max1), _mm_max_epu8(max2, max3));
				if (Bytes == 2) res = _mm_max_epu8(_mm_max_epu8(max0, max1), _mm_max_epu8(max2, max3));
				if (Bytes == 4) res = _mm_max_epu8(_mm_max_epu8(max0, max1), _mm_max_epu8(max2, max3));
			}

			// Shift and compare max values to last byte
			{
				if (Bytes == 1) res = _mm_max_epu8(res, _mm_srli_si128(res, 8));
				if (Bytes == 1) res = _mm_max_epu8(res, _mm_srli_si128(res, 4));
				if (Bytes == 1) res = _mm_max_epu8(res, _mm_srli_si128(res, 2));
				if (Bytes == 1) res = _mm_max_epu8(res, _mm_srli_si128(res, 1));

				if (Bytes == 2) res = _mm_max_epu16(res, _mm_srli_si128(res, 8));
				if (Bytes == 2) res = _mm_max_epu16(res, _mm_srli_si128(res, 4));
				if (Bytes == 2) res = _mm_max_epu16(res, _mm_srli_si128(res, 2));

				if (Bytes == 4) res = _mm_max_epu32(res, _mm_srli_si128(res, 8));
				if (Bytes == 4) res = _mm_max_epu32(res, _mm_srli_si128(res, 4));
			}
			result = (DepthValue)_mm_cvtsi128_si32(res);

			while (bytesLeft > 0)
			{
				FLAT_ASSERT(bytesLeft >= Bytes);

				if (result < *(DepthValue*)data)
					result = *(DepthValue*)data;
				data += Bytes;
				bytesLeft -= Bytes;
			}
		}
		else // sizeof(DepthValue) > 4
		{
			for (HierarchyIndex i = 0; i < getCount(); i++)
			{
				if (depths[i] > result)
					result = depths[i];
			}
		}
#ifdef MAX_PERF
		//{ // Correctness check
		//	SizeType check = 0;
		//	for (HierarchyIndex i = 0; i < getCount(); i++)
		//	{
		//		if (check < depths[i])
		//			check = depths[i];
		//	}
		//	FLAT_ASSERT(check == result);
		//}
#endif
		return result;
#endif
	}

	__declspec(noinline)
		DepthValue findMinDepthBetween(HierarchyIndex first, HierarchyIndex last) const
	{
		FLAT_ASSERT(last < getCount());
		FLAT_ASSERT(first <= last);

		DepthValue result = 0;
#if FLAT_USE_SIMD == false
		for (HierarchyIndex i = first; i <= last; i++)
		{
			if (depths[i] < result)
				result = depths[i];
		}
		return result;
#else
		enum { Bytes = sizeof(DepthValue) }; // Make Bytes an enum to ensure it is used as compile-time constant
		static_assert(Bytes == 1 || Bytes == 2 || Bytes == 4 || Bytes > 4, "Byte count mismatch");

		if (Bytes <= 4 && first + 16 / Bytes <= last) // DepthValue == uint8_t / uint16_t / uint32_t
		{
			FLAT_ASSERT(Bytes == 1 || Bytes == 2 || Bytes == 4);

			SizeType bytesLeft = (last + 1 - first) * Bytes;

			FLAT_ASSERT(bytesLeft / Bytes < getCount());

			const uint8_t* data = (const uint8_t*)depths.getPointer() + first;

			while (bytesLeft > 0 && (((uintptr_t)data) & 0xF) != 0)
			{
				if (result > *data)
					result = *data;
				data += Bytes;
				bytesLeft -= Bytes;
			}

			__m128i res = _mm_setzero_si128();
			__m128i min0 = _mm_setzero_si128();
			__m128i min1 = _mm_setzero_si128();
			__m128i min2 = _mm_setzero_si128();
			__m128i min3 = _mm_setzero_si128();

			while (bytesLeft >= 64)
			{
				__m128i in0 = _mm_load_si128((const __m128i*)data + 0);
				__m128i in1 = _mm_load_si128((const __m128i*)data + 1);
				__m128i in2 = _mm_load_si128((const __m128i*)data + 2);
				__m128i in3 = _mm_load_si128((const __m128i*)data + 3);

				if (Bytes == 1) min0 = _mm_min_epu8(min0, in0);
				if (Bytes == 1) min1 = _mm_min_epu8(min1, in1);
				if (Bytes == 1) min2 = _mm_min_epu8(min2, in2);
				if (Bytes == 1) min3 = _mm_min_epu8(min3, in3);

				if (Bytes == 2) min0 = _mm_min_epu16(min0, in0);
				if (Bytes == 2) min1 = _mm_min_epu16(min1, in1);
				if (Bytes == 2) min2 = _mm_min_epu16(min2, in2);
				if (Bytes == 2) min3 = _mm_min_epu16(min3, in3);

				if (Bytes == 4) min0 = _mm_min_epu32(min0, in0);
				if (Bytes == 4) min1 = _mm_min_epu32(min1, in1);
				if (Bytes == 4) min2 = _mm_min_epu32(min2, in2);
				if (Bytes == 4) min3 = _mm_min_epu32(min3, in3);

				data += 64;
				bytesLeft -= 64;
			}

			if (bytesLeft >= 48)
			{
				__m128i in0 = _mm_load_si128((const __m128i*)data + 0);
				__m128i in1 = _mm_load_si128((const __m128i*)data + 1);
				__m128i in2 = _mm_load_si128((const __m128i*)data + 2);

				if (Bytes == 1) min0 = _mm_min_epu8(min0, in0);
				if (Bytes == 1) min1 = _mm_min_epu8(min1, in1);
				if (Bytes == 1) min2 = _mm_min_epu8(min2, in2);

				if (Bytes == 2) min0 = _mm_min_epu16(min0, in0);
				if (Bytes == 2) min1 = _mm_min_epu16(min1, in1);
				if (Bytes == 2) min2 = _mm_min_epu16(min2, in2);

				if (Bytes == 4) min0 = _mm_min_epu32(min0, in0);
				if (Bytes == 4) min1 = _mm_min_epu32(min1, in1);
				if (Bytes == 4) min2 = _mm_min_epu32(min2, in2);

				data += 48;
				bytesLeft -= 48;
			}
			else if (bytesLeft >= 32)
			{
				__m128i in0 = _mm_load_si128((const __m128i*)data + 0);
				__m128i in1 = _mm_load_si128((const __m128i*)data + 1);

				if (Bytes == 1) min0 = _mm_min_epu8(min0, in0);
				if (Bytes == 1) min1 = _mm_min_epu8(min1, in1);

				if (Bytes == 2) min0 = _mm_min_epu16(min0, in0);
				if (Bytes == 2) min1 = _mm_min_epu16(min1, in1);

				if (Bytes == 4) min0 = _mm_min_epu32(min0, in0);
				if (Bytes == 4) min1 = _mm_min_epu32(min1, in1);

				data += 32;
				bytesLeft -= 32;
			}
			else if (bytesLeft >= 16)
			{
				__m128i in0 = _mm_load_si128((const __m128i*)data + 0);

				if (Bytes == 1) min0 = _mm_min_epu8(min0, in0);
				if (Bytes == 2) min0 = _mm_min_epu16(min0, in0);
				if (Bytes == 4) min0 = _mm_min_epu32(min0, in0);

				data += 16;
				bytesLeft -= 16;
			}

			// Collect all min values
			{
				if (Bytes == 1) res = _mm_min_epu8(_mm_min_epu8(min0, min1), _mm_min_epu8(min2, min3));
				if (Bytes == 2) res = _mm_min_epu8(_mm_min_epu8(min0, min1), _mm_min_epu8(min2, min3));
				if (Bytes == 4) res = _mm_min_epu8(_mm_min_epu8(min0, min1), _mm_min_epu8(min2, min3));
			}

			// Shift and compare min values to last byte
			{
				if (Bytes == 1) res = _mm_min_epu8(res, _mm_srli_si128(res, 8));
				if (Bytes == 1) res = _mm_min_epu8(res, _mm_srli_si128(res, 4));
				if (Bytes == 1) res = _mm_min_epu8(res, _mm_srli_si128(res, 2));
				if (Bytes == 1) res = _mm_min_epu8(res, _mm_srli_si128(res, 1));

				if (Bytes == 2) res = _mm_min_epu16(res, _mm_srli_si128(res, 8));
				if (Bytes == 2) res = _mm_min_epu16(res, _mm_srli_si128(res, 4));
				if (Bytes == 2) res = _mm_min_epu16(res, _mm_srli_si128(res, 2));

				if (Bytes == 4) res = _mm_min_epu32(res, _mm_srli_si128(res, 8));
				if (Bytes == 4) res = _mm_min_epu32(res, _mm_srli_si128(res, 4));
			}
			result = (DepthValue)_mm_cvtsi128_si32(res);

			while (bytesLeft > 0)
			{
				FLAT_ASSERT(bytesLeft >= Bytes);

				if (result > *(DepthValue*)data)
					result = *(DepthValue*)data;
				data += Bytes;
				bytesLeft -= Bytes;
			}
		}
		else // sizeof(DepthValue) > 4
		{
			for (HierarchyIndex i = first; i <= last; i++)
			{
				if (result > depths[i])
					result = depths[i];
			}
			return result;
		}
#ifdef MAX_PERF
		//{ // Correctness check
		//	SizeType check = 0;
		//	for (HierarchyIndex i = first; i <= last; i++)
		//	{
		//		if (check > depths[i])
		//			check = depths[i];
		//	}
		//	FLAT_ASSERT(check == result);
		//}
#endif
		return result;
#endif
	}

	static HierarchyIndex getIndexNotFound() { return HierarchyIndex(~0); }
};





struct DefaultSorter
{
	static const bool UseSorting = true;

	template<typename T>
	inline static bool isFirst(const T& a, const T& b) { return a < b; }
};

template<typename ValueType, typename Sorter = DefaultSorter>
class FlatHierarchy : public FlatHierarchyBase
{
public:

	FLAT_VECTOR<ValueType> values;

	FlatHierarchy(SizeType reserveSize)
	{
		values.reserve(reserveSize);
		depths.reserve(reserveSize);
	}
	~FlatHierarchy()
	{
	}

	HierarchyIndex createRootNode(const ValueType& value)
	{
		HierarchyIndex newIndex = getCount();

		if (Sorter::UseSorting == true)
		{
			// Insert to sorted position
			DepthValue targetDepth = 0;
			for (SizeType i = 0; i < newIndex; ++i)
			{
				if (depths[i] == targetDepth && Sorter::isFirst(value, values[i]))
				{
					newIndex = i;
					break;
				}
			}

			values.insert(newIndex, value);
			depths.insert(newIndex, (DepthValue)0U);
		}
		else
		{
			values.pushBack(value);
			depths.pushBack((DepthValue)0U);
		}

		return newIndex;
	}

	HierarchyIndex createNodeAsChildOf(HierarchyIndex parentIndex, const ValueType& value)
	{
		SizeType newIndex = parentIndex + 1;

		if (Sorter::UseSorting == true)
		{
			// Default to last possible index
			newIndex = getLastDescendant(parentIndex) + 1;

			// Insert to sorted position
			DepthValue targetDepth = depths[parentIndex] + 1;
			for (SizeType i = parentIndex + 1; i < newIndex; ++i)
			{
				if (depths[i] == targetDepth && Sorter::isFirst(value, values[i]))
				{
					newIndex = i;
					break;
				}
			}
		}

		FLAT_ASSERT(newIndex > parentIndex);

		SizeType newParentCount = depths[parentIndex] + 1;
		FLAT_ASSERT(newParentCount < FLAT_MAXDEPTH); // Over flow protection

		values.insert(newIndex, value);
		depths.insert(newIndex, newParentCount);
		return newIndex;
	}
	HierarchyIndex getLastDescendant(HierarchyIndex parentIndex)
	{
		HierarchyIndex result = parentIndex + 1;
		DepthValue parentDepth = depths[parentIndex];
		while (result < getCount() && depths[result] > parentDepth)
		{
			++result;
		}
		return result - 1;
	}

	HierarchyIndex makeChildOf(HierarchyIndex child, HierarchyIndex parent)
	{
		//printf("Make %d child of %d\n", child, parent);

		FLAT_ASSERT(child != parent && "Self-adoption");
		FLAT_ASSERT(!linearIsChildOf(parent, child) && "Incest");
		//FLAT_ASSERT((depths[child] != depths[parent] + 1 || !linearIsChildOf(child, parent)) && "Re-parenting");

		SizeType dest = parent + 1; // Default destination position is right after parent
		SizeType count = getLastDescendant(child) - child + 1; // Descendant count including the child
		FLAT_ASSERT(count > 0);

		if (Sorter::UseSorting == true)
		{
			// Find a destination position that will have the child sorted among its siblings

			dest = getLastDescendant(parent) + 1; // Default to last possible index

			DepthValue targetDepth = depths[parent] + 1;
			for (SizeType i = parent + 1; i < dest; ++i)
			{
				if (depths[i] == targetDepth && Sorter::isFirst(values[child], values[i]))
				{
					dest = i;
					break;
				}
			}
		}
		else
		{
			// Commented to unify behavior with pointer trees
			//if (child > parent)
			//{
			//	// If child's index is greater than parent, find a place in parents children that is closest to child's current position
			//	DepthValue targetDepth = depths[parent] + 1;
			//	for (HierarchyIndex i = parent + 1; i < child; ++i)
			//	{
			//		if (depths[i] > targetDepth)
			//			continue;
			//		if (depths[i] < targetDepth)
			//			break;
			//		dest = i;
			//	}
			//}
		}

		// Do the move

		SizeType source = child;


		DepthValue depthDiff = depths[parent] + 1 - depths[child];

		if (source != dest && source + count != dest)
		{
			move(source, dest, count);
		}

		if (source < dest)
		{
			FLAT_ASSERT(source + count <= dest);
			dest -= count;
		}

		for (SizeType i = 0; i < count; i++)
		{
			depths[dest + i] += depthDiff;
#define TO_STR_IMPL(p) #p
#define TO_STR(p) TO_STR_IMPL(p)
			FLAT_ASSERT(depths[dest + i] < FLAT_MAXDEPTH && "Over/Under-flow threat detected: " TO_STR(FLAT_MAXDEPTH)); // Over flow protection
		}
		return dest;
	}

	void erase(HierarchyIndex child)
	{
		SizeType count = getLastDescendant(child) - child + 1;

		const SizeType toShift = getCount() - (child + count);
		FLAT_MEMMOVE(depths.getPointer() + child, depths.getPointer() + child + count, toShift * sizeof(DepthValue));
		FLAT_MEMMOVE(values.getPointer() + child, values.getPointer() + child + count, toShift * sizeof(ValueType));

		depths.resize(depths.getSize() - count);
		values.resize(values.getSize() - count);
	}

	void move(SizeType source, SizeType dest, SizeType count)
	{
		static const SizeType MaxStackCount = 1024 / sizeof(ValueType);
		if (count <= MaxStackCount)
		{
			// Get buffers for values
			DepthValue   stackDepthBuffer[MaxStackCount];
			ValueType stackValueBuffer[MaxStackCount];
			moveImp(source, dest, count, stackDepthBuffer, stackValueBuffer);
		}
		else
		{
			DepthValue* depthBuffer = (DepthValue*)malloc((sizeof(DepthValue) + sizeof(ValueType)) * count);
			ValueType* valueBuffer = (ValueType*)(depthBuffer + count);
			moveImp(source, dest, count, depthBuffer, valueBuffer);
			free(depthBuffer);
		}
	}

	inline void moveImp(SizeType source, SizeType dest, SizeType count, DepthValue* depthBuffer, ValueType* valueBuffer)
	{
		DepthValue* dPtr = depths.getPointer();
		ValueType* vPtr = values.getPointer();

		// Push to buffer
		FLAT_MEMCPY(depthBuffer, dPtr + source, count * sizeof(DepthValue));
		FLAT_MEMCPY(valueBuffer, vPtr + source, count * sizeof(ValueType));

		// Shift buffer backward or forward depending on the order of source and destination
		if (source < dest)
		{
			FLAT_ASSERT(source + count < dest);
			const SizeType toMove = dest - (source + count);

			FLAT_MEMMOVE(dPtr + source, dPtr + source + count, toMove * sizeof(DepthValue));
			FLAT_MEMMOVE(vPtr + source, vPtr + source + count, toMove * sizeof(ValueType));
			dest -= count;
		}
		else
		{
			FLAT_ASSERT(dest < source);
			const SizeType toMove = source - dest;

			FLAT_MEMMOVE(dPtr + dest + count, dPtr + dest, toMove * sizeof(DepthValue));
			FLAT_MEMMOVE(vPtr + dest + count, vPtr + dest, toMove * sizeof(ValueType));
		}

		FLAT_MEMCPY(dPtr + dest, depthBuffer, count * sizeof(DepthValue));
		FLAT_MEMCPY(vPtr + dest, valueBuffer, count * sizeof(ValueType));
	}

	inline bool linearIsChildOf(HierarchyIndex child, HierarchyIndex parent)
	{
		if (child <= parent || depths[child] <= depths[parent])
			return false;

		if (parent + 1 == child)
		{
			FLAT_ASSERT(depths[parent] + 1 == depths[child]);
			return true;
		}

		return depths[parent] < findMinDepthBetween(parent + 1, child - 1);
	}

	HierarchyIndex findValue(const ValueType& valueType, HierarchyIndex startingFrom = 0)
	{
		// Linear search
		for (HierarchyIndex i = startingFrom, end = getCount(); i < end; ++i)
		{
			if (values[i] == valueType)
				return i;
		}
		return getIndexNotFound();
	}
	HierarchyIndex findValueAsChildOf(const ValueType& valueType, HierarchyIndex parent)
	{
		// Linear search below parents depth starting from parent index
		DepthValue parentDepth = depths[parent];
		for (HierarchyIndex i = parent + 1, end = getCount(); i < end && parentDepth < depths[i]; ++i)
		{
			if (values[i] == valueType)
				return i;
		}
		return getIndexNotFound();
	}
};











#endif // FLAT_FLATHIERARCHY_H
