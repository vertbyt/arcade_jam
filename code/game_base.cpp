
//
// Basic types and utility functions
//

#include <stdlib.h>
#include <stdio.h>

#include <stdarg.h>
#include <stdint.h>

typedef int8_t   s8;
typedef uint8_t  u8;
typedef int16_t  s16;
typedef uint16_t u16;
typedef int32_t  s32;
typedef uint32_t u32;
typedef int64_t  s64;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef s32 b32;

#define global_var static
#define local_persist static
#define internal static

#define Stmnt(x) do{ x; }while(0)

#if defined(PLATFORM_WEB)
#define Assert(ext)
#else
#define Assert(exp) Stmnt(if(!(exp)){ *(int *)0 = 0; })
#endif

#define Loop(var_name, times) for(s64 var_name = 0; var_name < times; ++var_name)

#define ArrayCount(arr) (sizeof(arr)/sizeof(arr[0]))

#define U32FromPtr(ptr) (u32)((char *)ptr - (char *)0)
#define U64FromPtr(ptr) (u64)((char *)ptr - (char *)0)

#define IntFromPtr(ptr) U32FromPtr(ptr)
#define PtrFromInt(n) (void *)((char *)0 + (n))

#define _Member(T, m) (((T*)0)->m)
#define MemberOffset(T, m) IntFromPtr(&_Member(T, m))

#define Min(a, b) ((a) < (b) ? (a) : (b))
#define Max(a, b) ((a) > (b) ? (a) : (b))

#define Abs(v)  ((v) >= 0 ? (v) : (-(v)))
#define Sign(v) ((v) > 0 ? 1 : (v) < 0 ? -1 : 0)

#define Ceil(v)   ((s32)((v) + 0.9999))
#define Floor(v)  ((s32)(v))

#define KB(n) (n*1024ULL)
#define MB(n) (KB(n)*1024ULL)
#define GB(n) (MB(n)*1024ULL)
#define TB(n) (GB(n)*1024ULL)

void zero_memory(u8* ptr, s64 size) {
  u64 *p64 = (u64 *)ptr;
  s64 s0 = size/sizeof(u64);
  Loop(i, s0) p64[i] = 0;
  
  u8 *p8 = ptr + s0*sizeof(u64);
  s64 s1 = size - s0*sizeof(u64);
  Loop(i, s1) p8[i] = 0;
}

b32 cstr_equal(char* a, char* b) {
  while(*a != '\0' && *b != '\0') {
    if(*a != *b) return false;
    a += 1;
    b += 1;
  }
  
  if(*a != *b) return false;
  return true;
}