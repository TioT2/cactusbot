/**
 * @brief arena allocator implementation file
 */

#ifndef CB_ARENA_H_
#define CB_ARENA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/// @brief arena allocator constructor
typedef struct __CbArenaImpl * CbArena;

/**
 * @brief arena constructor
 * 
 * @return created arena allocator
 */
CbArena cbArenaCtor( void );

/**
 * @brief arena destructor
 * 
 * @param[in,out] arena arena to destroy
 */
void cbArenaDtor( CbArena arena );

/**
 * @brief arena allocation function
 * 
 * @param[in] arena arena pointer (non-null)
 * @param[in] size  allocation size
 * 
 * @return allocated memory pointer
 * 
 * @note function returns null if something went wrong or size == 0
 * @note allocation result is aligned by size of 'max_align_t' type
 * @note allocation result is filled with zeros
 */
void * cbArenaAlloc( CbArena arena, size_t size );

#ifdef __cplusplus
}
#endif

#endif // !defined(CB_ARENA_H_)

// cb_arena.h
