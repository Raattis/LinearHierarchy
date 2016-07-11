#pragma once

#include <stdio.h>      /* printf, fgets */
#include <stdlib.h>     /* atoi */

#define FLAT_ASSERTS_ENABLED true
#include "FlatHierarchy.h"

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
	if (!l.getIsCacheValid())
	{
		printf(".");
		return;
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
			SizeType parentDepth = c == 0 ? 0 : l.depths[l.getParentIndex(r, c)];
			bool hasDirectChildren = false;

			for (SizeType i = c + 1; i < l.getCount(); i++)
			{
				if (parentDepth + 1 == l.depths[i])
				{
					if (l.getParentIndex(r, i) == l.getParentIndex(r, c))
						hasDirectChildren = true;
					break;
				}
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

void FLAT_test()
{
	HierarchyType l(15, 70);
	l.createRootNode(getHandle("Root"));
	l.createRootNode(getHandle("R22t"));
	uint32_t random = 7151 * time(NULL);
	//random = 17711551;
	printf("Random seed: %u\n", random);

	l.makeCacheValid();
	l.keepCacheValid = false;

	struct Random
	{
		static uint32_t get(uint32_t& random)
		{
			return ++random * 71551 + (171377117 * ++random) >> 16;
		}
	};

	SizeType eraseCount = 0;
	SizeType addCount = 0;
	SizeType rootCount = 0;
	SizeType moveCount = 0;

	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);

	for (SizeType j = 0; j < 100; j++)
	{
		SizeType triesLeft = 30;
		while (--triesLeft > 2)
		{
			SizeType child = Random::get(random) % l.getCount();
			if (child == 0)
				continue;
			l.erase(child);
			++eraseCount;
		}

		triesLeft = 1000;
		while(--triesLeft > 2)
		{
			char name[4] = { 'A' + Random::get(random) % 28, 'a' + Random::get(random) % 28, 'a' + Random::get(random) % 28, 'a' + Random::get(random) % 28 };
			Handle h = getHandle(name);
			SizeType child = Random::get(random) % l.getCount();

			if (h.text[0] == 'R')
			{
				l.createRootNode(getHandle(name));
				++rootCount;
			}
			else if (h.text[0] >= 'P')
			{
				SizeType parent = h.number % l.getCount();
				if (parent == child)
					continue;
				else if (parent < child)
				{
					l.makeChildOf(child, parent);
				}
				else
				{
					l.makeChildOf(parent, child);
				}
				++moveCount;
			}
			else
			{
				l.createNodeAsChildOf(Random::get(random) % l.getCount(), getHandle(name));
				++addCount;
			}
		}
	}

	LARGE_INTEGER li2;
	QueryPerformanceCounter(&li2);

	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);

	double PCFreq = double(freq.QuadPart) / 1.0;
	double diff = double(li2.QuadPart - li.QuadPart) / PCFreq;

	//printAll(l);

	printf("%.4f s, adds: %d, erases: %d, moves: %d, rootAdds: %d\n", diff, addCount, eraseCount, moveCount, rootCount);
	printf("Count: %u, cacheRows: %u, cacheColumns: %u, bufferCount: %u \n", l.getCount(), l.hierarchyCache.cacheRows.getSize(), l.hierarchyCache.DEBUG_getColumnCapacity(), l.hierarchyCache.buffers.getSize());
	for (size_t i = 0; i < l.hierarchyCache.buffers.getSize(); i++)
	{
		printf("buffers[%u].capacity = %u\n", i, l.hierarchyCache.buffers[i].capacity);
	}
	
	system("pause");









	while (false)
	{
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

			printf("\n%d %s child of %d\n", child, l.isChildOf(child, parent) ? "IS" : "is NOT", parent);

			for (SizeType col = 0; col < l.getCount(); col++)
			{
				printf("%d, ", l.depths[col]);
			}
			printf("\n\n");
			for (SizeType rowNumber = 0, end = l.findMaxDepth(); rowNumber + 1 < end; rowNumber++)
			{
				HierarchyCacheRow& row = l.hierarchyCache.row(rowNumber);
				for (SizeType col = 0; col < l.getCount(); col++)
				{
					printf("%d, ", row.column(col));
				}
				printf("\n");
			}
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