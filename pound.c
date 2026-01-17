/*
Licensed under the MIT license (https://opensource.org/license/mit/)

Sources:
- "https://github.com/EpicGamesExt/raddebugger"
- "https://github.com/gingerBill/gb"
*/

#ifndef POUNDC_H
#define POUNDC_H

/* ENVIRONMENT */
// ENVIRONMENT: language
#if defined(__cplusplus)
    #define LANG_CPP 1
#else
    #define LANG_C 1
#endif

// ENVIRONMENT: compiler and os
// MSVC
#if defined(_MSC_VER)
    #define COMPILER_MSVC 1
    // os
    #if defined(_WIN32)
        #define OS_WINDOWS 1
    #else
        #error compiler/OS not supported
    #endif
    // arch
    #if defined(_M_AMD64)
        #define ARCH_X64 1
    #elif defined(_M_IX86)
        #define ARCH_X86 1
    #elif defined(_M_ARM64)
        #define ARCH_ARM64 1
    #elif defined(_M_ARM)
        #define ARCH_ARM32 1
    #else
        #error architecture not supported
    #endif

// CLANG
#elif defined(__clang__)
    #define COMPILER_CLANG 1
    // os
    #if defined(_WIN32)
        #define OS_WINDOWS 1
    #elif defined(__gnu_linux__) || defined(__linux__)
        #define OS_LINUX 1
    #elif defined(__APPLE__) && defined(__MACH__)
        #define OS_MAC 1
    #else
        #error compiler/OS not supported
    #endif
    // arch
    #if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64)
        #define ARCH_X64 1
    #elif defined(i386) || defined(__i386) || defined(__i386__)
        #define ARCH_X86 1
    #elif defined(__aarch64__)
        #define ARCH_ARM64 1
    #elif defined(__arm__)
        #define ARCH_ARM32 1
    #else
        #error architecture not supported
    #endif

// GCC
#elif defined(__GNUC__) || defined(__GNUG__)
    #define COMPILER_GCC 1
    // os
    #if defined(__gnu_linux__) || defined(__linux__)
        #define OS_LINUX 1
    #else
        #error compiler/OS not supported
    #endif
    // arch
    #if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64)
        #define ARCH_X64 1
    #elif defined(i386) || defined(__i386) || defined(__i386__)
        #define ARCH_X86 1
    #elif defined(__aarch64__)
        #define ARCH_ARM64 1
    #elif defined(__arm__)
        #define ARCH_ARM32 1
    #else
        #error architecture not supported
    #endif

// unknown compiler
#else
    #error compiler not supported
#endif

// ENVIRONMENT: architecture
#if defined(ARCH_X64)
    #define ARCH_64BIT 1
#elif defined(ARCH_X86)
    #define ARCH_32BIT 1
#endif

/* CORE */
#if OS_LINUX
#define _GNU_SOURCE 1 // angn: NOTE: not sure if this works on all linux systems
                      // but it defines missing symbols like 'madvise'
#endif

#include <stdint.h>
#include <stdio.h>
#include <string.h>

// CORE: keywords
// angn: stolen from Casey Muratori
#define internal      static
#define global        static
#define local_persist static

// angn: TODO: rodata

#if COMPILER_MSVC
    #define thread_internal __declspec(thread)
#elif COMPILER_CLANG || COMPILER_GCC
    #define thread_internal __thread
#else
    #error thread_internal undefined
#endif

#if COMPILER_MSVC
    #define force_inline __forceinline
#elif COMPILER_CLANG || COMPILER_GCC
    #define force_inline __attribute__((always_inline))
#else
    #error force_inline undefined
#endif

#if COMPILER_MSVC
    #define no_inline __declspec(noinline)
#elif COMPILER_CLANG || COMPILER_GCC
    #define no_inline __attribute__((noinline))
#else
    #error no_inline undefined
#endif

// CORE: primitive types
typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
typedef int8_t S8;
typedef int16_t S16;
typedef int32_t S32;
typedef int64_t S64;
typedef S8 B8;
typedef S16 B16;
typedef S32 B32;
typedef S64 B64;
typedef float F32;
typedef double F64;

typedef void VoidProc(void);

// CORE: type utils
// convert
#if ARCH_64BIT
    #define IntFromPtr(p) ((U64)(p))
#elif ARCH_32BIT
    #define IntFromPtr(p) ((U32)(p))
#else
    #error unknown architecture
#endif
#define PtrFromInt(i) (void*)(i)

// structs and compound types
#define MemberOf(t,m) (((t*)0)->m)
#define OffsetOf(t,m) IntFromPtr(&Member(t,m))

#if LANG_CPP
    #define zero_struct {}
#else
    #define zero_struct {0}
#endif

#define StaticArrayLength(xs) (sizeof(xs) / sizeof(*(xs)))

// bits
#define U64FromTwoU32s(a,b)    ((((U64)(a)) << 32) | ((U64)(b)))

// align
#define AlignUpPow2(x,b) (((x)+(b)-1) & (~((b)-1)))
#define AlignDownPow2(x,b) ((x) & (~((b)-1)))
#define IsPow2(x) ((x)!=0 && ((x) & ((x)-1))==0)
#define IsPow2OrZero(x) ((((x)-1) & (x)) == 0)

#if COMPILER_MSVC
    #define AlignOf(t) __alignof(t)
#elif COMPILER_CLANG
    #define AlignOf(t) __alignof(t)
#elif COMPILER_GCC
    #define AlignOf(t) __alignof__(t)
#else
    #error AlignOf undefined
#endif

#if COMPILER_MSVC
    #define AlignAs(n) __declspec(align(n))
#elif COMPILER_CLANG || COMPILER_GCC
    #define AlignAs(n) __attribute__((aligned(n)))
#else
    #error AlignAs undefined
#endif

// CORE: numbers
#define KiloBytes(n) (((U64)(n)) << 10)
#define MegaBytes(n) (((U64)(n)) << 20)
#define GigaBytes(n) (((U64)(n)) << 30)
#define TeraBytes(n) (((U64)(n)) << 40)

// CORE: math
#define Min(x,y) (((x) < (y)) ? (x) : (y))
#define Max(x,y) (((x) > (y)) ? (x) : (y))
#define Clamp(b,x,t) (((x) < (b)) ? (b) : ((x) > t) ? (t) : (x))
#define Lerp(x,y,a) ((x) + (((y) - (x)) * a))

// CORE: loop macros
// angn: e.g. for EachIndex(i, 8)
#define EachIndex(i,count) (U64 i = 0; i < (count); i += 1)
#define EachStaticArray(i,xs) (U64 i = 0; i < StaticArrayLength(xs); i += 1)
#define EachEnum(t,i) (t i = (t)0; i < t##_COUNT; i = (t)(i+1))
#define EachEnumSkipZero(t,i) (t i = (t)1; i < t##_COUNT; i = (t)(i+1))
#define EachRange(i,range) (U64 i = (range).min; i < (range).max; i += 1)
#define EachSLLNode(t,n,first,next) (t *(n) = first; (n) != 0; n = (n)->next)

// angn: lets us defer an action to the end of a block, helpful for cleanup
#define Defer(init,cleanup) for(int _i_ = ((init), 0); !_i_; _i_ += 1, (cleanup))
#define DeferChecked(init,cleanup) for(int _i_ = 2 * !(init); (_i_ == 2 ? ((cleanup), 0) : !_i_); _i_ += 1, (cleanup))

// CORE: development macros
#define StringConcatInternal__(x,y) x##y
#define StringConcat(x,y) StringConcatInternal__(x, y)

#define StringifyInternal__(x) #x
#define Stringify(x) StringifyInternal__(x)

#define Swap(t,a,b) do{t temp__=(a);(a)=(b);(b)=temp__;}while(0)

#define Cast(t,x) ((t)(x))

// CORE: development utils
#if COMPILER_MSVC
    #define DebugTrap() __debugbreak()
#elif COMPILER_CLANG || COMPILER_GCC
    #if ARCH_X64 || ARCH_X86
        #define DebugTrap() __asm volatile ("int $3" ::: "memory")
    #else
        #define DebugTrap() __builtin_trap() // angn: NOTE: __builtin_trap raises SIGILL
    #endif
#else
    #error missing DebugTrap
#endif

#define AssertForce(b) do{ if(!(b)) {DebugTrap();} }while(0)
#if BUILD_DEBUG
    #define Assert(b) AssertForce(b)
#else
    #define Assert(b) (void)(b)
#endif
#define NotImplemented() Assert(!"Not Implemented!")
#define StaticAssert(b,id) global U8 StringConcat(id, __LINE__)[(b) ? 1 : -1]

#if BUILD_DEBUG
    #define NotUsed(x) (void)(x)
#else
    #define NotUsed(x) (void)0
#endif

// angn: TODO: CORE: atomics

// CORE: linked lists
// angn: TODO: add more helpers

// CORE: linkedlists: singly linked as stack
#define SLLStackPush_N(s,n,next) ((n)->next=(s), (s)=(n))
#define SLLStackPop_N(s,next) ((s)=(s)->next)

// angn: TODO: CORE: generic string hashing function

/* PROTO Math */
// PROTO Math: vector
typedef union Vec2F32 Vec2F32;
union Vec2F32
{
    struct
    {
        F32 x;
        F32 y;
    };
    F32 v[2];
};

typedef union Vec3F32 Vec3F32;
union Vec3F32
{
    struct
    {
        F32 x;
        F32 y;
        F32 z;
    };
    struct
    {
        Vec2F32 xy;
        F32 _z0;
    };
    struct
    {
        F32 _x0;
        Vec2F32 yz;
    };
    F32 v[3];
};

typedef union Vec4F32 Vec4F32;
union Vec4F32
{
    struct
    {
        F32 x;
        F32 y;
        F32 z;
        F32 w;
    };
    struct
    {
        Vec2F32 xy;
        Vec2F32 zw;
    };
    struct
    {
        Vec3F32 xyz;
        F32 _z0;
    };
    struct
    {
        F32 _x0;
        Vec3F32 yzw;
    };
    F32 v[4];
};

typedef union Vec2S32 Vec2S32;
union Vec2S32
{
    struct
    {
        S32 x;
        S32 y;
    };
    S32 v[2];
};

typedef union Vec2S64 Vec2S64;
union Vec2S64
{
    struct
    {
        S64 x;
        S64 y;
    };
    S64 v[2];
};

// PROTO Math: matrix
typedef struct Mat3x3F32 Mat3x3F32;
struct Mat3x3F32
{
    F32 v[3][3];
};

typedef struct Mat4x4F32 Mat4x4F32;
struct Mat4x4F32
{
    F32 v[4][4];
};

// PROTO Math: range 1D
typedef union Range1U32 Range1U32;
union Range1U32
{
    struct
    {
        U32 min;
        U32 max;
    };
    U32 v[2];
};

typedef union Range1S32 Range1S32;
union Range1S32
{
    struct
    {
        S32 min;
        S32 max;
    };
    S32 v[2];
};

typedef union Range1U64 Range1U64;
union Range1U64
{
    struct
    {
        U64 min;
        U64 max;
    };
    U64 v[2];
};

typedef union Range1S64 Range1S64;
union Range1S64
{
    struct
    {
        S64 min;
        S64 max;
    };
    S64 v[2];
};

typedef union Range1F32 Range1F32;
union Range1F32
{
    struct
    {
        F32 min;
        F32 max;
    };
    F32 v[2];
};

// PROTO Math: range 2D
typedef union Range2S32 Range2S32;
union Range2S32
{
    struct
    {
        Vec2S32 min;
        Vec2S32 max;
    };
    struct
    {
        Vec2S32 p0;
        Vec2S32 p1;
    };
    struct
    {
        S32 x0;
        S32 y0;
        S32 x1;
        S32 y1;
    };
    Vec2S32 v[2];
};

typedef union Range2S64 Range2S64;
union Range2S64
{
    struct
    {
        Vec2S64 min;
        Vec2S64 max;
    };
    struct
    {
        Vec2S64 p0;
        Vec2S64 p1;
    };
    struct
    {
        S64 x0;
        S64 y0;
        S64 x1;
        S64 y1;
    };
    Vec2S64 v[2];
};

typedef union Range2F32 Range2F32;
union Range2F32
{
    struct
    {
        Vec2F32 min;
        Vec2F32 max;
    };
    struct
    {
        Vec2F32 p0;
        Vec2F32 p1;
    };
    struct
    {
        F32 x0;
        F32 y0;
        F32 x1;
        F32 y1;
    };
    Vec2F32 v[2];
};

/* PROTO String */
typedef struct String8 String8;
struct String8
{
    U8 *string;
    U64 size;
};

internal String8
string8_make(
        U8 *str,
        U64 size);

internal String8
string8_from_cstring(
        char *cstr);

internal String8
string8_from_range(
        U8 *first,
        U8 *one_past_last);

internal String8
string8_make_zero(
        void);

internal String8
string8_slice(
        String8 str,
        Range1U64 range);

/* PROTO Arena */
#define ARENA_HEADER_SIZE 128

typedef U64 ArenaFlags;
enum
{
    ArenaFlags_NoChain = (1<<0),
    ArenaFlags_LargePages = (1<<1),
};

typedef struct ArenaParams ArenaParams;
struct ArenaParams
{
    ArenaFlags flags;
    U64 reserve_size;
    U64 commit_size;
    void *optional_backing_buffer;
    char *allocation_site_file;
    int allocation_site_line;
};

typedef struct Arena Arena;
struct Arena
{
    Arena *prev_arena;
    Arena *curr_arena;
    ArenaFlags flags;
    U64 commit_size;
    U64 reserve_size;
    U64 base_alloc;
    U64 pos_alloc;
    U64 committed;
    U64 reserved;
    char *allocation_site_file;
    int allocation_site_line;
};
StaticAssert(sizeof(Arena) <= ARENA_HEADER_SIZE, arena_header_size_check);

typedef struct TempArena TempArena;
struct TempArena
{
    Arena *arena;
    U64 pos_alloc;
};

#define ARENA_DEFAULT_RESERVE_SIZE MegaBytes(64) // should be enough for most arenas
#define ARENA_DEFAULT_COMMIT_SIZE KiloBytes(64) // most OS are pretty close to this

// angn: TODO: pass via a structure, to make override-able parameters (thats very cool @rfleury)
#define arena_make(...) \
    arena_make_( \
            &(ArenaParams){ \
                .reserve_size = ARENA_DEFAULT_RESERVE_SIZE, \
                .commit_size = ARENA_DEFAULT_COMMIT_SIZE, \
                .allocation_site_file = __FILE__, \
                .allocation_site_line = __LINE__, \
                __VA_ARGS__ \
            })

internal Arena *
arena_make_(
        ArenaParams *params);

internal void
arena_destroy(
        Arena *arena);

internal void *
arena_push(
        Arena *arena,
        U64 size,
        U64 align,
        B32 zero);

internal U64
arena_pos(
        Arena *arena);

internal void
arena_pop_to(
        Arena *arena,
        U64 pos_alloc);

internal void
arena_clear(
        Arena *arena);

internal void
arena_pop(
        Arena *arena,
        U64 size);

internal TempArena
temp_arena_begin(
        Arena *arena);

internal void
temp_arena_end(
        TempArena temparena);

#define arena_push_array_no_zero_aligned(a,t,n,align) (t *)arena_push((a), sizeof(t)*(n), (align), (0))
#define arena_push_array_aligned(a,t,n,align) (t *)arena_push((a), sizeof(t)*(n), (align), (1))
#define arena_push_array_no_zero(a,t,n) arena_push_array_no_zero_aligned(a, t, n, Max(8, AlignOf(t)))
#define arena_push_array(a,t,n) arena_push_array_aligned(a, t, n, Max(8, AlignOf(t)))

#define arena_pop_array(a,t,n) arena_pop(a, sizeof(t) * (n))

/* PROTO OS */
// PROTO OS: system info
typedef struct OS_SystemInfo OS_SystemInfo;
struct OS_SystemInfo
{
    U32 logical_processor_count;
    U64 page_size;
    U64 large_page_size;
    U64 allocation_granularity;
};

internal OS_SystemInfo *
os_get_system_info(
        void);

int
os_init(
        void);

// PROTO OS: memory
internal void *
os_reserve(
        U64 size);

internal void
os_release(
        void *ptr,
        U64 size);

internal void *
os_reserve_large(
        U64 size);

internal B32
os_commit_large(
        void *ptr, U64 size);

internal B32
os_commit(
        void *ptr,
        U64 size);

internal void
os_decommit(
        void *ptr,
        U64 size);

// PROTO OS: linux
#if OS_LINUX

#include <sys/mman.h>
#include <sys/sysinfo.h>
#include <unistd.h>

typedef struct OS_Linux_State OS_Linux_State;
struct OS_Linux_State
{
    Arena *arena;
    OS_SystemInfo system_info;
};

global OS_Linux_State g_os_linux_state;

Arena *os_get_arena(void) { return(g_os_linux_state.arena); }
OS_SystemInfo *os_get_system_info(void) { return(&g_os_linux_state.system_info); }

#endif // OS_LINUX

#endif // POUNDC_H





/* IMPL STRING */
#if IMPL_POUNDC_STRING
#undef IMPL_POUNDC_STRING

internal String8
string8_make(
        U8 *str,
        U64 size)
{
    return((String8)
            {
                .string = str,
                .size = size,
            });
}

internal String8
string8_from_cstring(
        char *cstr)
{
    return((String8)
            {
                .string = (U8 *)cstr,
                .size = (U64)strlen(cstr),
            });
}

internal String8
string8_from_range(
        U8 *first,
        U8 *one_past_last)
{
    return((String8)
            {
                .string = first,
                .size = (U64)(one_past_last - first),
            });
}

internal String8
string8_make_zero(
        void)
{
    return((String8){0});
}

internal String8
string8_slice(
        String8 str,
        Range1U64 range)
{
    Assert(range.v[0] <= range.v[1]);
    U64 shift_right = Min(range.v[0], str.size - 1);
    U64 shift_left = range.v[1];
    return((String8)
            {
                .string = str.string + shift_right,
                .size = str.size - shift_right - shift_left,
            });
}

#endif // IMPL_POUNDC_STRING


/* IMPL OS */
#if IMPL_POUNDC_OS
#undef IMPL_POUNDC_OS

// IMPL OS: linux
#if OS_LINUX
int
os_init(
        void)
{
    // setup OS info
    {
        OS_SystemInfo *info = &g_os_linux_state.system_info;
        info->logical_processor_count = (U32)get_nprocs();
        info->page_size = (U64)getpagesize();
        info->large_page_size = MegaBytes(2);
        info->allocation_granularity = info->page_size;

        // angn: TODO: multicore by default, thread context allocator

        // global allocator
        g_os_linux_state.arena = arena_make();
    }

    return(0);
}

internal void *
os_reserve(
        U64 size)
{
    void *map = mmap(0, size, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if(map == MAP_FAILED) { map = 0; }
    return(map);
}

internal void
os_release(
        void *ptr,
        U64 size)
{
    munmap(ptr, size);
}

internal void *
os_reserve_large(
        U64 size)
{
    void *map = mmap(0, size, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_HUGETLB, -1, 0);
    if(map == MAP_FAILED) { map = 0; }
    return(map);
}

internal B32
os_commit_large(
        void *ptr, U64 size)
{
    mprotect(ptr, size, PROT_READ|PROT_WRITE);
    return(1);
}

internal B32
os_commit(
        void *ptr,
        U64 size)
{
    return(mprotect(ptr, size, PROT_READ|PROT_WRITE) == 0);
}

internal void
os_decommit(
        void *ptr,
        U64 size)
{
    madvise(ptr, size, MADV_DONTNEED);
    mprotect(ptr, size, PROT_NONE);
}

#endif // OS_LINUX

#endif // IMPL_POUNDC_OS

/* ARENA */
#if IMPL_POUNDC_ARENA
#undef IMPL_POUNDC_ARENA

internal Arena *
arena_make_(
        ArenaParams *params)
{
    U64 reserve_size = 0;
    U64 commit_size = 0;
    // align reserve and commit sizes to OS sizes
    if(params->flags & ArenaFlags_LargePages)
    {
        reserve_size = AlignUpPow2(params->reserve_size, os_get_system_info()->large_page_size);
        commit_size = AlignUpPow2(params->commit_size, os_get_system_info()->large_page_size);
    }
    else
    {
        reserve_size = AlignUpPow2(params->reserve_size, os_get_system_info()->page_size);
        commit_size = AlignUpPow2(params->commit_size, os_get_system_info()->page_size);
    }

    // reserve and commit initial buffer
    void *base = params->optional_backing_buffer;
    if(base == 0)
    {
        if(params->flags & ArenaFlags_LargePages)
        {
            base = os_reserve_large(reserve_size);
            os_commit_large(base, commit_size);
        }
        else
        {
            base = os_reserve(reserve_size);
            os_commit(base, commit_size);
        }
    }

    Arena *arena = (Arena *)base;
    arena->curr_arena = arena;
    arena->flags = params->flags;
    arena->commit_size = params->commit_size;
    arena->reserve_size = params->reserve_size;
    arena->base_alloc = 0;
    arena->pos_alloc = ARENA_HEADER_SIZE;
    arena->committed = commit_size;
    arena->reserved = reserve_size;
    arena->allocation_site_file = params->allocation_site_file;
    arena->allocation_site_line = params->allocation_site_line;
    return(arena);
}

internal void
arena_destroy(
        Arena *arena)
{
    Arena *prev = 0;
    for(Arena *n = arena->curr_arena;
            n != 0;
            n = prev)
    {
        prev = n->prev_arena;
        os_release(n, n->reserved);
    }
}

internal void *
arena_push(
        Arena *arena,
        U64 size,
        U64 align,
        B32 zero)
{
    Arena *current = arena->curr_arena;
    U64 pos_pre = AlignUpPow2(current->pos_alloc, align);
    U64 pos_post = pos_pre + size;

    // chain a new block
    if(current->reserved < pos_post && !(arena->flags & ArenaFlags_NoChain))
    {
        Arena *new_block = 0;

        // find out the amount of space required
        U64 reserve_size = current->reserve_size;
        U64 commit_size = current->commit_size;
        if(size + ARENA_HEADER_SIZE > reserve_size)
        {
            reserve_size = AlignUpPow2(size + ARENA_HEADER_SIZE, align);
            commit_size = AlignUpPow2(size + ARENA_HEADER_SIZE, align);
        }

        // allocate
        new_block =
            arena_make(
                    .flags = current->flags,
                    .reserve_size = reserve_size,
                    .commit_size = commit_size,
                    .allocation_site_file = current->allocation_site_file,
                    .allocation_site_line = current->allocation_site_line);

        // setup next arena
        new_block->base_alloc = current->base_alloc + current->reserved;
        SLLStackPush_N(arena->curr_arena, new_block, prev_arena);

        current = new_block;
        pos_pre = AlignUpPow2(current->pos_alloc, align);
        pos_post = pos_pre + size;
    }

    // amount of memory to zero
    U64 size_to_zero = 0;
    if(zero)
    {
        size_to_zero = Min(current->committed, pos_post) - pos_pre;
    }

    // commit past requested amount
    if(current->committed < pos_post)
    {
        U64 commit_post_aligned = pos_post + current->commit_size - 1;
        commit_post_aligned -= commit_post_aligned % current->commit_size;
        U64 commit_post_clamped = Min(commit_post_aligned, current->reserved);
        U64 commit_size = commit_post_clamped - current->committed;
        U8 *commit_ptr = (U8 *)current + current->committed;
        if(current->flags & ArenaFlags_LargePages)
        {
            os_commit_large(commit_ptr, commit_size);
        }
        else
        {
            os_commit(commit_ptr, commit_size);
        }
        current->committed = commit_post_clamped;
    }

    // add to the new block
    void *result = 0;
    if(current->committed >= pos_post)
    {
        result = (U8 *)current + pos_pre;
        current->pos_alloc = pos_post;
        if(size_to_zero != 0)
        {
            memset(result, 0, size_to_zero);
        }
    }

    return(result);
}

internal U64
arena_pos(
        Arena *arena)
{
    Arena *current = arena->curr_arena;
    return(current->base_alloc + current->pos_alloc);
}

internal void
arena_pop_to(
        Arena *arena,
        U64 pos_alloc)
{
    U64 pos = Max(ARENA_HEADER_SIZE, pos_alloc);
    Arena *current = arena->curr_arena;

    // clear any memory segments in the chain that we popped past
    for(Arena *prev = 0;
            current->base_alloc >= pos;
            current = prev)
    {
        prev = current->prev_arena;
        os_release(current, current->reserved);
    }

    // shift down the position
    arena->curr_arena = current;
    U64 new_pos = pos - current->base_alloc;
    AssertForce(new_pos <= current->pos_alloc);
    current->pos_alloc = new_pos;
}

internal void
arena_clear(
        Arena *arena)
{
    arena_pop_to(arena, 0);
}

internal void
arena_pop(
        Arena *arena,
        U64 size)
{
    U64 pos = arena_pos(arena);
    arena_pop_to(arena, size < pos ? pos - size : pos);
}

internal TempArena
temp_arena_begin(
        Arena *arena)
{
    return((TempArena){.arena = arena, .pos_alloc = arena->pos_alloc});
}

internal void
temp_arena_end(
        TempArena temparena)
{
    arena_pop_to(temparena.arena, temparena.pos_alloc);
}

#endif // IMPL_POUNDC_ARENA
