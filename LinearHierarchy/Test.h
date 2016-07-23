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
#include <inttypes.h>
#include <time.h>
#include <windows.h>
#include "FastHash.h"

typedef uint32_t SizeType;

#ifdef _DEBUG
const SizeType ArrBaseTestSizesCount = 2;
const SizeType ArrTransformTestSizesCount = 6;
const SizeType ArrTransformIterationsCount = 2;
const SizeType TransformRoundNumber = 2;

const SizeType DebugBaseTestRoundNumber = 3;

#elif defined(MAX_PERF)
const SizeType ArrBaseTestSizesCount = 12;
const SizeType ArrTransformTestSizesCount = 12;
const SizeType ArrTransformIterationsCount = 2;
const SizeType TransformRoundNumber = 100;

#else
const SizeType ArrBaseTestSizesCount = 12;
const SizeType ArrTransformTestSizesCount = 12;
const SizeType ArrTransformIterationsCount = 2;
const SizeType TransformRoundNumber = 10;
#endif

const SizeType ArrBaseTestSizes[]        = { 10,     50,    100,   200,   500,  1000, 2000, 5000, 10000, 20000 };
const SizeType ArrBaseTestRoundNumbers[] = { 100000, 20000, 10000, 5000,  2000, 1000, 200,  50,   5,     4 };

const SizeType ArrTransformTestSizes[]  = { 10, 20, 50, 100, 200, 500, 1000, 1500, 2000, 2500, 3000, 3500 }; // , 5000, 10000 };
const SizeType ArrTransformIterations[] = { 1, 10 };

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
	static const bool UseSorting = false;
	inline static bool isFirst(const Transform& a, const Transform& b) { return a.pos.x < b.pos.x; }
};

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

		// Base tests only
		StatErase = 5,
		StatMove = 6,
		StatNthNode = 7,
		StatLeafTravel = 8,
		StatCalcDepth = 9,
		StatCalcCount = 10,
		StatTravelDepth = 11,
		StatTravelMax = 12,
		StatBaseMax = 13,

		// Transform tests only
		StatTransformIt1 = 5,
		StatTransformIt10 = 5,
		StatTransformMax = 6,
	};

	// tree type, tree size, statistics
	double   baseTestStats[5][15][StatBaseMax] = {};
	uint32_t baseTestCounts[5][15][StatBaseMax] = {};

	double   trnsTestStats[3][15][StatTransformMax] = {};
	uint32_t trnsTestCounts[3][15][StatTransformMax] = {};

	uint32_t CurrentTreeType = 0;
	uint32_t CurrentTreeSize = 0;
	bool CurrentlyTestingTransforms = false;

	double* getStat(StatNumber statNumber)
	{
		if (!CurrentlyTestingTransforms)
		{
			FLAT_ASSERT(statNumber < StatBaseMax);
			++(baseTestCounts[CurrentTreeType][CurrentTreeSize][statNumber]);
			return &baseTestStats[CurrentTreeType][CurrentTreeSize][statNumber];
		}
		else
		{
			FLAT_ASSERT(statNumber < StatTransformMax);
			++(trnsTestCounts[CurrentTreeType][CurrentTreeSize][statNumber]);
			return &trnsTestStats[CurrentTreeType][CurrentTreeSize][statNumber];
		}
	}
	double maxStat(StatNumber statNumber, double value)
	{
		if (!CurrentlyTestingTransforms)
		{
			FLAT_ASSERT(statNumber < StatBaseMax);
			if (baseTestStats[CurrentTreeType][CurrentTreeSize][statNumber] < value)
				baseTestStats[CurrentTreeType][CurrentTreeSize][statNumber] = value;
			return baseTestStats[CurrentTreeType][CurrentTreeSize][statNumber];
		}
		else
		{
			FLAT_ASSERT(statNumber < StatTransformMax);
			if (trnsTestStats[CurrentTreeType][CurrentTreeSize][statNumber] < value)
				trnsTestStats[CurrentTreeType][CurrentTreeSize][statNumber] = value;
			return trnsTestStats[CurrentTreeType][CurrentTreeSize][statNumber];
		}
	}
	double maxStat(StatNumber statNumber)
	{
		return maxStat(statNumber, -10000000);
	}
	double avgStat(StatNumber statNumber)
	{
		if (!CurrentlyTestingTransforms)
		{
			FLAT_ASSERT(statNumber < StatBaseMax);
			if (baseTestCounts[CurrentTreeType][CurrentTreeSize][statNumber] == 0)
				return 0;
			return baseTestStats[CurrentTreeType][CurrentTreeSize][statNumber] / double(baseTestCounts[CurrentTreeType][CurrentTreeSize][statNumber]);
		}
		else
		{
			FLAT_ASSERT(statNumber < StatTransformMax);
			if (trnsTestCounts[CurrentTreeType][CurrentTreeSize][statNumber] == 0)
				return 0;
			return trnsTestStats[CurrentTreeType][CurrentTreeSize][statNumber] / double(trnsTestCounts[CurrentTreeType][CurrentTreeSize][statNumber]);
		}
	}
	SizeType countStat(StatNumber statNumber)
	{
		if (!CurrentlyTestingTransforms)
		{
			FLAT_ASSERT(statNumber < StatBaseMax);
			return baseTestCounts[CurrentTreeType][CurrentTreeSize][statNumber];
		}
		else
		{
			FLAT_ASSERT(statNumber < StatTransformMax);
			return trnsTestCounts[CurrentTreeType][CurrentTreeSize][statNumber];
		}
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

namespace
{
	const uint32_t TransformBufferSize = 1024 * 32;
	Transform TransformBuffer[TransformBufferSize];
	SizeType CurrentTransform = 0;
	void setTransform(const Transform& t)
	{
		if (!CheckingHashes)
			TransformBuffer[CurrentTransform++] = t;
		else
			FLAT_ASSERT(TransformBuffer[CurrentTransform++].equals(t));
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
	static uint32_t counter;
	uint64_t x; /* The state must be seeded with a nonzero value. */

	static uint64_t randomState[2];

	static void init(uint32_t seed = InitSeed)
	{
		random = seed;
		randomState[0] = uint64_t((1181783497276652981U / 3U + 1181783497276652981U / 13U) * 71551U);
		randomState[1] = uint64_t(1181783497276652981U);
		counter = 0;	
	}
	
	static uint64_t xorshift128plus() {
		uint64_t x = randomState[0];
		uint64_t const y = randomState[1];
		randomState[0] = y;
		x ^= x << 23; // a
		randomState[1] = x ^ y ^ (x >> 17) ^ (y >> 26); // b, c
		return randomState[1] + y;
	}

	static uint32_t get(SizeType min, SizeType max)
	{
		random = (uint32_t)xorshift128plus();
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
uint64_t Random::randomState[2] = {};







void baseTestEndPrints(double diff)
{
	printf("\n");
	printf("%.4f s, adds: %d, erases: %d, moves: %d\n", diff, countStat(StatAdd), countStat(StatErase), countStat(StatMove));
	printf("average count: %.1lf, depth: %.1lf, max count: %.0lf, depth: %.0lf\n", avgStat(StatCountAvg), avgStat(StatDepthAvg), maxStat(StatCountMax), maxStat(StatDepthMax));
	printf("average travel depth: %.1lf\n", avgStat(StatTravelDepth));

	printf("\n\n\n");

#define PRTINT_HALP(p) countStat(Stat ## p), avgStat( Stat ## p)
	printf("Average Add        -\t%d\t-  %.4lf us\n", PRTINT_HALP(Add));
	printf("Average Move       -\t%d\t-  %.4lf us\n", PRTINT_HALP(Move));
	printf("Average Erase      -\t%d\t-  %.4lf us\n", PRTINT_HALP(Erase));
	printf("Average CalcDepth  -\t%d\t-  %.4lf us\n", PRTINT_HALP(CalcDepth));
	printf("Average CalcCount  -\t%d\t-  %.4lf us\n", PRTINT_HALP(CalcCount));
	printf("Average NthNode    -\t%d\t-  %.4lf us\n", PRTINT_HALP(NthNode));
	printf("Average LeafTravel -\t%d\t-  %.4lf us\n", PRTINT_HALP(LeafTravel));
#undef PRTINT_HALP

		printf("\n");
}


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

	for (SizeType fullRoundNumber = 0; fullRoundNumber < ArrBaseTestSizesCount; fullRoundNumber++)
	{
		CurrentTreeSize = fullRoundNumber;

		SizeType COUNT = 0;

		HierarchyType l(70);
		l.createRootNode(getHandle("Root"));

		LastDescendantCache descendantCache;
		NextSiblingCache siblingCache;

		COUNT += 1;

		const SizeType targetCount = (SizeType)ArrBaseTestSizes[fullRoundNumber];

		printf("\nTargetCount: %u\n", targetCount);

#ifdef _DEBUG
		const SizeType RoundNumber = DebugBaseTestRoundNumber;
#else
		const SizeType RoundNumber = ArrBaseTestRoundNumbers[fullRoundNumber];
#endif

		for (SizeType j = 0; j < RoundNumber; j++)
		{
			if(ArrBaseTestRoundNumbers[fullRoundNumber] <= 1000)
				printf(".");

			// Add

			while (COUNT < targetCount)
			{
				char name[4] = { 'A' + (char)Random::get(0,26), 'a' + (char)Random::get(0,26), 'a' + (char)Random::get(0,26), 'a' + (char)Random::get(0,26) };
				Handle handle = getHandle(name);

				SizeType parent = Random::get(0, COUNT);
				if (VerbosePrinting)
				{
					char buffer[5]; buffer[4] = '\0';
					char buffer2[5]; buffer2[4] = '\0';
					printf("Add %s to #%d \"%s\"\n", getName(handle, buffer), parent, getName(l.values[parent], buffer2));
				}

				{

					descendantCache.cacheIsValid = false;
					if (FLAT_CACHE_CONDITION)
						descendantCache.makeCacheValid(l);

					ScopedProfiler p(getStat(StatAdd));
					if (FLAT_NO_CACHE_CONDITION)
						l.createNodeAsChildOf(parent, handle);
					else if (FLAT_CACHE_CONDITION || FLAT_CACHE_UNPREP_CONDITION)
						createNodeAsChildOf(l, descendantCache, parent, handle);
					else
						FLAT_ASSERT(!"No condition matched.");
				}

				++COUNT;

				if (CheckHashes && CurrentHash < HashBufferSize)
				{
					setHash(SuperFastHash((char*)l.depths.getPointer(), l.getCount() * sizeof(HierarchyType::DepthValue)));
				}
				if (CheckCounts && CurrentCounter < HashBufferSize)
				{
					setCount(l.getCount());
				}
			}

			// Move

			for (SizeType doMoves = 0; doMoves < targetCount * 0.1 || doMoves < 2; doMoves++)
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

				{
					descendantCache.cacheIsValid = false;
					if (FLAT_CACHE_CONDITION)
						descendantCache.makeCacheValid(l);

					ScopedProfiler p(getStat(StatMove));
					if (FLAT_NO_CACHE_CONDITION)
						l.makeChildOf(child, parent);
					else if (FLAT_CACHE_CONDITION || FLAT_CACHE_UNPREP_CONDITION)
						makeChildOf(l, descendantCache, child, parent);
					else
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

				if (VerbosePrinting)
				{
					PRINT::print(l);
				}
			}

			// Travel
			
			siblingCache.cacheIsValid = false; // Invalidate cache before starting to travel, just in case
			if (FLAT_CACHE_CONDITION)
				siblingCache.makeCacheValid(l);

			for (SizeType doTravels = 0; doTravels < targetCount * 0.2; doTravels++)
			{
				// travel to leaf

				SizeType travelLoopCount = 0;
				SizeType current = 0;
				SizeType childCount = 0;

				if (FLAT_CACHE_UNPREP_CONDITION)
					siblingCache.cacheIsValid = false;

				ScopedProfiler prof(getStat(StatLeafTravel));

				if (FLAT_NO_CACHE_CONDITION)
					childCount = countDirectChildren(l, current);
				else if (FLAT_CACHE_CONDITION || FLAT_CACHE_UNPREP_CONDITION)
					childCount = countDirectChildren(l, siblingCache, current);
				else
					FLAT_ASSERT(!"No condition matched");


				// Loop until in leaf
				while (childCount > 0)
				{
					++travelLoopCount;
					if (FLAT_NO_CACHE_CONDITION)
					{
						current = getNthChild(l, current, Random::get(0, childCount));
						childCount = countDirectChildren(l, current);
					}
					else if (FLAT_CACHE_CONDITION || FLAT_CACHE_UNPREP_CONDITION)
					{
						current = getNthChild(l, siblingCache, current, Random::get(0, childCount));
						childCount = countDirectChildren(l, siblingCache, current);
					}
					else
					{
						FLAT_ASSERT(!"No condition matched");
					}
				}

				*getStat(StatTravelDepth) += travelLoopCount;
				maxStat(StatTravelMax, travelLoopCount);

				if (CheckHashes && CurrentHash < HashBufferSize)
				{
					setHash(SuperFastHash((char*)l.depths.getPointer(), l.getCount() * sizeof(HierarchyType::DepthValue)));
				}
				if (CheckCounts && CurrentCounter < HashBufferSize)
				{
					setCount(l.getCount());
				}
			}

			// Find depth and count
			for (SizeType doFind = 0; doFind < targetCount * 0.1; doFind++)
			{
				SizeType depth = 0;
				SizeType count = 0;
				{
					ScopedProfiler prof(getStat(StatCalcDepth));
					depth = l.findMaxDepth();
				}

				{
					ScopedProfiler prof(getStat(StatCalcCount));
					count = l.getCount();
				}

				*getStat(StatCountAvg) += count;
				*getStat(StatDepthAvg) += depth;

				maxStat(StatCountMax, count);
				maxStat(StatDepthMax, depth);
			}

			// Erase

			while (COUNT > targetCount * 0.8 && COUNT > 2)
			{
				FLAT_ASSERT(COUNT > 1);
				SizeType child = Random::get(1, COUNT);

				if (VerbosePrinting)
				{
					printf("Erase: %d\n", child);
				}

				{
					descendantCache.cacheIsValid = false;
					if (FLAT_CACHE_CONDITION)
						descendantCache.makeCacheValid(l);

					ScopedProfiler prof(getStat(StatErase));
					if (FLAT_NO_CACHE_CONDITION)
						l.erase(child);
					else if (FLAT_CACHE_CONDITION || FLAT_CACHE_UNPREP_CONDITION)
						erase(l, descendantCache, child);
					else
						FLAT_ASSERT(!"ASDF!!! No condition matched");
				}

				COUNT = l.getCount();

				if (CheckHashes && CurrentHash < HashBufferSize)
				{
					setHash(SuperFastHash((char*)l.depths.getPointer(), l.getCount() * sizeof(HierarchyType::DepthValue)));
				}
				if (CheckCounts && CurrentCounter < HashBufferSize)
				{
					setCount(l.getCount());
				}
			}
		}

		baseTestEndPrints(0);
	}
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

	for (SizeType fullRoundNumber = 0; fullRoundNumber < ArrBaseTestSizesCount; fullRoundNumber++)
	{
		CurrentTreeSize = fullRoundNumber;

		SizeType COUNT = 0;

		Tree tree;
		tree.root = tree.createNode(getHandle("Root"), NULL);

		COUNT += 1;

		const SizeType targetCount = (SizeType)ArrBaseTestSizes[fullRoundNumber];

		printf("\nTargetCount: %u\n", targetCount);

#ifdef _DEBUG
		const SizeType RoundNumber = DebugBaseTestRoundNumber;
#else
		const SizeType RoundNumber = ArrBaseTestRoundNumbers[fullRoundNumber];
#endif

		for (SizeType j = 0; j < RoundNumber; j++)
		{
			if (ArrBaseTestRoundNumbers[fullRoundNumber] <= 1000)
				printf(".");

			// Add

			while (COUNT < targetCount)
			{
				char name[4] = { 'A' + (char)Random::get(0,26), 'a' + (char)Random::get(0,26), 'a' + (char)Random::get(0,26), 'a' + (char)Random::get(0,26) };
				Handle handle = getHandle(name);

				SizeType parent = Random::get(0, COUNT);
				Node* parentNode = NULL;
				{
					ScopedProfiler prof(getStat(StatNthNode));
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
					ScopedProfiler prof(getStat(StatAdd));
					node = tree.createNode(handle, parentNode);
				}
				FLAT_ASSERT(isChildOf(node, tree.root));

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
					COUNT = findCount(tree.root);
					FLAT_ASSERT(oldCount == COUNT);

					setCount(COUNT);
				}
			}

			// Move

			for (SizeType doMoves = 0; doMoves < targetCount * 0.1 || doMoves < 2; doMoves++)
			{
				SizeType parent = Random::get(1, COUNT - 2);
				SizeType child = Random::get(parent + 2, COUNT);
				FLAT_ASSERT(parent < child);

				{
					Node* node = NULL;
					Node* parentNode = NULL;
					{
						ScopedProfiler prof(getStat(StatNthNode));
						bool success = tryGetNthNode(tree.root, (RivalTreeNodeBase*&)node, child);
						FLAT_ASSERT(success);
					}
					{
						ScopedProfiler prof(getStat(StatNthNode));
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
						ScopedProfiler prof(getStat(StatMove));
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
						COUNT = findCount(tree.root);
						FLAT_ASSERT(oldCount == COUNT);

						setCount(COUNT);
					}
				}
			}

			// Travel

			for (SizeType doTravels = 0; doTravels < targetCount * 0.2; doTravels++)
			{
				// travel to leaf

				ScopedProfiler prof(getStat(StatLeafTravel));

				Node* currentNode = tree.root;

				SizeType travelLoopCount = 0;

				// Loop until in leaf
				while (currentNode->children.getSize() > 0)
				{
					++travelLoopCount;
					currentNode = (Node*)currentNode->children[Random::get(0, currentNode->children.getSize())];
				}

				*getStat(StatTravelDepth) += travelLoopCount;
				maxStat(StatTravelMax, travelLoopCount);

				if (CheckCounts && CurrentCounter < HashBufferSize)
				{
					SizeType oldCount = COUNT;
					COUNT = findCount(tree.root);
					FLAT_ASSERT(oldCount == COUNT);

					setCount(COUNT);
				}
			}

			// Find depth and count
			for (SizeType doFind = 0; doFind < targetCount * 0.1; doFind++)
			{
				SizeType depth = 0;
				SizeType count = 0;
				{
					ScopedProfiler prof(getStat(StatCalcDepth));
					depth = findDepth(tree.root);
				}

				{
					ScopedProfiler prof(getStat(StatCalcCount));
					count = findCount(tree.root);
				}

				*getStat(StatCountAvg) += count;
				*getStat(StatDepthAvg) += depth;

				maxStat(StatCountMax, count);
				maxStat(StatDepthMax, depth);
			}

			// Erase

			while (COUNT > targetCount * 0.8 && COUNT > 2)
			{
				FLAT_ASSERT(COUNT > 1);
				SizeType child = Random::get(1, COUNT);

				if (VerbosePrinting)
				{
					printf("Erase: %d\n", child);
				}

				Node* node = NULL;

				{
					ScopedProfiler prof(getStat(StatNthNode));
					bool success = tryGetNthNode(tree.root, (RivalTreeNodeBase*&)node, child);
					FLAT_ASSERT(success);
				}
				FLAT_ASSERT(node != tree.root);


				{
					isChildOf(node, tree.root);

					ScopedProfiler prof(getStat(StatErase));
					tree.eraseNode(node);
				}

				COUNT = findCount(tree.root);

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
					COUNT = findCount(tree.root);
					FLAT_ASSERT(oldCount == COUNT);

					setCount(COUNT);
				}
			}
		}

		baseTestEndPrints(0);
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

uint32_t TranformTestResultHash = 0;

namespace
{
	static Transform getRandomTransform()
	{
		SizeType x = Random::get(0, 900);
		SizeType y = Random::get(0, 900);
		SizeType w = Random::get(100, 1000 - x);
		SizeType h = Random::get(100, 1000 - y);

		return Transform(x / 1000.0f, y / 1000.0f, w / 1000.0f, h / 1000.0f);
	}
};

void FLAT_TransformTest()
{
	for (SizeType fullRoundNumber = 0; fullRoundNumber < ArrTransformTestSizesCount * ArrTransformIterationsCount; fullRoundNumber++)
	{

		const SizeType TransformTestSize = ArrTransformTestSizes[fullRoundNumber / ArrTransformIterationsCount];
		const SizeType TransformIterations = ArrTransformIterations[fullRoundNumber % ArrTransformIterationsCount];

		CurrentTreeSize = fullRoundNumber / ArrTransformIterationsCount;

		printf("\nSize:     %u\n", TransformTestSize);
		printf("Iterations: %u\n", TransformIterations);

		for (SizeType rn = 0; rn < TransformRoundNumber; rn++)
		{
			const bool IsFirstRound = fullRoundNumber == 0 && rn == 0;

			typedef FlatHierarchy<Transform, TransformSorter> HierarchyType;
			HierarchyType l(TransformTestSize);
			l.createRootNode(Transform(0, 0, 1, 1));

			// Create tranforms
			for (SizeType aa = l.getCount(); aa < TransformTestSize; aa++)
			{
				SizeType parent = Random::get(0, aa);
				Transform trns = getRandomTransform();

				{
					ScopedProfiler prof(getStat(StatAdd));
					l.createNodeAsChildOf(parent, trns);
				}
			}

			// Check that transforms are correct
			{
				if (CheckHashes && IsFirstRound)
					for (SizeType i = 0; i < l.getCount(); i++)
						if (CurrentTransform < TransformBufferSize)
							setTransform(l.values[i]);
			}

			FLAT_VECTOR<Transform> resultTransforms;
			resultTransforms.reserve(TransformTestSize);

			// Get results
			{
				Transform tempBuffer[256];
				FLAT_ASSERT(l.findMaxDepth() + 1 < 256);

				for (SizeType iteration = 0; iteration < TransformIterations; iteration++)
				{
					resultTransforms.clear();

					ScopedProfiler prof(getStat(iteration == 0 ? StatTransformIt1 : StatTransformIt10));

					resultTransforms.pushBack(l.values[0]);
					tempBuffer[0] = l.values[0];

					if (CheckHashes && IsFirstRound && CurrentTransform < TransformBufferSize)
						setTransform(resultTransforms.getBack());
					if (CheckHashes && IsFirstRound && CurrentHash < HashBufferSize)
						setHash(SuperFastHash((char*)resultTransforms.getPointer(), resultTransforms.getSize() * sizeof(Transform)));

					for (SizeType i = 1, end = l.getCount(); i < end; i++)
					{
						FLAT_ASSERT(l.depths[i] > 0);
						Transform myTransform = Transform::multiply(tempBuffer[l.depths[i] - 1], l.values[i]);
						resultTransforms.pushBack(myTransform);
						tempBuffer[l.depths[i]] = myTransform;

						if (CheckHashes && iteration == 0 && IsFirstRound)
						{
							if (CheckHashes && IsFirstRound && CurrentTransform < TransformBufferSize)
								setTransform(resultTransforms.getBack());
							if (CheckHashes && IsFirstRound && CurrentHash < HashBufferSize)
								setHash(SuperFastHash((char*)resultTransforms.getPointer(), resultTransforms.getSize() * sizeof(Transform)));
						}
					}
				}
			}

			if (IsFirstRound)
			{
				if (TranformTestResultHash == 0)
					TranformTestResultHash = SuperFastHash((char*)resultTransforms.getPointer(), resultTransforms.getSize() * sizeof(Transform));
				else
					FLAT_ASSERT(TranformTestResultHash == SuperFastHash((char*)resultTransforms.getPointer(), resultTransforms.getSize() * sizeof(Transform)));
				FLAT_ASSERT(TranformTestResultHash != 0);
			}

			// Find count and depth, max and average
			{
				SizeType depth = l.findMaxDepth();
				SizeType count = l.getCount();

				*getStat(StatCountAvg) += count;
				*getStat(StatDepthAvg) += depth;

				maxStat(StatCountMax, count);
				maxStat(StatDepthMax, depth);
			}
		}

		printf("Average Add                %.4lf\n", avgStat(StatAdd));
		printf("Average Transform once     %.4lf\n", avgStat(StatTransformIt1));
		printf("Average Transform 10 times %.4lf\n", avgStat(StatTransformIt1));
		printf("\n");
		printf("Average Depth              %.1lf\n", avgStat(StatDepthAvg));
		printf("Average Count              %.1lf\n", avgStat(StatCountAvg));
		printf("\n\n");

	}
}


template<typename Tree>
void Other_Tree_TransformTest_Impl()
{
	for (SizeType fullRoundNumber = 0; fullRoundNumber < ArrTransformTestSizesCount * ArrTransformIterationsCount; fullRoundNumber++)
	{
		const SizeType TransformTestSize = ArrTransformTestSizes[fullRoundNumber / ArrTransformIterationsCount];
		const SizeType TransformIterations = ArrTransformIterations[fullRoundNumber % ArrTransformIterationsCount];
		
		CurrentTreeSize = fullRoundNumber / ArrTransformIterationsCount;

		printf("\nSize:     %u\n", TransformTestSize);
		printf("Iterations: %u\n", TransformIterations);

		for (SizeType rn = 0; rn < TransformRoundNumber; rn++)
		{
			const bool IsFirstRound = fullRoundNumber == 0 && rn == 0;

			struct Lolmbda
			{
				static void recurseCheck(const Tree::Node* node)
				{
					if (CheckHashes && CurrentTransform < TransformBufferSize)
						setTransform(node->value);

					for (SizeType i = 0; i < node->children.getSize(); i++)
					{
						recurseCheck((const Tree::Node*)node->children[i]);
					}
				}
				static void recurse(Tree::Node* node, const Transform& parentTransform, FLAT_VECTOR<Transform>& resultTransforms)
				{
					Transform myTransform = Transform::multiply(parentTransform, node->value);
					resultTransforms.pushBack(myTransform);

					for (SizeType i = 0; i < node->children.getSize(); i++)
					{
						recurse((Tree::Node*)node->children[i], myTransform, resultTransforms);
					}
				}
				static void recurseChecked(Tree::Node* node, const Transform& parentTransform, FLAT_VECTOR<Transform>& resultTransforms)
				{
					Transform myTransform = Transform::multiply(parentTransform, node->value);
					resultTransforms.pushBack(myTransform);

					if (CheckHashes && CurrentTransform < TransformBufferSize)
						setTransform(myTransform);
					if (CheckHashes && CurrentHash < HashBufferSize)
						setHash(SuperFastHash((char*)resultTransforms.getPointer(), resultTransforms.getSize() * sizeof(Transform)));

					for (SizeType i = 0; i < node->children.getSize(); i++)
					{
						recurseChecked((Tree::Node*)node->children[i], myTransform, resultTransforms);
					}
				}
			};

			Tree tree;
			tree.root = tree.createNode(Transform(0, 0, 1, 1), NULL);

			// Create transforms
			for (SizeType aa = findCount(tree.root); aa < TransformTestSize; aa++)
			{
				SizeType parentNumber = Random::get(0, aa);
				Transform trns = getRandomTransform();

				Tree::Node* parent = NULL;
				{
					bool success = tryGetNthNode(tree.root, (RivalTreeNodeBase*&)parent, parentNumber);
					FLAT_ASSERT(success);
				}

				{
					ScopedProfiler prof(getStat(StatAdd));
					tree.createNode(trns, parent);
				}

			}

			// Check that transforms are correct
			{
				if (CheckHashes && IsFirstRound)
					Lolmbda::recurseCheck(tree.root);
			}

			FLAT_VECTOR<Transform> resultTransforms;
			resultTransforms.reserve(TransformTestSize);

			// Get results
			{

				for (SizeType iteration = 0; iteration < TransformIterations; iteration++)
				{
					resultTransforms.clear();

					ScopedProfiler prof(getStat(iteration == 0 ? StatTransformIt1 : StatTransformIt10));

					if (CheckHashes && iteration == 0 && IsFirstRound)
					{
						Lolmbda::recurseChecked(tree.root, Transform(), resultTransforms);
					}
					else
					{
						Lolmbda::recurse(tree.root, Transform(), resultTransforms);
					}
				}
			}

			if (IsFirstRound)
			{
				if (TranformTestResultHash == 0)
				{
					TranformTestResultHash = SuperFastHash((char*)resultTransforms.getPointer(), resultTransforms.getSize() * sizeof(Transform));
				}
				else
				{
					FLAT_ASSERT(TranformTestResultHash == SuperFastHash((char*)resultTransforms.getPointer(), resultTransforms.getSize() * sizeof(Transform)));
				}
				FLAT_ASSERT(TranformTestResultHash != 0);
			}

			// Count and depth, max and average
			{
				SizeType depth = findDepth(tree.root);
				SizeType count = findCount(tree.root);

				*getStat(StatCountAvg) += count;
				*getStat(StatDepthAvg) += depth;

				maxStat(StatCountMax, count);
				maxStat(StatDepthMax, depth);
			}
		}

		printf("Average Add                %.4lf\n", avgStat(StatAdd));
		printf("Average Transform once     %.4lf\n", avgStat(StatTransformIt1));
		printf("Average Transform 10 times %.4lf\n", avgStat(StatTransformIt1));
		printf("\n");
		printf("Average Depth              %.1lf\n", avgStat(StatDepthAvg));
		printf("Average Count              %.1lf\n", avgStat(StatCountAvg));
		printf("\n\n");
	}
}

void RIVAL_TransformTest()
{
	Other_Tree_TransformTest_Impl<RivalTree<Transform, TransformSorter> >();
}
void NAIVE_TransformTest()
{
	Other_Tree_TransformTest_Impl<NaiveTree<Transform, TransformSorter> >();
}




void test()
{
	enum
	{
		Flat1 = 1 << 0,
		Flat2 = 1 << 1,
		Flat3 = 1 << 2,
		Rival = 1 << 3,
		Naive = 1 << 4,
		Flat1Transform = 1 << 5,
		RivalTransform = 1 << 6,
		NaiveTransform = 1 << 7,
		Every = 1 << 8
	};

	const uint32_t TestMask
		=
		//Flat1 |
		//Flat2 |
		//Flat3 |
		//Rival |
		//Naive |
		Flat1Transform |
		RivalTransform |
		NaiveTransform |
		//Every |
#ifdef MAX_PERF
		Every |
#endif
		0;

	memset(LogBuffer, ' ', sizeof(LogBuffer));

	uint32_t seed = 1771551 * (SizeType)time(NULL);
	seed = 1771551;

	// Flat
	if ((TestMask & Flat1) != 0 || (TestMask & Every) != 0)
	{
		Rundi = 0;
		CurrentTreeType = 0;

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
		CurrentTreeType = 1;

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
		CurrentTreeType = 2;

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
		CurrentTreeType = 3;

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
		CurrentTreeType = 4;

		printf("\nBuffered Tree\n");
		Random::init(seed);
		RIVAL_Test();
		CheckingHashes = true;
		CurrentHash = 0;
		CurrentCounter = 0;
	}



	//
	// Transform tests
	//

	CurrentlyTestingTransforms = true;

	// Reset hashing for Transform test
	CheckingHashes = false;

	// Flat Transform
	if ((TestMask & Flat1Transform) != 0 || (TestMask & Every) != 0)
	{
		printf("\nFlat Transform\n");

		CurrentTreeType = 0;
		Rundi = 0;
		Random::init(seed);
		FLAT_TransformTest();
		CheckingHashes = true;
		CurrentHash = 0;
		CurrentCounter = 0;
		CurrentTransform = 0;
	}

	// Rival Transform
	if ((TestMask & RivalTransform) != 0 || (TestMask & Every) != 0)
	{
		printf("\nBuffered Tree Transform\n");

		Rundi = 0;
		CurrentTreeType = 1;

		Random::init(seed);
		RIVAL_TransformTest();
		CheckingHashes = true;
		CurrentHash = 0;
		CurrentCounter = 0;
		CurrentTransform = 0;
	}
	// Naive Transform
	if ((TestMask & NaiveTransform) != 0 || (TestMask & Every) != 0)
	{
		printf("\nNaive Tree Transform\n");

		Rundi = 0;
		CurrentTreeType = 2;

		Random::init(seed);
		NAIVE_TransformTest();
		CheckingHashes = true;
		CurrentHash = 0;
		CurrentCounter = 0;
		CurrentTransform = 0;
	}
	// Print summary about depth and count
	{
		CurrentlyTestingTransforms = false;
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
		logTable("\n");
	}

	for (SizeType baseOrTransform = 0; baseOrTransform < 2; baseOrTransform++)
	{
		char buffer[4096];

		bool LogBaseNow = baseOrTransform == 0;
		if (LogBaseNow)
		{
			sprintf_s(buffer, "Tree \tCount \tAdd \tMove \tErase \tFind max depth \tFind count \tNth node \tLeaf travel\n");
			logTable(buffer);
		}
		else
		{
			logTable("\nTree \tCount \tDepth average \tAdd \t");
			for (SizeType i = 0; i < ArrTransformIterationsCount; i++)
			{
				char buffer[64];
				sprintf_s(buffer, "Iterations: %d\t", ArrTransformIterations[i]);
				logTable(buffer);
			}
			logTable("\n");
		}

		for (SizeType treeSize = 0; treeSize < (LogBaseNow ? ArrBaseTestSizesCount : ArrTransformTestSizesCount); treeSize++)
		{
			for (SizeType treeType = 0; treeType < (LogBaseNow ? 5U : 3U); treeType++)
			{
				const char* treeName
					= treeType == 0 ? "Flat"
					: treeType == 1 ? (LogBaseNow ? "Flat Cached" : "Pooled")
					: treeType == 2 ? (LogBaseNow ? "Flat Cold" : "Naive")
					: treeType == 3 ? "Pooled"
					: "Naive";

				CurrentTreeType = treeType;
				CurrentTreeSize = treeSize;
				CurrentlyTestingTransforms = !LogBaseNow;

				if (LogBaseNow)
				{

					sprintf_s(buffer, "%s\t%.0lf\t", treeName, maxStat(StatCountMax));
					logTable(buffer);
					logTableDouble(avgStat(StatAdd));
					logTableDouble(avgStat(StatMove));
					logTableDouble(avgStat(StatErase));
					logTableDouble(avgStat(StatCalcDepth));
					logTableDouble(avgStat(StatCalcCount));
					logTableDouble(avgStat(StatNthNode));
					logTableDouble(avgStat(StatLeafTravel));
				}
				else
				{
					FLAT_ASSERT(maxStat(StatCountMax) == avgStat(StatCountAvg));
					sprintf_s(buffer, "%s \t%.0lf \t%.2lf \t", treeName, maxStat(StatCountMax), avgStat(StatDepthAvg));
					logTable(buffer);
					logTableDouble(avgStat(StatAdd));
					logTableDouble(avgStat(StatTransformIt1));
					logTableDouble(avgStat(StatTransformIt10));
				}
				logTable("\n");
			}
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
	}
#endif

	system("pause");
}
