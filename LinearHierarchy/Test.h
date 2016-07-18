#pragma once

#include <stdio.h>      /* printf, fgets */
#include <stdlib.h>     /* atoi, rand, srand */

#define FLAT_ASSERTS_ENABLED true
#include "FlatHierarchy.h"
#include "HierarchyCache.h"

struct Handle
{
	union {
		char text[4];
		uint32_t number;
	};
};
static_assert(sizeof(Handle) == 4, "Size of handle must be 4");

struct HandleSorter
{
	static const bool UseSorting = true;

	inline static bool isFirst(const Handle& a, const Handle& b) { return a.number < b.number; }
};

typedef FlatHierarchy<Handle, HandleSorter> HierarchyType;

Handle getHandle(const char* ptr)
{
	return *reinterpret_cast<const Handle*>(ptr);
}
const char* getName(Handle handle, char* buffer)
{
	memcpy(buffer, &handle, 4);
	return buffer;
}

typedef size_t SizeType;

void printAll(const HierarchyType& l)
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
		if(l.depths[c] < 10)
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

#include <time.h>
#include <windows.h>
BOOL WINAPI QueryPerformanceCounter(_Out_ LARGE_INTEGER *lpPerformanceCount);

class ScopedProfiler
{
public:
	double* cumulator;
	double* cumulative;
	uint32_t* counter;
	LARGE_INTEGER start;

	ScopedProfiler(double* cumulator = NULL)
		: cumulator(cumulator)
		, cumulative(NULL)
		, counter(NULL)
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
		if(cumulator != NULL)
			(*cumulator) += diff;
		if(cumulative != NULL)
		{
			assert(counter);
			(*cumulative) = ((*cumulative) * (*counter) + diff) / ((*counter) + 1);
		}
		
		return diff;
	}

	~ScopedProfiler()
	{
		stop();
	}

};


void FLAT_test()
{
	HierarchyType l(70);
	
	l.createRootNode(getHandle("Root"));
	l.createRootNode(getHandle("R22t"));
	uint32_t random = 7151 * time(NULL);
	//random = 1771551;
	srand(random);

	printf("Random seed: %u\n", random);

	struct Random
	{
		static uint32_t get(uint32_t& random)
		{
			int r = rand();
			random = *reinterpret_cast<uint32_t*>(&r);
			return random;
		}
	};


	double cumulativeAdd = 0.0f;
	double cumulativeAddRoot = 0.0f;
	double cumulativeMove = 0.0f;
	double cumulativeCachedMove = 0.0f;
	double cumulativeCachedMoveNoPreCache = 0.0f;
	double cumulativeErase = 0.0f;
	double cumulativeCachedErase = 0.0f;
	double cumulativeCachedEraseNoPreCache = 0.0f;
	uint32_t cumulativeAddCount = 0;
	uint32_t cumulativeAddRootCount = 0;
	uint32_t cumulativeMoveCount = 0;
	uint32_t cumulativeCachedMoveCount = 0;
	uint32_t cumulativeCachedMoveNoPreCacheCount = 0;
	uint32_t cumulativeEraseCount = 0;
	uint32_t cumulativeCachedEraseCount = 0;
	uint32_t cumulativeCachedEraseNoPreCacheCount = 0;
	

	double cumulativeSize = 0;
	double cumulativeDepth = 0;
	SizeType maxCount = 0;
	SizeType maxDepth = 0;

	SizeType eraseCount = 0;
	SizeType addCount = 0;
	SizeType rootCount = 0;
	SizeType moveCount = 0;

	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	const SizeType RoundNumber = 1000;
	for (SizeType j = 0; j < RoundNumber; j++)
	{
		SizeType triesLeft = 30;
		while (--triesLeft > 2)
		{
			SizeType child = Random::get(random) % l.getCount();
			if (child == 0)
				continue;

			if (random % 3 == 0)
			{
				ScopedProfiler p(&cumulativeErase); ++cumulativeEraseCount;
				l.erase(child);
			}
			else if (random % 3 == 1)
			{
				LastDescendantCache descendantCache;
				descendantCache.makeCacheValid(l);

				ScopedProfiler p(&cumulativeCachedErase); ++cumulativeCachedEraseCount;
				erase(l, descendantCache, l.getCount() / 2);
			}
			else
			{
				LastDescendantCache descendantCache;

				ScopedProfiler p(&cumulativeCachedEraseNoPreCache); ++cumulativeCachedEraseNoPreCacheCount;
				erase(l, descendantCache, l.getCount() / 2);
			}
			++eraseCount;
		}
		cumulativeSize += l.getCount() / (2.0 * RoundNumber);
		cumulativeDepth += l.findMaxDepth() / (2.0 * RoundNumber);

		triesLeft = 300;
		while(--triesLeft > 2)
		{
			char name[4] = { 'A' + Random::get(random) % 28, 'a' + Random::get(random) % 28, 'a' + Random::get(random) % 28, 'a' + Random::get(random) % 28 };
			Handle handle = getHandle(name);
			SizeType child = Random::get(random) % l.getCount();

			if (handle.text[0] == 'R' && handle.text[1] == 'o')
			{
				++rootCount;

				ScopedProfiler p(&cumulativeAddRoot); ++cumulativeAddRootCount;
				l.createRootNode(handle);
			}
			else if (handle.text[0] >= 'P')
			{
				SizeType parent = handle.number % l.getCount();
				if (parent == child)
					continue;
				else
				{
					if (parent > child)
					{
						SizeType temp = parent;
						parent = child;
						child = temp;
					}

					if (l.linearIsChildOf(child, parent))
					{
						SizeType newParent;
						{
							++rootCount;

							ScopedProfiler p(&cumulativeAddRoot); ++cumulativeAddRootCount;
							newParent = l.createRootNode(handle);
						}

						if (newParent <= child)
							child += 1;
						parent = newParent;
					}

					if (random % 3 == 0)
					{
						LastDescendantCache descendantCache;
						descendantCache.makeCacheValid(l);

						ScopedProfiler p(&cumulativeCachedMove); ++cumulativeCachedMoveCount;
						makeChildOf(l, descendantCache, child, parent);
					}
					else if (random % 3 == 1)
					{
						LastDescendantCache descendantCache;

						ScopedProfiler p(&cumulativeCachedMoveNoPreCache); ++cumulativeCachedMoveNoPreCacheCount;
						makeChildOf(l, descendantCache, child, parent);
					}
					else
					{
						ScopedProfiler p(&cumulativeMove); ++cumulativeMoveCount;
						l.makeChildOf(child, parent);
					}

				}
				++moveCount;
			}
			else
			{
				++addCount;
				SizeType parent = Random::get(random) % l.getCount();

				ScopedProfiler p(&cumulativeAdd); ++cumulativeAddCount;
				l.createNodeAsChildOf(parent, getHandle(name));
			}
		}
		cumulativeSize += l.getCount() / (2.0 * RoundNumber);
		cumulativeDepth += l.findMaxDepth() / (2.0 * RoundNumber);

		maxCount = (maxCount < l.getCount() ? l.getCount() : maxCount);
		maxDepth = (maxDepth < l.findMaxDepth() ? l.findMaxDepth() : maxDepth);
	}

	LARGE_INTEGER li2;
	QueryPerformanceCounter(&li2);

	if (l.getCount() < 400)
		printAll(l);

	// Print performance results
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
	
		printf("\n\n\n");



		printf("Average Add                   - %d\t-  %.4lf\tus\n", cumulativeAddCount, cumulativeAdd / double(cumulativeAddCount));
		printf("Average AddRoot               - %d\t-  %.4lf\tus\n", cumulativeAddRootCount, cumulativeAddRoot / double(cumulativeAddRootCount));
		printf("Average Move                  - %d\t-  %.4lf\tus\n", cumulativeMoveCount, cumulativeMove / double(cumulativeMoveCount));
		printf("Average CachedMove            - %d\t-  %.4lf\tus\n", cumulativeCachedMoveCount, cumulativeCachedMove / double(cumulativeCachedMoveCount));
		printf("Average CachedMoveNoPreCache  - %d\t-  %.4lf\tus\n", cumulativeCachedMoveNoPreCacheCount, cumulativeCachedMoveNoPreCache / double(cumulativeCachedMoveNoPreCacheCount));
		printf("Average Erase                 - %d\t-  %.4lf\tus\n", cumulativeEraseCount, cumulativeErase / double(cumulativeEraseCount));
		printf("Average CachedErase           - %d\t-  %.4lf\tus\n", cumulativeCachedEraseCount, cumulativeCachedErase / double(cumulativeCachedEraseCount));
		printf("Average CachedEraseNoPreCache - %d\t-  %.4lf\tus\n", cumulativeCachedEraseNoPreCacheCount, cumulativeCachedEraseNoPreCache / double(cumulativeCachedEraseNoPreCacheCount));
		printf("\n");


		PCFreq /= 1000000.0;

		LastDescendantCache descendantCache;

		QueryPerformanceCounter(&li);
		SizeType a = l.createNodeAsChildOf(0, getHandle("0000"));
		QueryPerformanceCounter(&li2);
		printf("Single add took : %.5f us\n", double(li2.QuadPart - li.QuadPart) / PCFreq);

		SizeType b = l.createNodeAsChildOf(a, getHandle("0001"));

		QueryPerformanceCounter(&li);
		l.makeChildOf(b, 0);
		QueryPerformanceCounter(&li2);
		printf("Single move took : %.5f us\n", double(li2.QuadPart - li.QuadPart) / PCFreq);

		SizeType b1 = l.createNodeAsChildOf(a, getHandle("0002"));
		
		descendantCache.makeCacheValid(l);

		QueryPerformanceCounter(&li);
		makeChildOf(l, descendantCache, b1, 0);
		QueryPerformanceCounter(&li2);
		printf("Single cached move took : %.5f us\n", double(li2.QuadPart - li.QuadPart) / PCFreq);

		SizeType b2 = l.createNodeAsChildOf(a, getHandle("0003"));

		QueryPerformanceCounter(&li);
		makeChildOf(l, descendantCache, b2, 0);
		QueryPerformanceCounter(&li2);
		printf("Non-precached cached move took : %.5f us\n", double(li2.QuadPart - li.QuadPart) / PCFreq);



		//QueryPerformanceCounter(&li);
		//l.erase(0);
		//QueryPerformanceCounter(&li2);
		//printf("Single erase took : %.5f us\n", double(li2.QuadPart - li.QuadPart) / PCFreq);

		descendantCache.makeCacheValid(l);

		QueryPerformanceCounter(&li);
		erase(l, descendantCache, 0);
		QueryPerformanceCounter(&li2);
		printf("Single cached erase took : %.5f us\n", double(li2.QuadPart - li.QuadPart) / PCFreq);

		system("pause");

	}








	while (false)
	{
		//cache.makeCacheValid(l);
		printAll(l);

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
}