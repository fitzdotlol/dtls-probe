#pragma once

// for size_t
#include <stdio.h>

#include "rf.h"

typedef struct filetree_node_t {
    struct filetree_node_t *parent;
    struct filetree_node_t **children;

    char *path;
    char *filename;
    resource_t *res;

    bool expanded;
} filetree_node_t;

filetree_node_t* filetree_fromRFFile(const char *filename);
filetree_node_t* filetree_fromWorkspacePath(const char *path);
void filetree_free(filetree_node_t *root);

void filetree_merge(filetree_node_t *destNode, filetree_node_t *srcNode);
void filetree_appendFromPath(filetree_node_t *root, const char *path, int depth);

size_t filetree_calculateLength(filetree_node_t *node);
void filetree_printNode(filetree_node_t *node, int depth);
void filetree_print(filetree_node_t *root);
filetree_node_t* filetree_getChildWithFilename(filetree_node_t *node, const char *filename);

resource_t* filetree_flattenToResources(filetree_node_t *tree);
