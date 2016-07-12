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

void FLAT_test()
{
	HierarchyType l(70);
	HierarchyCache cache;
	
	//l.createRootNode(getHandle("Root"));
	//l.createNodeAsChildOf(0, getHandle(" C1 "));
	//l.createNodeAsChildOf(1, getHandle("C11 "));
	//l.createNodeAsChildOf(2, getHandle("C111"));
	//l.createNodeAsChildOf(1, getHandle("C12 "));
	//l.createNodeAsChildOf(1, getHandle("C13 "));
	//SizeType a = l.createNodeAsChildOf(0, getHandle(" C2 "));
	//l.createNodeAsChildOf(a, getHandle("C21 "));
	//l.createNodeAsChildOf(a, getHandle("C22 "));
	//a = l.createNodeAsChildOf(a, getHandle("C23 "));
	//l.createNodeAsChildOf(a, getHandle("C231"));
	//
	//
	//cache.makeCacheValid(l);
	//printAll(l);
	//
	//l.makeChildOf(1, a);
	//
	//cache.makeCacheValid(l);
	//printAll(l);
	//system("pause");
	//
	//return;

	l.createRootNode(getHandle("Root"));
	l.createRootNode(getHandle("R22t"));
	uint32_t random = 7151 * time(NULL);
	//random = 1771551;
	printf("Random seed: %u\n", random);

	struct Random
	{
		static uint32_t get(uint32_t& random)
		{
			return ++random * 71551 + (171377117 * ++random) >> 16;
		}
	};


	double averageAdd = 0.0f;
	double averageMove = 0.0f;
	double averageErase = 0.0f;

	double averageSize = 0;
	double averageDepth = 0;
	SizeType maxCount = 0;
	SizeType maxDepth = 0;

	SizeType eraseCount = 0;
	SizeType addCount = 0;
	SizeType rootCount = 0;
	SizeType moveCount = 0;

	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	const SizeType RoundNumber = 300;
	for (SizeType j = 0; j < RoundNumber; j++)
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
		averageSize += l.getCount() / (2.0 * RoundNumber);
		averageDepth += l.findMaxDepth() / (2.0 * RoundNumber);

		triesLeft = 100;
		while(--triesLeft > 2)
		{
			char name[4] = { 'A' + Random::get(random) % 28, 'a' + Random::get(random) % 28, 'a' + Random::get(random) % 28, 'a' + Random::get(random) % 28 };
			Handle h = getHandle(name);
			SizeType child = Random::get(random) % l.getCount();

			if (h.text[0] == 'R' && h.text[1] == 'o')
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
		averageSize += l.getCount() / (2.0 * RoundNumber);
		averageDepth += l.findMaxDepth() / (2.0 * RoundNumber);

		maxCount = (maxCount < l.getCount() ? l.getCount() : maxCount);
		maxDepth = (maxDepth < l.findMaxDepth() ? l.findMaxDepth() : maxDepth);

		{
			LARGE_INTEGER tempLi;
			LARGE_INTEGER tempLi2;
			LARGE_INTEGER freq;
			QueryPerformanceFrequency(&freq);
			double div = double(freq.QuadPart) / 1000.0 * double(RoundNumber);

			QueryPerformanceCounter(&tempLi);
			SizeType a = l.createNodeAsChildOf(0, getHandle("0000"));
			QueryPerformanceCounter(&tempLi2);
			averageAdd += double(tempLi2.QuadPart - tempLi.QuadPart) / div;

			SizeType b = l.createNodeAsChildOf(a, getHandle("0001"));
			QueryPerformanceCounter(&tempLi);
			l.makeChildOf(b, 0);
			QueryPerformanceCounter(&tempLi2);
			averageMove += double(tempLi2.QuadPart - tempLi.QuadPart) / div;

			QueryPerformanceCounter(&tempLi);
			l.erase(l.getCount() / 2);
			QueryPerformanceCounter(&tempLi2);
			averageErase += double(tempLi2.QuadPart - tempLi.QuadPart) / div;
		}

		//printAll(l);
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
		printf("Count: %u, cacheRows: %u, cacheColumns: %u, bufferCount: %u \n", l.getCount(), cache.cacheRows.getSize(), cache.DEBUG_getColumnCapacity(), cache.buffers.getSize());
		for (size_t i = 0; i < cache.buffers.getSize(); i++)
		{
			printf("buffers[%u].capacity = %u\n", i, cache.buffers[i].capacity);
		}
		printf("Average count: %.1lf, depth: %.1lf, max count: %d, depth: %d\n", averageSize, averageDepth, maxCount, maxDepth);
	
		printf("\n\n\n");

		PCFreq /= 1000.0;


		printf("Average add: %.4lf ms, move: %.4lf ms, erase: %.4lf ms\n\n", averageAdd, averageMove, averageErase);

		QueryPerformanceCounter(&li);
		SizeType a = l.createNodeAsChildOf(0, getHandle("0000"));
		QueryPerformanceCounter(&li2);
		printf("Singel add took : %.5f ms\n", double(li2.QuadPart - li.QuadPart) / PCFreq);

		QueryPerformanceCounter(&li);
		l.makeChildOf(a + 1, 0);
		QueryPerformanceCounter(&li2);
		printf("Singel move took : %.5f ms\n", double(li2.QuadPart - li.QuadPart) / PCFreq);

		QueryPerformanceCounter(&li);
		l.erase(0);
		QueryPerformanceCounter(&li2);
		printf("Singel erase took : %.5f ms\n", double(li2.QuadPart - li.QuadPart) / PCFreq);

		system("pause");

	}








	while (false)
	{
		cache.makeCacheValid(l);
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

			printf("\n%d %s child of %d\n", child, cache.isChildOf(child, parent, l.depths[parent]) ? "IS" : "is NOT", parent);

			for (SizeType col = 0; col < l.getCount(); col++)
			{
				printf("%d, ", l.depths[col]);
			}
			printf("\n\n");
			for (SizeType rowNumber = 0, end = l.findMaxDepth(); rowNumber + 1 < end; rowNumber++)
			{
				HierarchyCache::Row& row = cache.row(rowNumber);
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