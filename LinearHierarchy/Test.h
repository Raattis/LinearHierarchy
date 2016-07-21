#pragma once

#ifdef MAX_PERF
	#define FLAT_ASSERTS_ENABLED false
#else
	#define FLAT_ASSERTS_ENABLED true
#endif

#include "FlatHierarchy.h"
#include "HierarchyCache.h"
#include "RivalTree.h"

#include <stdio.h>      /* printf, fgets */
#include <stdlib.h>     /* atoi, rand, srand */
#include <inttypes.h>
#include <time.h>
#include <windows.h>
#include "FastHash.h"

typedef uint32_t SizeType;

#ifdef _DEBUG
const SizeType RoundNumber = 100;
const SizeType EraseNumber = 10;
const SizeType AddNumber = 200;
#elif defined(MAX_PERF)
const SizeType RoundNumber = 10000;
const SizeType EraseNumber = 20;
const SizeType AddNumber = 400;
#else
const SizeType RoundNumber = 1000;
const SizeType EraseNumber = 20;
const SizeType AddNumber = 400;

//const SizeType RoundNumber = 100;
//const SizeType EraseNumber = 20;
//const SizeType AddNumber = 400;
#endif

#ifdef MAX_PERF
	const bool VerbosePrinting = false;
	const bool CheckHashes = false;
	const bool CheckCounts = false;
#else
	const bool VerbosePrinting = false;
	const bool CheckHashes = true;
	const bool CheckCounts = true;
#endif

SizeType Rundi = 0;

#if false
#define FLAT_NO_CACHE_CONDITION     (Random::random % 8 <= 3)
#define FLAT_CACHE_CONDITION        (Random::random % 8 > 3 && Random::random % 8 <= 6)
#define FLAT_CACHE_UNPREP_CONDITION (Random::random % 8 == 7)
#elif false
#define FLAT_NO_CACHE_CONDITION     (true)
#define FLAT_CACHE_CONDITION        (false)
#define FLAT_CACHE_UNPREP_CONDITION (false)
#else
#define FLAT_NO_CACHE_CONDITION     (Rundi == 0)
#define FLAT_CACHE_CONDITION        (Rundi == 1)
#define FLAT_CACHE_UNPREP_CONDITION (Rundi == 2)
#endif

struct Handle
{
	union {
		char text[4];
		uint32_t number;
	};

	bool operator==(const Handle& other)
	{
		return number == other.number;
	}
	bool operator<(const Handle& other)
	{
		return number < other.number;
	}

	String toString() const
	{
		if (!this)
			return String("NULL", 4);
		return String(text, 4);
	}
};
static_assert(sizeof(Handle) == 4, "Size of handle must be 4");

struct HandleSorter
{
	static const bool UseSorting = true;

	inline static bool isFirst(const Handle& a, const Handle& b) { return a.number < b.number; }
};

Handle getHandle(const char* ptr)
{
	return *reinterpret_cast<const Handle*>(ptr);
}
const char* getName(Handle handle, char* buffer)
{
	memcpy(buffer, &handle, 4);
	return buffer;
}

namespace // Logger
{
	char LogBuffer[1024 * 16];
	SizeType LogBufferProgress = 0;
	void logTable(const char* msg)
	{
		SizeType l = (SizeType)strlen(msg);
		memcpy(LogBuffer + LogBufferProgress, msg, l);
		LogBufferProgress += l;
	}
	void logTableEnd()
	{
		LogBuffer[LogBufferProgress] = '\0';
	}
	void logTableDouble(double value)
	{
		char buffer[128];
		errno_t err = sprintf_s(buffer, "%.4lf\t", value);
		FLAT_ASSERT(err >= 0);
		logTable(buffer);
	}
}

namespace
{
	const uint32_t HashBufferSize = 1024 * 32;
	uint32_t HashBuffer[HashBufferSize];
	uint32_t CountBuffer[HashBufferSize];
	bool CheckingHashes = false;
	SizeType CurrentHash = 0;
	SizeType CurrentCounter = 0;

	void setHash(uint32_t hash)
	{
		if (!CheckingHashes)
			HashBuffer[CurrentHash++] = hash;
		else
			FLAT_ASSERT(HashBuffer[CurrentHash++] == hash);
	}
	void setCount(uint32_t count)
	{
		if (!CheckingHashes)
			CountBuffer[CurrentCounter++] = count;
		else
			FLAT_ASSERT(CountBuffer[CurrentCounter++] == count);
	}
}


SizeType MaxCount = 0;
SizeType MaxDepth = 0;
SizeType MaxTravelDepth = 0;
double AverageCount = 0;
double AverageDepth = 0;
double AverageTravelDepth = 0;


class ScopedProfiler
{
public:
	double* cumulator;
	LARGE_INTEGER start;

	ScopedProfiler(double* cumulator = NULL)
		: cumulator(cumulator)
	{
		QueryPerformanceCounter(&start);
	}

	double stop()
	{
		LARGE_INTEGER end;
		QueryPerformanceCounter(&end);

		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		double div = double(freq.QuadPart) / 1000000.0;

		double diff = double(end.QuadPart - start.QuadPart) / div;
		if (cumulator != NULL)
			(*cumulator) += diff;

		return diff;
	}

	~ScopedProfiler()
	{
		stop();
	}

};

struct Random
{
	static const uint32_t InitSeed = 1771551;
	static uint32_t random;
	static uint32_t counter;
	static void init(uint32_t seed = InitSeed)
	{
		random = seed;
		srand(seed);
		counter = 0;
	}
	static uint32_t get(SizeType min, SizeType max)
	{
		int r = rand();
		random = *reinterpret_cast<uint32_t*>(&r);
		uint32_t result = max <= min ? max : min + random % (max - min);
		++counter;
		//if (VerbosePrinting)
		//{
		//	printf("RandomRange. Counter: %u, Value: %u, Min: %u, Max: %u\n", counter, result, min, max);
		//}
		return result;
	}
};
uint32_t Random::random = 0;
uint32_t Random::counter = 0;

void FLAT_Test()
{
	typedef FlatHierarchy<Handle, HandleSorter> HierarchyType;

	struct PRINT
	{
		static void print(const HierarchyType& l)
		{
			printf("\n");
			for (SizeType c = 0; c < l.getCount(); c++)
			{
				char buffer[5];
				buffer[4] = '\0';
				printf("%s", getName(l.values[c], buffer));
			}
			printf("\n");
			for (SizeType c = 0; c < l.getCount(); c++)
			{
				if (l.depths[c] < 10)
					printf("  %u ", l.depths[c]);
				else
					printf(" %u ", l.depths[c]);
			}
			printf("\n");
			for (SizeType c = 0; c < l.getCount() + 1; c++)
			{
				printf("----");
			}

			printf("\n");
			for (SizeType c = 0; c < l.getCount(); c++)
			{
				if (c < 10)
					printf(" %d  ", c);
				else
					printf(" %d ", c);
			}

			printf("\n");


			char buffer[4096];
			for (SizeType r = 0, end = l.findMaxDepth() + 1; r < end; r++)
			{
				buffer[l.getCount() * 4] = '\0';

				// Iterate columns backwards to benefit from linear parent lookup
				for (SizeType c = 0; c < l.getCount(); ++c)
				{
					bool hasDirectChildren = false;

					for (SizeType i = c + 1; i < l.getCount(); i++)
					{
						if (l.depths[i] > r + 1)
							continue;
						hasDirectChildren = l.depths[i] == r + 1;
						break;
					}

					if (l.depths[c] == r)
						getName(l.values[c], buffer + c * 4);
					else if (l.depths[c] == r + 1)
						memcpy(buffer + c * 4, (hasDirectChildren ? "„¦„Ÿ" : "„¢  "), 4);
					else if (l.depths[c] > r && hasDirectChildren)
						memcpy(buffer + c * 4, "„Ÿ„Ÿ", 4);
					else
						memcpy(buffer + c * 4, "    ", 4);
				}
				printf("%s\n", buffer);
			}
			printf("\n");
		}
	};

	HierarchyType l(70);

	l.createRootNode(getHandle("Root"));

	printf("Random seed: %u\n", Random::random);


	double cumulativeAdd = 0.0f;
	double cumulativeAddRoot = 0.0f;
	double cumulativeMove = 0.0f;
	double cumulativeErase = 0.0f;
	uint32_t cumulativeAddCount = 0;
	uint32_t cumulativeAddRootCount = 0;
	uint32_t cumulativeMoveCount = 0;
	uint32_t cumulativeEraseCount = 0;

	double calculateSizeTime = 0;
	double calculateDepthTime = 0;
	uint32_t calculateSizeTimeCount = 0;
	uint32_t calculateDepthTimeCount = 0;

	double cumulativeSize = 0;
	double cumulativeDepth = 0;
	SizeType maxCount = 0;
	SizeType maxDepth = 0;

	SizeType eraseCount = 0;
	SizeType addCount = 0;
	SizeType rootCount = 0;
	SizeType moveCount = 0;

	double cumulativeTravel = 0;
	SizeType cumulativeTravelCount = 0;
	SizeType cumulativeTravelDepth = 0;


	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);

	for (SizeType j = 0; j < RoundNumber; j++)
	{
		SizeType triesLeft = EraseNumber;
		while (--triesLeft > 2 && l.getCount() > 50)
		{
			SizeType child = Random::get(1, l.getCount());

			if (VerbosePrinting)
			{
				printf("Erase: %d\n", child);
			}

			if (FLAT_NO_CACHE_CONDITION)
			{
				ScopedProfiler p(&cumulativeErase); ++cumulativeEraseCount;
				l.erase(child);
			}
			else if (FLAT_CACHE_CONDITION)
			{
				LastDescendantCache descendantCache;
				descendantCache.makeCacheValid(l);

				ScopedProfiler p(&cumulativeErase); ++cumulativeEraseCount;
				erase(l, descendantCache, child);
			}
			else if (FLAT_CACHE_UNPREP_CONDITION)
			{
				LastDescendantCache descendantCache;

				ScopedProfiler p(&cumulativeErase); ++cumulativeEraseCount;
				erase(l, descendantCache, child);
			}
			else
			{
				FLAT_ASSERT(!"ASDF!!! No condition matched");
			}
			++eraseCount;

			if (CheckHashes && CurrentHash < HashBufferSize)
			{
				setHash(SuperFastHash((char*)l.depths.getPointer(), l.getCount() * sizeof(HierarchyType::DepthValue)));
			}
			if (CheckCounts && CurrentCounter < HashBufferSize)
			{
				setCount(l.getCount());
			}
		}

		triesLeft = AddNumber;
		while (--triesLeft > 2)
		{
			SizeType COUNT = l.getCount();

			char name[4] = { 'A' + (char)Random::get(0,26), 'a' + (char)Random::get(0,26), 'a' + (char)Random::get(0,26), 'a' + (char)Random::get(0,26) };
			Handle handle = getHandle(name);

			if (COUNT > 3 && Random::get(0, 16) < 3)
			{
				// travel to leaf

				SizeType travelLoopCount = 0;
				SizeType current = 0;
				SizeType childCount = 0;

				NextSiblingCache cache;
				if (FLAT_CACHE_CONDITION)
				{
					cache.makeCacheValid(l);
				}
				
				ScopedProfiler prof(&cumulativeTravel); ++cumulativeTravelCount;

				if (FLAT_NO_CACHE_CONDITION)
				{
					childCount = countDirectChildren(l, current);
				}
				else if (FLAT_CACHE_CONDITION)
				{
					childCount = countDirectChildren(l, cache, current);
				}
				else if (FLAT_CACHE_UNPREP_CONDITION)
				{
					childCount = countDirectChildren(l, cache, current);
				}
				else
				{
					FLAT_ASSERT(!"No condition matched");
				}


				// Loop until in leaf
				while (childCount > 0)
				{
					++travelLoopCount;
					if (FLAT_NO_CACHE_CONDITION)
					{
						current = getNthChild(l, current, Random::get(0, childCount));
						childCount = countDirectChildren(l, current);
					}
					else if (FLAT_CACHE_CONDITION)
					{
						current = getNthChild(l, cache, current, Random::get(0, childCount));
						childCount = countDirectChildren(l, cache, current);
					}
					else if (FLAT_CACHE_UNPREP_CONDITION)
					{
						current = getNthChild(l, cache, current, Random::get(0, childCount));
						childCount = countDirectChildren(l, cache, current);
					}
					else
					{
						FLAT_ASSERT(!"No condition matched");
					}
				}

				if (MaxTravelDepth < travelLoopCount)
					MaxTravelDepth = travelLoopCount;
				cumulativeTravelDepth += travelLoopCount;
				
				if (CheckHashes && CurrentHash < HashBufferSize)
				{
					setHash(SuperFastHash((char*)l.depths.getPointer(), l.getCount() * sizeof(HierarchyType::DepthValue)));
				}
				if (CheckCounts && CurrentCounter < HashBufferSize)
				{
					setCount(l.getCount());
				}
			}
			else if (COUNT > 3 && Random::get(0, 16) < 5)
			{
				SizeType parent = Random::get(1, COUNT - 2);
				SizeType child = Random::get(parent + 2, COUNT);
				FLAT_ASSERT(parent < child);

				if (VerbosePrinting)
				{
					PRINT::print(l);
					char buffer[5]; buffer[4] = '\0';
					char buffer2[5]; buffer2[4] = '\0';
					printf("Make #%d \"%s\" into #%d \"%s\"'s child\n", child, getName(l.values[child], buffer), parent, getName(l.values[parent], buffer2));
				}

				// Do moves
				{

					if (FLAT_NO_CACHE_CONDITION)
					{
						ScopedProfiler p(&cumulativeMove); ++cumulativeMoveCount;
						l.makeChildOf(child, parent);
					}
					else if (FLAT_CACHE_CONDITION)
					{
						LastDescendantCache descendantCache;
						descendantCache.makeCacheValid(l);

						ScopedProfiler p(&cumulativeMove); ++cumulativeMoveCount;
						makeChildOf(l, descendantCache, child, parent);
					}
					else if (FLAT_CACHE_UNPREP_CONDITION)
					{
						LastDescendantCache descendantCache;

						ScopedProfiler p(&cumulativeMove); ++cumulativeMoveCount;
						makeChildOf(l, descendantCache, child, parent);
					}
					else
					{
						FLAT_ASSERT(!"No condition matched");
					}

					if (CheckHashes && CurrentHash < HashBufferSize)
					{
						setHash(SuperFastHash((char*)l.depths.getPointer(), l.getCount() * sizeof(HierarchyType::DepthValue)));
					}
					if (CheckCounts && CurrentCounter < HashBufferSize)
					{
						setCount(l.getCount());
					}
				}

				if (VerbosePrinting)
				{
					PRINT::print(l);
				}
				++moveCount;
			}
			else
			{
				++addCount;
				
				SizeType parent = Random::get(0, COUNT);

				if (VerbosePrinting)
				{
					char buffer[5]; buffer[4] = '\0';
					char buffer2[5]; buffer2[4] = '\0';
					printf("Add %s to #%d \"%s\"\n", getName(handle, buffer), parent, getName(l.values[parent], buffer2));
				}

				if (FLAT_NO_CACHE_CONDITION)
				{
					ScopedProfiler p(&cumulativeAdd); ++cumulativeAddCount;
					l.createNodeAsChildOf(parent, handle);
				}
				else if (FLAT_CACHE_CONDITION)
				{
					LastDescendantCache descendantCache;
					descendantCache.makeCacheValid(l);

					ScopedProfiler p(&cumulativeAdd); ++cumulativeAddCount;
					createNodeAsChildOf(l, descendantCache, parent, handle);
				}
				else if (FLAT_CACHE_UNPREP_CONDITION)
				{
					LastDescendantCache descendantCache;

					ScopedProfiler p(&cumulativeAdd); ++cumulativeAddCount;
					createNodeAsChildOf(l, descendantCache, parent, handle);
				}
				else
				{
					FLAT_ASSERT(!"No condition matched.");
				}

				if (CheckHashes && CurrentHash < HashBufferSize)
				{
					setHash(SuperFastHash((char*)l.depths.getPointer(), l.getCount() * sizeof(HierarchyType::DepthValue)));
				}
				if (CheckCounts && CurrentCounter < HashBufferSize)
				{
					setCount(l.getCount());
				}
			}
			//else
			//{
			//	++rootCount;
			//
			//	if (FLAT_NO_CACHE_CONDITION)
			//	{
			//		ScopedProfiler p(&cumulativeAddRoot); ++cumulativeAddRootCount;
			//		l.createRootNode(handle);
			//	}
			//	else if (FLAT_CACHE_CONDITION)
			//	{
			//		LastDescendantCache descendantCache;
			//		descendantCache.makeCacheValid(l);
			//
			//		ScopedProfiler p(&cumulativeAddRoot); ++cumulativeAddRootCount;
			//		createRootNode(l, descendantCache, getHandle(name));
			//	}
			//	else if (FLAT_CACHE_UNPREP_CONDITION)
			//	{
			//		LastDescendantCache descendantCache;
			//
			//		ScopedProfiler p(&cumulativeAddRoot); ++cumulativeAddRootCount;
			//		createRootNode(l, descendantCache, getHandle(name));
			//	}
			//	else
			//	{
			//		FLAT_ASSERT(!"No condition matched.");
			//	}
			//}
		}

		SizeType count = 0;
		SizeType depth = 0;

		{
			ScopedProfiler prof(&calculateDepthTime); ++calculateDepthTimeCount;
			depth = l.findMaxDepth();
		}

		{
			ScopedProfiler prof(&calculateSizeTime); ++calculateSizeTimeCount;
			count = l.getCount();
		}

		cumulativeSize += count / double(RoundNumber);
		cumulativeDepth += depth / double(RoundNumber);

		maxCount = (maxCount < count ? count : maxCount);
		maxDepth = (maxDepth < depth ? depth : maxDepth);
	}

	LARGE_INTEGER li2;
	QueryPerformanceCounter(&li2);

	if (VerbosePrinting)
	{
		if (l.getCount() < 60)
			PRINT::print(l);
	}

	// Total time
	{
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);

		double PCFreq = double(freq.QuadPart) / 1.0;
		double diff = double(li2.QuadPart - li.QuadPart) / PCFreq;

		printf("%.4f s, adds: %d, erases: %d, moves: %d, rootAdds: %d\n", diff, addCount, eraseCount, moveCount, rootCount);
		//printf("Count: %u, cacheRows: %u, cacheColumns: %u, bufferCount: %u \n", l.getCount(), cache.cacheRows.getSize(), cache.DEBUG_getColumnCapacity(), cache.buffers.getSize());
		//for (size_t i = 0; i < cache.buffers.getSize(); i++)
		//{
		//	printf("buffers[%u].capacity = %u\n", i, cache.buffers[i].capacity);
		//}
		printf("average count: %.1lf, depth: %.1lf, max count: %d, depth: %d\n", cumulativeSize, cumulativeDepth, maxCount, maxDepth);
		printf("average travel depth: %.1lf\n", double(cumulativeTravelDepth) / double(cumulativeTravelCount));

		printf("\n\n\n");

		logTableDouble(cumulativeAdd / double(cumulativeAddCount));
		logTableDouble(cumulativeMove / double(cumulativeMoveCount));
		logTableDouble(cumulativeErase / double(cumulativeEraseCount));
		logTableDouble(calculateDepthTime / double(calculateDepthTimeCount));
		logTableDouble(calculateSizeTime / double(calculateSizeTimeCount));
		logTableDouble(0.0);
		logTableDouble(cumulativeTravel / double(cumulativeTravelCount));

		printf("Average Add                   -\t%d\t-  %.4lf us\n", cumulativeAddCount, cumulativeAdd / double(cumulativeAddCount));
		if(cumulativeAddRootCount) printf("Average AddRoot               -\t%d\t-  %.4lf us\n", cumulativeAddRootCount, cumulativeAddRoot / double(cumulativeAddRootCount));
		printf("Average Move                  -\t%d\t-  %.4lf us\n", cumulativeMoveCount, cumulativeMove / double(cumulativeMoveCount));
		printf("Average Erase                 -\t%d\t-  %.4lf us\n", cumulativeEraseCount, cumulativeErase / double(cumulativeEraseCount));
		printf("Average CalculateDepth        -\t%d\t-  %.4lf us\n", calculateDepthTimeCount, calculateDepthTime / double(calculateDepthTimeCount));
		printf("Average CalculateSize         -\t%d\t-  %.4lf us\n", calculateSizeTimeCount, calculateSizeTime / double(calculateSizeTimeCount));
		printf("Average Seek                  -\t%d\t-  %.4lf us\n", 0, 0.0);
		printf("Average Travel                -\t%d\t-  %.4lf us\n", cumulativeTravelCount, cumulativeTravel / double(cumulativeTravelCount));
		
		MaxCount = maxCount;
		MaxDepth = maxDepth;
		AverageCount = cumulativeSize;
		AverageDepth = cumulativeDepth;
		AverageTravelDepth = double(cumulativeTravelDepth) / double(cumulativeTravelCount);

		printf("\n");
	}


	/*
	while (false)
	{
		//cache.makeCacheValid(l);
		PRINT::print(l);

		int command;
		char buffer[256];
		printf("Enter command (1: createRoot, 2: createChild, 3: makeChildOf, 4: erase child, 5: isChildOf, 6: exit): ");

		fgets(buffer, 256, stdin);
		command = atoi(buffer);

		if (command == 1)
		{
			printf("Enter root name character: ");
			fgets(buffer, 256, stdin);
			Handle h = getHandle(buffer);

			l.createRootNode(h);
		}
		else if (command == 2)
		{
			printf("Enter child name character: ");
			fgets(buffer, 256, stdin);
			Handle h = getHandle(buffer);

			printf("Enter parent index: ");
			fgets(buffer, 256, stdin);
			SizeType parent = atoi(buffer);

			l.createNodeAsChildOf(parent, h);
		}
		else if (command == 3)
		{
			printf("Enter child index: ");
			fgets(buffer, 256, stdin);
			SizeType child = atoi(buffer);

			printf("Enter parent index: ");
			fgets(buffer, 256, stdin);
			SizeType parent = atoi(buffer);

			l.makeChildOf(child, parent);
		}
		else if (command == 4)
		{
			printf("Enter child index: ");
			fgets(buffer, 256, stdin);
			SizeType child = atoi(buffer);

			l.erase(child);
		}
		else if (command == 5)
		{
			printf("Enter child index: ");
			fgets(buffer, 256, stdin);
			SizeType child = atoi(buffer);

			printf("Enter parent index: ");
			fgets(buffer, 256, stdin);
			SizeType parent = atoi(buffer);

			//printf("\n%d %s child of %d\n", child, cache.isChildOf(child, parent, l.depths[parent]) ? "IS" : "is NOT", parent);

			for (SizeType col = 0; col < l.getCount(); col++)
			{
				printf("%d, ", l.depths[col]);
			}
			printf("\n\n");
			//for (SizeType rowNumber = 0, end = l.findMaxDepth(); rowNumber + 1 < end; rowNumber++)
			//{
			//	HierarchyCache::Row& row = cache.row(rowNumber);
			//	for (SizeType col = 0; col < l.getCount(); col++)
			//	{
			//		printf("%d, ", row.column(col));
			//	}
			//	printf("\n");
			//}
		}
		else if (command == 6)
		{
			break;
		}
		else
		{
			printf("Bad command: \"%s\". Try again.", buffer);
		}

	}
	*/
}


bool tryGetNthNode(const RivalTreeNodeBase* root, RivalTreeNodeBase*& result, SizeType n)
{
	struct TEMP
	{
		static bool recurse(const RivalTreeNodeBase* root, RivalTreeNodeBase*& result, SizeType& n)
		{
			if (n == 0)
			{
				result = const_cast<RivalTreeNodeBase*>(root);
				return true;
			}
			--n;

			for (SizeType i = 0; i < root->children.getSize(); i++)
			{
				if (recurse(root->children[i], result, n))
					return true;
			}
			return false;
		}
	};
	SizeType counter = n;
	bool success = TEMP::recurse(root, result, counter);
	FLAT_ASSERTF(success, "Failed to get node with N: %u", n);
	return success;
}

template<typename Tree>
void Other_Tree_Test_Impl()
{
	typedef Tree::Node Node;

	struct PRINT
	{
		static void print(const Tree& tree)
		{
			struct Temp
			{
				static void recurse(const RivalTreeNodeBase& node, SizeType& column, SizeType row = 0)
				{
					for (SizeType i = 1; i < row; i++)
					{
						printf("  ");
					}
					if (row > 0)
					{
						if (node.rightSibling) // Next sibling exists
						{
							printf("„¥");
						}
						else
						{
							printf("„¤");
						}
					}


					char buffer[5];
					buffer[4] = '\0';
					printf("%s\n", getName(((Node&)node).value, buffer));

					++column;

					for (SizeType i = 0; i < node.children.getSize(); i++)
					{
						recurse(*node.children[i], column, row + 1);
					}
				}

			};

			SizeType col = 0;
			Temp::recurse(*tree.root, col);
			printf("\n");
		}
	};

#ifdef MAX_PERF
	const bool enableSanityChecks = false;
#else
	const bool enableSanityChecks = false;
#endif

	Tree tree;
	tree.root = tree.createNode(getHandle("Root"), NULL);

	printf("Random seed: %u\n", Random::random);

	double cumulativeAdd = 0.0f;
	double cumulativeMove = 0.0f;
	double cumulativeErase = 0.0f;
	uint32_t cumulativeAddCount = 0;
	uint32_t cumulativeMoveCount = 0;
	uint32_t cumulativeEraseCount = 0;

	double calculateSizeTime = 0;
	double calculateDepthTime = 0;
	uint32_t calculateSizeTimeCount = 0;
	uint32_t calculateDepthTimeCount = 0;

	SizeType eraseCount = 0;
	SizeType addCount = 0;
	SizeType moveCount = 0;

	double cumulativeSize = 0;
	double cumulativeDepth = 0;
	SizeType maxCount = 0;
	SizeType maxDepth = 0;

	double cumulativeSeekTime = 0;
	uint32_t cumulativeSeekTimeCount = 0;

	double cumulativeTravel = 0;
	SizeType cumulativeTravelCount = 0;
	SizeType cumulativeTravelDepth = 0;


	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);

	for (SizeType j = 0; j < RoundNumber; j++)
	{
		SizeType COUNT = count(tree.root);

		SizeType triesLeft = EraseNumber;
		while (--triesLeft > 2 && COUNT > 50)
		{
			SizeType child = Random::get(1, COUNT);

			if (VerbosePrinting)
			{
				printf("Erase: %d\n", child);
			}

			Node* node = NULL;

			{
				ScopedProfiler prof(&cumulativeSeekTime); ++cumulativeSeekTimeCount;
				bool success = tryGetNthNode(tree.root, (RivalTreeNodeBase*&)node, child);
				FLAT_ASSERT(success);
			}
			FLAT_ASSERT(node != tree.root);


			{
				isChildOf(node, tree.root);

				ScopedProfiler p(&cumulativeErase); ++cumulativeEraseCount;
				tree.eraseNode(node);
			}

			COUNT = count(tree.root);
			++eraseCount;

			if (enableSanityChecks)
			{
				if (VerbosePrinting)
				{
					PRINT::print(tree);
				}
				tree.root->sanityCheck();
			}

			if (CheckCounts && CurrentCounter < HashBufferSize)
			{
				SizeType oldCount = COUNT;
				COUNT = count(tree.root);
				FLAT_ASSERT(oldCount == COUNT);

				setCount(COUNT);
			}
		}


		triesLeft = AddNumber;
		while (--triesLeft > 2)
		{
			char name[4] = { 'A' + (char)Random::get(0,26), 'a' + (char)Random::get(0,26), 'a' + (char)Random::get(0,26), 'a' + (char)Random::get(0,26) };
			Handle handle = getHandle(name);


			if (COUNT > 3 && Random::get(0, 16) < 3)
			{
				// travel to leaf

				ScopedProfiler prof(&cumulativeTravel); ++cumulativeTravelCount;

				Node* currentNode = tree.root;

				// Loop until in leaf
				while (currentNode->children.getSize() > 0)
				{
					++cumulativeTravelDepth;

					ScopedProfiler p(&cumulativeMove); ++cumulativeMoveCount;
					currentNode = (Node*)currentNode->children[Random::get(0, currentNode->children.getSize())];
				}

				if (CheckCounts && CurrentCounter < HashBufferSize)
				{
					SizeType oldCount = COUNT;
					COUNT = count(tree.root);
					FLAT_ASSERT(oldCount == COUNT);

					setCount(COUNT);
				}
			}
			else if (COUNT > 3 && Random::get(0, 16) < 5)
			{
				SizeType parent = Random::get(1, COUNT - 2);
				SizeType child = Random::get(parent + 2, COUNT);
				FLAT_ASSERT(parent < child);

				{
					Node* node = NULL;
					Node* parentNode = NULL;
					{
						ScopedProfiler prof(&cumulativeSeekTime); ++cumulativeSeekTimeCount;
						bool success = tryGetNthNode(tree.root, (RivalTreeNodeBase*&)node, child);
						FLAT_ASSERT(success);
					}
					{
						ScopedProfiler prof(&cumulativeSeekTime); ++cumulativeSeekTimeCount;
						bool success = tryGetNthNode(tree.root, (RivalTreeNodeBase*&)parentNode, parent);
						FLAT_ASSERT(success);
					}

					FLAT_ASSERT(child != 0 || isChildOf(parentNode, node));

					if (isChildOf(parentNode, node))
					{
						Node* temp = node;
						node = parentNode;
						parentNode = temp;
					}

					FLAT_ASSERT(!isChildOf(parentNode, node));

					if (VerbosePrinting)
					{
						PRINT::print(tree);
						char buffer[5]; buffer[4] = '\0';
						char buffer2[5]; buffer2[4] = '\0';
						printf("Make #%d \"%s\" into #%d \"%s\"'s child\n", child, getName(node->value, buffer), parent, getName(parentNode->value, buffer2));
					}

					{
						FLAT_ASSERT(node != tree.root);
						ScopedProfiler p(&cumulativeMove); ++cumulativeMoveCount;
						tree.makeChildOf(node, parentNode);
					}

					FLAT_ASSERT(!isChildOf(parentNode, node));


					if (VerbosePrinting)
					{
						PRINT::print(tree);
					}
					if (enableSanityChecks)
					{
						tree.root->sanityCheck();
					}

					if (CheckCounts && CurrentCounter < HashBufferSize)
					{
						SizeType oldCount = COUNT;
						COUNT = count(tree.root);
						FLAT_ASSERT(oldCount == COUNT);

						setCount(COUNT);
					}
				}

				++moveCount;
			}
			else
			{

				SizeType parent = Random::get(0, COUNT);

				Node* parentNode = NULL;
				{
					ScopedProfiler prof(&cumulativeSeekTime); ++cumulativeSeekTimeCount;
					bool success = tryGetNthNode(tree.root, (RivalTreeNodeBase*&)parentNode, parent);
					FLAT_ASSERT(success);
				}
				FLAT_ASSERT(parent == 0 || isChildOf(parentNode, tree.root));

				if (VerbosePrinting)
				{
					char buffer[5]; buffer[4] = '\0';
					char buffer2[5]; buffer2[4] = '\0';
					printf("Add %s to #%d \"%s\"\n", getName(handle, buffer), parent, getName(parentNode->value, buffer2));
				}

				Node* node = NULL;
				{
					ScopedProfiler p(&cumulativeAdd); ++cumulativeAddCount;
					node = tree.createNode(handle, parentNode);
				}
				FLAT_ASSERT(isChildOf(node, tree.root));

				++addCount;
				++COUNT;

				if (enableSanityChecks)
				{
					if (VerbosePrinting)
					{
						PRINT::print(tree);
					}
					tree.root->sanityCheck();
				}

				if (CheckCounts && CurrentCounter < HashBufferSize)
				{
					SizeType oldCount = COUNT;
					COUNT = count(tree.root);
					FLAT_ASSERT(oldCount == COUNT);

					setCount(COUNT);
				}
			}
		}

		SizeType currentCount = 0;
		SizeType currentDepth = 0;

		{
			ScopedProfiler prof(&calculateSizeTime); ++calculateSizeTimeCount;
			currentCount = count(tree.root);
		}

		{
			ScopedProfiler prof(&calculateDepthTime); ++calculateDepthTimeCount;
			currentDepth = depth(tree.root);
		}

		cumulativeSize += currentCount / double(RoundNumber);
		cumulativeDepth += currentDepth / double(RoundNumber);

		maxCount = (maxCount < currentCount ? currentCount : maxCount);
		maxDepth = (maxDepth < currentDepth ? currentDepth : maxDepth);
	}

	LARGE_INTEGER li2;
	QueryPerformanceCounter(&li2);

	if (VerbosePrinting)
	{
		PRINT::print(tree);
	}

	// Print performance results
	{
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);

		double PCFreq = double(freq.QuadPart) / 1.0;
		double diff = double(li2.QuadPart - li.QuadPart) / PCFreq;

		printf("%.4lf s, seek: %.3lf s, adds: %d, erases: %d, moves: %d \n", diff, cumulativeSeekTime / PCFreq, addCount, eraseCount, moveCount);
		printf("average count: %.1lf, depth: %.1lf, max count: %d, depth: %d\n", cumulativeSize, cumulativeDepth, maxCount, maxDepth);
		printf("average travel depth: %.1lf\n", double(cumulativeTravelDepth) / double(cumulativeTravelCount));

		printf("\n\n\n");

		logTableDouble(cumulativeAdd / double(cumulativeAddCount));
		logTableDouble(cumulativeMove / double(cumulativeMoveCount));
		logTableDouble(cumulativeErase / double(cumulativeEraseCount));
		logTableDouble(calculateDepthTime / double(calculateDepthTimeCount));
		logTableDouble(calculateSizeTime / double(calculateSizeTimeCount));
		logTableDouble(cumulativeSeekTime / double(cumulativeSeekTimeCount));
		logTableDouble(cumulativeTravel / double(cumulativeTravelCount));

		printf("Average Add                   -\t%d\t-  %.4lf us\n", cumulativeAddCount, cumulativeAdd / double(cumulativeAddCount));
		printf("Average Move                  -\t%d\t-  %.4lf us\n", cumulativeMoveCount, cumulativeMove / double(cumulativeMoveCount));
		printf("Average Erase                 -\t%d\t-  %.4lf us\n", cumulativeEraseCount, cumulativeErase / double(cumulativeEraseCount));
		printf("Average CalculateDepth        -\t%d\t-  %.4lf us\n", calculateDepthTimeCount, calculateDepthTime / double(calculateDepthTimeCount));
		printf("Average CalculateSize         -\t%d\t-  %.4lf us\n", calculateSizeTimeCount, calculateSizeTime / double(calculateSizeTimeCount));
		printf("Average Seek                  -\t%d\t-  %.4lf us\n", cumulativeSeekTimeCount, cumulativeSeekTime / double(cumulativeSeekTimeCount));
		printf("Average Travel                -\t%d\t-  %.4lf us\n", cumulativeTravelCount, cumulativeTravel / double(cumulativeTravelCount));

		MaxCount = maxCount;
		MaxDepth = maxDepth;
		AverageCount = cumulativeSize;
		AverageDepth = cumulativeDepth;
		AverageTravelDepth = double(cumulativeTravelDepth) / double(cumulativeTravelCount);

		printf("\n");
	}
}

void RIVAL_Test()
{
	Other_Tree_Test_Impl<RivalTree<Handle, HandleSorter> >();
}

void NAIVE_Test()
{
	Other_Tree_Test_Impl<NaiveTree<Handle, HandleSorter> >();
}




void test()
{
	memset(LogBuffer, ' ', sizeof(LogBuffer));

	uint32_t seed = 1771551 * (SizeType)time(NULL);
	seed = 1771551;

	// Table hearder
	{
		logTable("\t");
		logTable("Add\t");
		logTable("Move\t");
		logTable("Remove\t");
		logTable("Depth max\t");
		logTable("Count\t");
		logTable("Nth node\t");
		logTable("Leaf travel\t");
	}

	enum
	{
		Flat1 = 1 << 0,
		Flat2 = 1 << 1,
		Flat3 = 1 << 2,
		Rival = 1 << 3,
		Naive = 1 << 4,
		Every = 1 << 6
	};

	const uint32_t TestMask
		=
		Flat1 |
		//Flat2 |
		//Flat3 |
		Rival |
		Naive |
		Every |
		0;


	// Flat
	if ((TestMask & Flat1) != 0 || (TestMask & Every) != 0)
	{	
		Rundi = 0;
		logTable("\nFlat Tree\t");
		printf("\nFlat Tree\n");
		Random::init(seed);
		FLAT_Test();
		CheckingHashes = true;
		CurrentHash = 0;
		CurrentCounter = 0;
	}

	// Hot cache
	if ((TestMask & Flat2) != 0 || (TestMask & Every) != 0)
	{
		Rundi = 1;

		logTable("\nFlat Tree Cached\t");
		printf("\nFlat Tree Cached\n");
		Random::init(seed);
		FLAT_Test();
		CheckingHashes = true;
		CurrentHash = 0;
		CurrentCounter = 0;
	}

	// Cold cache
	if ((TestMask & Flat3) != 0 || (TestMask & Every) != 0)
	{
		Rundi = 2;

		logTable("\nFlat Tree With Cold Cache\t");
		printf("\nFlat Tree With Cold Cache\n");
		Random::init(seed);
		FLAT_Test();
		CheckingHashes = true;
		CurrentHash = 0;
		CurrentCounter = 0;
	}

	// Naive tree
	if ((TestMask & Naive) != 0 || (TestMask & Every) != 0)
	{
		logTable("\nNaive Tree\t");
		printf("\nNaive Tree\n");
		Random::init(seed);
		NAIVE_Test();
		CheckingHashes = true;
		CurrentHash = 0;
		CurrentCounter = 0;
	}

	// Rival tree
	if ((TestMask & Rival) != 0 || (TestMask & Every) != 0)
	{
		logTable("\nBuffered Tree\t");
		printf("\nBuffered Tree\n");
		Random::init(seed);
		RIVAL_Test();
		CheckingHashes = true;
		CurrentHash = 0;
		CurrentCounter = 0;
	}

	logTable("\n\nMax Count\t");
	logTableDouble(double(MaxCount));
	logTable("\nAverage Count\t");
	logTableDouble(double(AverageCount));
	logTable("\nMax Depth\t");
	logTableDouble(double(MaxDepth));
	logTable("\nAverage Depth\t");
	logTableDouble(double(AverageDepth));
	logTable("\nMax Travel Depth\t");
	logTableDouble(double(MaxTravelDepth));
	logTable("\nAverage Travel Depth\t");
	logTableDouble(double(AverageTravelDepth));

	logTableEnd();

	for (SizeType i = 0; i < sizeof(LogBuffer); i++)
	{
		if (LogBuffer[i] == '.')
		{
			LogBuffer[i] = ',';
		}
	}
	printf("%s\n", LogBuffer);

#ifdef MAX_PERF
	{
		FILE* f = NULL;
		errno_t err = fopen_s(&f, "output.csv", "w");
		FLAT_ASSERT(err >= 0);
		fprintf(f, "%s", LogBuffer);
		fclose(f);
	}
#endif

	system("pause");
}
