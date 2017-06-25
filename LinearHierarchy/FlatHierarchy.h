#ifndef FLAT_FLATHIERARCHY_H
#define FLAT_FLATHIERARCHY_H

//------------------------------------------------------------------
// FlatHierarchy.h
//
// August 2016, Riku Rajaniemi
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//------------------------------------------------------------------

#if FLAT_ASSERTS_ENABLED == true
	
	#ifdef _WIN32
		#if defined(FLAT_ALLOW_INCLUDES)
			#include <stdio.h>    // printf
			#include <windows.h>  // system
			#define FLAT_ASSERT_IMPL(expr, fmt, ...) \
				do{ if(!(expr)) { \
					printf("\nAssertion failed.\nExpr: \"%s\"\nin %s() at %s:%u\n", #expr, __FUNCTION__, __FILE__, __LINE__); \
					printf(fmt, ##__VA_ARGS__); \
					system("pause"); \
					__debugbreak(); \
				} }while(false)
		#else
			#define FLAT_ASSERT_IMPL(expr, fmt, ...) \
				do{ if(!(expr)) { \
					__debugbreak(); \
				} }while(false)		
		#endif
		#define FLAT_ASSERT(expr) FLAT_ASSERT_IMPL(expr, "%s", "")
		#define FLAT_ASSERTF(expr, fmt, ...) FLAT_ASSERT_IMPL(expr, "Message: " fmt "\n", __VA_ARGS__)

	#else // _WIN32

		#include <assert.h>
		
		#define FLAT_ASSERT_IMPL(expr, fmt, ...) \
			do{ if(!(expr)) { \
				printf("\nAssertion failed.\nExpr: \"%s\"\nin %s() at %s:%u\n", #expr, __FUNCTION__, __FILE__, __LINE__); \
				printf(fmt, ##__VA_ARGS__); \
				assert(false); \
			} }while (false)

		#define FLAT_ASSERT(expr) FLAT_ASSERT_IMPL(expr, "%s", "")
		#define FLAT_ASSERTF(expr, fmt, ...) FLAT_ASSERT_IMPL(expr, "Message: " fmt "\n", __VA_ARGS__)

	#endif // _WIN32

#else

	#define FLAT_ASSERT(expr) do{}while(false)
	#define FLAT_ASSERTF(expr, fmt, ...) do{}while(false)

#endif

#ifndef FLAT_ERROR
	#define FLAT_ERROR(p) FLAT_ASSERT(0 && (p))
#endif

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long int uint32_t;
typedef unsigned long long int uint64_t;
typedef unsigned long long int uintptr_t;

static_assert(sizeof(uint8_t) == 1, "uint8_t isn't actually 1 byte long");
static_assert(sizeof(uint16_t) == 2, "uint16_t isn't actually 2 bytes long");
static_assert(sizeof(uint32_t) == 4, "uint32_t isn't actually 4 bytes long");
static_assert(sizeof(uint64_t) == 8, "uint64_t isn't actually 8 bytes long");
static_assert(sizeof(uintptr_t) == sizeof(void*), "uintptr_t isn't actually pointer lenght");


// Exclude most significant bit to catch roll over errors
#define FLAT_MAXDEPTH ((FLAT_DEPTHTYPE)(~0) >> 1)


#if FLAT_ALLOW_INCLUDES == true
	#ifndef FLAT_MEMCPY
		#include <string.h> /* memcpy, memmove*/
		#define FLAT_MEMCPY(dst, src, length) memcpy(dst, src, length)
	#endif
	#ifndef FLAT_MEMMOVE
		#include <string.h> /* memcpy, memmove*/
		#define FLAT_MEMMOVE(dst, src, length) memmove(dst, src, length)
	#endif
#else
	#ifndef FLAT_MEMCPY
		inline void flat_memcpy_impl(char* dst, const char* src, uint32_t length)
		{
			FLAT_ASSERT(dst != 0);
			FLAT_ASSERT(src != 0);
			FLAT_ASSERT(src != dst);
			FLAT_ASSERT(dst + length <= src || src + length <= dst);

			for (uint32_t i = 0; i < length; i++)
			{
				dst[i] = src[i];
			}
		}
		#define FLAT_MEMCPY(dst, src, length) flat_memcpy_impl((char*)(dst), (const char*)(src), length)
	#endif

	#ifndef FLAT_MEMMOVE
		inline void flat_memmove_impl(char* dst, const char* src, uint32_t length)
		{
			FLAT_ASSERT(dst != 0);
			FLAT_ASSERT(src != 0);
			FLAT_ASSERT(src != dst);

			if (dst + length <= src || src + length <= dst)
			{
				// No overlap
				FLAT_MEMCPY(dst, src, length);
			}
			else if(dst < src)
			{
				for (uint32_t i = 0; i < length; i++)
				{
					dst[i] = src[i];
				}
			}
			else
			{
				for (uint32_t i = length; i-- > 0;)
				{
					dst[i] = src[i];
				}
			}
		}
		#define FLAT_MEMMOVE(dst, src, length) flat_memmove_impl((char*)(dst), (const char*)(src), length)
	#endif
#endif

#ifndef FLAT_USE_SIMD
#if FLAT_ALLOW_INCLUDES == true
	#define FLAT_USE_SIMD true
#else
	#define FLAT_USE_SIMD false
#endif
#endif

#if FLAT_USE_SIMD == true
	#if FLAT_ALLOW_INCLUDES == true
		#include <immintrin.h>
	#else
		#error "Cannot use SIMD without includes. Define FLAT_ALLOW_INCLUDES as true or do NOT define FLAT_USE_SIMD as true"
	#endif
#endif

#define FLAT_SIZETYPE uint32_t
#define FLAT_DEPTHTYPE uint16_t

#ifndef FLAT_ALLOC
#ifdef _WIN32
	#define FLAT_ALLOC(size) _aligned_malloc(size, 16)
	#define FLAT_FREE(ptr) _aligned_free(ptr)
#else
	#define FLAT_ALLOC(size) malloc(size);
	#define FLAT_FREE(ptr) free(ptr)
#endif
#endif

#ifndef FLAT_FREE
	#error "FLAT_ALLOC was defined but FLAT_FREE was not"
#endif

#ifndef FLAT_VECTOR
	template<typename ValueType>
	class flat_vector_impl
	{
		flat_vector_impl(const flat_vector_impl&) { } // private move constructor to avoid mistakes
		void operator=(const flat_vector_impl&) { }   // private move assignment to avoid mistakes
	public:
		typedef uint32_t SizeType;

		flat_vector_impl()
			: buffer(nullptr)
			, size(0)
			, capacity(0)
		{
		}
		~flat_vector_impl()
		{
			if (buffer != nullptr)
			{
				FLAT_FREE(buffer);
			}

		}

		SizeType size;
		SizeType capacity;
		ValueType* buffer;

		SizeType getSize() const { return size; }
		SizeType getCapacity() const { return capacity; }

		ValueType* getPointer() { return buffer; }
		const ValueType* getPointer() const { return buffer; }

		ValueType& getBack() { FLAT_ASSERT(size > 0); return buffer[size - 1]; }
		const ValueType& getBack() const { FLAT_ASSERT(size > 0); return buffer[size - 1]; }

		void clear() { size = 0; }
		void resize(SizeType newSize) { FLAT_ASSERT(newSize < ((SizeType)(~0) >> 1)); reserve(newSize); size = newSize; }
		ValueType operator[] (SizeType index) const { FLAT_ASSERT(index < size); return buffer[index]; }
		ValueType& operator[] (SizeType index) { FLAT_ASSERT(index < size); return buffer[index]; }

		void reserve(SizeType capacity)
		{
			if (capacity > this->capacity)
			{
				ValueType* temp = (ValueType*)FLAT_ALLOC( capacity * sizeof(ValueType) );
				if (buffer != nullptr)
				{
					FLAT_MEMCPY((char*)temp, (char*)buffer, size * sizeof(ValueType));
					FLAT_FREE(buffer);
				}

				buffer = temp;
				this->capacity = capacity;
			}
		}

		void pushBack(const ValueType& v)
		{
			if (size < capacity)
			{
				buffer[size++] = v;
			}
			else
			{
				reserve((1 + size) * 2);
				buffer[size++] = v;
			}
		}
		void insert(SizeType index, const ValueType& v)
		{
			FLAT_ASSERT(index <= size);

			if (size < capacity)
			{
				FLAT_MEMMOVE(buffer + index + 1, buffer + index, (size - index) * sizeof(ValueType));
			}
			else if (buffer != nullptr)
			{
				FLAT_ASSERT(capacity == size);
				FLAT_ASSERT(capacity > 0);

				capacity = capacity * 2;

				ValueType* temp = (ValueType*)FLAT_ALLOC(capacity * sizeof(ValueType));
				FLAT_MEMCPY(temp,             buffer,          index         * sizeof(ValueType));
				FLAT_MEMCPY(temp + index + 1, buffer + index, (size - index) * sizeof(ValueType));
				FLAT_FREE(buffer);
				buffer = temp;
			}
			else
			{
				FLAT_ASSERT(size == 0);
				FLAT_ASSERT(capacity == 0);

				capacity = 16;
				buffer = (ValueType*)FLAT_ALLOC(capacity * sizeof(ValueType));
			}

			buffer[index] = v;
			++size;
		}

		void zero(SizeType start, SizeType end)
		{
			FLAT_ASSERT(end <= size);
			for (SizeType i = start; i < end; i++)
			{
				buffer[i] = 0;
			}
		}
		void erase(SizeType index)
		{
			FLAT_ASSERT(index < size);
			
			FLAT_MEMMOVE(buffer + index, buffer + index + 1, (size - index - 1) *  sizeof(ValueType));
			--size;
		}
	};
	
	#define FLAT_VECTOR flat_vector_impl
#endif

/////////////////////////////////////////////////////////////////
//
// Base implementation containing only a depths-vector
// Mostly used by cache structures to determine node relations
//
/////////////////////////////////////////////////////////////////
class FlatHierarchyBase
{
public:
	typedef FLAT_SIZETYPE SizeType;
	typedef FLAT_DEPTHTYPE DepthValue;
	typedef SizeType HierarchyIndex;

	FLAT_VECTOR<DepthValue> depths;



	SizeType getCount() const
	{
		return depths.getSize();
	}



	__declspec(noinline)
		DepthValue findMaxDepth() const
	{
		DepthValue result = 0;
#if FLAT_USE_SIMD == false
		for (HierarchyIndex i = 0; i < getCount(); i++)
		{
			if (depths[i] > result)
				result = depths[i];
		}
		return result;
#else
		enum { Bytes = sizeof(DepthValue) }; // Make Bytes an enum to ensure it is used as compile-time constant
		static_assert(Bytes == 1 || Bytes == 2 || Bytes == 4 || Bytes > 4, "Byte count mismatch");

		if (Bytes <= 4) // DepthValue == uint8_t / uint16_t / uint32_t
		{
			FLAT_ASSERT(Bytes == 1 || Bytes == 2 || Bytes == 4);

			SizeType bytesLeft = getCount() * Bytes;
			const uint8_t* data = (const uint8_t*)depths.getPointer();

			FLAT_ASSERT((((uintptr_t)data) & 0xF) == 0);

			while (bytesLeft > 0 && (((uintptr_t)data) & 0xF) != 0)
			{
				if (result < *data)
					result = *data;
				data += Bytes;
				bytesLeft -= Bytes;
			}

			FLAT_ASSERT((((uintptr_t)data) & 0xF) == 0);

			__m128i res = _mm_setzero_si128();
			__m128i max0 = _mm_setzero_si128();
			__m128i max1 = _mm_setzero_si128();
			__m128i max2 = _mm_setzero_si128();
			__m128i max3 = _mm_setzero_si128();

			while (bytesLeft >= 64)
			{
				__m128i in0 = _mm_load_si128((const __m128i*)data + 0);
				__m128i in1 = _mm_load_si128((const __m128i*)data + 1);
				__m128i in2 = _mm_load_si128((const __m128i*)data + 2);
				__m128i in3 = _mm_load_si128((const __m128i*)data + 3);

				if (Bytes == 1) max0 = _mm_max_epu8(max0, in0);
				if (Bytes == 1) max1 = _mm_max_epu8(max1, in1);
				if (Bytes == 1) max2 = _mm_max_epu8(max2, in2);
				if (Bytes == 1) max3 = _mm_max_epu8(max3, in3);

				if (Bytes == 2) max0 = _mm_max_epu16(max0, in0);
				if (Bytes == 2) max1 = _mm_max_epu16(max1, in1);
				if (Bytes == 2) max2 = _mm_max_epu16(max2, in2);
				if (Bytes == 2) max3 = _mm_max_epu16(max3, in3);

				if (Bytes == 4) max0 = _mm_max_epu32(max0, in0);
				if (Bytes == 4) max1 = _mm_max_epu32(max1, in1);
				if (Bytes == 4) max2 = _mm_max_epu32(max2, in2);
				if (Bytes == 4) max3 = _mm_max_epu32(max3, in3);

				data += 64;
				bytesLeft -= 64;
			}

			if (bytesLeft >= 48)
			{
				__m128i in0 = _mm_load_si128((const __m128i*)data + 0);
				__m128i in1 = _mm_load_si128((const __m128i*)data + 1);
				__m128i in2 = _mm_load_si128((const __m128i*)data + 2);

				if (Bytes == 1) max0 = _mm_max_epu8(max0, in0);
				if (Bytes == 1) max1 = _mm_max_epu8(max1, in1);
				if (Bytes == 1) max2 = _mm_max_epu8(max2, in2);

				if (Bytes == 2) max0 = _mm_max_epu16(max0, in0);
				if (Bytes == 2) max1 = _mm_max_epu16(max1, in1);
				if (Bytes == 2) max2 = _mm_max_epu16(max2, in2);

				if (Bytes == 4) max0 = _mm_max_epu32(max0, in0);
				if (Bytes == 4) max1 = _mm_max_epu32(max1, in1);
				if (Bytes == 4) max2 = _mm_max_epu32(max2, in2);

				data += 48;
				bytesLeft -= 48;
			}
			else if (bytesLeft >= 32)
			{
				__m128i in0 = _mm_load_si128((const __m128i*)data + 0);
				__m128i in1 = _mm_load_si128((const __m128i*)data + 1);

				if (Bytes == 1) max0 = _mm_max_epu8(max0, in0);
				if (Bytes == 1) max1 = _mm_max_epu8(max1, in1);

				if (Bytes == 2) max0 = _mm_max_epu16(max0, in0);
				if (Bytes == 2) max1 = _mm_max_epu16(max1, in1);

				if (Bytes == 4) max0 = _mm_max_epu32(max0, in0);
				if (Bytes == 4) max1 = _mm_max_epu32(max1, in1);

				data += 32;
				bytesLeft -= 32;
			}
			else if (bytesLeft >= 16)
			{
				__m128i in0 = _mm_load_si128((const __m128i*)data + 0);

				if (Bytes == 1) max0 = _mm_max_epu8(max0, in0);
				if (Bytes == 2) max0 = _mm_max_epu16(max0, in0);
				if (Bytes == 4) max0 = _mm_max_epu32(max0, in0);

				data += 16;
				bytesLeft -= 16;
			}

			// Collect all max values
			{
				if (Bytes == 1) res = _mm_max_epu8(_mm_max_epu8(max0, max1), _mm_max_epu8(max2, max3));
				if (Bytes == 2) res = _mm_max_epu8(_mm_max_epu8(max0, max1), _mm_max_epu8(max2, max3));
				if (Bytes == 4) res = _mm_max_epu8(_mm_max_epu8(max0, max1), _mm_max_epu8(max2, max3));
			}

			// Shift and compare max values to last byte
			{
				if (Bytes == 1) res = _mm_max_epu8(res, _mm_srli_si128(res, 8));
				if (Bytes == 1) res = _mm_max_epu8(res, _mm_srli_si128(res, 4));
				if (Bytes == 1) res = _mm_max_epu8(res, _mm_srli_si128(res, 2));
				if (Bytes == 1) res = _mm_max_epu8(res, _mm_srli_si128(res, 1));

				if (Bytes == 2) res = _mm_max_epu16(res, _mm_srli_si128(res, 8));
				if (Bytes == 2) res = _mm_max_epu16(res, _mm_srli_si128(res, 4));
				if (Bytes == 2) res = _mm_max_epu16(res, _mm_srli_si128(res, 2));

				if (Bytes == 4) res = _mm_max_epu32(res, _mm_srli_si128(res, 8));
				if (Bytes == 4) res = _mm_max_epu32(res, _mm_srli_si128(res, 4));
			}
			result = (DepthValue)_mm_cvtsi128_si32(res);

			while (bytesLeft > 0)
			{
				FLAT_ASSERT(bytesLeft >= Bytes);

				if (result < *(DepthValue*)data)
					result = *(DepthValue*)data;
				data += Bytes;
				bytesLeft -= Bytes;
			}
		}
		else // sizeof(DepthValue) > 4
		{
			for (HierarchyIndex i = 0; i < getCount(); i++)
			{
				if (depths[i] > result)
					result = depths[i];
			}
		}
#ifdef _DEBUG
		{ // Correctness check
			SizeType check = 0;
			for (HierarchyIndex i = 0; i < getCount(); i++)
			{
				if (check < depths[i])
					check = depths[i];
			}
			FLAT_ASSERT(check == result);
		}
#endif
		return result;
#endif
	}

	__declspec(noinline)
		DepthValue findMinDepthBetween(HierarchyIndex first, HierarchyIndex last) const
	{
		FLAT_ASSERT(last < getCount());
		FLAT_ASSERT(first <= last);

		DepthValue result = 0;
#if FLAT_USE_SIMD == false
		for (HierarchyIndex i = first; i <= last; i++)
		{
			if (depths[i] < result)
				result = depths[i];
		}
		return result;
#else
		enum { Bytes = sizeof(DepthValue) }; // Make Bytes an enum to ensure it is used as compile-time constant
		static_assert(Bytes == 1 || Bytes == 2 || Bytes == 4 || Bytes > 4, "Byte count mismatch");

		if (Bytes <= 4 && first + 16 / Bytes <= last) // DepthValue == uint8_t / uint16_t / uint32_t
		{
			FLAT_ASSERT(Bytes == 1 || Bytes == 2 || Bytes == 4);

			SizeType bytesLeft = (last + 1 - first) * Bytes;

			FLAT_ASSERT(bytesLeft / Bytes < getCount());

			const uint8_t* data = (const uint8_t*)depths.getPointer() + first;

			while (bytesLeft > 0 && (((uintptr_t)data) & 0xF) != 0)
			{
				if (result > *data)
					result = *data;
				data += Bytes;
				bytesLeft -= Bytes;
			}

			__m128i res = _mm_setzero_si128();
			__m128i min0 = _mm_setzero_si128();
			__m128i min1 = _mm_setzero_si128();
			__m128i min2 = _mm_setzero_si128();
			__m128i min3 = _mm_setzero_si128();

			while (bytesLeft >= 64)
			{
				__m128i in0 = _mm_load_si128((const __m128i*)data + 0);
				__m128i in1 = _mm_load_si128((const __m128i*)data + 1);
				__m128i in2 = _mm_load_si128((const __m128i*)data + 2);
				__m128i in3 = _mm_load_si128((const __m128i*)data + 3);

				if (Bytes == 1) min0 = _mm_min_epu8(min0, in0);
				if (Bytes == 1) min1 = _mm_min_epu8(min1, in1);
				if (Bytes == 1) min2 = _mm_min_epu8(min2, in2);
				if (Bytes == 1) min3 = _mm_min_epu8(min3, in3);

				if (Bytes == 2) min0 = _mm_min_epu16(min0, in0);
				if (Bytes == 2) min1 = _mm_min_epu16(min1, in1);
				if (Bytes == 2) min2 = _mm_min_epu16(min2, in2);
				if (Bytes == 2) min3 = _mm_min_epu16(min3, in3);

				if (Bytes == 4) min0 = _mm_min_epu32(min0, in0);
				if (Bytes == 4) min1 = _mm_min_epu32(min1, in1);
				if (Bytes == 4) min2 = _mm_min_epu32(min2, in2);
				if (Bytes == 4) min3 = _mm_min_epu32(min3, in3);

				data += 64;
				bytesLeft -= 64;
			}

			if (bytesLeft >= 48)
			{
				__m128i in0 = _mm_load_si128((const __m128i*)data + 0);
				__m128i in1 = _mm_load_si128((const __m128i*)data + 1);
				__m128i in2 = _mm_load_si128((const __m128i*)data + 2);

				if (Bytes == 1) min0 = _mm_min_epu8(min0, in0);
				if (Bytes == 1) min1 = _mm_min_epu8(min1, in1);
				if (Bytes == 1) min2 = _mm_min_epu8(min2, in2);

				if (Bytes == 2) min0 = _mm_min_epu16(min0, in0);
				if (Bytes == 2) min1 = _mm_min_epu16(min1, in1);
				if (Bytes == 2) min2 = _mm_min_epu16(min2, in2);

				if (Bytes == 4) min0 = _mm_min_epu32(min0, in0);
				if (Bytes == 4) min1 = _mm_min_epu32(min1, in1);
				if (Bytes == 4) min2 = _mm_min_epu32(min2, in2);

				data += 48;
				bytesLeft -= 48;
			}
			else if (bytesLeft >= 32)
			{
				__m128i in0 = _mm_load_si128((const __m128i*)data + 0);
				__m128i in1 = _mm_load_si128((const __m128i*)data + 1);

				if (Bytes == 1) min0 = _mm_min_epu8(min0, in0);
				if (Bytes == 1) min1 = _mm_min_epu8(min1, in1);

				if (Bytes == 2) min0 = _mm_min_epu16(min0, in0);
				if (Bytes == 2) min1 = _mm_min_epu16(min1, in1);

				if (Bytes == 4) min0 = _mm_min_epu32(min0, in0);
				if (Bytes == 4) min1 = _mm_min_epu32(min1, in1);

				data += 32;
				bytesLeft -= 32;
			}
			else if (bytesLeft >= 16)
			{
				__m128i in0 = _mm_load_si128((const __m128i*)data + 0);

				if (Bytes == 1) min0 = _mm_min_epu8(min0, in0);
				if (Bytes == 2) min0 = _mm_min_epu16(min0, in0);
				if (Bytes == 4) min0 = _mm_min_epu32(min0, in0);

				data += 16;
				bytesLeft -= 16;
			}

			// Collect all min values
			{
				if (Bytes == 1) res = _mm_min_epu8(_mm_min_epu8(min0, min1), _mm_min_epu8(min2, min3));
				if (Bytes == 2) res = _mm_min_epu8(_mm_min_epu8(min0, min1), _mm_min_epu8(min2, min3));
				if (Bytes == 4) res = _mm_min_epu8(_mm_min_epu8(min0, min1), _mm_min_epu8(min2, min3));
			}

			// Shift and compare min values to last byte
			{
				if (Bytes == 1) res = _mm_min_epu8(res, _mm_srli_si128(res, 8));
				if (Bytes == 1) res = _mm_min_epu8(res, _mm_srli_si128(res, 4));
				if (Bytes == 1) res = _mm_min_epu8(res, _mm_srli_si128(res, 2));
				if (Bytes == 1) res = _mm_min_epu8(res, _mm_srli_si128(res, 1));

				if (Bytes == 2) res = _mm_min_epu16(res, _mm_srli_si128(res, 8));
				if (Bytes == 2) res = _mm_min_epu16(res, _mm_srli_si128(res, 4));
				if (Bytes == 2) res = _mm_min_epu16(res, _mm_srli_si128(res, 2));

				if (Bytes == 4) res = _mm_min_epu32(res, _mm_srli_si128(res, 8));
				if (Bytes == 4) res = _mm_min_epu32(res, _mm_srli_si128(res, 4));
			}
			result = (DepthValue)_mm_cvtsi128_si32(res);

			while (bytesLeft > 0)
			{
				FLAT_ASSERT(bytesLeft >= Bytes);

				if (result > *(DepthValue*)data)
					result = *(DepthValue*)data;
				data += Bytes;
				bytesLeft -= Bytes;
			}
		}
		else // sizeof(DepthValue) > 4
		{
			for (HierarchyIndex i = first; i <= last; i++)
			{
				if (result > depths[i])
					result = depths[i];
			}
			return result;
		}
#ifdef _DEBUG
		{ // Correctness check
			SizeType check = 0;
			for (HierarchyIndex i = first; i <= last; i++)
			{
				if (check > depths[i])
					check = depths[i];
			}
			FLAT_ASSERT(check == result);
		}
#endif
		return result;
#endif
	}

	static HierarchyIndex getIndexNotFound() { return HierarchyIndex(~0); }
};





struct DefaultSorter
{
	static const bool UseSorting = true;

	template<typename T>
	inline static bool isFirst(const T& a, const T& b) { return a < b; }
};

template<typename ValueType, typename Sorter = DefaultSorter>
class FlatHierarchy : public FlatHierarchyBase
{
public:

	FLAT_VECTOR<ValueType> values;

	FlatHierarchy(SizeType reserveSize = 0)
	{
		values.reserve(reserveSize);
		depths.reserve(reserveSize);
	}
	~FlatHierarchy()
	{
	}

	HierarchyIndex createRootNode(const ValueType& value)
	{
		HierarchyIndex newIndex = getCount();

		if (Sorter::UseSorting == true)
		{
			// Insert to sorted position
			DepthValue targetDepth = 0;
			for (SizeType i = 0; i < newIndex; ++i)
			{
				if (depths[i] == targetDepth && Sorter::isFirst(value, values[i]))
				{
					newIndex = i;
					break;
				}
			}

			values.insert(newIndex, value);
			depths.insert(newIndex, (DepthValue)0U);
		}
		else
		{
			values.pushBack(value);
			depths.pushBack((DepthValue)0U);
		}

		return newIndex;
	}

	HierarchyIndex createNodeAsChildOf(HierarchyIndex parentIndex, const ValueType& value)
	{
		SizeType newIndex = parentIndex + 1;

		if (Sorter::UseSorting == true)
		{
			// Default to last possible index
			newIndex = getLastDescendant(parentIndex) + 1;

			// Insert to sorted position
			DepthValue targetDepth = depths[parentIndex] + 1;
			for (SizeType i = parentIndex + 1; i < newIndex; ++i)
			{
				if (depths[i] == targetDepth && Sorter::isFirst(value, values[i]))
				{
					newIndex = i;
					break;
				}
			}
		}

		FLAT_ASSERT(newIndex > parentIndex);

		FLAT_DEPTHTYPE newParentCount = depths[parentIndex] + 1;
		FLAT_ASSERT(newParentCount < FLAT_MAXDEPTH); // Over flow protection

		values.insert(newIndex, value);
		depths.insert(newIndex, newParentCount);
		return newIndex;
	}
	HierarchyIndex getLastDescendant(HierarchyIndex parentIndex)
	{
		HierarchyIndex result = parentIndex + 1;
		DepthValue parentDepth = depths[parentIndex];
		while (result < getCount() && depths[result] > parentDepth)
		{
			++result;
		}
		return result - 1;
	}

	HierarchyIndex makeChildOf(HierarchyIndex child, HierarchyIndex parent)
	{
		//printf("Make %d child of %d\n", child, parent);

		FLAT_ASSERT(child != parent && "Self-adoption");
		FLAT_ASSERT(!linearIsChildOf(parent, child) && "Incest");
		//FLAT_ASSERT((depths[child] != depths[parent] + 1 || !linearIsChildOf(child, parent)) && "Re-parenting");

		SizeType dest = parent + 1; // Default destination position is right after parent
		SizeType count = getLastDescendant(child) - child + 1; // Descendant count including the child
		FLAT_ASSERT(count > 0);

		if (Sorter::UseSorting == true)
		{
			// Find a destination position that will have the child sorted among its siblings

			dest = getLastDescendant(parent) + 1; // Default to last possible index

			DepthValue targetDepth = depths[parent] + 1;
			for (SizeType i = parent + 1; i < dest; ++i)
			{
				if (depths[i] == targetDepth && Sorter::isFirst(values[child], values[i]))
				{
					dest = i;
					break;
				}
			}
		}
		else
		{
			// Commented to unify behavior with pointer trees
			//if (child > parent)
			//{
			//	// If child's index is greater than parent, find a place in parents children that is closest to child's current position
			//	DepthValue targetDepth = depths[parent] + 1;
			//	for (HierarchyIndex i = parent + 1; i < child; ++i)
			//	{
			//		if (depths[i] > targetDepth)
			//			continue;
			//		if (depths[i] < targetDepth)
			//			break;
			//		dest = i;
			//	}
			//}
		}

		// Do the move

		SizeType source = child;


		DepthValue depthDiff = depths[parent] + 1 - depths[child];

		if (source != dest && source + count != dest)
		{
			move(source, dest, count);
		}

		if (source < dest)
		{
			FLAT_ASSERT(source + count <= dest);
			dest -= count;
		}

		for (SizeType i = 0; i < count; i++)
		{
			depths[dest + i] += depthDiff;
#define TO_STR_IMPL(p) #p
#define TO_STR(p) TO_STR_IMPL(p)
			FLAT_ASSERT(depths[dest + i] < FLAT_MAXDEPTH && "Over/Under-flow threat detected: " TO_STR(FLAT_MAXDEPTH)); // Over flow protection
		}
		return dest;
	}

	void erase(HierarchyIndex child)
	{
		SizeType count = getLastDescendant(child) - child + 1;

		const SizeType toShift = getCount() - (child + count);
		FLAT_MEMMOVE(depths.getPointer() + child, depths.getPointer() + child + count, toShift * sizeof(DepthValue));
		FLAT_MEMMOVE(values.getPointer() + child, values.getPointer() + child + count, toShift * sizeof(ValueType));

		depths.resize(depths.getSize() - count);
		values.resize(values.getSize() - count);
	}
	void move(SizeType source, SizeType dest, SizeType count)
	{
#if 1
		SizeType low = source < dest ? source : dest;
		SizeType mid = source < dest ? source + count : source;
		SizeType high = source < dest ? dest + count : source + count;

		bool low_small = mid - low < high - mid;
		SizeType small_start = low_small ? low              : mid;
		SizeType small_count = low_small ? mid - low        : high - mid;
		SizeType small_dest  = low_small ? low + high - mid : low;
		SizeType large_start = low_small ? mid              : low;
		SizeType large_count = low_small ? high - mid       : mid - low;
		SizeType large_dest  = low_small ? low              : low + high - mid;

		static const SizeType static_buffer_size = 1024;
		char stack_buffer[static_buffer_size];
		char* temp_buffer = stack_buffer;
		if (small_count * sizeof(ValueType) > static_buffer_size)
			temp_buffer = (char*)malloc(small_count * sizeof(ValueType));

		//printf("memcpy(X, %d, %d); ", small_start, small_count);
		//printf("memmove(%d, %d, %d); ", large_dest, large_start, large_count);
		//printf("memcpy(%d, X, %d);\n", small_dest, small_count);

		FLAT_MEMCPY (temp_buffer                     , depths.getPointer() + small_start, small_count);
		FLAT_MEMMOVE(depths.getPointer() + large_dest, depths.getPointer() + large_start, large_count);
		FLAT_MEMCPY (depths.getPointer() + small_dest, temp_buffer                      , small_count);
		
		FLAT_MEMCPY (temp_buffer                     , values.getPointer() + small_start, small_count);
		FLAT_MEMMOVE(values.getPointer() + large_dest, values.getPointer() + large_start, large_count);
		FLAT_MEMCPY (values.getPointer() + small_dest, temp_buffer                      , small_count);

		if (small_count * sizeof(ValueType) > static_buffer_size)
			free(temp_buffer);
#endif
	}
	inline void moveImp(SizeType source, SizeType dest, SizeType count, DepthValue* depthBuffer, ValueType* valueBuffer)
	{
		FLAT_ASSERT(source + count <= dest || dest + count < source);

		DepthValue* dPtr = depths.getPointer();
		ValueType* vPtr = values.getPointer();

		// Push to buffer
		FLAT_MEMCPY(depthBuffer, dPtr + source, (count) * sizeof(DepthValue));
		FLAT_MEMCPY(valueBuffer, vPtr + source, (count) * sizeof(ValueType));

		const bool source_first = source < dest;
		const SizeType moveFrom = source_first ? source + count : dest;
		const SizeType moveTo   = source_first ? source         : dest + count;
		const SizeType toMove   = source_first ? dest - source  : source - dest;
		FLAT_MEMMOVE(dPtr + moveTo, dPtr + moveFrom, (toMove)* sizeof(DepthValue));
		FLAT_MEMMOVE(vPtr + moveTo, vPtr + moveFrom, (toMove) * sizeof(ValueType));

		FLAT_MEMCPY(dPtr + dest, depthBuffer, (count) * sizeof(DepthValue));
		FLAT_MEMCPY(vPtr + dest, valueBuffer, (count) * sizeof(ValueType));


		//printf("memcpy(X, %d, %d); ", source, count);
		//printf("memmove(%d, %d, %d); ", moveTo, moveFrom, toMove);
		//printf("memcpy(%d, X, %d);\n", dest, count);
	}

	inline bool linearIsChildOf(HierarchyIndex child, HierarchyIndex parent)
	{
		if (child <= parent || depths[child] <= depths[parent])
			return false;

		if (parent + 1 == child)
		{
			FLAT_ASSERT(depths[parent] + 1 == depths[child]);
			return true;
		}

		return depths[parent] < findMinDepthBetween(parent + 1, child - 1);
	}

	HierarchyIndex findValue(const ValueType& valueType, HierarchyIndex startingFrom = 0)
	{
		// Linear search
		for (HierarchyIndex i = startingFrom, end = getCount(); i < end; ++i)
		{
			if (values[i] == valueType)
				return i;
		}
		return getIndexNotFound();
	}
	HierarchyIndex findValueAsChildOf(const ValueType& valueType, HierarchyIndex parent)
	{
		// Linear search below parents depth starting from parent index
		DepthValue parentDepth = depths[parent];
		for (HierarchyIndex i = parent + 1, end = getCount(); i < end && parentDepth < depths[i]; ++i)
		{
			if (values[i] == valueType)
				return i;
		}
		return getIndexNotFound();
	}
};











#endif // FLAT_FLATHIERARCHY_H
