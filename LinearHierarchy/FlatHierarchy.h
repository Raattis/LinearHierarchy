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
		FLAT_ASSERT((depths[child] > depths[parent] + 1 || !linearIsChildOf(child, parent)) && "Re-parenting");
		
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
		for (HierarchyIndex c = parent; c < child; ++c)
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











struct HierarchyCache
{
	typedef FLAT_SIZETYPE SizeType;
	typedef SizeType RelativeParentIndex;
	typedef FLAT_DEPTHTYPE RowIndex;
	typedef SizeType ColumnIndex;

	struct Row
	{
		typedef FLAT_SIZETYPE SizeType;
		typedef SizeType RelativeParentIndex;
		typedef SizeType ColumnIndex;

		RelativeParentIndex* cacheValues;

#if FLAT_DEBUGGING_ENABLED == true
	private:
		SizeType DEBUG_columnCount;
	public:
		Row(RelativeParentIndex* cacheValues, SizeType columnCount)
			: cacheValues(cacheValues)
			, DEBUG_columnCount(columnCount)
		{
		}
		Row(const Row& other)
			: cacheValues(other.cacheValues)
			, DEBUG_columnCount(other.DEBUG_columnCount)
		{
		}
#else
		Row(RelativeParentIndex* cacheValues, SizeType columnCount)
			: cacheValues(cacheValues)
		{
		}
		Row(const Row& other)
			: cacheValues(other.cacheValues)
		{
		}
#endif

		inline RelativeParentIndex column(ColumnIndex index) const
		{
			FLAT_DEBUG_ASSERT(index < DEBUG_columnCount);
			return cacheValues[index];
		}
		inline RelativeParentIndex& column(ColumnIndex index)
		{
			FLAT_DEBUG_ASSERT(index < DEBUG_columnCount);
			return cacheValues[index];
		}
	};

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
	}
	~HierarchyCache()
	{
		for (SizeType i = 0; i < buffers.getSize(); i++)
		{
			buffers[i].erase();
		}
	}

	FLAT_VECTOR<Buffer> buffers;
	FLAT_VECTOR<Row> cacheRows;

	// This is set to false after reserve messes up the cache. User can also set it to false.
	// Automatically becomes true when makeCacheValid() is run.
	bool cacheIsValid;
private:
	RowIndex rowCapacity;
	SizeType columnCapacity;
public:
	SizeType DEBUG_getColumnCapacity() const
	{
		return columnCapacity;
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
					cacheRows.pushBack(Row(b.buffer + i * reserveColumns, reserveColumns));
				}
				FLAT_ASSERT(reserveRows == cacheRows.getSize());

				if (cacheIsValid)
				{
					// Keep cache valid

					for (RowIndex row = oldRowCount; row < cacheRows.getSize(); row++)
					{
						FLAT_MEMSET(cacheRows[row].cacheValues, 0, sizeof(Row::RelativeParentIndex) * columnCapacity);
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
		while (cacheRows.getSize() < targetRowCount)
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
				cacheRows.pushBack(Row(b.buffer + (columnCapacity * i), columnCapacity));
			}

		}

		FLAT_ASSERT(reserveRows <= cacheRows.getSize());
	}

	Row& row(RowIndex depth)
	{
		FLAT_ASSERT(depth < cacheRows.getSize());
		return cacheRows[depth];
	}
	const Row& row(RowIndex depth) const
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
			FLAT_MEMSET(cacheRows[row].cacheValues + columnIndex, 0, sizeof(Row::RelativeParentIndex) * (columnCapacity - columnIndex));
		}

		for (RowIndex row = rowIndex; row < cacheRows.getSize(); row++)
		{
			FLAT_MEMSET(cacheRows[row].cacheValues, 0, sizeof(Row::RelativeParentIndex) * columnCapacity);
		}
	}

	inline bool isChildOf(ColumnIndex child, ColumnIndex parent, RowIndex parentDepth) const
	{
		FLAT_ASSERT(cacheIsValid);
		FLAT_ASSERT(parent <= child || (child - parent) == row(parentDepth).column(child));
		return (child - parent) == row(parentDepth).column(child);
	}
	inline bool getParentIndex(RowIndex parentDepth, ColumnIndex child) const
	{
		FLAT_ASSERT(cacheIsValid);
		return row(parentDepth).column(child);
	}
	void makeCacheValid(const FlatHierarchyBase& hierarchy, RowIndex maxDepth = 0)
	{
		if (maxDepth == 0)
			maxDepth = hierarchy.findMaxDepth();

		printf("makeCacheValid(%d)\n", maxDepth);

		FLAT_ASSERT(maxDepth == hierarchy.findMaxDepth());

		reserve(maxDepth, hierarchy.getCount());
		cacheIsValid = true;

		clearValuesOutSide(maxDepth, hierarchy.getCount());

		for (RowIndex rowNumber = 0; rowNumber < maxDepth; rowNumber++)
		{
			Row& row = this->row(rowNumber);

			row.column(0) = 0;
			for (ColumnIndex i = 1; i < hierarchy.getCount(); i++)
			{
				if (hierarchy.depths[i] > rowNumber)
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

	void partialUpdate(const FlatHierarchyBase& hierarchy, ColumnIndex startColumn, ColumnIndex endColumn)
	{
		FLAT_ASSERT(endColumn <= hierarchy.getCount());

		//printf("partialCacheUpdate: start: %d, count: %d\n", startColumn, endColumn - startColumn);

		for (RowIndex rowNumber = 0, end = hierarchy.findMaxDepth(); rowNumber < end; rowNumber++)
		{
			Row& row = this->row(rowNumber);

			ColumnIndex i = startColumn;
			if (i == 0)
			{
				row.column(0) = 0;
				++i;
			}

			for (; i < endColumn; i++)
			{
				if (hierarchy.depths[i] > rowNumber)
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

};

struct ArrayCache
{
	typedef FlatHierarchyBase::SizeType SizeType;
	typedef FlatHierarchyBase::SizeType CacheValue;
	typedef FlatHierarchyBase::HierarchyIndex HierarchyIndex;
	typedef FlatHierarchyBase::DepthValue DepthValue;

	FLAT_VECTOR<CacheValue> cacheValues;
	bool cacheIsValid;

	CacheValue operator[] (HierarchyIndex index)
	{
		FLAT_ASSERT(index < cacheValues.getSize());
		return cacheValues[index];
	}

	void clear()
	{
		FLAT_MEMSET(cacheValues.getPointer(), 0, sizeof(CacheValue) * cacheValues.getSize());
	}

	void reserve(SizeType capacity)
	{
		if (cacheValues.getSize() < capacity)
		{
			SizeType oldSize = cacheValues.getSize();
			cacheValues.resize(getNextPowerOfTwo(capacity - 1));
			if(cacheIsValid)
				FLAT_MEMSET(cacheValues.getPointer() + oldSize, 0, sizeof(CacheValue) * (cacheValues.getSize() - oldSize));
		}
	}
};

struct NextSiblingCache : public ArrayCache
{
	HierarchyIndex getNextSibling(HierarchyIndex parent) const
	{
		FLAT_ASSERT(parent < cacheValues.getSize());
		return cacheValues[parent];
	}

	// O(MaxDepth/BufferCapacity * N)
	void makeCacheValid(const FlatHierarchyBase& h)
	{
		cacheIsValid = true;
		SizeType maxDepth = 0; 

		static const SizeType BufferCapacity = 4096 / sizeof(HierarchyIndex);
		for (DepthValue currentDepth = 0; currentDepth <= maxDepth; currentDepth += BufferCapacity)
		{
			HierarchyIndex buffer[BufferCapacity];
			FLAT_MEMSET(buffer, FlatHierarchyBase::getIndexNotFound(), sizeof(HierarchyIndex) * BufferCapacity);

			for (HierarchyIndex i = h.getCount(); i-- > 0; )
			{
				DepthValue d = h.depths[i];
				if (d - currentDepth < BufferCapacity)
				{
					cacheValues[i] = buffer[d - currentDepth];
					buffer[d - currentDepth] = i;
				}
				if (maxDepth < d)
					maxDepth = d;
			}
		}
	}
};


#endif // FLAT_FLATHIERARCHY_H
