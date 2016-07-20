#pragma once

#define FLAT_ASSERTS_ENABLED true
#include "FlatHierarchy.h"
#include "HierarchyCache.h"
#include "RivalTree.h"

#include <stdio.h>      /* printf, fgets */
#include <stdlib.h>     /* atoi, rand, srand */
#include <inttypes.h>
#include <time.h>
#include <windows.h>
BOOL WINAPI QueryPerformanceCounter(_Out_ LARGE_INTEGER *lpPerformanceCount);

typedef uint32_t SizeType;

const SizeType RoundNumber = 10000;
const SizeType EraseNumber = 10;
const SizeType AddNumber = 100;

SizeType Rundi = 0;

#if false
#define FLAT_NO_CACHE_CONDITION     (Random::random % 3 == 0)
#define FLAT_CACHE_CONDITION        (Random::random % 3 == 1)
#define FLAT_CACHE_UNPREP_CONDITION (Random::random % 3 == 2)
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

namespace // Logger
{
	char LogBuffer[1024 * 16];
	SizeType LogBufferProgress = 0;
	void logTable(const char* msg)
	{
		SizeType l = strlen(msg);
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
		sprintf(buffer, "%.4lf\t", value);
		logTable(buffer);
	}
}

namespace
{
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
}

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
	static void init(uint32_t seed = InitSeed)
	{
		random = seed;
		srand(seed);
	}
	static uint32_t get()
	{
		int r = rand();
		random = *reinterpret_cast<uint32_t*>(&r);
		return random;
	}
};
uint32_t Random::random = 0;

void FLAT_Test()
{
	HierarchyType l(70);

	l.createRootNode(getHandle("Root"));
	l.createRootNode(getHandle("R22t"));

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

	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);

	for (SizeType j = 0; j < RoundNumber; j++)
	{
		SizeType triesLeft = EraseNumber;
		while (--triesLeft > 2 && l.getCount() > 50)
		{
			SizeType child = Random::get() % l.getCount();
			if (child == 0)
				continue;

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
		}

		triesLeft = AddNumber;
		while (l.getCount() < 128) // || --triesLeft > 2)
		{
			if (Random::get() % 16 < 12 && l.getCount() > 2)
			{
				SizeType child = Random::get() % l.getCount();
				SizeType parent = Random::get() % l.getCount();
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
						continue;
					}

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

				}
				++moveCount;
			}
			else
			{
				char name[4] = { 'A' + Random::get() % 26, 'a' + Random::get() % 26, 'a' + Random::get() % 26, 'a' + Random::get() % 26 };
				Handle handle = getHandle(name);

				SizeType parent = Random::get() % l.getCount();

				if (handle.text[0] == 'R' && handle.text[1] == 'o')
				{
					++rootCount;

					if (FLAT_NO_CACHE_CONDITION)
					{
						ScopedProfiler p(&cumulativeAddRoot); ++cumulativeAddRootCount;
						l.createRootNode(handle);
					}
					else if (FLAT_CACHE_CONDITION)
					{
						LastDescendantCache descendantCache;
						descendantCache.makeCacheValid(l);

						ScopedProfiler p(&cumulativeAddRoot); ++cumulativeAddRootCount;
						createRootNode(l, descendantCache, getHandle(name));
					}
					else if (FLAT_CACHE_UNPREP_CONDITION)
					{
						LastDescendantCache descendantCache;

						ScopedProfiler p(&cumulativeAddRoot); ++cumulativeAddRootCount;
						createRootNode(l, descendantCache, getHandle(name));
					}
					else
					{
						FLAT_ASSERT(!"No condition matched.");
					}
				}
				else
				{
					++addCount;

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
				}
			}
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

	if (l.getCount() < 400)
		printAll(l);

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

		printf("\n\n\n");
	}

	// Print performance results
	{
		logTableDouble(cumulativeAdd / double(cumulativeAddCount));
		logTableDouble(cumulativeMove / double(cumulativeMoveCount));
		logTableDouble(cumulativeErase / double(cumulativeEraseCount));
		logTableDouble(calculateDepthTime / double(calculateDepthTimeCount));
		logTableDouble(calculateSizeTime / double(calculateSizeTimeCount));

		printf("Average Add                   - %d\t-  %.4lf\tus\n", cumulativeAddCount, cumulativeAdd / double(cumulativeAddCount));
		printf("Average AddRoot               - %d\t-  %.4lf\tus\n", cumulativeAddRootCount, cumulativeAddRoot / double(cumulativeAddRootCount));
		printf("Average Move                  - %d\t-  %.4lf\tus\n", cumulativeMoveCount, cumulativeMove / double(cumulativeMoveCount));
		printf("Average Erase                 - %d\t-  %.4lf\tus\n", cumulativeEraseCount, cumulativeErase / double(cumulativeEraseCount));
		printf("Average CalculateDepth        - %d\t-  %.4lf\tus\n", calculateDepthTimeCount, calculateDepthTime / double(calculateDepthTimeCount));
		printf("Average CalculateSize         - %d\t-  %.4lf\tus\n", calculateSizeTimeCount, calculateSizeTime / double(calculateSizeTimeCount));

		printf("\n");
	}


	/*
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
	*/
}


typedef RivalTree<Handle, HandleSorter> Tree;
typedef Tree::Node Node;

void printAll(const Tree& tree)
{
	struct Temp
	{
		static void recurse(const Node& node, SizeType& column, SizeType row = 0)
		{
			for (SizeType i = 1; i < row; i++)
			{
				printf("  ");
			}
			if (row > 0)
			{
				if (node.rightSibling)
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
			printf("%s\n", getName(node.value, buffer));

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




void RIVAL_Test()
{
	typedef FLAT_VECTOR<Node*> NodeVector;
	NodeVector allNodes;
	allNodes.reserve(16000);

	Tree tree;
	tree.root = tree.createNode(getHandle("Root"), NULL);
	allNodes.pushBack(tree.root);

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



	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);

	for (SizeType j = 0; j < RoundNumber; j++)
	{
		SizeType triesLeft = EraseNumber;
		while (--triesLeft > 2 && allNodes.getSize() > 50)
		{
			SizeType child = Random::get() % allNodes.getSize();
			if (child == 0)
				continue;

			Node* node = allNodes[child];

			struct Temp
			{
				static void swapout(NodeVector& allNodes, SizeType index)
				{
					allNodes[index] = allNodes.getBack();
					allNodes.resize(allNodes.getSize() - 1);
				}

				static void recursiveErase(NodeVector& allNodes, Node* node)
				{
					for (SizeType i = 0; i < allNodes.getSize(); i++)
					{
						if (allNodes[i] == node)
						{
							swapout(allNodes, i);
							break;
						}
					}

					for (SizeType i = 0; i < node->children.getSize(); i++)
					{
						recursiveErase(allNodes, node->children[i]);
					}
				}
			};

			Temp::recursiveErase(allNodes, node);

			{
				ScopedProfiler p(&cumulativeErase); ++cumulativeEraseCount;
				tree.eraseNode(node);
			}

			++eraseCount;

			//printAll(tree);
			//tree.root->sanityCheck();
		}

		triesLeft = AddNumber;
		while (allNodes.getSize() < 400 || --triesLeft > 2)
		{

			if (Random::get() % 16 < 5 && allNodes.getSize() > 2)
			{
				SizeType child = Random::get() % allNodes.getSize();
				SizeType parent = Random::get() % allNodes.getSize();
				if (parent == child)
					continue;
				else
				{
					Node* node = allNodes[child];
					Node* parentNode = allNodes[parent];


					if (tree.isChildOf(parentNode, node))
					{
						Node* temp = node;
						node = parentNode;
						parentNode = temp;
					}

					if (node->parent == parentNode)
						continue;

					{
						ScopedProfiler p(&cumulativeMove); ++cumulativeMoveCount;
						tree.makeChildOf(node, parentNode);
					}

					//printAll(tree);
					//tree.root->sanityCheck();
				}
				++moveCount;
			}
			else
			{
				++addCount;
				char name[4] = { 'A' + Random::get() % 26, 'a' + Random::get() % 26, 'a' + Random::get() % 26, 'a' + Random::get() % 26 };
				Handle handle = getHandle(name);

				SizeType parent = Random::get() % allNodes.getSize();

				Node* node = NULL;
				{
					ScopedProfiler p(&cumulativeAdd); ++cumulativeAddCount;
					node = tree.createNode(handle, allNodes[parent]);
				}

				allNodes.pushBack(node);

				//printAll(tree);
				//tree.root->sanityCheck();
			}
		}

		struct Temp
		{
			static SizeType count(const Node* node)
			{
				SizeType result = 1;
				for (SizeType i = 0; i < node->children.getSize(); i++)
				{
					if (node->value.number != 0)
						result += count(node->children[i]);
				}
				return result;
			}

			static SizeType depth(const Node* node)
			{
				SizeType maxDepth = 0;
				for (SizeType i = 0; i < node->children.getSize(); i++)
				{
					SizeType d = depth(node->children[i]);
					if (d > maxDepth)
						maxDepth = d;
				}
				return maxDepth + 1;
			}
		};

		SizeType count = 0;
		SizeType depth = 0;

		{
			ScopedProfiler prof(&calculateSizeTime); ++calculateSizeTimeCount;
			count = Temp::count(tree.root);
		}

		{
			ScopedProfiler prof(&calculateDepthTime); ++calculateDepthTimeCount;
			depth = Temp::depth(tree.root);
		}

		cumulativeSize += count / double(RoundNumber);
		cumulativeDepth += depth / double(RoundNumber);

		maxCount = (maxCount < count ? count : maxCount);
		maxDepth = (maxDepth < depth ? depth : maxDepth);
	}

	LARGE_INTEGER li2;
	QueryPerformanceCounter(&li2);

	//printAll(tree);

	// Print performance results
	{
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);

		double PCFreq = double(freq.QuadPart) / 1.0;
		double diff = double(li2.QuadPart - li.QuadPart) / PCFreq;

		printf("%.4f s, adds: %d, erases: %d, moves: %d \n", diff, addCount, eraseCount, moveCount);
		printf("average count: %.1lf, depth: %.1lf, max count: %d, depth: %d\n", cumulativeSize, cumulativeDepth, maxCount, maxDepth);

		printf("\n\n\n");

		logTableDouble(cumulativeAdd / double(cumulativeAddCount));
		logTableDouble(cumulativeMove / double(cumulativeMoveCount));
		logTableDouble(cumulativeErase / double(cumulativeEraseCount));
		logTableDouble(calculateDepthTime / double(calculateDepthTimeCount));
		logTableDouble(calculateSizeTime / double(calculateSizeTimeCount));

		printf("Average Add                   - %d\t-  %.4lf\tus\n", cumulativeAddCount, cumulativeAdd / double(cumulativeAddCount));
		printf("Average Move                  - %d\t-  %.4lf\tus\n", cumulativeMoveCount, cumulativeMove / double(cumulativeMoveCount));
		printf("Average Erase                 - %d\t-  %.4lf\tus\n", cumulativeEraseCount, cumulativeErase / double(cumulativeEraseCount));

		printf("Average CalculateDepth        - %d\t-  %.4lf\tus\n", calculateDepthTimeCount, calculateDepthTime / double(calculateDepthTimeCount));
		printf("Average CalculateSize         - %d\t-  %.4lf\tus\n", calculateSizeTimeCount, calculateSizeTime / double(calculateSizeTimeCount));

		printf("\n");

	}
}





void test()
{
	memset(LogBuffer, ' ', sizeof(LogBuffer));

	uint32_t seed = 1771551 * time(NULL);
	seed = 1771551;

	// Table hearder
	{
		logTable("\t");
		logTable("Add\t");
		logTable("Move\t");
		logTable("Remove\t");
		logTable("Depth\t");
		logTable("Count\t");
	}

	// Rival tree
	if(false)
	{
		logTable("\nRival\t");
		Random::init(seed);
		RIVAL_Test();
	}

	// Flat
	if(true)
	{
		Rundi = 0;
		logTable("\nFlat\t");
		Random::init(seed);
		FLAT_Test();
	}

	// Hot cache
	if(false)
	{
		Rundi = 1;

		logTable("\nFlatC\t");
		Random::init(seed);
		FLAT_Test();
	}

	// Cold cache
	if(false)
	{
		Rundi = 2;

		logTable("\nFlatU\t");
		Random::init(seed);
		FLAT_Test();
	}

	logTableEnd();

	for (SizeType i = 0; i < sizeof(LogBuffer); i++)
	{
		if (LogBuffer[i] == '.')
		{
			LogBuffer[i] = ',';
		}
	}
	printf("%s\n", LogBuffer);

	FILE* f = fopen("output.csv", "w");
	fprintf(f, "%s", LogBuffer);
	fclose(f);

	system("pause");
}
