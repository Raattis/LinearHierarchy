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

	char* ptr;
};