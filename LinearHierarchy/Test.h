#pragma once

#ifdef MAX_PERF
#define FLAT_ASSERTS_ENABLED false
#define FLAT_ALLOW_INCLUDES true
#else
#define FLAT_ASSERTS_ENABLED true
#define FLAT_ALLOW_INCLUDES true
#endif


#include "FastHash.h"
#include "FlatHierarchy.h"
#include "HierarchyCache.h"
#include "RivalTree.h"
#include "MultiwayTree.h"

#include <stdio.h>      /* printf */
#include <windows.h>

typedef uint32_t SizeType;

#ifdef _DEBUG
const SizeType ArrTestSizesCount = 3;
const SizeType ArrTestRoundNumbers[] = { 50, 25, 10 };
const SizeType ArrTestSizes[]        = { 10, 25, 50 };

#elif true //&& false
const SizeType ArrTestSizesCount = 11;
const SizeType ArrTestRoundNumbers[] = { 1000,  500,  500,    500,   100,  100,     30,   10,    5,     3,     3 };
const SizeType ArrTestSizes[]        = {   10,   25,   50,    100,   250,  500,   1000, 2500, 5000, 10000, 25000 };

#else
const SizeType ArrTestSizesCount = 6;
const SizeType ArrTestRoundNumbers[] = { 50, 10,  2,   2,   2,   2,    2,    2,    2,     2,     2 };
const SizeType ArrTestSizes[]        = { 10, 25, 50, 100, 250, 500, 1000, 2500, 5000, 10000, 25000 };

#endif


#ifdef MAX_PERF
const bool DoCacheFlushing = true;
const bool VerbosePrinting = false;
const bool CheckHashes = false;
#else
const bool DoCacheFlushing = false;
const bool VerbosePrinting = false;
const bool CheckHashes = true;
#endif

const SizeType TreeCount = 7;

enum
{
	Flat1 = 1 << 0,
	Flat2 = 1 << 1,
	Flat3 = 1 << 2,
	Rival = 1 << 3,
	Naive = 1 << 4,
	Nulti = 1 << 5,
	Multi = 1 << 6,
	Every = 1 << 7
	};

const uint32_t TestMask
	=
	//Flat1 |
	//Flat2 |
	//Flat3 |
	//Rival |
	Naive |
	Nulti |
	//Multi |
	Every |
#ifdef MAX_PERF
	Every |
#endif
	0;

#define FLAT_NO_CACHE_CONDITION     (CurrentTreeType == 0)
#define FLAT_CACHE_CONDITION        (CurrentTreeType == 1)
#define FLAT_CACHE_UNPREP_CONDITION (CurrentTreeType == 2)

struct Vector2
{
	float x, y;

	Vector2()
		: x(0), y(0)
	{
	}

	Vector2(float x, float y)
		: x(x), y(y)
	{
	}
	Vector2(const Vector2& other)
		: Vector2(other.x, other.y)
	{
	}

	inline Vector2& operator+=(const Vector2& other)
	{
		x += other.x;
		y += other.y;
		return *this;
	}
	inline Vector2& operator-=(const Vector2& other)
	{
		x -= other.x;
		y -= other.y;
		return *this;
	}

	friend Vector2 operator+(const Vector2& a, const Vector2& b)
	{
		Vector2 result(a.x, a.y);
		result += b;
		return result;
	}
	friend Vector2 operator-(const Vector2& a, const Vector2& b)
	{
		Vector2 result(a.x, a.y);
		result -= b;
		return result;
	}
	friend Vector2 operator*(const Vector2& a, const Vector2& b)
	{
		return Vector2(a.x * b.x, a.y * b.y);
	}


	String toString() const
	{
		FLAT_ASSERT(this);
		String result("VC2(");
		result += x;
		result += ", ";
		result += y;
		result += ")";
		return result;
	}


};

struct Transform
{
#ifndef MAX_PERF
	union
	{
		char name[4];
		uint32_t nameInt;
	};
#endif
	Vector2 pos;
	Vector2 size;

	Transform()
		: pos()
		, size(1, 1)
	{
	}
	Transform(const Transform& other)
		: pos(other.pos)
		, size(other.size)
#ifndef MAX_PERF
		, nameInt(other.nameInt)
#endif
	{
	}
	Transform(const Vector2& pos, const Vector2& size)
		: pos(pos)
		, size(size)
	{
	}
	Transform(float x, float y, float width, float height)
		: pos(x, y)
		, size(width, height)
	{
	}

	Transform& multiply(const Transform& a)
	{
		return Transform(a.pos + a.size * pos, a.size * size);
	}
	static Transform multiply(const Transform& a, const Transform& b)
	{
		return Transform(a.pos + a.size * b.pos, a.size * b.size);
	}

	bool equals(const Transform& other)
	{
		return pos.x == other.pos.x && pos.y == other.pos.y && size.x == other.size.x && size.y == other.size.y;
	}

	String toString() const
	{
		FLAT_ASSERT(this);
		if (!this)
			return String("NULL");

		String result("Trns(Pos: ");
		result += pos.toString();
		result += ", Size: ";
		result += size.toString();
		result += ")";
		return result;
	}
};

struct TransformSorter
{
	static const bool UseSorting = true;
	inline static bool isFirst(const Transform& a, const Transform& b) { return a.pos.x < b.pos.x; }
};

namespace // Logger
{
	char LogBuffer[1024 * 16];
	SizeType LogBufferProgress = 0;
	void logTable(const char* msg)
	{
		SizeType length = (SizeType)strlen(msg);
		memcpy(LogBuffer + LogBufferProgress, msg, length);
		LogBufferProgress += length;
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
	void logTableInt(SizeType value)
	{
		char buffer[128];
		errno_t err = sprintf_s(buffer, "%u\t", value);
		FLAT_ASSERT(err >= 0);
		logTable(buffer);
	}
}

namespace // Result storage
{
	enum StatNumber
	{
		// Common
		StatCountAvg = 0,
		StatDepthAvg = 1,
		StatCountMax = 2,
		StatDepthMax = 3,
		StatAdd = 4,
		StatErase = 5,
		StatMove = 6,
		StatNthNode = 7,
		StatLeafTravel = 8,
		StatCalcDepth = 9,
		StatCalcCount = 10,
		StatTravelDepth = 11,
		StatTravelMax = 12,
		StatTransformIt1 = 13,
		StatTransformIt10 = 14,
		StatMax = 15,
	};

	// tree type, tree size, statistics
	double   baseTestStats[TreeCount][15][StatMax] = {};
	uint32_t baseTestCounts[TreeCount][15][StatMax] = {};

	uint32_t CurrentTreeType = 0;
	uint32_t CurrentTreeSize = 0;

	double* getStat(StatNumber statNumber)
	{
		FLAT_ASSERT(statNumber < StatMax);
		++(baseTestCounts[CurrentTreeType][CurrentTreeSize][statNumber]);
		return &baseTestStats[CurrentTreeType][CurrentTreeSize][statNumber];
	}
	double maxStat(StatNumber statNumber, double value)
	{
		FLAT_ASSERT(statNumber < StatMax);
		if (baseTestStats[CurrentTreeType][CurrentTreeSize][statNumber] < value)
			baseTestStats[CurrentTreeType][CurrentTreeSize][statNumber] = value;
		return baseTestStats[CurrentTreeType][CurrentTreeSize][statNumber];
	}
	double maxStat(StatNumber statNumber)
	{
		return maxStat(statNumber, -10000000);
	}
	double avgStat(StatNumber statNumber)
	{
		FLAT_ASSERT(statNumber < StatMax);
		if (baseTestCounts[CurrentTreeType][CurrentTreeSize][statNumber] == 0)
			return 0;
		return baseTestStats[CurrentTreeType][CurrentTreeSize][statNumber] / double(baseTestCounts[CurrentTreeType][CurrentTreeSize][statNumber]);
	}
	SizeType countStat(StatNumber statNumber)
	{
		FLAT_ASSERT(statNumber < StatMax);
		return baseTestCounts[CurrentTreeType][CurrentTreeSize][statNumber];
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

	uint32_t testResultHashes[TreeCount] = {};

	uint32_t runningHash = 0;

	void setHash(uint32_t hash)
	{
		if (VerbosePrinting)
			printf("Hash: %x\n", hash);

		if (!CheckingHashes)
			HashBuffer[CurrentHash++] = hash;
		else
			FLAT_ASSERTF(HashBuffer[CurrentHash++] == hash, "Expected %x got %x", HashBuffer[CurrentHash - 1], hash);
	}
	void setCount(uint32_t count)
	{
		if (VerbosePrinting)
			printf("Count: %d\n", count);

		if (!CheckingHashes)
			CountBuffer[CurrentCounter++] = count;
		else
			FLAT_ASSERT(CountBuffer[CurrentCounter++] == count);
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
	if (DoCacheFlushing)
	{
		//ScopedProfiler sdf(0, true);
		shuffleMemory();
		//printf("%.0lf\n", sdf.stop());
	}

		QueryPerformanceCounter(&start);
	}

	ScopedProfiler(double* cumulator, bool dontFlush)
		: cumulator(cumulator)
	{
		if(DoCacheFlushing && !dontFlush)
			shuffleMemory();

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
	// Mersenne twister
	// from https://en.wikipedia.org/wiki/Mersenne_twister#Python_implementation

	static uint32_t mt[624];
	static uint32_t index;
	static uint32_t random;
	static uint32_t counter;
	static uint32_t defaultSeed;
	static bool hashingEnabled;

	static void init(uint32_t seed = 0)
	{
		if (seed == 0)
			seed = defaultSeed;


		index = 624;
		mt[0] = seed;
		for (uint32_t i = 1; i < 624; ++i)
		{
			mt[i] = 1812433253 * (mt[i - 1] ^ (mt[i - 1] >> 30)) + i;
		}

		bool temp = hashingEnabled;
		hashingEnabled = false;

		Random::Extract(); Random::index = 624; Random::Extract(); Random::index = 624;
		Random::Extract(); Random::index = 624; Random::Extract(); Random::index = 624;
		Random::Extract(); Random::index = 624; Random::Extract(); Random::index = 624;

		hashingEnabled = temp;
	}

	static uint32_t Extract()
	{
		if (index >= 624)
		{
			// Twist
			for (uint32_t i = 0; i < 624; ++i)
			{
				uint32_t y = ((mt[i] & 0x80000000) + (mt[(i + 1) % 624] & 0x7fffffff));
				mt[i] = (mt[(i + 397) % 624] ^ y) >> 1;

				if ((y % 2) != 0)
					mt[i] = mt[i] ^ 0x9908B0DF;
			}
			index = 0;

			if (CheckHashes)
				DoHash();
		}

		// Extract
		{
			uint32_t y = mt[index];

			y = y ^ (y >> 11);
			y = y ^ ((y << 7) & 2636928640);
			y = y ^ ((y << 15) & 4022730752);
			y = y ^ (y >> 18);

			++index;

			return y;
		}
	}

	static void DoHash()
	{
		if (CheckHashes && CurrentHash < HashBufferSize && hashingEnabled)
		{
			setHash(SuperFastHash((char*)mt, 624 * sizeof(uint32_t)));
		}
	}

	static uint32_t get(uint32_t min, uint32_t max)
	{
		random = Extract();
		uint32_t result = max <= min ? max : min + random % (max - min);
		++counter;

		//if (VerbosePrinting)
		//{
		//	printf("RandomRange. Counter: %u, Value: %u, Min: %u, Max: %u, Raw: %u\n", counter, result, min, max, random);
		//}
		return result;
	}
};
uint32_t Random::mt[624] = {};
uint32_t Random::index = 0;
uint32_t Random::random = 0;
uint32_t Random::counter = 0;
uint32_t Random::defaultSeed = {};
bool Random::hashingEnabled = false;






///
// 
// 
// 
//  Test loop function
// 
// 
// 
///

template<typename Tree>
void testTree()
{
	CurrentHash = 0;
	CurrentCounter = 0;
	runningHash = 0;

	Random::init();

	for (SizeType arrSizeIndex = 0; arrSizeIndex < ArrTestSizesCount; ++arrSizeIndex)
	{
		CurrentTreeSize = arrSizeIndex;

		const SizeType TestNodeCount = ArrTestSizes[arrSizeIndex];

#ifdef _DEBUG
		const SizeType TestRepeatCount = ArrTestRoundNumbers[arrSizeIndex];
#else
		const SizeType TestRepeatCount = ArrTestRoundNumbers[arrSizeIndex];
#endif

		printf("\nTest size: %d\n", TestNodeCount);
		Tree t;

		for (SizeType repeatNumber = 0; repeatNumber < TestRepeatCount; repeatNumber++)
		{
			test_createTree(t);
			SizeType nodeCount = 1;

			printf("\rCurrentHash: %08x, progress: %d / %d   ", runningHash, repeatNumber + 1, TestRepeatCount);

			{
				struct LOLMBDA
				{
					static void add(Tree& t, SizeType parentIndex, SizeType childCount, SizeType& nodeCount, const SizeType targetNodeCount)
					{
						FLAT_ASSERT(nodeCount < targetNodeCount);
						SizeType index = test_addChild(t, nodeCount, parentIndex);
						nodeCount += 1;

						if (VerbosePrinting)
						{
							test_print(t);
						}

						if (CheckHashes)
						{
							test_setHash(t);
						}

						for (SizeType i = 0; i < childCount; ++i)
						{
							if (nodeCount >= targetNodeCount)
								return;
							add(t, index, childCount - 1 - i, nodeCount, targetNodeCount);
						}
					}
				};

				SizeType childCount = 2;
				while (nodeCount < TestNodeCount)
				{
					SizeType nodeIndex = test_addChild(t, nodeCount, 0);
					nodeCount += 1;

					if (VerbosePrinting)
					{
						test_print(t);
					}

					if (CheckHashes)
					{
						test_setHash(t);
					}

					if(nodeCount < TestNodeCount)
					{
						LOLMBDA::add(t, nodeIndex, childCount, nodeCount, TestNodeCount);
						childCount += 1;
					}
				}
			}

			for (SizeType doTravels = 0; doTravels < TestNodeCount * 0.2; doTravels++)
			{
				test_travelToLeaf(t, nodeCount);
			}
			for (SizeType doFind = 0; doFind < TestNodeCount * 0.1; doFind++)
			{
				test_findDepthAndCount(t, nodeCount);
			}

			test_multiplyTransforms(t, nodeCount, 3);

			for (SizeType doMoves = 0; doMoves < TestNodeCount * 0.1 || doMoves < 2; doMoves++)
			{
				SizeType parent = Random::get(1, nodeCount - 2);
				SizeType child = Random::get(parent + 2, nodeCount);
				test_moveChildren(t, nodeCount, child, parent);

				if (CheckHashes)
				{
					test_setHash(t);
				}
			}

			while (nodeCount > TestNodeCount * 0.8 && nodeCount > 2)
			{
				SizeType child = Random::get(1, nodeCount);
				test_removeNode(t, nodeCount, child);
				if (CheckHashes)
				{
					test_setHash(t);
				}
			}

			if(repeatNumber + 1 < TestRepeatCount) // Don't erase on last repeat so there is something to print
			{
				test_removeNode(t, nodeCount, 0);
			}
		}

		test_print(t);

		testResultHashes[CurrentTreeType] = runningHash;
		printf("\n\nResultHash: %x\n", runningHash);
		printf("adds: %d, erases: %d, moves: %d\n", countStat(StatAdd), countStat(StatErase), countStat(StatMove));
		printf("average count: %.1lf, depth: %.1lf, max count: %.0lf, depth: %.0lf\n", avgStat(StatCountAvg), avgStat(StatDepthAvg), maxStat(StatCountMax), maxStat(StatDepthMax));
		printf("average travel depth: %.1lf\n", avgStat(StatTravelDepth));

		printf("\n\n\n");

#define PRTINT_HALP(p) countStat(Stat ## p), avgStat( Stat ## p)
		printf("Average Add         -\t%d\t-  %.4lf us\n", PRTINT_HALP(Add));
		printf("Average Move        -\t%d\t-  %.4lf us\n", PRTINT_HALP(Move));
		printf("Average Erase       -\t%d\t-  %.4lf us\n", PRTINT_HALP(Erase));
		printf("Average CalcDepth   -\t%d\t-  %.4lf us\n", PRTINT_HALP(CalcDepth));
		printf("Average CalcCount   -\t%d\t-  %.4lf us\n", PRTINT_HALP(CalcCount));
		printf("Average NthNode     -\t%d\t-  %.4lf us\n", PRTINT_HALP(NthNode));
		printf("Average LeafTravel  -\t%d\t-  %.4lf us\n", PRTINT_HALP(LeafTravel));
		printf("Average Transform1  -\t%d\t-  %.4lf us\n", PRTINT_HALP(TransformIt1));
		printf("Average Transform10 -\t%d\t-  %.4lf us\n", PRTINT_HALP(TransformIt10));
#undef PRTINT_HALP

		printf("\n");
	}

	CheckingHashes = true;
}






///
// 
// 
// 
//  Flat tree test functions
// 
// 
// 
///

void test_createTree(FlatHierarchy<Transform, TransformSorter>& tree)
{
	Transform t;
#ifndef MAX_PERF
	t.nameInt = *reinterpret_cast<SizeType*>("Root");
#endif
	FLAT_ASSERT(tree.getCount() == 0);
	tree.createRootNode(t);
}

void test_setHash(const FlatHierarchy<Transform, TransformSorter>& tree)
{
	runningHash ^= SuperFastHash((char*)tree.values.getPointer(), tree.values.getSize() * sizeof(Transform));
	if (CurrentHash < HashBufferSize)
	{
		setHash(runningHash);
	}
}

static Transform makeTransform()
{
	SizeType x = Random::get(0, 900);
	SizeType y = Random::get(0, 900);
	SizeType w = Random::get(100, 1000 - x);
	SizeType h = Random::get(100, 1000 - y);

	Transform value(x / 1000.0f, y / 1000.0f, w / 1000.0f, h / 1000.0f);
#ifndef MAX_PERF
	value.name[0] = (char)Random::get('A', 'Z'); value.name[1] = (char)Random::get('a', 'z');
	value.name[2] = (char)Random::get('a', 'z'); value.name[3] = (char)Random::get('a', 'z');
#endif
	return value;
}

SizeType test_addChild(FlatHierarchy<Transform, TransformSorter>& tree, SizeType nodeCount, SizeType parentIndex)
{
	LastDescendantCache descendantCache;
	descendantCache.cacheIsValid = false;
	if (FLAT_CACHE_CONDITION)
		descendantCache.makeCacheValid(tree);

	SizeType childIndex = ~0U;

	Transform value = makeTransform();
	shuffleMemory();

	if (FLAT_NO_CACHE_CONDITION)
	{
		ScopedProfiler p(getStat(StatAdd));
		childIndex = tree.createNodeAsChildOf(parentIndex, value);
	}
	else if (FLAT_CACHE_CONDITION || FLAT_CACHE_UNPREP_CONDITION)
	{
		ScopedProfiler p(getStat(StatAdd));
		childIndex = createNodeAsChildOf(tree, descendantCache, parentIndex, value);
	}
	else
		FLAT_ASSERT(!"No condition matched.");
	return childIndex;
}

void test_moveChildren(FlatHierarchy<Transform, TransformSorter>& tree, SizeType nodeCount, SizeType childIndex, SizeType newParentIndex)
{
	FLAT_ASSERT(newParentIndex < childIndex);

	LastDescendantCache descendantCache;
	descendantCache.cacheIsValid = false;
	if (FLAT_CACHE_CONDITION)
		descendantCache.makeCacheValid(tree);

	ScopedProfiler p(getStat(StatMove));
	if (FLAT_NO_CACHE_CONDITION)
		tree.makeChildOf(childIndex, newParentIndex);
	else if (FLAT_CACHE_CONDITION || FLAT_CACHE_UNPREP_CONDITION)
		makeChildOf(tree, descendantCache, childIndex, newParentIndex);
	else
		FLAT_ASSERT(!"No condition matched");
}

void test_travelToLeaf(const FlatHierarchy<Transform, TransformSorter>& tree, SizeType nodeCount)
{
	// travel to leaf

	SizeType travelLoopCount = 0;
	SizeType current = 0;
	SizeType childCount = 0;

	NextSiblingCache siblingCache;

	siblingCache.cacheIsValid = false; // Invalidate cache before starting to travel, just in case
	if (FLAT_CACHE_CONDITION)
		siblingCache.makeCacheValid(tree);

	if (FLAT_CACHE_UNPREP_CONDITION)
		siblingCache.cacheIsValid = false;

	ScopedProfiler prof(getStat(StatLeafTravel));

	if (FLAT_NO_CACHE_CONDITION)
		childCount = countDirectChildren(tree, current);
	else if (FLAT_CACHE_CONDITION || FLAT_CACHE_UNPREP_CONDITION)
		childCount = countDirectChildren(tree, siblingCache, current);
	else
		FLAT_ASSERT(!"No condition matched");


	// Loop until in leaf
	while (childCount > 0)
	{
		++travelLoopCount;
		if (FLAT_NO_CACHE_CONDITION)
		{
			current = getNthChild(tree, current, Random::get(0, childCount));
			childCount = countDirectChildren(tree, current);
		}
		else if (FLAT_CACHE_CONDITION || FLAT_CACHE_UNPREP_CONDITION)
		{
			current = getNthChild(tree, siblingCache, current, Random::get(0, childCount));
			childCount = countDirectChildren(tree, siblingCache, current);
		}
		else
		{
			FLAT_ASSERT(!"No condition matched");
		}
	}

	*getStat(StatTravelDepth) += travelLoopCount;
	maxStat(StatTravelMax, travelLoopCount);
}

void test_findDepthAndCount(const FlatHierarchy<Transform, TransformSorter>& tree, SizeType nodeCount)
{
	SizeType depth = 0;
	SizeType count = 0;
	{
		ScopedProfiler prof(getStat(StatCalcDepth));
		depth = tree.findMaxDepth();
	}

	{
		ScopedProfiler prof(getStat(StatCalcCount));
		count = tree.getCount();
	}
	FLAT_ASSERT(count == nodeCount);

	*getStat(StatCountAvg) += count;
	*getStat(StatDepthAvg) += depth;

	maxStat(StatCountMax, count);
	maxStat(StatDepthMax, depth);
}

void test_multiplyTransforms(const FlatHierarchy<Transform, TransformSorter>& tree, SizeType nodeCount, const SizeType TransformIterations)
{
	if (CurrentTreeType != 0)
		return;

	FLAT_VECTOR<Transform> resultTransforms;
	resultTransforms.reserve(nodeCount);

	Transform tempBuffer[256];
	FLAT_ASSERT(tree.findMaxDepth() + 1 < 256);

	{
		// Only log first and last iterations

		ScopedProfiler prof(getStat(StatTransformIt1));

		resultTransforms.pushBack(tree.values[0]);
		tempBuffer[0] = tree.values[0];

		for (SizeType i = 1, end = tree.getCount(); i < end; i++)
		{
			FLAT_ASSERT(tree.depths[i] > 0);
			Transform myTransform = Transform::multiply(tempBuffer[tree.depths[i] - 1], tree.values[i]);
			resultTransforms.pushBack(myTransform);
			tempBuffer[tree.depths[i]] = myTransform;
		}
	}

	for (SizeType iteration = 1; iteration + 1 < TransformIterations; ++iteration)
	{
		resultTransforms.clear();
		resultTransforms.pushBack(tree.values[0]);
		tempBuffer[0] = tree.values[0];

		for (SizeType i = 1, end = tree.getCount(); i < end; i++)
		{
			FLAT_ASSERT(tree.depths[i] > 0);
			Transform myTransform = Transform::multiply(tempBuffer[tree.depths[i] - 1], tree.values[i]);
			resultTransforms.pushBack(myTransform);
			tempBuffer[tree.depths[i]] = myTransform;
		}
	}

	{
		// Only log first and last iterations
		resultTransforms.clear();

		ScopedProfiler prof(getStat(StatTransformIt10), true);

		resultTransforms.pushBack(tree.values[0]);
		tempBuffer[0] = tree.values[0];

		for (SizeType i = 1, end = tree.getCount(); i < end; i++)
		{
			FLAT_ASSERT(tree.depths[i] > 0);
			Transform myTransform = Transform::multiply(tempBuffer[tree.depths[i] - 1], tree.values[i]);
			resultTransforms.pushBack(myTransform);
			tempBuffer[tree.depths[i]] = myTransform;
		}
	}
}

void test_removeNode(FlatHierarchy<Transform, TransformSorter>& tree, SizeType& nodeCount, SizeType index)
{
	{
		ScopedProfiler prof(getStat(StatErase));
		tree.erase(index);
	}

	nodeCount = tree.getCount();
}


void test_print(const FlatHierarchy<Transform, TransformSorter>& tree)
{
#ifndef MAX_PERF
	struct LOLMBDA
	{
		static const char* getName(Transform& t, char* buffer)
		{
			memcpy(buffer, t.name, 4);
			return buffer;
		}
	};

	printf("\n\n");
	for (SizeType c = 0; c < tree.getCount(); c++)
	{
		char buffer[5];
		buffer[4] = '\0';
		printf("%s ", LOLMBDA::getName(tree.values[c], buffer));
	}
	printf("\n");
	for (SizeType c = 0; c < tree.getCount(); c++)
	{
		if (tree.depths[c] < 10)
			printf("  %u  ", tree.depths[c]);
		else
			printf("  %u ", tree.depths[c]);
	}
	printf("\n");
	for (SizeType c = 0; c < tree.getCount() + 1; c++)
	{
		printf("-----");
	}

	printf("\n");
	for (SizeType c = 0; c < tree.getCount(); c++)
	{
		if (c < 10)
			printf("  %d  ", c);
		else
			printf("  %d ", c);
	}

	printf("\n");


	char buffer[4096];
	for (SizeType r = 0, end = tree.findMaxDepth() + 5; r < end; r++)
	{
		buffer[tree.getCount() * 5] = '\0';

		// Iterate columns backwards to benefit from linear parent lookup
		for (SizeType c = 0; c < tree.getCount(); ++c)
		{
			bool hasDirectChildren = false;

			for (SizeType i = c + 1; i < tree.getCount(); i++)
			{
				if (tree.depths[i] > r + 1)
					continue;
				hasDirectChildren = tree.depths[i] == r + 1;
				break;
			}

			if (tree.depths[c] == r)
			{
				LOLMBDA::getName(tree.values[c], buffer + c * 5);
				*(buffer + c * 5 + 4) = ' ';
			}
			else if (tree.depths[c] == r + 1)
				memcpy(buffer + c * 5, (hasDirectChildren ? "-v---" : "-v   "), 5);
			else if (tree.depths[c] > r && hasDirectChildren)
				memcpy(buffer + c * 5, "-----", 5);
			//else if (tree.depths[c] < r + 1 && tree.depths[c] + 4U >= r)
			//{
			//	char tempBuffer[128];
			//	sprintf(tempBuffer, "%0.5f", *((&((tree.values.getPointer() + c)->pos.x)) + (r - 1 - tree.depths[c])));
			//	memcpy(buffer + c * 5, tempBuffer+2, 5);
			//}
			else
				memcpy(buffer + c * 5, "     ", 5);
		}
		printf("%s\n", buffer);
	}
	printf("\n");
#endif
}





///
// 
// 
// 
//  Pointer tree test functions
// 
// 
// 
///


template<typename Tree>
void test_createTree(Tree& tree)
{
	Transform t;
#ifndef MAX_PERF
	t.nameInt = *reinterpret_cast<SizeType*>("Root");
#endif
	FLAT_ASSERT(tree.root == NULL);
	tree.root = tree.createNode(t, NULL);
}

template<typename Tree>
void test_setHash(const Tree& tree)
{
	struct HASH_GATHER
	{
		static void recurse(Tree::Node* n, FLAT_VECTOR<Transform>& result)
		{
			result.pushBack(n->value);

			for (SizeType i = 0, childCount = getChildCount(n); i < childCount; i++)
			{
				recurse((Tree::Node*)getNthChild(n, i), result);
			}
		}
	};

	FLAT_VECTOR<Transform> result;
	HASH_GATHER::recurse(tree.root, result);

	runningHash ^= SuperFastHash((char*)result.getPointer(), result.getSize() * sizeof(Transform));
	if (CurrentHash < HashBufferSize)
	{
		setHash(runningHash);
	}
}

namespace
{
	static MultiwayTreeNodeBase* getNthNode(MultiwayTreeNodeBase* root, SizeType target)
	{
		struct LOLMBDA
		{
			static bool recurse(MultiwayTreeNodeBase* node, SizeType& n, MultiwayTreeNodeBase*& result)
			{
				if (node == NULL)
				{
					return false;
				}
				if (n == 0)
				{
					result = node;
					return true;
				}
				--n;
				if (recurse(node->child, n, result))
				{
					return true;
				}
				return recurse(node->sibling, n, result);
			}
		};

		MultiwayTreeNodeBase* result = NULL;
		LOLMBDA::recurse(root, target, result);
		return result;
	}

	static RivalTreeNodeBase* getNthNode(RivalTreeNodeBase* root, SizeType target)
	{
		struct LOLMBDA
		{
			static RivalTreeNodeBase* recurse(RivalTreeNodeBase* n, SizeType& current)
			{
				if (current == 0)
				{
					return n;
				}
				--current;

				for (SizeType i = 0, end = getChildCount(n); i < end; i++)
				{
					if (RivalTreeNodeBase* result = recurse(getNthChild(n, i), current))
						return result;
				}
				return NULL;
			}
		};

		RivalTreeNodeBase* result = LOLMBDA::recurse(root, target);
		return result;
	}

	template<typename Node>
	static SizeType findNodeNumber(Node* root, Node* target)
	{
		struct LOLMBDA
		{
			static bool recurse(Node* n, Node* target, SizeType& result)
			{
				if (n == target)
				{
					return true;
				}
				++result;

				for (SizeType i = 0, end = getChildCount(n); i < end; i++)
				{
					if (recurse((Node*)getNthChild(n, i), target, result))
						return true;
				}
				return false;
			}
		};

		SizeType result = 0;
		if (!LOLMBDA::recurse(root, target, result))
		{
			FLAT_ASSERT(!"Couldn't find target node.");
		}
		else
		{
			FLAT_ASSERT(getNthNode(root, result) == target);
		}
		return result;
	}
}

template<typename Tree>
SizeType test_addChild(Tree& tree, SizeType nodeCount, SizeType parentIndex)
{
	typedef Tree::Node Node;
	Node* parentNode = NULL;
	{
		ScopedProfiler prof(getStat(StatNthNode));
		parentNode = (Node*)getNthNode(tree.root, parentIndex);
		FLAT_ASSERT(parentNode != NULL);
	}
	FLAT_ASSERT(parentIndex == 0 || isChildOf(parentNode, tree.root));

	Transform value = makeTransform();
	Node* node = NULL;

	{
		ScopedProfiler prof(getStat(StatAdd));
		node = tree.createNode(value, parentNode);
	}
	FLAT_ASSERT(isChildOf(node, tree.root));

	FLAT_ASSERT(findCount(tree.root) == 1 + nodeCount);
	return findNodeNumber(tree.root, node);
}

template<typename Tree>
void test_moveChildren(Tree& tree, SizeType nodeCount, SizeType childIndex, SizeType newParentIndex)
{
	FLAT_ASSERTF(findCount(tree.root) == nodeCount, "NodeCount doesn't match: %d vs %d", findCount(tree.root), nodeCount);
	FLAT_ASSERT(newParentIndex < childIndex);

	typedef Tree::Node Node;

	Node* node = NULL;
	Node* parentNode = NULL;
	{
		ScopedProfiler prof(getStat(StatNthNode));
		node = (Node*)getNthNode(tree.root, childIndex);
		FLAT_ASSERT(node != NULL);
	}
	{
		ScopedProfiler prof(getStat(StatNthNode));
		parentNode = (Node*)getNthNode(tree.root, newParentIndex);
		FLAT_ASSERT(parentNode);
	}

	FLAT_ASSERT(childIndex != 0 || isChildOf(parentNode, node));

	if (isChildOf(parentNode, node))
	{
		Node* temp = node;
		node = parentNode;
		parentNode = temp;
	}

	FLAT_ASSERT(!isChildOf(parentNode, node));

	{
		FLAT_ASSERT(node != tree.root);
		ScopedProfiler prof(getStat(StatMove));
		tree.makeChildOf(node, parentNode);
	}

	FLAT_ASSERT(!isChildOf(parentNode, node));

	FLAT_ASSERTF(findCount(tree.root) == nodeCount, "NodeCount doesn't match: %d vs %d", findCount(tree.root), nodeCount);
}

template<typename Tree>
void test_travelToLeaf(const Tree& tree, SizeType nodeCount)
{
	// travel to leaf

	typedef Tree::Node Node;

	ScopedProfiler prof(getStat(StatLeafTravel));
	Node* currentNode = tree.root;

	SizeType travelLoopCount = 0;
	SizeType childCount = getChildCount(currentNode);

	// Loop until in leaf
	while (childCount > 0)
	{
		++travelLoopCount;
		currentNode = (Node*)getNthChild(currentNode, Random::get(0, childCount) );
		childCount = getChildCount(currentNode);
	}

	*getStat(StatTravelDepth) += travelLoopCount;
	maxStat(StatTravelMax, travelLoopCount);
}

template<typename Tree>
void test_findDepthAndCount(const Tree& tree, SizeType nodeCount)
{
	struct LOLMBDA
	{
		static SizeType count(const MultiwayTreeNodeBase* node)
		{
			return findCount(node);
		}
		static SizeType depth(const MultiwayTreeNodeBase* node)
		{
			return findDepth(node);
		}
		static SizeType count(const RivalTreeNodeBase* node)
		{
			SizeType result = 1;
			for (SizeType i = 0; i < node->children.getSize(); i++)
			{
				result += count(node->children[i]);
			}
			return result;
		}
		static SizeType depth(const RivalTreeNodeBase* node)
		{
			SizeType result = 0;
			for (SizeType i = 0; i < node->children.getSize(); i++)
			{
				SizeType d = depth(node->children[i]);
				if (d > result)
					result = d;
			}
			return result + 1;
		}
	};

	SizeType depth = 0;
	SizeType count = 0;
	{
		ScopedProfiler prof(getStat(StatCalcDepth));
		depth = LOLMBDA::depth(tree.root);
	}

	{
		ScopedProfiler prof(getStat(StatCalcCount));
		count = LOLMBDA::count(tree.root);
	}
	FLAT_ASSERT(count == nodeCount);

	*getStat(StatCountAvg) += count;
	*getStat(StatDepthAvg) += depth;

	maxStat(StatCountMax, count);
	maxStat(StatDepthMax, depth);
}

template<typename Tree>
void test_multiplyTransforms(const Tree& tree, SizeType nodeCount, const SizeType TransformIterations)
{
	struct LOLMBDA
	{
		static void recurse(Tree::Node* node, const Transform& parentTransform, FLAT_VECTOR<Transform>& rt)
		{
			Transform myTransform = Transform::multiply(parentTransform, node->value);
			rt.pushBack(myTransform);

			for (SizeType i = 0, childCount = getChildCount(node); i < childCount; i++)
			{
				recurse((Tree::Node*)getNthChild(node, i), myTransform, rt);
			}
		}
	};


	FLAT_VECTOR<Transform> resultTransforms;
	resultTransforms.reserve(nodeCount);

	{
		// Only log first and last iterations

		ScopedProfiler prof(getStat(StatTransformIt1));
		LOLMBDA::recurse(tree.root, Transform(), resultTransforms);
	}

	for (SizeType iteration = 1; iteration + 1 < TransformIterations; ++iteration)
	{
		LOLMBDA::recurse(tree.root, Transform(), resultTransforms);
	}

	{
		// Only log first and last iterations

		ScopedProfiler prof(getStat(StatTransformIt10), true);
		LOLMBDA::recurse(tree.root, Transform(), resultTransforms);
	}
}

template<typename Tree>
void test_removeNode(Tree& tree, SizeType& nodeCount, SizeType child)
{
	FLAT_ASSERT(nodeCount > 1 || child == 0);
	typedef Tree::Node Node;
	Node* node = NULL;

	{
		ScopedProfiler prof(getStat(StatNthNode));
		node = (Node*)getNthNode(tree.root, child);
		FLAT_ASSERT(node != NULL);
	}

	FLAT_ASSERT(child == 0 || node != tree.root);

	if (child == 0)
	{
		tree.removeRoot();
	}
	else
	{
		ScopedProfiler prof(getStat(StatErase));
		tree.eraseNode(node);
	}

	if (child == 0)
	{
		// Remove root
		nodeCount = 0;
		return;
	}

	nodeCount = findCount(tree.root);
}

template<typename Tree>
void test_print_impl(const Tree& tree)
{
#ifndef MAX_PERF
	struct LOLMBDA
	{
		static void recurse(Tree::Node* n, FlatHierarchy<Transform, TransformSorter>& result, SizeType depth = 0)
		{
			result.values.pushBack(n->value);
			result.depths.pushBack((FlatHierarchy<Transform, TransformSorter>::DepthValue) depth);

			for (SizeType i = 0; i < n->children.getSize(); i++)
			{
				recurse((Tree::Node*)n->children[i], result, depth + 1);
			}
		}
	};

	if (tree.root == NULL)
		return;

	FlatHierarchy<Transform, TransformSorter> temp;
	LOLMBDA::recurse(tree.root, temp);

	test_print(temp);
#endif
}

template<typename Tree>
void test_print_multiway_impl(const Tree& tree)
{
#ifndef MAX_PERF
	struct LOLMBDA
	{
		static void recurse(Tree::Node* n, FlatHierarchy<Transform, TransformSorter>& result, SizeType depth = 0)
		{
			result.values.pushBack(n->value);
			result.depths.pushBack((FlatHierarchy<Transform, TransformSorter>::DepthValue) depth);

			MultiwayTreeNodeBase* child = n->child;

			while (child != NULL)
			{
				recurse((Tree::Node*)child, result, depth + 1);
				child = child->sibling;
			}
		}
	};

	if (tree.root == NULL)
		return;

	FlatHierarchy<Transform, TransformSorter> temp;
	LOLMBDA::recurse(tree.root, temp);

	test_print(temp);
#endif
}

void test_print(NaiveTree<Transform, TransformSorter>& tree)
{
	test_print_impl(tree);
}
void test_print(RivalTree<Transform, TransformSorter>& tree)
{
	test_print_impl(tree);
}

void test_print(MultiwayTree<Transform, TransformSorter>& tree)
{
	test_print_multiway_impl(tree);
}
void test_print(NaiveMultiwayTree<Transform, TransformSorter>& tree)
{
	test_print_multiway_impl(tree);
}








///
// 
// 
// 
//  Main test function
// 
// 
// 
///

void test()
{
	ScopedProfiler wholeTestProfiler;

	{
		Random::hashingEnabled = false;
		Random::init(1771551);
		Random::defaultSeed = Random::Extract();
		Random::init();
		Random::hashingEnabled = true;
	}

	memset(LogBuffer, ' ', sizeof(LogBuffer));

	try
	{
		// Flat
		if ((TestMask & Flat1) != 0 || (TestMask & Every) != 0)
		{
			CurrentTreeType = 0;

			printf("\nFlat Tree\n");
			testTree< FlatHierarchy<Transform, TransformSorter> >();
		}

		// Hot cache
		if ((TestMask & Flat2) != 0 || (TestMask & Every) != 0)
		{
			CurrentTreeType = 1;

			testTree< FlatHierarchy<Transform, TransformSorter> >();
		}

		// Cold cache
		if ((TestMask & Flat3) != 0 || (TestMask & Every) != 0)
		{
			CurrentTreeType = 2;

			printf("\nFlat Tree With Cold Cache\n");

			testTree< FlatHierarchy<Transform, TransformSorter> >();
		}

		// Naive tree
		if ((TestMask & Naive) != 0 || (TestMask & Every) != 0)
		{
			CurrentTreeType = 3;

			printf("\nNaive Pointer Tree\n");

			testTree< NaiveTree<Transform, TransformSorter> >();
		}

		// Rival tree
		if ((TestMask & Rival) != 0 || (TestMask & Every) != 0)
		{
			CurrentTreeType = 4;

			printf("\nPooled Pointer Tree\n");

			testTree< RivalTree<Transform, TransformSorter> >();
		}

		// Naive multiway tree
		if ((TestMask & Nulti) != 0 || (TestMask & Every) != 0)
		{
			CurrentTreeType = 5;

			printf("\nNaive Multiway Tree\n");

			testTree< NaiveMultiwayTree<Transform, TransformSorter> >();
		}


		// Multiway tree
		if ((TestMask & Multi) != 0 || (TestMask & Every) != 0)
		{
			CurrentTreeType = 6;

			printf("\nPooled Multiway Tree\n");

			testTree< MultiwayTree<Transform, TransformSorter> >();
		}


	}
	catch (...)
	{
	}
	// Print summary about depth and count
	{
		logTable("\tMax Count\t");
		logTable("Average Count\t");
		logTable("Max Depth\t");
		logTable("Average Depth\t");
		logTable("Max Travel Depth\t");
		logTable("Average Travel Depth\n\t");
		logTableDouble(maxStat(StatCountMax));
		logTableDouble(avgStat(StatCountAvg));
		logTableDouble(maxStat(StatDepthMax));
		logTableDouble(maxStat(StatDepthAvg));
		logTableDouble(maxStat(StatTravelDepth));
		logTableDouble(avgStat(StatTravelDepth));
		logTable("\n");
	}

	// Log base stats
	for (StatNumber stat = StatAdd; stat < StatMax; stat = (StatNumber)((uint32_t)stat + 1))
	{
		// size 
		{
			logTable("\n\tCount\t");
			for (SizeType treeSize = 0; treeSize < ArrTestSizesCount; treeSize++)
			{
				logTableInt(ArrTestSizes[treeSize]);
			}
			logTable("\n");
		}

		static const char* statNames[] = {
			"Add",
			"Erase",
			"Move",
			"Nth Node",
			"Leaf travel",
			"Find max depth",
			"Find count",
			"Traveled depth average",
			"Travel depth max",
			"Transform mult first",
			"Transform mult last" };

		logTable(statNames[stat - StatAdd]);
		logTable("\t");

		for (SizeType treeType = 0; treeType < TreeCount; treeType++)
		{
			static const char* treeNames[] = { "Flat", "Flat cached", "Flat cold", "Naive Pointer", "Pooled Pointer", "Naive Multiway", "Pooled Multiway"};
			const char* treeName = treeNames[treeType];

			logTable(treeName);
			logTable("\t");

			for (SizeType treeSize = 0; treeSize < ArrTestSizesCount; treeSize++)
			{
				CurrentTreeType = treeType;
				CurrentTreeSize = treeSize;

				if(stat == StatTravelMax)
					logTableInt((uint32_t)maxStat(stat));
				else
					logTableDouble(avgStat(stat));
			}
			logTable("\n");
			if (treeType + 1 < TreeCount)
				logTable("\t");
		}
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

#ifdef MAX_PERF
	{
		FILE* f = NULL;
		errno_t err = fopen_s(&f, "output.csv", "w");
		FLAT_ASSERT(err >= 0);
		fprintf(f, "%s", LogBuffer);
		fclose(f);

		printf("\nOutput saved to output.csv\n");
	}
#endif

	// Print hashes
	if(CheckHashes)
	{
		for (SizeType i = 0; i < TreeCount; i++)
		{
			printf("%x  ", testResultHashes[i]);
		}
		printf("\n");
		printf("\n");
	}

	printf("\nTest completed in %.2lf seconds.\n\n", wholeTestProfiler.stop() / 1000000.0);

	system("pause");
}