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
	#define FLAT_MAX_MAXDEPTH ((1 << 15) - 1)
	#define FLAT_MAXDEPTH (((FLAT_DEPTHTYPE)(~0) >> 1) < FLAT_MAX_MAXDEPTH ? ((FLAT_DEPTHTYPE)(~0) >> 1) : (FLAT_DEPTHTYPE)FLAT_MAX_MAXDEPTH)
#endif

#ifndef FLAT_ASSERT
	#if FLAT_ASSERTS_ENABLED == true
		#include <assert.h>
		#define FLAT_ASSERT(expr) assert(expr)
	#else
		#define FLAT_ASSERT(expr) do{}while(false)
	#endif
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

struct DefaultSorter
{
	static const bool UseSorting = true;

	template<typename T>
	inline static bool isFirst(const T& a, const T& b) { return a < b; }
};

struct HierarchyCacheRow
{
	typedef FLAT_SIZETYPE SizeType;
	typedef SizeType RelativeParentIndex;
	typedef SizeType ColumnIndex;

	RelativeParentIndex* cacheValues;

#if FLAT_ASSERTS_ENABLED
private:
	SizeType DEBUG_columnCount;
public:
	HierarchyCacheRow(RelativeParentIndex* cacheValues, SizeType columnCount)
		: cacheValues(cacheValues)
		, DEBUG_columnCount(columnCount)
	{
	}
	HierarchyCacheRow(const HierarchyCacheRow& other)
		: cacheValues(other.cacheValues)
		, DEBUG_columnCount(other.DEBUG_columnCount)
	{
	}
#else
	HierarchyCacheRow(RelativeParentIndex* cacheValues, SizeType columnCount)
		: cacheValues(cacheValues)
	{
	}
	HierarchyCacheRow(const HierarchyCacheRow& other)
		: cacheValues(other.cacheValues)
	{
	}
#endif

	inline RelativeParentIndex column(ColumnIndex index) const
	{
		FLAT_ASSERT(index < DEBUG_columnCount);
		return cacheValues[index];
	}
	inline RelativeParentIndex& column(ColumnIndex index)
	{
		FLAT_ASSERT(index < DEBUG_columnCount);
		return cacheValues[index];
	}
};

struct HierarchyCache
{
	typedef FLAT_SIZETYPE SizeType;
	typedef SizeType RelativeParentIndex;
	typedef FLAT_DEPTHTYPE RowIndex;
	typedef SizeType ColumnIndex;
private:
	struct Buffer
	{
		Buffer()
			: capacity(0)
			, buffer(NULL)
		{
			FLAT_ASSERT(!"What are you doing here?");
		}

		Buffer(SizeType capacity)
			: capacity(capacity)
			, buffer((RelativeParentIndex*)malloc(sizeof(RelativeParentIndex) * capacity))
		{
			FLAT_ASSERT(capacity > 0);
			FLAT_ASSERT(buffer);
		}
		void erase()
		{
			FLAT_ASSERT(buffer != NULL);
			free(buffer);
			buffer = NULL;
			capacity = 0;
		}

		RelativeParentIndex* buffer;
		SizeType capacity;
	};

public:
	HierarchyCache()
		: cacheIsValid(false)
		, rowCapacity(0)
		, columnCapacity(0)
	{
	}
	HierarchyCache(RowIndex reserveRows, ColumnIndex reserveColumns)
		: cacheIsValid(false)
		, rowCapacity(0)
		, columnCapacity(0)
	{
		reserve(reserveRows, reserveColumns);
		cacheIsValid = false;
	}
	~HierarchyCache()
	{
		for (SizeType i = 0; i < buffers.getSize(); i++)
		{
			buffers[i].erase();
		}
	}

	FLAT_VECTOR<Buffer> buffers;
	FLAT_VECTOR<HierarchyCacheRow> cacheRows;
	bool cacheIsValid;
private:
	RowIndex rowCapacity;
	SizeType columnCapacity;
public:
	SizeType DEBUG_getColumnCapacity() const
	{
		return columnCapacity;
	}
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

	void reserve(RowIndex reserveRows, SizeType reserveColumns)
	{
		if (reserveRows <= rowCapacity && reserveColumns <= columnCapacity)
			return;
		
		reserveColumns = getNextPowerOfTwo(reserveColumns - 1);
		
		if (reserveRows > rowCapacity && reserveColumns <= columnCapacity)
		{
			// Only adding new rows

			// Store the row count that was requested, not the actual number of cacheRows (allows shuffling memory later if columns are added)
			rowCapacity = reserveRows;

			if (reserveRows > cacheRows.getSize())
			{
				SizeType oldRowCount = cacheRows.getSize();
				RowIndex targetRowCount = reserveRows + 1024 / reserveColumns;
				cacheRows.reserve(targetRowCount);
				buffers.pushBack(Buffer((reserveColumns)* (targetRowCount - cacheRows.getSize())));

				Buffer& b = buffers.getBack();
				for (SizeType i = 0, end = targetRowCount - cacheRows.getSize(); i < end; i++)
				{
					cacheRows.pushBack(HierarchyCacheRow(b.buffer + i * reserveColumns, reserveColumns));
				}
				FLAT_ASSERT(reserveRows == cacheRows.getSize());

				if (cacheIsValid)
				{
					// Keep cache valid

					for (RowIndex row = oldRowCount; row < cacheRows.getSize(); row++)
					{
						FLAT_MEMSET(cacheRows[row].cacheValues, 0, sizeof(HierarchyCacheRow::RelativeParentIndex) * columnCapacity);
					}
				}
			}
			return;
		}

		// Adding columns and possibly also rows
		
		cacheIsValid = false; // Shuffling row buffers will always invalidate cache
		columnCapacity = reserveColumns;

		// Free buffers that have become smaller than columnCapacity
		{
			for (SizeType i = 0; i < buffers.getSize(); )
			{
				if (buffers[i].capacity < columnCapacity)
				{
					buffers[i].erase();
					buffers[i] = buffers.getBack();
					buffers.resize(buffers.getSize() - 1);
					continue;
				}
				++i;
			}
		}
		
		RowIndex targetRowCount = rowCapacity;
		if (reserveRows > rowCapacity)
		{
			rowCapacity = reserveRows;
			targetRowCount = reserveRows + 1024 / columnCapacity;
		}

		cacheRows.clear();
		cacheRows.reserve(targetRowCount);
		
		// Recycle buffers
		SizeType bufferIndex = 0;
		while(cacheRows.getSize() < targetRowCount)
		{
			if (bufferIndex >= buffers.getSize())
			{
				// Create new buffer

				SizeType bufferRowCount = getNextPowerOfTwo(targetRowCount - cacheRows.getSize() - 1);
				FLAT_ASSERT(bufferRowCount >= targetRowCount - cacheRows.getSize());
				buffers.pushBack(Buffer(columnCapacity * bufferRowCount));
			}

			// Distribute buffers to cacheRows
			Buffer& b = buffers[bufferIndex];
			++bufferIndex;

			FLAT_ASSERT(b.capacity >= columnCapacity);
			for (SizeType i = 0, end = b.capacity / columnCapacity; i < end; ++i)
			{
				cacheRows.pushBack(HierarchyCacheRow(b.buffer + (columnCapacity * i), columnCapacity));
			}

		}

		FLAT_ASSERT(reserveRows <= cacheRows.getSize());
	}

	HierarchyCacheRow& row(RowIndex depth)
	{
		FLAT_ASSERT(depth < cacheRows.getSize());
		return cacheRows[depth];
	}
	const HierarchyCacheRow& row(RowIndex depth) const
	{
		FLAT_ASSERT(depth < cacheRows.getSize());
		return cacheRows[depth];
	}

	void clearValuesOutSide(RowIndex rowIndex, ColumnIndex columnIndex)
	{
		FLAT_ASSERT(columnIndex <= columnCapacity);
		FLAT_ASSERT(rowIndex < cacheRows.getSize());

		for (RowIndex row = 0; row < rowIndex; row++)
		{
			FLAT_MEMSET(cacheRows[row].cacheValues + columnIndex, 0, sizeof(HierarchyCacheRow::RelativeParentIndex) * (columnCapacity - columnIndex));
		}

		for (RowIndex row = rowIndex; row < cacheRows.getSize(); row++)
		{
			FLAT_MEMSET(cacheRows[row].cacheValues, 0, sizeof(HierarchyCacheRow::RelativeParentIndex) * columnCapacity);
		}
	}
};

template<typename ValueType, typename Sorter = DefaultSorter>
class FlatHierarchy
{
public:
	typedef FLAT_SIZETYPE SizeType;
	typedef FLAT_DEPTHTYPE DepthValue;
	typedef SizeType HierarchyIndex;
	typedef SizeType RelativeParentIndex;
	typedef SizeType RelativeChildIndex;
	typedef SizeType RelativeSiblingIndex;

	FLAT_VECTOR<ValueType> values;
	FLAT_VECTOR<DepthValue> depths;

	FLAT_VECTOR<RelativeChildIndex> lastDescendantCache;
	FLAT_VECTOR<RelativeChildIndex> nextSiblingCache;

	HierarchyCache hierarchyCache;
	bool keepCacheValid;
	bool getIsCacheValid() const { return hierarchyCache.cacheIsValid; }

	FlatHierarchy()
		: keepCacheValid(false)
		, hierarchyCache()
	{
	}
	FlatHierarchy(SizeType reserveDepth, SizeType reserveSize)
		: keepCacheValid(false)
		, hierarchyCache(reserveDepth, reserveSize)
	{
		values.reserve(reserveSize);
		depths.reserve(reserveSize);
		hierarchyCache.reserve(reserveDepth, reserveSize);
	}
	~FlatHierarchy()
	{
	}

	RelativeParentIndex getCacheValue(DepthValue row, HierarchyIndex column) const
	{
		FLAT_ASSERT(getIsCacheValid());
		return hierarchyCache.row(row).column(column);
	}
	HierarchyIndex getParentIndex(DepthValue depth, HierarchyIndex child) const
	{
		FLAT_ASSERT(getIsCacheValid());
		FLAT_ASSERT(child >= hierarchyCache.row(depth).column(child));
		return child - hierarchyCache.row(depth).column(child);
	}
	HierarchyIndex getParentIndexRegardlessOfCache(DepthValue depth, HierarchyIndex child) const
	{
		if (getIsCacheValid())
		{
			return getParentIndex(depth, child);
		}
		
		for (HierarchyIndex i = child + 1; i-- > 0; )
		{
			if (depths[i] == depth)
				return i;
		}
		FLAT_ASSERT(!"Everybody must have a parent... What is this sorcery?")
		return 0;
	}
	void makeCacheValid(DepthValue maxDepth = 0)
	{
		if (maxDepth == 0)
			maxDepth = findMaxDepth();

		printf("makeCacheValid(%d)\n", maxDepth);

		FLAT_ASSERT(maxDepth == findMaxDepth());

		hierarchyCache.reserve(maxDepth, getCount());
		hierarchyCache.cacheIsValid = true;

		hierarchyCache.clearValuesOutSide(maxDepth, getCount());

		for (DepthValue rowNumber = 0; rowNumber < maxDepth; rowNumber++)
		{
			HierarchyCacheRow& row = hierarchyCache.row(rowNumber);

			row.column(0) = 0;
			for (HierarchyIndex i = 1; i < getCount(); i++)
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

	void partialCacheUpdate(HierarchyIndex startColumn, HierarchyIndex endColumn)
	{
		if (!getIsCacheValid())
			return;

		FLAT_ASSERT(endColumn <= getCount());

		//printf("partialCacheUpdate: start: %d, count: %d\n", startColumn, endColumn - startColumn);

		for (DepthValue rowNumber = 0, end = findMaxDepth(); rowNumber < end; rowNumber++)
		{
			HierarchyCacheRow& row = hierarchyCache.row(rowNumber);

			HierarchyIndex i = startColumn;
			if (i == 0)
			{
				row.column(0) = 0;
				++i;
			}

			for (; i < endColumn; i++)
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

		if(!keepCacheValid || !getIsCacheValid())
			hierarchyCache.cacheIsValid = false;
		else
		{
			hierarchyCache.reserve(1, getCount());
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

		FLAT_ASSERT(newIndex > parentIndex);

		SizeType newParentCount = depths[parentIndex] + 1;
		FLAT_ASSERT(newParentCount < FLAT_MAXDEPTH); // Over flow protection

		values.insert(newIndex, value);
		depths.insert(newIndex, newParentCount);

		if (!keepCacheValid || !getIsCacheValid())
			hierarchyCache.cacheIsValid = false;
		else
		{
			hierarchyCache.reserve(newParentCount, getCount());
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
		
		if (child == parent)
		{
			FLAT_ERROR("Cannot be own parent.");
			return child;
		}

		if (isChildOf(child, parent, false) && depths[child] <= depths[parent] + 1)
		{
			// Already direct child of parent (or same)
			return child;
		}
		if (isChildOf(parent, child, false))
		{
			FLAT_ERROR("That's incest, yo!");
			return child;
		}

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
				HierarchyCacheRow& mapRow = hierarchyCache.row(depths[parent]);
				DepthValue targetDepth = depths[parent] + 1;
				while (mapRow.column(dest) == dest - parent && (dest > child + count || depths[dest] != targetDepth))
				{
					++dest;
					FLAT_ASSERT(dest <= getCount());
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
			FLAT_ASSERT(depths[dest + i] < FLAT_MAXDEPTH); // Over flow protection
		}

		if (!keepCacheValid || !getIsCacheValid())
		{
			hierarchyCache.cacheIsValid = false;
		}
		else
		{
			hierarchyCache.reserve(findMaxDepth(), getCount());
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
		FLAT_MEMMOVE(depths.getPointer() + child, depths.getPointer() + child + count, toShift * sizeof(DepthValue));
		FLAT_MEMMOVE(values.getPointer() + child, values.getPointer() + child + count, toShift * sizeof(ValueType));

		depths.resize(depths.getSize() - count);
		values.resize(values.getSize() - count);

		if (!keepCacheValid || !getIsCacheValid())
			hierarchyCache.cacheIsValid = false;
		else
			partialCacheUpdate(child, getCount());
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

	inline bool isChildOf(HierarchyIndex child, HierarchyIndex parent) const
	{
		FLAT_ASSERT(getIsCacheValid());
		FLAT_ASSERT(parent <= child || (child - parent) != getCacheValue(depths[parent], child));
		return (child - parent) == getCacheValue(depths[parent], child);
	}
	inline bool isChildOf(HierarchyIndex child, HierarchyIndex parent, bool updateCacheIfNecessary)
	{
		if (getIsCacheValid())
			return isChildOf(child, parent);

		if (parent > child || depths[parent] > depths[child])
			return false;

		if (!updateCacheIfNecessary)
		{
			if (child == parent)
				return true;

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
	inline bool isChildOf(HierarchyIndex child, HierarchyIndex parent, bool updateCacheIfNecessary) const
	{
		FLAT_ASSERT((!updateCacheIfNecessary) && "Calling const version of isChildOf with updateCacheIfNecessary as true");

		if (getIsCacheValid())
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

#endif // FLAT_FLATHIERARCHY_H
