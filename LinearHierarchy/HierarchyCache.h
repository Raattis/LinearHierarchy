#ifndef FLAT_HIERARCHYCACHE_H
#define FLAT_HIERARCHYCACHE_H

#include "FlatAssert.h"
#include "FlatHierarchy.h"

#ifdef _DEBUG
	#define FLAT_DEBUG_ASSERT(...) FLAT_ASSERT(__VA_ARGS__)
#else
	#define FLAT_DEBUG_ASSERT(...) do{}while(false)
#endif

#ifndef FLAT_DEBUGGING_ENABLED
	#ifdef _DEBUG
		#define FLAT_DEBUGGING_ENABLED true
	#else
		#define FLAT_DEBUGGING_ENABLED false
	#endif
#endif

#ifndef FLAT_MEMSET
	#include <string.h> /* memset */
	#define FLAT_MEMSET(dst, value, length) memset(dst, value, length)
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
				RowIndex oldRowCount = (RowIndex)cacheRows.getSize();
				RowIndex targetRowCount = reserveRows + (RowIndex)(1024 / reserveColumns);
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
			targetRowCount = (reserveRows + 1024 / columnCapacity) > FLAT_MAXDEPTH ? FLAT_MAXDEPTH : (RowIndex)(reserveRows + 1024 / columnCapacity);
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
	inline ColumnIndex getParentIndex(RowIndex parentDepth, ColumnIndex child) const
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

	ArrayCache()
		: cacheIsValid(false)
	{
	}

	CacheValue operator[] (HierarchyIndex index) const
	{
		FLAT_ASSERT(cacheIsValid);
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
			if (cacheIsValid)
				FLAT_MEMSET(cacheValues.getPointer() + oldSize, 0, sizeof(CacheValue) * (cacheValues.getSize() - oldSize));
		}
	}
};

struct NextSiblingCache : public ArrayCache
{
	// O(MaxDepth/BufferCapacity * N)
	void makeCacheValid(const FlatHierarchyBase& h)
	{
		cacheIsValid = true;
		SizeType maxDepth = 0;

		cacheValues.resize(h.getCount());

		if (h.getCount() == 0)
			return;

		const HierarchyIndex lastIndex = h.getCount() - 1;
		cacheValues[lastIndex] = FlatHierarchyBase::getIndexNotFound();

		const SizeType BufferCapacity = (4096 / sizeof(HierarchyIndex)) < 1 ? 1 : 4096 / sizeof(HierarchyIndex);
		for (SizeType currentDepth = 0; currentDepth <= maxDepth; currentDepth += BufferCapacity)
		{
			HierarchyIndex buffer[BufferCapacity];
			for (HierarchyIndex i = 0, end = BufferCapacity; i < end; i++)
			{
				buffer[i] = FlatHierarchyBase::getIndexNotFound();
			}

			SizeType previousDepthValue = h.depths[lastIndex];
			buffer[previousDepthValue - currentDepth] = lastIndex;

			for (HierarchyIndex i = lastIndex; i-- > 0; )
			{
				SizeType d = (SizeType)(h.depths[i]);
				if (d - currentDepth < BufferCapacity)
				{
					FLAT_ASSERT(d + 1 >= previousDepthValue);
					cacheValues[i] = buffer[d - currentDepth];
					buffer[d - currentDepth] = i;

					if (d < previousDepthValue) // Depth decreases
					{
						FLAT_ASSERT(d + 1 == previousDepthValue && "Depth should never decrease by more than 1 when iterating backwards.");
						buffer[previousDepthValue - currentDepth] = FlatHierarchyBase::getIndexNotFound();
					}
				}

				previousDepthValue = d;

				if (maxDepth < d)
					maxDepth = d;
			}
		}
	}

	// O(1)
	HierarchyIndex getNextSibling(HierarchyIndex index) const
	{
		FLAT_ASSERT(cacheIsValid);
		FLAT_ASSERT(index < cacheValues.getSize());
		return cacheValues[index];
	}
	HierarchyIndex getNextSibling(const FlatHierarchyBase& h, HierarchyIndex index)
	{
		FLAT_ASSERT(index < h.getCount());
		if (!cacheIsValid)
			makeCacheValid(h);
		return getNextSibling(index);
	}
};





struct LastDescendantCache : public ArrayCache
{
	// O(MaxDepth/BufferCapacity * N)
	void makeCacheValid(const FlatHierarchyBase& h)
	{
		cacheIsValid = true;
		SizeType maxDepth = 0;

		cacheValues.resize(h.getCount());

		if (h.getCount() == 0)
			return;

		const HierarchyIndex lastIndex = h.getCount() - 1;
		cacheValues[lastIndex] = lastIndex;

		const SizeType BufferCapacity = (4096 / sizeof(HierarchyIndex)) < 1 ? 1 : 4096 / sizeof(HierarchyIndex);
		for (SizeType currentDepth = 0; currentDepth <= maxDepth; currentDepth += BufferCapacity)
		{
			HierarchyIndex buffer[BufferCapacity];
			for (HierarchyIndex i = 0, end = BufferCapacity; i < end; i++)
			{
				buffer[i] = lastIndex;
			}

			SizeType previousDepthValue = h.depths[lastIndex];

			for (HierarchyIndex i = lastIndex; i-- > 0; )
			{
				SizeType d = (SizeType)(h.depths[i]);
				if (d - currentDepth < BufferCapacity)
				{
					if (d == previousDepthValue)    // Depth stays the same
					{
						cacheValues[i] = i;
					}
					else if (d > previousDepthValue) // Depth increases
					{
						cacheValues[i] = i;
						buffer[d - currentDepth] = i;
						for (SizeType d2 = previousDepthValue; d2 <= d; ++d2)
						{
							buffer[d2 - currentDepth] = i;
						}
					}
					else                             // Depth decreases
					{
						cacheValues[i] = buffer[d - currentDepth];
						for (SizeType d2 = d; d2 <= previousDepthValue; ++d2)
						{
							buffer[d2 - currentDepth] = i;
						}
					}
				}

				previousDepthValue = d;

				if (maxDepth < d)
					maxDepth = d;
			}
		}
	}

	// O(1)
	HierarchyIndex getLastDescendant(HierarchyIndex index) const
	{
		FLAT_ASSERT(cacheIsValid);
		FLAT_ASSERT(index < cacheValues.getSize());
		return cacheValues[index];
	}
	HierarchyIndex getLastDescendant(const FlatHierarchyBase& h, HierarchyIndex index)
	{
		FLAT_ASSERT(index < h.getCount());
		if (!cacheIsValid)
			makeCacheValid(h);
		return getLastDescendant(index);
	}
};










template<typename ValueType, typename Sorter>
FlatHierarchyBase::HierarchyIndex makeChildOf(FlatHierarchy<ValueType, Sorter>& h, LastDescendantCache& descendantCache, FlatHierarchyBase::HierarchyIndex child, FlatHierarchyBase::HierarchyIndex parent)
{
	FLAT_ASSERT(child != parent && "Self-adoption");
	FLAT_ASSERT(!h.linearIsChildOf(parent, child) && "Incest");
	FLAT_ASSERT((h.depths[child] != h.depths[parent] + 1 || !h.linearIsChildOf(child, parent)) && "Re-parenting");

	SizeType dest = parent + 1; // Default destination position is right after parent
	SizeType count = descendantCache.getLastDescendant(h, child) - child + 1; // Descendant count including the child
	FLAT_ASSERT(count > 0);

	if (Sorter::UseSorting == true)
	{
		// Find a destination position that will have the child sorted among its siblings

		dest = descendantCache.getLastDescendant(h, parent) + 1; // Default to last possible index

		FlatHierarchyBase::DepthValue targetDepth = h.depths[parent] + 1;
		for (SizeType i = parent + 1; i < dest; ++i)
		{
			if (h.depths[i] == targetDepth && Sorter::isFirst(h.values[child], h.values[i]))
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
			// Commented to unify behavior with pointer trees
			//// If child's index is greater than parent, find a place in parents children that is closest to child's current position
			//FlatHierarchyBase::DepthValue targetDepth = h.depths[parent] + 1;
			//
			//for (FlatHierarchyBase::HierarchyIndex currentPlace = parent + 1
			//	; currentPlace < child
			//	; currentPlace = descendantCache.getLastDescendant(h, currentPlace) + 1)
			//{
			//	FLAT_ASSERT(h.depths[currentPlace] <= targetDepth);
			//	if (h.depths[currentPlace] < targetDepth)
			//		break;
			//	dest = currentPlace;
			//}
		}
	}

	// Do the move

	SizeType source = child;


	FlatHierarchyBase::DepthValue depthDiff = h.depths[parent] + 1 - h.depths[child];

	if (source != dest && source + count != dest)
	{
		h.move(source, dest, count);
	}

	if (source < dest)
	{
		FLAT_ASSERT(source + count <= dest);
		dest -= count;
	}

	for (SizeType i = 0; i < count; i++)
	{
		h.depths[dest + i] += depthDiff;
		FLAT_ASSERT(h.depths[dest + i] < FLAT_MAXDEPTH && "Over/Under-flow threat detected"); // Over flow protection
	}

	descendantCache.cacheIsValid = false;
	// TODO: Update cache

	return dest;
}





template<typename ValueType, typename Sorter>
void erase(FlatHierarchy<ValueType, Sorter>& h, LastDescendantCache& descendantCache, FlatHierarchyBase::HierarchyIndex child)
{
	SizeType count = descendantCache.getLastDescendant(h, child) - child + 1;

	const SizeType toShift = h.getCount() - (child + count);
	FLAT_MEMMOVE(h.depths.getPointer() + child, h.depths.getPointer() + child + count, toShift * sizeof(FlatHierarchyBase::DepthValue));
	FLAT_MEMMOVE(h.values.getPointer() + child, h.values.getPointer() + child + count, toShift * sizeof(ValueType));

	h.depths.resize(h.depths.getSize() - count);
	h.values.resize(h.values.getSize() - count);

	// TODO: Update cache
	descendantCache.cacheIsValid = false;

}

template<typename ValueType, typename Sorter>
FlatHierarchyBase::HierarchyIndex createNodeAsChildOf(FlatHierarchy<ValueType, Sorter>& h, LastDescendantCache& descendantCache, FlatHierarchyBase::HierarchyIndex parentIndex, const ValueType& value)
{
	SizeType newIndex = parentIndex + 1;

	if (Sorter::UseSorting == true)
	{
		// Default to last possible index
		newIndex = descendantCache.getLastDescendant(h, parentIndex) + 1;

		// Insert to sorted position
		FlatHierarchyBase::DepthValue targetDepth = h.depths[parentIndex] + 1;
		for (SizeType i = parentIndex + 1; i < newIndex; ++i)
		{
			if (h.depths[i] == targetDepth && Sorter::isFirst(value, h.values[i]))
			{
				newIndex = i;
				break;
			}
		}
	}
	else
	{
		// NOTE: Cache structure is not useful if not using sorting. Post warning?
	}

	FLAT_ASSERT(newIndex > parentIndex);

	FLAT_DEPTHTYPE newParentCount = h.depths[parentIndex] + 1;
	FLAT_ASSERT(newParentCount < FLAT_MAXDEPTH); // Over flow protection

	h.values.insert(newIndex, value);
	h.depths.insert(newIndex, newParentCount);

	// TODO: Update cache
	descendantCache.cacheIsValid = false;

	return newIndex;
}

template<typename ValueType, typename Sorter>
FlatHierarchyBase::HierarchyIndex createRootNode(FlatHierarchyBase& h, NextSiblingCache& siblingCache, const ValueType& value)
{
	FlatHierarchyBase::HierarchyIndex newIndex = h.getCount();

	if (Sorter::UseSorting == true)
	{
		// Insert to sorted position

		FlatHierarchyBase::HierarchyIndex currentPlace = 0;
		while (currentPlace < newIndex)
		{
			if (Sorter::isFirst(value, h.values[currentPlace]))
			{
				newIndex = currentPlace;
				break;
			}
			currentPlace = siblingCache[currentPlace];
		}

		h.values.insert(newIndex, value);
		h.depths.insert(newIndex, (FlatHierarchyBase::DepthValue)0U);
	}
	else
	{
		// NOTE: Cache structure is not useful if not using sorting. Post warning?

		h.values.pushBack(value);
		h.depths.pushBack((FlatHierarchyBase::DepthValue)0U);
	}

	// TODO: Update cache
	siblingCache.cacheIsValid = false;

	return newIndex;
}

template<typename ValueType, typename Sorter>
FlatHierarchyBase::HierarchyIndex createRootNode(FlatHierarchy<ValueType, Sorter>& h, LastDescendantCache& descendantCache, const ValueType& value)
{
	FlatHierarchyBase::HierarchyIndex newIndex = h.getCount();

	if (Sorter::UseSorting == true)
	{
		// Insert to sorted position

		if (!descendantCache.cacheIsValid)
			descendantCache.makeCacheValid(h);

		FlatHierarchyBase::HierarchyIndex currentPlace = 0;
		while (currentPlace < newIndex)
		{
			if (Sorter::isFirst(value, h.values[currentPlace]))
			{
				newIndex = currentPlace;
				break;
			}
			currentPlace = descendantCache[currentPlace] + 1;
		}

		h.values.insert(newIndex, value);
		h.depths.insert(newIndex, (FlatHierarchyBase::DepthValue)0U);
	}
	else
	{
		// NOTE: Cache structure is not useful if not using sorting. Post warning?

		h.values.pushBack(value);
		h.depths.pushBack((FlatHierarchyBase::DepthValue)0U);
	}

	// TODO: Update cache
	descendantCache.cacheIsValid = false;

	return newIndex;
}


FlatHierarchyBase::HierarchyIndex countDirectChildren(const FlatHierarchyBase& h, NextSiblingCache& siblingCache, FlatHierarchyBase::HierarchyIndex parent)
{
	SizeType result = 0;
	if (parent + 1 >= h.getCount() || h.depths[parent] + 1 != h.depths[parent + 1])
		return result;
	++result;

	FlatHierarchyBase::HierarchyIndex current = siblingCache.getNextSibling(h, parent + 1);

	while (current < h.getCount())
	{
#ifndef MAX_PERF // Asserts are disabled on MAX_PERF
		SizeType old = current;
#endif
		++result;
		current = siblingCache.getNextSibling(current);

		FLAT_ASSERT(old < current);
		FLAT_ASSERT(result < h.getCount());
	}

	return result;
}

FlatHierarchyBase::HierarchyIndex getNthChild(const FlatHierarchyBase& h, NextSiblingCache& siblingCache, FlatHierarchyBase::HierarchyIndex parent, SizeType n)
{
	FlatHierarchyBase::HierarchyIndex current = parent + 1;
	FLAT_ASSERT(current < h.getCount() && h.depths[parent] + 1 == h.depths[current]);

	if (n-- == 0)
		return current;

	// Only now refresh cache if needed
	current = siblingCache.getNextSibling(h, current);
	FLAT_ASSERT(current < h.getCount() && h.depths[parent] + 1 == h.depths[current]);

	while (n-- > 0)
	{
#ifndef MAX_PERF // Asserts are disabled on MAX_PERF
		SizeType old = current;
#endif
		current = siblingCache.getNextSibling(current);
		FLAT_ASSERT(old < current);
		FLAT_ASSERT(current < h.getCount() && h.depths[parent] + 1 == h.depths[current]);
	}

	return current;
}


FlatHierarchyBase::HierarchyIndex countDirectChildren(const FlatHierarchyBase& h, FlatHierarchyBase::HierarchyIndex parent)
{
	FLAT_ASSERT(parent < h.getCount());

	SizeType result = 0;
	
	const FlatHierarchyBase::DepthValue targetDepth = h.depths[parent] + 1;
	FlatHierarchyBase::HierarchyIndex current = parent + 1;

	while (current < h.getCount() && targetDepth <= h.depths[current])
	{
		if (targetDepth == h.depths[current])
			++result;
		++current;
	}

	return result;
}

FlatHierarchyBase::HierarchyIndex getNthChild(const FlatHierarchyBase& h, FlatHierarchyBase::HierarchyIndex parent, SizeType n)
{
	FLAT_ASSERT(parent < h.getCount());

	const FlatHierarchyBase::DepthValue targetDepth = h.depths[parent] + 1;
	FlatHierarchyBase::HierarchyIndex current = parent + 1;

	FLAT_ASSERT(current < h.getCount() && targetDepth == h.depths[current]);

	while (n > 0)
	{
		++current;
		if (targetDepth == h.depths[current])
		{
			--n;
		}
		FLAT_ASSERT(current < h.getCount() && targetDepth <= h.depths[current]);
	}

	return current;
}

#endif