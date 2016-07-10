#ifndef LHR_LINEARHIERARCHY_H
#define LHR_LINEARHIERARCHY_H

#ifndef LHR_VECTOR
	#include <vector>
	template<typename ValueType>
	class LHR_Vector_Type : public std::vector<ValueType>
	{
	public:
		void pushBack(const ValueType& v) { push_back(v); }
		size_t getSize() const { return size(); }
		void insert(size_t index, const ValueType& v) { std::vector<ValueType>::insert(begin() + index, v); }
		const ValueType& getBack() const { return back(); }
		size_t getCapacity() const { return capacity(); }
		void zero(size_t start, size_t end) { return std::fill(begin() + start, begin() + end, 0); }
		ValueType* getPointer() { return &*begin(); }
	};

	#define LHR_VECTOR LHR_Vector_Type
#endif
	
#ifndef LHR_SIZETYPE
	#include <inttypes.h>
	#define LHR_SIZETYPE uint32_t
#endif

#ifndef LHR_DEPTHTYPE
	#include <inttypes.h>
	#define LHR_DEPTHTYPE uint8_t
#endif

#ifndef LHR_ASSERT
	#if LHR_ASSERTS_ENABLED == true
		#include <assert.h>
		#define LHR_ASSERT(expr) assert(expr)
	#else
		#define LHR_ASSERT(expr) do{}while(false)
	#endif
#endif

#ifndef LHR_ERROR
	#define LHR_ERROR(string) LHR_ASSERT(!(string))
#endif
	
#ifndef LHR_MEMCPY
	#include <string.h>
	#define LHR_MEMSET memset
#endif
#ifndef LHR_MEMCPY
	#include <string.h>
	#define LHR_MEMCPY memcpy
#endif
#ifndef LHR_MEMMOVE
	#include <string.h>
	#define LHR_MEMMOVE memcpy
#endif

struct DefaultSorter
{
	static const bool UseSorting = true;

	template<typename T>
	inline static bool isFirst(const T& a, const T& b) { return a < b; }
};

template<typename ValueType, typename Sorter = DefaultSorter>
class LinearHierarchy
{
public:
	typedef LHR_SIZETYPE SizeType;
	typedef LHR_DEPTHTYPE DepthValue;
	typedef SizeType HierarchyIndex;
	typedef SizeType RelativeParentIndex;

	LinearHierarchy()
		: cacheIsValid(false)
		, keepCacheValid(false)
	{
	}
	LinearHierarchy(SizeType reserveSize)
		: cacheIsValid(false)
		, keepCacheValid(false)
	{
		values.reserve(reserveSize);
		depths.reserve(reserveSize);
	}
	~LinearHierarchy()
	{
		for (DepthValue i = 0; i < hierarchyCache.getRows(); i++)
		{
			LHR_ASSERT(hierarchyCache.cacheRows[i]);
			delete hierarchyCache.cacheRows[i];
		}
	}

	RelativeParentIndex getCacheValue(DepthValue row, HierarchyIndex column) const
	{
		LHR_ASSERT(cacheIsValid);
		return hierarchyCache.row(row).column(column);
	}
	HierarchyIndex getParentIndex(DepthValue depth, HierarchyIndex child) const
	{
		LHR_ASSERT(cacheIsValid);
		LHR_ASSERT(child >= hierarchyCache.row(depth).column(child));
		return child - hierarchyCache.row(depth).column(child);
	}

	LHR_VECTOR<ValueType> values;
	LHR_VECTOR<DepthValue> depths;

	struct HierarchyCacheRow
	{
		LHR_VECTOR<RelativeParentIndex> cacheValues;

		SizeType getColumns() const
		{
			return cacheValues.getSize();
		}
		RelativeParentIndex column(RelativeParentIndex index) const
		{
			LHR_ASSERT(index < cacheValues.getSize());
			return cacheValues[index];
		}
		RelativeParentIndex& column(RelativeParentIndex index)
		{
			LHR_ASSERT(index < cacheValues.getSize());
			return cacheValues[index];
		}
	};
	struct HierarchyCache
	{
		HierarchyCache()
		{
			cacheRows.pushBack(new HierarchyCacheRow()); // Add sentinel row
			cacheRows.getBack()->cacheValues.resize(1);  // Add sentinel column 
			cacheRows.getBack()->cacheValues[0] = 0;
		}

		LHR_VECTOR<HierarchyCacheRow*> cacheRows;

		HierarchyCacheRow& row(DepthValue depth)
		{
			LHR_ASSERT(depth < getRows());
			LHR_ASSERT(cacheRows[depth] != NULL);
			return *cacheRows[depth];
		}
		const HierarchyCacheRow& row(DepthValue depth) const
		{
			LHR_ASSERT(depth < getRows());
			LHR_ASSERT(cacheRows[depth] != NULL);
			return *cacheRows[depth];
		}
		DepthValue getRows() const
		{
			return cacheRows.getSize();
		}

		void grow(DepthValue rows, SizeType columns)
		{
			rows    += 1;
			columns = (cacheRows.getSize() == 0 || columns >= cacheRows[0]->getColumns() ? columns : cacheRows[0]->getColumns()) + 1;

			cacheRows.reserve(rows);
			while (cacheRows.getSize() < rows)
			{
				cacheRows.pushBack(new HierarchyCacheRow());
			}
			for (SizeType rowNumber = 0; rowNumber < cacheRows.getSize(); rowNumber++)
			{
				HierarchyCacheRow& r = row(rowNumber);
				if (r.cacheValues.getSize() < columns)
				{
					SizeType oldSize = r.cacheValues.getSize();
					r.cacheValues.resize(columns);
					LHR_MEMSET(r.cacheValues.getPointer() + oldSize, 0, sizeof(ValueType) * (columns - oldSize));
				}
			}
		}
	};

	HierarchyCache hierarchyCache;
	bool cacheIsValid;
	bool keepCacheValid;

	void makeCacheValid(DepthValue maxDepth = 0, SizeType columnCount = 0)
	{
		//printf("makeCacheValid(%d, %d)\n", maxDepth, columnCount);
		cacheIsValid = true;

		if (maxDepth == 0)
			maxDepth = findMaxDepth();
		if (columnCount == 0)
			columnCount = getCount();

		hierarchyCache.grow(maxDepth, columnCount);
		partialCacheUpdate(0, columnCount);
		
		// Sentinel column
		for (DepthValue rowNumber = 0; rowNumber < hierarchyCache.getRows(); rowNumber++)
		{
			hierarchyCache.row(rowNumber).column(getCount()) = 0;
		}
	}

	void partialCacheUpdate(HierarchyIndex startColumn, HierarchyIndex endColumn)
	{
		if (!cacheIsValid)
			return;

		if (startColumn == 0)
		{
			//printf("partialCacheUpdate: 0\n", startColumn, endColumn);
			++startColumn;
			for (DepthValue rowNumber = 0; rowNumber < hierarchyCache.getRows(); rowNumber++)
			{
				hierarchyCache.row(rowNumber).column(0) = 0;
			}
		}
		LHR_ASSERT(endColumn <= getCount());

		//printf("partialCacheUpdate: start: %d, count: %d\n", startColumn, endColumn - startColumn);

		for (DepthValue rowNumber = 0; rowNumber < hierarchyCache.getRows(); rowNumber++)
		{
			HierarchyCacheRow& row = hierarchyCache.row(rowNumber);

			for (HierarchyIndex i = startColumn; i < endColumn; i++)
			{
				if (depths[i] > rowNumber)
				{
					row.column(i) = row.column(i - 1) + 1;
				}
				else
				{
					row.column(i) = 0;
				}
			}
		}
	}

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

		if(!keepCacheValid || !cacheIsValid)
			cacheIsValid = false;
		else
		{
			hierarchyCache.grow(1, getCount());
			partialCacheUpdate(newIndex, getCount());
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

		LHR_ASSERT(newIndex > parentIndex);

		SizeType newParentCount = depths[parentIndex] + 1;
		LHR_ASSERT(newParentCount < 100);

		values.insert(newIndex, value);
		depths.insert(newIndex, newParentCount);

		if (!keepCacheValid || !cacheIsValid)
			cacheIsValid = false;
		else
		{
			hierarchyCache.grow(newParentCount, getCount());
			partialCacheUpdate(newIndex, getCount());
		}
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
		if (isChildOf(child, parent, false) && depths[child] <= depths[parent] + 1)
		{
			// Already direct child of parent (or same)
			return child;
		}
		if (isChildOf(parent, child, false))
		{
			LHR_ERROR("That's incest, yo!");
			return child;
		}

		SizeType dest = parent + 1; // Default destination position is right after parent
		SizeType count = getLastDescendant(child) - child + 1; // Descendant count including the child
		LHR_ASSERT(count > 0);

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
				HierarchyCacheRow& mapRow = hierarchyCache.row(depths[parent]);
				DepthValue targetDepth = depths[parent] + 1;
				while (mapRow.column(dest) == dest - parent && (dest > child + count || depths[dest] != targetDepth))
				{
					++dest;
					LHR_ASSERT(dest <= getCount());
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
			LHR_ASSERT(source + count <= dest);
			dest -= count;
		}

		for (SizeType i = 0; i < count; i++)
		{
			depths[dest + i] += depthDiff;
		}

		if (!keepCacheValid || !cacheIsValid)
		{
			cacheIsValid = false;
		}
		else
		{
			hierarchyCache.grow(findMaxDepth(), getCount());
			SizeType start = source < dest ? source : dest;
			SizeType end = getCount(); // TODO: Get old root parent's lastDescendant if it is larger that source+count OR dest+count
			partialCacheUpdate(start, end);
		}

		return dest;
	}

	void erase(HierarchyIndex child)
	{
		SizeType count = getLastDescendant(child) - child + 1;

		const SizeType toShift = getCount() - (child + count);
		LHR_MEMMOVE(depths.getPointer() + child, depths.getPointer() + child + count, toShift * sizeof(DepthValue));
		LHR_MEMMOVE(values.getPointer() + child, values.getPointer() + child + count, toShift * sizeof(ValueType));

		depths.resize(depths.getSize() - count);
		values.resize(values.getSize() - count);


		if (!keepCacheValid || !cacheIsValid)
			cacheIsValid = false;
		else
			partialCacheUpdate(child, getCount());
	}

	void move(SizeType source, SizeType dest, SizeType count)
	{
		static const SizeType MaxStackBytes = 4 / sizeof(ValueType); // Test... Real size would be closer to 1024
		if (count <= MaxStackBytes)
		{
			// Get buffers for values
			DepthValue   stackDepthBuffer[MaxStackBytes];
			ValueType stackValueBuffer[MaxStackBytes];
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
		LHR_MEMCPY(depthBuffer, dPtr + source, count * sizeof(DepthValue));
		LHR_MEMCPY(valueBuffer, vPtr + source, count * sizeof(ValueType) );

		// Shift buffer backward or forward depending on the order of source and destination
		if (source < dest)
		{
			LHR_ASSERT(source + count < dest);
			const SizeType toMove = dest - (source + count);

			LHR_MEMMOVE(dPtr + source, dPtr + source + count, toMove * sizeof(DepthValue));
			LHR_MEMMOVE(vPtr + source, vPtr + source + count, toMove * sizeof(ValueType));
			dest -= count;
		}
		else
		{
			LHR_ASSERT(dest < source);
			const SizeType toMove = source - dest;

			LHR_MEMMOVE(dPtr + dest + count, dPtr + dest, toMove * sizeof(DepthValue));
			LHR_MEMMOVE(vPtr + dest + count, vPtr + dest, toMove * sizeof(ValueType));
		}

		LHR_MEMCPY(dPtr + dest, depthBuffer, count * sizeof(DepthValue));
		LHR_MEMCPY(vPtr + dest, valueBuffer, count * sizeof(ValueType));
	}

	inline bool isChildOf(HierarchyIndex child, HierarchyIndex parent) const
	{
		LHR_ASSERT(cacheIsValid);
		LHR_ASSERT(parent <= child || (child - parent) != getCacheValue(depths[parent], child));
		return (child - parent) == getCacheValue(depths[parent], child);
	}
	inline bool isChildOf(HierarchyIndex child, HierarchyIndex parent, bool partialCacheUpdateIfNecessary)
	{
		if (cacheIsValid)
			return isChildOf(child, parent);

		if (parent > child || depths[parent] > depths[child])
			return false;

		if (!partialCacheUpdateIfNecessary)
		{
			DepthValue parentDepth = depths[parent];
			for (HierarchyIndex c = child; c-- > parent; )
			{
				if (depths[c] <= parentDepth && c > parent)
					return false;
			}
			return true;
		}
		else
		{
			makeCacheValid();
			return isChildOf(child, parent);
		}
	}
	inline bool isChildOf(HierarchyIndex child, HierarchyIndex parent, bool partialCacheUpdateIfNecessary) const
	{
		LHR_ASSERT((!partialCacheUpdateIfNecessary) && "Calling const version of isChildOf with partialCacheUpdateIfNecessary as true");

		if (cacheIsValid)
			return isChildOf(child, parent);

		if (parent > child && depths[parent] > depths[child])
			return false;

		DepthValue parentDepth = depths[parent];
		for (HierarchyIndex c = child; c-- > parent; )
		{
			if (depths[c] <= parentDepth && c > parent)
				return false;
		}
		return true;
	}
};

#endif // LHR_LINEARHIERARCHY_H
