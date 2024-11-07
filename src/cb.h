/**
 * @brief cactusbot declaration file
 */

#ifndef CB_H_
#define CB_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif // defined(__cplusplus)

/// @brief the main project structure - cactusbot
typedef struct __CbImpl * Cb;

/**
 * @brief cactus bot constructor
 * 
 * @param[in] rootEntry root entry name
 * 
 * @return cactusbot handle, may return null if construction failed.
 */
Cb cbCtor( const char *rootEntry );

/**
 * @brief cactus bot destructor
 * 
 * @param[in] self cactusbot pointer
 */
void cbDtor( Cb self );

/// @brief entry representation structure
typedef struct __CbIter {
    Cb                self; ///< cb pointer
    struct __CbNode **node; ///< pointer to pointer to current node
} CbIter;

/**
 * @brief root entry getting function
 * 
 * @param[in] self cb pointer
 * 
 * @return new iterator
 */
CbIter cbIter( Cb self );

/**
 * @brief next element getting function
 * 
 * @param[in] entry iterator
 * @param[in] left true if go to to correct, false otherwise
 */
void cbIterNext( CbIter *entry, bool isCorrect );

/**
 * @brief node text getting function
 * 
 * @param[in] iter iterator (non-null)
 * 
 * @return iterator text
 */
const char * cbIterGetText( const CbIter *iter );

/**
 * @brief leaf condition checking function
 * 
 * @param[in] entry entry to get status of
 * 
 * @return true if iterator points to leaf, false otherwise.
 */
bool cbIterFinished( const CbIter *entry );

/**
 * @brief next branch insertion function
 * 
 * @param[in] entry     entry pointer
 * @param[in] condition condition pointer
 * @param[in] correct   correct condition string
 * 
 * @return true if succeeded, false if something went wrong
 */
bool cbIterInsertCorrect( CbIter *entry, const char *condition, const char *correct );

/**
 * @brief CF dot text dumping function
 * 
 * @param[out] out  destination file
 * @param[in]  self self pointer
 */
void cbDumpDot( FILE *out, const Cb self );

/**
 * @brief CF text dumping function
 * 
 * @param[out] out  destination file
 * @param[in]  self self pointer
 */
void cbDump( FILE *out, const Cb self );

/**
 * @brief CB from text parsing function
 * 
 * @param[in]  str string to parse CB from (non-null, zero-terminated.)
 * @param[out] dst parsing destination (non-null)
 * 
 * @return true if parsed, false if not.
 */
bool cbParse( const char *str, Cb *dst );

#ifdef __cplusplus
}
#endif // defined(__cplusplus)

#endif // !defined(cb_h_)

// cb.h
