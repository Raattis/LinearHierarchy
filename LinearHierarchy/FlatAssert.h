#pragma once

#include <string.h>   // strncpy, strlen
#include <stdlib.h>   // malloc
#include <stdio.h>    // printf, fgets

typedef unsigned long uint32_t;
typedef uint32_t SizeType;

struct String
{
	String(const char* ptr, size_t len)
	{
		this->ptr = (char*)malloc((len + 1) * sizeof(char));
		errno_t err = strncpy_s(this->ptr, (len + 1) * sizeof(char), ptr, len);
		//FLAT_ASSERT(err);
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
		//FLAT_ASSERT(err >= 0);
		err = strncpy_s(newPtr + oldLen, newLen + 1, other, newLen - oldLen);
		//FLAT_ASSERT(err >= 0);
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

#ifdef _WIN32
__declspec(noinline)
#endif
void flushCache()
{

	// Shuffle some memory around in unoptimizable order to cause the CPU cache to be flushed

	SizeType memSize = 6 * 1024 * 1024; // 6M to flush most of the CPU cache
	static SizeType* mem = NULL;
	if(mem == NULL)
		mem = (SizeType*)malloc(sizeof(SizeType) * memSize);

	const SizeType d = 4;           // Poke in this many places
	const SizeType p = memSize / d; // Every piece is this size

	// Short burst of aggressive jumping around in memory
	SizeType i = 0;
	for (; i < 200; i++)
	{
		SizeType p = memSize / 32;
		mem[i + 0 * p] = mem[i + 3 * p];
		mem[i + 2 * p] = mem[i + 1 * p];
		mem[i + 4 * p] = mem[i + 7 * p];
		mem[i + 6 * p] = mem[i + 5 * p];
		mem[i + 8 * p] = mem[i + 11 * p];
		mem[i + 10 * p] = mem[i + 9 * p];
		mem[i + 12 * p] = mem[i + 15 * p];
		mem[i + 14 * p] = mem[i + 13 * p];
		mem[i + 16 * p] = mem[i + 23 * p];
		mem[i + 18 * p] = mem[i + 21 * p];
		mem[i + 20 * p] = mem[i + 19 * p];
		mem[i + 22 * p] = mem[i + 17 * p];
		mem[i + 24 * p] = mem[i + 31 * p];
		mem[i + 26 * p] = mem[i + 29 * p];
		mem[i + 28 * p] = mem[i + 27 * p];
		mem[i + 30 * p] = mem[i + 25 * p];
	}

	// Long stretch of doing fever jumps in memory but handling that much larger chunk of it
	for (i = 0; i < p; i++)
	{
		mem[i + 0 * p] = mem[i + 3 * p];
		mem[i + 2 * p] = mem[i + 1 * p];
	
		if (d >= 8)
		{
			mem[i + 4 * p] = mem[i + 7 * p];
			mem[i + 6 * p] = mem[i + 5 * p];
		}
	
		if (d >= 12)
		{
			mem[i +  8 * p] = mem[i + 11 * p];
			mem[i + 10 * p] = mem[i +  9 * p];
		}
	
		if (d >= 16)
		{
			mem[i + 12 * p] = mem[i + 15 * p];
			mem[i + 14 * p] = mem[i + 13 * p];
		}
	
		if (d >= 24)
		{
			mem[i + 16 * p] = mem[i + 23 * p];
			mem[i + 18 * p] = mem[i + 21 * p];
			mem[i + 20 * p] = mem[i + 19 * p];
			mem[i + 22 * p] = mem[i + 17 * p];
		}
		if (d >= 32)
		{
			mem[i + 24 * p] = mem[i + 31 * p];
			mem[i + 26 * p] = mem[i + 29 * p];
			mem[i + 28 * p] = mem[i + 27 * p];
			mem[i + 30 * p] = mem[i + 25 * p];
		}
	}

	// Some more aggressive shuffling just for good measure
	for (i = 0; i < memSize / 32; i += memSize / 32 / 10)
	{
		SizeType p = memSize / 32;
		mem[i + 0 * p] = mem[i + 3 * p];
		mem[i + 2 * p] = mem[i + 1 * p];
		mem[i + 4 * p] = mem[i + 7 * p];
		mem[i + 6 * p] = mem[i + 5 * p];
		mem[i + 8 * p] = mem[i + 11 * p];
		mem[i + 10 * p] = mem[i + 9 * p];
		mem[i + 12 * p] = mem[i + 15 * p];
		mem[i + 14 * p] = mem[i + 13 * p];
		mem[i + 16 * p] = mem[i + 23 * p];
		mem[i + 18 * p] = mem[i + 21 * p];
		mem[i + 20 * p] = mem[i + 19 * p];
		mem[i + 22 * p] = mem[i + 17 * p];
		mem[i + 24 * p] = mem[i + 31 * p];
		mem[i + 26 * p] = mem[i + 29 * p];
		mem[i + 28 * p] = mem[i + 27 * p];
		mem[i + 30 * p] = mem[i + 25 * p];
	}

}
