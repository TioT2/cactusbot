/**
 * @brief cactusbot implementation file
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "cb.h"
#include "cb_arena.h"


typedef struct __CbStr {
    const char *begin;
    const char *end;
} CbStr;

#define CB_STR(str) ((CbStr) { (const char *)(str), (const char *)(str) + strlen((str)) })

/// @brief node structure forward declaration
typedef struct __CbNode CbNode;

/// @brief node structure
struct __CbNode {
    bool    isLeaf;     ///< if true leaf content should be used, interior otherwise
    CbNode *parent;     ///< parent node pointer

    union {
        struct {
            CbNode *left;  ///< left child
            CbNode *right; ///< right child
        } leaf; ///< leaf node contents

        struct {
            CbNode *correct;   ///< next node if correct
            CbNode *incorrect; ///< next node if incorrect
        } interior; ///< interior node contents
    };

    char text[1]; ///< node text
}; // struct __CbNode

/// @brief cactusbot implementation structure
typedef struct __CbImpl {
    CbNode       *treeRoot;     ///< root of main (quest) tree
    size_t        treeSize;     ///< count of elements in tree
    CbNode       *leafTreeRoot; ///< root of leaf tree
    size_t        leafTreeSize; ///< count of elements in leaf tree
    CbArena       arena;        ///< arena allocator
} CbImpl;

/**
 * @brief node allocation function
 * 
 * @param[in,out] arena arena to allocate node in pointer (non-null)
 * @param[in]     text  node text (non-null)
 * 
 * @return allocated node pointer
 */
static CbNode * cbAllocNode( CbArena arena, CbStr text ) {
    assert(arena != NULL);

    const size_t textSize = text.end - text.begin;
    CbNode *const node = (CbNode *)cbArenaAlloc(arena, sizeof(CbNode) + textSize);

    if (node == NULL)
        return NULL;

    memcpy(node->text, text.begin, textSize);

    return node;
} // cbAllocNode

Cb cbCtor( const char *rootEntry ) {
    CbImpl *impl = NULL;
    CbArena arena = NULL;
    CbNode *node = NULL;

    if (false
        || (arena = cbArenaCtor()) == NULL
        || (impl = (CbImpl *)cbArenaAlloc(arena, sizeof(CbImpl))) == NULL
        || (node = cbAllocNode(arena, CB_STR(rootEntry))) == NULL
    ) {
        cbArenaDtor(arena);
        return NULL;
    }

    impl->arena = arena;
    node->isLeaf = true;

    impl->treeRoot = node;
    impl->leafTreeRoot = node;

    impl->treeSize = 1;

    return impl;
} // cbCtor

void cbDtor( Cb self ) {
    if (self != NULL)
        cbArenaDtor(self->arena); // self is allocated by self->arena
} // cbDtor

CbIter cbIter( Cb const self ) {
    return (CbIter) {
        .self = self,
        .node = &self->treeRoot,
    };
} // cbIter

const char * cbIterGetText( const CbIter *const iter ) {
    assert(iter != NULL);

    return (*iter->node)->text;
} // cbIterGetText

void cbIterNext( CbIter *const entry, const bool isCorrect ) {
    assert(entry != NULL);

    if ((*entry->node)->isLeaf)
        return;

    if (isCorrect)
        entry->node = &(*entry->node)->interior.correct;
    else
        entry->node = &(*entry->node)->interior.incorrect;
} // cbIterNext

bool cbIterFinished( const CbIter *entry ) {
    return (*entry->node)->isLeaf;
} // cbIterFinished

bool cbIterInsertCorrect( CbIter *entry, const char *condition, const char *correct ) {
    if (!(*entry->node)->isLeaf)
        return false;

    CbNode *conditionNode = cbAllocNode(entry->self->arena, CB_STR(condition));
    CbNode *correctNode = cbAllocNode(entry->self->arena, CB_STR(correct));

    if (conditionNode == NULL || correctNode == NULL)
        return false;

    conditionNode->parent = (*entry->node)->parent;
    (*entry->node)->parent = conditionNode;

    conditionNode->interior.correct = correctNode;
    conditionNode->interior.incorrect = *entry->node;

    correctNode->isLeaf = true;
    correctNode->parent = conditionNode;

    *entry->node = conditionNode;

    entry->self->treeSize += 2;

    return true;
} // cbIterInsertCorrect

/**
 * @brief node dumping funciton
 * 
 * @param[out] out   output file
 * @param[in]  node  node to dump pointer
 * @param[in]  depth current node depth
 */
static void cbDumpNode( FILE *out, const CbNode *node, const size_t depth ) {
    for (size_t i = 0; i < depth * 4; i++)
        fputc(' ', out);

    if (node->isLeaf) {
        fprintf(out, "\"%s\"\n", node->text);
    } else {
        fprintf(out, "(\"%s\"\n", node->text);
        cbDumpNode(out, node->interior.correct,   depth + 1);
        cbDumpNode(out, node->interior.incorrect, depth + 1);

        for (size_t i = 0; i < depth * 4; i++)
            fputc(' ', out);
        fputs(")\n", out);
    }
} // cbDump

void cbDump( FILE *out, const Cb self ) {
    cbDumpNode(out, self->treeRoot, 0);
} // cbDump

typedef enum __CbTokenType {
    CB_TOKEN_LEFT_BRACKET,  ///< (
    CB_TOKEN_RIGHT_BRACKET, ///< )
    CB_TOKEN_STRING,        ///< "<smth>"
} CbTokenType;

typedef struct __CbToken {
    CbTokenType type;   ///< token type
    CbStr       string; ///< string (if token type is CB_TOKEN_STRING). actually, this structure is tagged union.
} CbToken;

static bool cbIsSpace( const char ch ) {
    return false
        || ch == ' '
        || ch == '\r'
        || ch == '\n'
        || ch == '\t'
    ;
} // cbIsSpace

/**
 * @brief next token parsing function
 * 
 * @param[in,out] str token string
 * @param[out]    dst parsing destination
 */
static bool cbNextToken( CbStr *const str, CbToken *const dst ) {
    while (str->begin < str->end && cbIsSpace(*str->begin))
        str->begin++;

    if (str->begin >= str->end)
        return false;

    switch (*str->begin) {
    case '(': {
        str->begin++;
        dst->type = CB_TOKEN_LEFT_BRACKET;
        return true;
    }

    case ')': {
        str->begin++;
        dst->type = CB_TOKEN_RIGHT_BRACKET;
        return true;
    }

    case '\"': {
        str->begin++;

        const char *strBegin = str->begin;
        while (str->begin < str->end && *str->begin != '\"')
            str->begin++;

        dst->string = {strBegin, str->begin};
        str->begin++;
        dst->type = CB_TOKEN_STRING;

        return true;
    }

    default:
        return false; // error?
    }
} // cbNextToken

/**
 * @brief node parsing function
 */
size_t cbParseNode( CbStr *const rest, CbArena arena, CbNode **dst ) {
    CbToken token = {};

    if (!cbNextToken(rest, &token))
        return 0;

    switch (token.type) {
    case CB_TOKEN_LEFT_BRACKET: {
        CbToken identToken = {};
        CbToken rightBracketToken = {};
        CbNode *correct = NULL;
        CbNode *incorrect = NULL;
        size_t correctCount = 0;
        size_t incorrectCount = 0;
        CbNode *node = NULL;

        if (false
            || !cbNextToken(rest, &identToken)
            || identToken.type != CB_TOKEN_STRING
            || (correctCount   = cbParseNode(rest, arena, &correct  ))  == 0
            || (incorrectCount = cbParseNode(rest, arena, &incorrect))  == 0
            || (node           = cbAllocNode(arena, identToken.string)) == NULL
            || !cbNextToken(rest, &rightBracketToken)
            || rightBracketToken.type != CB_TOKEN_RIGHT_BRACKET
        ) {
            return 0;
        }

        correct->parent = node;
        incorrect->parent = node;

        node->interior.correct   = correct;
        node->interior.incorrect = incorrect;

        *dst = node;

        return correctCount + incorrectCount + 1;
    }

    case CB_TOKEN_RIGHT_BRACKET: {
        return 0;
    }

    case CB_TOKEN_STRING: {
        CbNode *node = (CbNode *)cbAllocNode(arena, token.string);
        if (node == NULL)
            return 0;
        node->isLeaf = true;

        *dst = node;
        return 1;
    }
    }

    return 0;
} // cbParseNode

bool cbParse( const char *const str, Cb *const dst ) {
    CbStr text = CB_STR(str);

    CbArena arena = NULL;
    CbNode *node = NULL;
    size_t size = 0;
    CbImpl *impl = NULL;

    if (false
        || (arena = cbArenaCtor()) == NULL
        || (size = cbParseNode(&text, arena, &node)) == 0
        || (impl = (CbImpl *)cbArenaAlloc(arena, sizeof(CbImpl))) == NULL
    ) {
        cbArenaDtor(arena);
        return false;
    }

    impl->arena = arena;

    impl->leafTreeRoot = node;
    impl->leafTreeSize = 0; // not constructed yet

    impl->treeRoot = node;
    impl->treeSize = size;

    *dst = impl;

    return true;
} // cbParse

void cbDumpNodeDot( FILE *out, const CbNode *node, size_t *const nodeId ) {
    const size_t currId = (*nodeId)++;

    fprintf(out, "    node%zu [label = \"%s", currId, node->text);
    if (node->isLeaf)
        fprintf(out, "\", shape = box");
    else
        fprintf(out, "?\"");
    fprintf(out, "];\n");

    if (!node->isLeaf) {
        const size_t correctId = *nodeId;
        cbDumpNodeDot(out, node->interior.correct, nodeId);

        const size_t incorrectId = *nodeId;
        cbDumpNodeDot(out, node->interior.incorrect, nodeId);

        fprintf(out, "    node%zu -> node%zu [label = \"T\"];\n", currId, correctId);
        fprintf(out, "    node%zu -> node%zu [label = \"F\"];\n", currId, incorrectId);
    }
} // cbDumpNodeDot

void cbDumpDot( FILE *out, const Cb self ) {
    size_t nodeId = 0;

    fprintf(out, "digraph {\n");
    cbDumpNodeDot(out, self->treeRoot, &nodeId);
    fprintf(out, "}");

} // cbDumpDot

// cb.c
