#ifndef FLAT_FLATHIERARCHY_H
#define FLAT_FLATHIERARCHY_H

#ifndef FLAT_VECTOR
	#include <vector>
	template<typename ValueType>
	class FLAT_Vector_Type : public std::vector<ValueType>
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
		void erase(size_t index) { erase(begin() + index); }
	};

	#define FLAT_VECTOR FLAT_Vector_Type
#endif
	
#ifndef FLAT_SIZETYPE
	#include <inttypes.h>
	#define FLAT_SIZETYPE uint32_t
#endif
	
#ifndef FLAT_DEPTHTYPE
	#include <inttypes.h>
	#define FLAT_DEPTHTYPE uint8_t
#endif

static_assert((FLAT_DEPTHTYPE)(0) < (FLAT_DEPTHTYPE)(-1), "FLAT_DEPTHTYPE must be unsigned");

#ifndef FLAT_MAXDEPTH
	// Exclude most significant bit to prevent roll over errors
	#define FLAT_MAXDEPTH ((FLAT_DEPTHTYPE)(~0) >> 1)
#endif

#ifndef FLAT_ASSERT
	#if FLAT_ASSERTS_ENABLED == true
		#include <assert.h>
		#define FLAT_ASSERT(expr) assert(expr)
	#else
		#define FLAT_ASSERT(expr) do{}while(false)
	#endif
#endif

#ifndef FLAT_DEBUGGING_ENABLED
	#ifdef _DEBUG
		#define FLAT_DEBUGGING_ENABLED true
	#else
		#define FLAT_DEBUGGING_ENABLED false
	#endif
#endif

#if FLAT_DEBUGGING_ENABLED == true
	#define FLAT_DEBUG_ASSERT(...) FLAT_ASSERT(__VA_ARGS__)
#else
	#define FLAT_DEBUG_ASSERT(...) do{}while(false)
#endif

#ifndef FLAT_ERROR
	#define FLAT_ERROR(string) FLAT_ASSERT(!(string))
#endif
	
#ifndef FLAT_MEMCPY
	#include <string.h>
	#define FLAT_MEMSET memset
#endif
#ifndef FLAT_MEMCPY
	#include <string.h>
	#define FLAT_MEMCPY memcpy
#endif
#ifndef FLAT_MEMMOVE
	#include <string.h>
	#define FLAT_MEMMOVE memcpy
#endif


static uint32_t getNextPowerOfTwo(uint32_t v)
{
	FLAT_ASSERT((v & 0x80000000) == 0);

	uint32_t r = 0;
	while (v >>= 1)
	{
		++r;
	}
	return 1 << (r + 1);
}

struct DefaultSorter
{
	static const bool UseSorting = true;

	template<typename T>
	inline static bool isFirst(const T& a, const T& b) { return a < b; }
};

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
	DepthValue findMaxDepth() const
	{
		DepthValue result = 0;
		for (HierarchyIndex i = 0; i < getCount(); i++)
		{
			if (depths[i] > result)
				result = depths[i];
		}
		return result;
	}

	static HierarchyIndex getIndexNotFound() { return HierarchyIndex(~0); }
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
	HierarchyIndex createNodeAsChildOf(HierarchyIndex parentIndex, ValueType value)
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
		FLAT_ASSERT((depths[child] != depths[parent] + 1 || !linearIsChildOf(child, parent)) && "Re-parenting");
		
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
			if (child > parent)
			{
				// If child's index is greater than parent, find a place in parents children that is closest to child's current position
				DepthValue targetDepth = depths[parent] + 1;
				for (HierarchyIndex i = parent + 1; i < child; ++i)
				{
					if (depths[i] > targetDepth)
						continue;
					if (depths[i] < targetDepth)
						break;
					dest = i;
				}
			}
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
			FLAT_ASSERT(depths[dest + i] < FLAT_MAXDEPTH && "Over/Under-flow threat detected"); // Over flow protection
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
		FLAT_MEMCPY(valueBuffer, vPtr + source, count * sizeof(ValueType) );

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

		DepthValue parentDepth = depths[parent];
		for (HierarchyIndex c = parent + 1; c < child; ++c)
		{
			if (depths[c] <= parentDepth)
				return false;
		}
		return true;
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
