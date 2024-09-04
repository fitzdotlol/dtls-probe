#pragma once

struct resource_t;

typedef struct filetree_node_t {
    struct filetree_node_t *parent;
    struct filetree_node_t **children;

    char *path;
    char *filename;
    resource_t *res;

    bool expanded;
} filetree_node_t;
