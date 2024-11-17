/**
 * @brief cactusbot implementation file
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "cb.h"
#include "cb_arena.h"

/// @brief constant string slice
typedef struct __CbStr {
    const char *begin; ///< stirng slice begin (inclusive)
    const char *end;   ///< string slice end (exclusive)
} CbStr;

/// @brief string slice from constant string construction macro.
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
    impl->treeSize = 1;

    impl->leafTreeRoot = node;
    impl->leafTreeSize = 1;


    return impl;
} // cbCtor

void cbDtor( Cb self ) {
    if (self != NULL)
        cbArenaDtor(self->arena); // self is allocated by self->arena
} // cbDtor

/**
 * @brief node pointer in leaf tree searching function
 * 
 * @param[in] root leaf tree root (non-null)
 * @param[in] name leaf to insert name, non-null
 * 
 * @return (non-null) pointer to place to where node with 'name' name should be located.
 */
CbNode ** cbLeafTreeFind( CbNode **root, const char *name ) {
    while (*root != NULL) {
        const int cmp = strcmp(name, (*root)->text);

        if (cmp > 0)
            root = &(*root)->leaf.left;
        else if (cmp < 0)
            root = &(*root)->leaf.right;
        else
            return root;
    }

    return root;
} // cbLeafTreeInsert

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
    
    CbNode **leafTreeDst = cbLeafTreeFind(&entry->self->leafTreeRoot, correct);

    if (*leafTreeDst != NULL) // leaf is already added
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

    *leafTreeDst = correctNode;

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

        dst->string = (CbStr) {strBegin, str->begin};
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
size_t cbParseNode( CbStr *const rest, CbArena arena, CbNode **dst, CbNode **leafTreeRoot, size_t *leafTreeSize ) {
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
            || (correctCount   = cbParseNode(rest, arena, &correct  , leafTreeRoot, leafTreeSize))  == 0
            || (incorrectCount = cbParseNode(rest, arena, &incorrect, leafTreeRoot, leafTreeSize))  == 0
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
        CbNode *node = NULL;
        CbNode **leafTreeDst = NULL;

        if (false
            || (node = (CbNode *)cbAllocNode(arena, token.string)) == NULL
            || *(leafTreeDst = cbLeafTreeFind(leafTreeRoot, node->text)) != NULL
        )
            return 0;

        node->isLeaf = true;
        (*leafTreeSize)++;

        *dst = node;
        *leafTreeDst = node;

        return 1;
    }
    }

    return 0;
} // cbParseNode

bool cbParse( const char *const str, Cb *const dst ) {
    CbStr text = CB_STR(str);

    CbArena arena = NULL;

    CbNode * treeRoot = NULL;
    size_t   treeSize = 0;

    CbNode * leafTreeRoot = NULL;
    size_t   leafTreeSize = 0;

    CbImpl *impl = NULL;

    if (false
        || (arena = cbArenaCtor()) == NULL
        || (treeSize = cbParseNode(&text, arena, &treeRoot, &leafTreeRoot, &leafTreeSize)) == 0
        || (impl = (CbImpl *)cbArenaAlloc(arena, sizeof(CbImpl))) == NULL
    ) {
        cbArenaDtor(arena);
        return false;
    }

    impl->arena = arena;

    impl->leafTreeRoot = leafTreeRoot;
    impl->leafTreeSize = leafTreeSize;

    impl->treeRoot = treeRoot;
    impl->treeSize = treeSize;

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
    size_t nodeId = 1;

    fprintf(out, "digraph {\n");
    cbDumpNodeDot(out, self->treeRoot, &nodeId);
    fprintf(out, "}");
} // cbDumpDot

/**
 * @brief leaf tree node dumping in dot format function
 * 
 * @param[in,out] out    output file
 * @param[in]     node   node to dump
 * @param[in,out] nodeId last node identifier
 */
static void cbDbgLeafTreeNodeDumpDot( FILE *const out, const CbNode *const node, size_t *const nodeId ) {
    assert(node != NULL);
    assert(nodeId != NULL);
    assert(node->isLeaf); // ?

    const size_t currentId = (*nodeId)++;

    fprintf(out, "    node%zu [label = \"%s\"];\n", currentId, node->text);

    if (node->leaf.left != NULL) {
        const size_t id = *nodeId;
        cbDbgLeafTreeNodeDumpDot(out, node->leaf.left, nodeId);
        fprintf(out, "    node%zu -> node%zu [label = \"L\"];\n", currentId, id);
    }

    if (node->leaf.right != NULL) {
        const size_t id = *nodeId;
        cbDbgLeafTreeNodeDumpDot(out, node->leaf.right, nodeId);
        fprintf(out, "    node%zu -> node%zu [label = \"R\"];\n", currentId, id);
    }
} // cbDbgLeafTreeNodeDumpDot

void cbDbgLeafTreeDumpDot( FILE *out, const Cb self ) {
    assert(self != NULL);

    size_t nodeId = 1;

    fprintf(out, "digraph {\n");
    cbDbgLeafTreeNodeDumpDot(out, self->leafTreeRoot, &nodeId);
    fprintf(out, "}");
} // cbDbgLeafTreeDumpDot

CbDefineStatus cbDefine( const Cb self, const char *subject, CbDefIter *dst ) {
    const CbNode *node = *cbLeafTreeFind(&self->leafTreeRoot, subject);

    if (node == NULL)
        return CB_DEFINE_STATUS_NO_SUBJECT;

    if (dst != NULL)
        *dst = (CbDefIter) { .element = node };

    return node->parent == NULL
        ? CB_DEFINE_STATUS_NO_DEFINITION
        : CB_DEFINE_STATUS_OK;
} // cbDefine

const char * cbDefIterGetProperty( const CbDefIter *iter ) {
    assert(iter != NULL);
    assert(iter->element != NULL);
    assert(iter->element->parent != NULL);

    return iter->element->parent->text;
} // cbDefIterGetProperty

bool cbDefIterGetRelation( const CbDefIter *iter ) {
    assert(iter != NULL);
    assert(iter->element != NULL);
    assert(iter->element->parent != NULL);

    return iter->element->parent->interior.correct == iter->element;
} // cbDefIterGetRelation

bool cbDefIterNext( CbDefIter *iter ) {
    assert(iter != NULL);
    assert(iter->element != NULL);
    assert(iter->element->parent != NULL);

    iter->element = iter->element->parent;

    return iter->element->parent != NULL;
} // cbDefIterNext

// cb.c
