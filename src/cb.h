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

/// @brief object definition iterator representation structure
typedef struct __CbDefIter {
    const struct __CbNode *element; ///< element
} CbDefIter;

/// @brief object definition status
typedef enum __CbDefineStatus {
    CB_DEFINE_STATUS_OK,            ///< successfully created defIter
    CB_DEFINE_STATUS_NO_SUBJECT,    ///< no defined subject in definition database
    CB_DEFINE_STATUS_NO_DEFINITION, ///< no definition exists for the subject
} CbDefineStatus;

/**
 * @brief definition iterator getting function
 * 
 * @param[in]  self    cb pointer (non-null)
 * @param[in]  subject subject to defint name
 * @param[out] dst     iterator destination (nullable)
 * 
 * @return definition status
 * 
 * @note it's ok to use dst contents if functions returns CB_DEFINE_STATUS_OK.
 */
CbDefineStatus cbDefine( const Cb self, const char *subject, CbDefIter *dst );

/**
 * @brief next property getting function
 * 
 * @param[in,out] iter iterator to get next element of (non-null, not finished)
 * 
 * @return next property text. NULL if definition finished.
 */
const char * cbDefIterGetProperty( const CbDefIter *iter );

/**
 * @brief relation to next property getting function
 * 
 * @param[in] iter iterator to get relation to next property of (non-null, not finished)
 * 
 * @return true if defined object satisfies property got from cbDefIterGetProperty function.
 */
bool cbDefIterGetRelation( const CbDefIter *iter );

/**
 * @brief next property getting function
 * 
 * @param[in] iter iterator (non-null, not finished)
 * 
 * @return true if it's ok to continue definition iteration by iter, false if not.
 */
bool cbDefIterNext( CbDefIter *iter );

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

/***
 * Debug functions
 ***/

/**
 * @brief CF dot text dumping function
 * 
 * @param[out] out  destination file
 * @param[in]  self self pointer
 */
void cbDbgDumpDot( FILE *out, const Cb self );

/**
 * @brief leaf tree in dot format dumping function
 * 
 * @param[in,out] out output file
 * @param[in]     cb  cb to dump
 */
void cbDbgLeafTreeDumpDot( FILE *out, const Cb cb );

#ifdef __cplusplus
}
#endif // defined(__cplusplus)

#endif // !defined(cb_h_)

// cb.h
