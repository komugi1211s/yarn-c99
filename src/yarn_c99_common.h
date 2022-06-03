/* =========================================
 * Yarn C99 Common --
 * basic, shared implementation used throughout the library.
 * contains useful data types, multiple stacks, etc.
 * ========================================= */

#if !defined(YARN_C99_COMMON_INCLUDE)
#define YARN_C99_COMMON_INCLUDE

#include <stddef.h>
#include <stdint.h>

#if !defined(YARN_C99_DEF)
  #define YARN_C99_DEF extern
#endif

#if !defined(YARN_MALLOC) || !defined(YARN_FREE) || !defined(YARN_REALLOC)
  #if !defined(YARN_MALLOC) && !defined(YARN_FREE) && !defined(YARN_REALLOC)
    #include <stdlib.h>
    #define YARN_MALLOC(sz)       malloc(sz)
    #define YARN_FREE(ptr)        free(ptr)
    #define YARN_REALLOC(ptr, sz) realloc(ptr, sz)
  #else
    #error "you must define all yarn_{malloc,free,realloc} macro if you're going to define one of them."
  #endif
#endif

#define YARN_UNUSED(n) ((void)(sizeof(n)))
#define YARN_CONCAT1(a, b) a##b
#define YARN_CONCAT(a, b) YARN_CONCAT1(a, b)
#define YARN_LEN(n) (sizeof(n)/(sizeof(0[n])))

#define YARN_STATIC_ASSERT(cond, ident_message) \
    typedef char YARN_CONCAT(yarn_static_assert_line_, YARN_CONCAT(ident_message, __LINE__))[(cond) ? 1 : -1]; YARN_CONCAT(yarn_static_assert_line_, YARN_CONCAT(ident_message, __LINE__)) YARN_CONCAT(ver, __LINE__) = {0}; YARN_UNUSED(YARN_CONCAT(ver, __LINE__));\

/* =========================================
 * Hashmap.
 *
 * TODO: IT IS NOT TYPESAFE!!!!!!!!!!!!!!!!!!!:
 *   kvcreate takes type to it's first argument so it SEEMS like it is typesafe,
 *   but it's not. it does not store the information. it's misleading.
 *   it is actually storing the SIZE of the type. more on that below.
 * 
 * overview of how it's implemented:
 *   linear-probing, djb2 hashing.
 *   only supports char* as key, and key will be cloned on push.
 * 
 *   internal data is stored in char* buffer.
 *   like the diagram shown below;
 *
 * |================= entire buffer ==================|
 * [header-data|actual value][header-data|actual value]
 * |===== one "chunk"  =====|
 *
 * with map->element_size being set to the size of actual value.
 * on kvpush, it will take the size of passed value, and check against the map it's being inserted to,
 * and fires assertion error if does not.
 *
 * TODO: @testing -- this works, but complete and thorough test must be done.
 *
 * create hashmap with:
 *  map = yarn_kvcreate(element, capacity);
 *
 *  then, operate with:
 *    yarn_kvpush(&map, "hello", value);
 *    yarn_kvget(&map, "hello", &value); // will write into value
 *    yarn_kvhas(&map, "hello"); // returns 1 on true, 0 on false
 *    yarn_kvdelete(&map, "hello");
 *
 *    char *key;
 *    value value;
 *    yarn_kvforeach(&map, &key, &value) {
 *      // do whatever with value.
 *      // do not modify map while you're in foreach loop. do not free key.
 *      // foreach is unordered.
 *    }
 *
 *  once you're done:
 *    yarn_kvdestroy(&map); // every value stored inside will be unreachable.
 */

typedef struct {
    uint32_t hash;
    size_t   keylen;
    char    *key;
} yarn_kvpair_header;

typedef struct {
    char   *entries;
    size_t used;
    size_t capacity;
    size_t element_size; /* size of the actual value it's made for, not the size of chunk */
} yarn_kvmap;

#define yarn_kvcreate(element, caps) \
    yarn__kvmap_create(sizeof(element), (caps))

#define yarn_kvdestroy(map) \
    yarn__kvmap_destroy((map))

#define yarn_kvpush(map, key, value) \
    yarn__kvmap_pushsize((map), (key), &(value), sizeof(value))

#define yarn_kvget(map, key, value_ptr) \
    yarn__kvmap_get((map), (key), (value_ptr), ((value_ptr) ? sizeof(*value_ptr) : 0))

#define yarn_kvhas(map, key) \
    (yarn__kvmap_get((map), (key), 0, 0) != -1)

#define yarn_kvdelete(map, key) \
    yarn__kvmap_delete((map), (key))

#define yarn_kviternext(map, key_ptr, value_ptr, at)\
    yarn__kvmap_iternext((map),(key_ptr),(value_ptr),((value_ptr)?sizeof(*value_ptr):0), at)

/* TODO: super confusing and complex. fix it */
#define yarn_kvforeach(map, key_ptr, value_ptr) \
    for(int kv_iter = yarn_kviternext((map), (key_ptr), (value_ptr), 0); \
        kv_iter != -1; \
        kv_iter = yarn_kviternext((map), (key_ptr), (value_ptr), kv_iter))\
        if (kv_iter == -1) { break; } else

YARN_C99_DEF yarn_kvmap yarn__kvmap_create(size_t elem_size, size_t caps); /* returns newly created map with given capacity and element size. */
YARN_C99_DEF void       yarn__kvmap_destroy(yarn_kvmap *map);              /* frees entire map. content inside will be unreachable!! */

/* pushes element into map. element_size must match with map->impl->element_size. */
YARN_C99_DEF int yarn__kvmap_pushsize(yarn_kvmap *map, const char *key, void *value, size_t element_size);

/* get element from map, and write into value if given valid pointer. returns -1 if does not exist. */
YARN_C99_DEF int yarn__kvmap_get(yarn_kvmap *map, const char *key, void *value, size_t element_size);

/* delete element from map, and rearrange. */
YARN_C99_DEF int yarn__kvmap_delete(yarn_kvmap *map, const char *key);

/* recreate the whole map if the current map's used up space meets certain threshold. */
YARN_C99_DEF int yarn__kvmap_maybe_rehash(yarn_kvmap *map);

/* checks hashmap entry specified by at, and writes key/value into pointers.
 * returns (at + 1) if it can be continued, -1 otherwise. */
YARN_C99_DEF int yarn__kvmap_iternext(yarn_kvmap *map, char **key, void *value, size_t element_size, int at);


/* =============================================
 * Yarn allocator:
 *   Arena based allocator that has the same memory lifetime.
 *   frees everything at once.
 *   every pointer that allocates off of this will never be invalidated.
 *
 *   internally it allocates the chain of allocator chunk to achieve parsistence of memory.
 *   with (used = 0, caps = 0, buffer = 0) being the sentinel node.
 *
 *   every allocator chunk will be pushed inside the doubly linked list.
 */

typedef struct yarn_allocator_chunk yarn_allocator_chunk;

struct yarn_allocator_chunk {
    size_t used;
    size_t capacity;
    char   *buffer;

    yarn_allocator_chunk *prev;
    yarn_allocator_chunk *next;
};

typedef struct {
    yarn_allocator_chunk *sentinel;
    size_t next_caps;
} yarn_allocator;


YARN_C99_DEF yarn_allocator yarn_create_allocator(size_t initial_caps); /* Creates Allocator with initial size. */
YARN_C99_DEF void           yarn_destroy_allocator(yarn_allocator allocator); /* Destroys given allocator and free the memory. */

YARN_C99_DEF void *yarn_allocate(yarn_allocator *allocator, size_t size); /* Allocate given size from allocator. */

/*
 * Clears allocator and coalesces the allocation chunk.
 * every chunk will be freed, then the single biggest chunk will be allocated.
 * meaning if the allocator has the 4 256kb chunks, this will free everything and
 * reallocates 1 1024kb chunk (4 * 256kb = 1 * 1024kb).)
 */
YARN_C99_DEF void yarn_clear_allocator(yarn_allocator *allocator);

#endif
