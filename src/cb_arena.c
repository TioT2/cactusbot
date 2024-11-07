/**
 * @brief arena allocator implementation file
 */

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "cb_arena.h"

/// @brief arena-allocated block size
#define CB_ARENA_BLOCK_SIZE ((size_t)1024)

/// @brief allocation forward declaration
typedef struct __CbArenaAllocation CbArenaAllocation;

/// @brief arena block representation structure
struct __CbArenaAllocation {
    union {
        CbArenaAllocation *next;   ///< next allocation pointer
        max_align_t        _align; ///< alignment forcer
    };
    uint8_t            data[1]; ///< allocation data
}; // struct __CbArenaAllocation

/// @brief arena implementation structure
typedef struct __CbArenaImpl {
    CbArenaAllocation *allocations; ///< arena allocations stack
    void *curr;                     ///< current block pointer
    void *end;                      ///< block end pointer
} CbArenaImpl;

CbArena cbArenaCtor( void ) {
    CbArenaImpl *impl = (CbArenaImpl *)calloc(sizeof(CbArenaImpl), 1);

    if (impl == NULL)
        return NULL;
    
    *impl = (CbArenaImpl) {
        .allocations = NULL,
        .curr        = NULL,
        .end         = NULL,
    };

    return impl;
} // cbArenaCtor

void cbArenaDtor( CbArena const arena ) {
    if (arena == NULL)
        return;

    CbArenaAllocation *allocation = arena->allocations;

    while (allocation != NULL) {
        CbArenaAllocation *const next = allocation->next;
        free(allocation);
        allocation = next;
    }
    free(arena);
} // cbArenaDtor

/**
 * @brief up alignment function
 * 
 * @param[in] number    number to align up
 * @param[in] alignment required alignment
 * 
 * @return the least n: n >= number && n % alignment == 0
 */
static size_t cbArenaAlignUp( const size_t number, const size_t alignment ) {
    return alignment * (number / alignment + (size_t)(number % alignment != 0));
} // cbArenaAlignUp

void * cbArenaAlloc( CbArena const arena, const size_t size ) {
    assert(arena != NULL);

    void *allocationStart = (void *)cbArenaAlignUp((size_t)arena->curr, sizeof(max_align_t));
    void *allocationEnd = (uint8_t *)allocationStart + size;

    if (allocationEnd >= arena->curr) {
        // just in case if allocation size is somehow more than CB_ARENA_BLOCK_SIZE
        const size_t requiredBlockCount = cbArenaAlignUp(size, CB_ARENA_BLOCK_SIZE);

        // allocate new block
        CbArenaAllocation *newAllocation = (CbArenaAllocation *)calloc(
            sizeof(CbArenaAllocation) + requiredBlockCount * CB_ARENA_BLOCK_SIZE,
            1
        );

        if (newAllocation == NULL)
            return NULL;

        // append new allocation to allocation stack
        newAllocation->next = arena->allocations;
        arena->allocations = newAllocation;

        // setup curr/end pointers
        arena->curr = newAllocation->data;
        arena->end = newAllocation->data + requiredBlockCount * CB_ARENA_BLOCK_SIZE;

        // calculate new allocationStart/allocationEnd pair
        allocationStart = (void *)cbArenaAlignUp((size_t)arena->curr, sizeof(max_align_t));
        allocationEnd = (uint8_t *)allocationStart + size;
    }

    arena->curr = allocationEnd;

    return allocationStart;
} // cbArenaAlloc

// cb_arena.c
