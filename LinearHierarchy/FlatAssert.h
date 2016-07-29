#pragma once

#include <string.h>   // strncpy, strlen
#include <stdlib.h>   // malloc
#include <stdio.h>    // printf, fgets
#include <inttypes.h> // uint32_t

#if FLAT_ASSERTS_ENABLED == true
	#define FLAT_ASSERT_IMPL(expr, fmt, ...) \
		do{ if(!(expr)) { \
			printf("\nAssertion failed.\nExpr: \"%s\"\nin %s() at %s:%u\n", #expr, __FUNCTION__, __FILE__, __LINE__); \
			printf(fmt, ##__VA_ARGS__); \
			system("pause"); \
			__debugbreak(); \
		} }while(false)

	#define FLAT_ASSERT(expr) FLAT_ASSERT_IMPL(expr, "%s", "")
	#define FLAT_ASSERTF(expr, fmt, ...) FLAT_ASSERT_IMPL(expr, "Message: " fmt "\n", __VA_ARGS__)
#else
	#define FLAT_ASSERT(expr) do{}while(false)
	#define FLAT_ASSERTF(expr, fmt, ...) do{}while(false)
#endif

typedef uint32_t SizeType;

struct String
{
	String(const char* ptr, size_t len)
	{
		this->ptr = (char*)malloc((len + 1) * sizeof(char));
		errno_t err = strncpy_s(this->ptr, (len + 1) * sizeof(char), ptr, len);
		FLAT_ASSERT(err);
		this->ptr[len] = '\0';
	}
	String(const char* ptr)
		: String(ptr, strlen(ptr))
	{
	}
	String(const String& other)
		: String(other.ptr)
	{
	}
	~String()
	{
		free(ptr);
	}
	operator const char*() const
	{
		return ptr;
	}
	void operator +=(const char* other)
	{
		size_t oldLen = strlen(this->ptr);
		size_t newLen = oldLen + strlen(other);
		char* newPtr = (char*)malloc((newLen + 1) * sizeof(char));
		errno_t err = strncpy_s(newPtr, newLen + 1, this->ptr, oldLen);
		FLAT_ASSERT(err >= 0);
		err = strncpy_s(newPtr + oldLen, newLen + 1, other, newLen - oldLen);
		FLAT_ASSERT(err >= 0);
		newPtr[newLen] = '\0';
		free(ptr);
		ptr = newPtr;
	}

	void operator +=(float f)
	{
		char buffer[128];
		sprintf_s(buffer, "%f", f);
		operator +=(buffer);
	}

	char* ptr;
};

__declspec(noinline) void pingMemory()
{
	SizeType memSize = 6 * 1024 * 1024; // 6M to flush most of the CPU cache
	static SizeType* lol_mem = NULL;
	if(lol_mem == NULL)
		lol_mem = (SizeType*)malloc(sizeof(SizeType) * memSize);

	const SizeType d = 4;
	const SizeType p = memSize / d;

	SizeType i = 0;
	for (; i < 200; i++)
	{
		SizeType p = memSize / 32;
		lol_mem[i + 0 * p] = lol_mem[i + 3 * p];
		lol_mem[i + 2 * p] = lol_mem[i + 1 * p];

		//if (d >= 8)
		{
			lol_mem[i + 4 * p] = lol_mem[i + 7 * p];
			lol_mem[i + 6 * p] = lol_mem[i + 5 * p];
		}

		//if (d >= 12)
		{
			lol_mem[i + 8 * p] = lol_mem[i + 11 * p];
			lol_mem[i + 10 * p] = lol_mem[i + 9 * p];
		}

		//if (d >= 16)
		{
			lol_mem[i + 12 * p] = lol_mem[i + 15 * p];
			lol_mem[i + 14 * p] = lol_mem[i + 13 * p];
		}

		//if (d >= 24)
		{
			lol_mem[i + 16 * p] = lol_mem[i + 23 * p];
			lol_mem[i + 18 * p] = lol_mem[i + 21 * p];
			lol_mem[i + 20 * p] = lol_mem[i + 19 * p];
			lol_mem[i + 22 * p] = lol_mem[i + 17 * p];
		}
		//if (d >= 32)
		{
			lol_mem[i + 24 * p] = lol_mem[i + 31 * p];
			lol_mem[i + 26 * p] = lol_mem[i + 29 * p];
			lol_mem[i + 28 * p] = lol_mem[i + 27 * p];
			lol_mem[i + 30 * p] = lol_mem[i + 25 * p];
		}
	}

	for (i = 0 / 2; i < p; i++)
	{
		lol_mem[i + 0 * p] = lol_mem[i + 3 * p];
		lol_mem[i + 2 * p] = lol_mem[i + 1 * p];
	
		if (d >= 8)
		{
			lol_mem[i + 4 * p] = lol_mem[i + 7 * p];
			lol_mem[i + 6 * p] = lol_mem[i + 5 * p];
		}
	
		if (d >= 12)
		{
			lol_mem[i +  8 * p] = lol_mem[i + 11 * p];
			lol_mem[i + 10 * p] = lol_mem[i +  9 * p];
		}
	
		if (d >= 16)
		{
			lol_mem[i + 12 * p] = lol_mem[i + 15 * p];
			lol_mem[i + 14 * p] = lol_mem[i + 13 * p];
		}
	
		if (d >= 24)
		{
			lol_mem[i + 16 * p] = lol_mem[i + 23 * p];
			lol_mem[i + 18 * p] = lol_mem[i + 21 * p];
			lol_mem[i + 20 * p] = lol_mem[i + 19 * p];
			lol_mem[i + 22 * p] = lol_mem[i + 17 * p];
		}
		if (d >= 32)
		{
			lol_mem[i + 24 * p] = lol_mem[i + 31 * p];
			lol_mem[i + 26 * p] = lol_mem[i + 29 * p];
			lol_mem[i + 28 * p] = lol_mem[i + 27 * p];
			lol_mem[i + 30 * p] = lol_mem[i + 25 * p];
		}
	}

	for (i = 0; i < memSize / 32; i += memSize / 32 / 10)
	{
		SizeType p = memSize / 32;
		lol_mem[i + 0 * p] = lol_mem[i + 3 * p];
		lol_mem[i + 2 * p] = lol_mem[i + 1 * p];

		//if (d >= 8)
		{
			lol_mem[i + 4 * p] = lol_mem[i + 7 * p];
			lol_mem[i + 6 * p] = lol_mem[i + 5 * p];
		}

		//if (d >= 12)
		{
			lol_mem[i + 8 * p] = lol_mem[i + 11 * p];
			lol_mem[i + 10 * p] = lol_mem[i + 9 * p];
		}

		//if (d >= 16)
		{
			lol_mem[i + 12 * p] = lol_mem[i + 15 * p];
			lol_mem[i + 14 * p] = lol_mem[i + 13 * p];
		}

		//if (d >= 24)
		{
			lol_mem[i + 16 * p] = lol_mem[i + 23 * p];
			lol_mem[i + 18 * p] = lol_mem[i + 21 * p];
			lol_mem[i + 20 * p] = lol_mem[i + 19 * p];
			lol_mem[i + 22 * p] = lol_mem[i + 17 * p];
		}
		//if (d >= 32)
		{
			lol_mem[i + 24 * p] = lol_mem[i + 31 * p];
			lol_mem[i + 26 * p] = lol_mem[i + 29 * p];
			lol_mem[i + 28 * p] = lol_mem[i + 27 * p];
			lol_mem[i + 30 * p] = lol_mem[i + 25 * p];
		}
	}

}

//#pragma optimize("", off)
__declspec(noinline) void flushCache()
{
	// /*
	static volatile size_t lol_number = 0;
	if(lol_number == 0)
		lol_number = (size_t)(uintptr_t)malloc(1);
	//volatile SizeType buffer[10 * 1000]; // 256K
	size_t f = 0, g = 0, h = 0, i = 0, j = 0;
#define a(p) f = f; g = g; h = h; i = i; j = j;
#define b(p) a(p) a(p+5) a(p+10) a(p+15)
#define c(p) b(p) b(p+20) b(p+40) b(p+60) b(p+80) 
#define d(p) c(p) c(p+100) c(p+200) c(p+300) c(p+400)
#define	e(p) d(p+0) d(p+500) d(p+1000) d(p+1500) d(p+2000)
	//for (SizeType i = 0; i + 10000 <= sizeof(buffer) / sizeof(SizeType); i += 10000)
	{
		// 20k times 2 or 3 instructions
		//e(i +     0)e(i +  2500)e(i +  5000)e(i +  7500)
		//e(i + 10000)e(i + 12500)e(i + 15000)e(i + 17500)
		//e(i + 10000)e(i + 12500)e(i + 15000)e(i + 17500)
		//e(i + 10000)e(i + 12500)e(i + 15000)e(i + 17500)
		//e(i + 10000)e(i + 12500)e(i + 15000)e(i + 17500)
		//e(i + 10000)e(i + 12500)e(i + 15000)e(i + 17500)
		//e(i + 10000)e(i + 12500)e(i + 15000)e(i + 17500)
		//e(i + 10000)e(i + 12500)e(i + 15000)e(i + 17500)
		//e(i + 10000)e(i + 12500)e(i + 15000)e(i + 17500)
		//e(i + 10000)e(i + 12500)e(i + 15000)e(i + 17500)
		//e(i + 10000)e(i + 12500)e(i + 15000)e(i + 17500)
	}
#undef e
#undef d
#undef c
#undef b
#undef a

	pingMemory();
	// */
}
//#pragma optimize("", on)
