#include <stdio.h>
#include <assert.h>

#include <raylib.h>

#include "vendor/stb_ds.h"

#include "filetree.h"

filetree_node_t* filetree_fromRFFile(const char *filename)
{
    resource_t *resources = loadResourcesFromRFFile(filename);
    int lastDepth = 0;
    filetree_node_t *prevNode;

    size_t numResources = stbds_arrlenu(resources);
    for (int i = 0; i < numResources; ++i) {
        resource_t *res = resource_clone(&resources[i]);
        // FIXME: probably pull depth out of flags.
        // this isn't terribly expensive, but what's the point?
        int depth = res->flags & 0xff;

        filetree_node_t *node = (filetree_node_t*)calloc(1, sizeof(*node));
        node->parent = NULL;
        node->children = NULL;
        node->path = NULL;
        node->filename = strdup(res->filename);
        node->res = res;

        if (depth == 0) {
            node->filename = strdup("");
            prevNode = node;
            lastDepth = 0;
        } else {
            if (depth > lastDepth) {
                node->parent = prevNode;

                assert((depth-lastDepth) <= 1 && "descending too far at once??");
            } else if (depth < lastDepth) {
                int depthDiff = lastDepth - depth;

                filetree_node_t *n = prevNode;
                for (int j = 0; j < depthDiff; ++j) {
                    n = n->parent;
                }
                node->parent = n->parent;
            } else {
                node->parent = prevNode->parent;
            }

            stbds_arrput(node->parent->children, node);
            prevNode = node;
            lastDepth = depth;
        }
    }

    // FIXME: This is all stupid and basically to make the output match
    // the output of `filetree_fromWorkspacePath`. so... fix that?
    filetree_node_t *preRoot = prevNode;
    while (preRoot->parent) {
        preRoot = preRoot->parent;
    }

    filetree_node_t *root = (filetree_node_t*)calloc(1, sizeof(*root));

    preRoot->parent = root;
    stbds_arrput(root->children, preRoot);

    freeResources(resources);

    return root;
}

void filetree_appendFromPath(filetree_node_t *root, const char *path, int depth)
{
    FilePathList pathList = LoadDirectoryFiles(path);

    assert(depth <= 0xFF);

    for (int i = 0; i < pathList.count; ++i) {
        filetree_node_t *node = (filetree_node_t*)calloc(1, sizeof(*node));
        node->parent = root;
        node->children = NULL;
        node->path = strdup(pathList.paths[i]);

        resource_t *res = (resource_t*)calloc(1, sizeof(*res));
        node->res = res;

        if (IsPathFile(node->path)) {
            // is file
            node->filename = strdup(GetFileName(pathList.paths[i]));
            int size = GetFileLength(node->path);
            res->sizeCompressed = size;
            res->sizeUncompressed = size;
            res->flags = (RES_FLAG_OVERRIDE | RES_FLAG_NO_LOC);
        } else {
            node->filename = strdup(TextFormat("%s/", GetFileName(pathList.paths[i])));
            // FIXME: oh god
            if (!strcmp(node->filename, "data/")) {
                free(node->filename);
                node->filename = strdup("");
            }
            res->flags = (RES_FLAG_DIR);
            filetree_appendFromPath(node, node->path, depth + 1);
        }

        res->flags |= (depth & 0xFF);
        res->filename = strdup(node->filename);

        stbds_arrput(root->children, node);
    }
}

filetree_node_t* filetree_fromWorkspacePath(const char *path)
{
    filetree_node_t *root = (filetree_node_t*)calloc(1, sizeof(*root));
    root->path = strdup("");
    root->filename = strdup("");

    filetree_appendFromPath(root, path, 0);

    return root;
}


void filetree_printNode(filetree_node_t *node, int depth)
{
    int d = depth-1;

    for (int i = 0; i < d; ++i) {
        if (i == d-1) {
            printf("├ ");
        } else {
            printf("│ ");
        }
    }

    printf("%s\n", node->filename);

    size_t numChildren = stbds_arrlenu(node->children);
    for (int i = 0; i < numChildren; ++i) {
        filetree_node_t *child = node->children[i];
        filetree_printNode(child, depth+1);
    }
}

void filetree_print(filetree_node_t *root)
{
    filetree_printNode(root, 0);
}

void filetree_free(filetree_node_t *root)
{
    free(root->filename);
    free(root->path);

    size_t numChildren = stbds_arrlenu(root->children);
    for (int i = 0; i < numChildren; ++i) {
        filetree_free(root->children[i]);
    }

    free(root);
}

filetree_node_t* filetree_getChildWithFilename(filetree_node_t *node, const char *filename)
{
    size_t numChildren = stbds_arrlenu(node->children);

    for (int i = 0; i < numChildren; ++i) {
        filetree_node_t *child = node->children[i];
        if (!strcmp(child->filename, filename)) {
            return child;
        }
    }

    return NULL;
}

void filetree_merge(filetree_node_t *destNode, filetree_node_t *srcNode)
{
    // if node is a file, replace resource
    if (srcNode->res && !(srcNode->res->flags & RES_FLAG_DIR)) {
        destNode->res = resource_clone(srcNode->res);
        printf("replacing %s...\n", destNode->path);
    }

    size_t numSrcChildren = stbds_arrlenu(srcNode->children);

    for (int i = 0; i < numSrcChildren; ++i) {
        filetree_node_t *srcChild = srcNode->children[i];
        filetree_node_t *destChild = filetree_getChildWithFilename(destNode, srcChild->filename);

        if (destChild) {
            // if dest has child with src filename, recurse
            filetree_merge(destChild, srcChild);
        } else {
            // otherwise, insert cloned src
            printf("FIXME: <<< can't insert %s yet!\n", srcChild->path);
        }
    }
}

size_t filetree_calculateLength(filetree_node_t *node)
{
    size_t len = 1;

    size_t numChildren = stbds_arrlenu(node->children);
    for (int i = 0; i < numChildren; ++i) {
        len += filetree_calculateLength(node->children[i]);
    }

    return len;
}

void filetree_flattenToResourcesInner(filetree_node_t *node, resource_t *resources)
{
    // XXX: should this really ever be null?
    if (node->res) {
        resource_t res = { 0 };
        res.packOffset = node->res->packOffset;
        res.filename = strdup(node->res->filename);
        res.sizeCompressed = node->res->sizeCompressed;
        res.sizeUncompressed = node->res->sizeUncompressed;
        res.timestamp = node->res->timestamp;
        res.flags = node->res->flags;
        stbds_arrput(resources, res);
    }

    size_t numChildren = stbds_arrlenu(node->children);
    for (int i = 0; i < numChildren; ++i) {
        filetree_flattenToResourcesInner(node->children[i], resources);
    }
}

resource_t* filetree_flattenToResources(filetree_node_t *tree)
{
    // NOTE: failure to allocate enough memory causes stbds to
    // reallocate mid-run and completely ruin everything.
    resource_t *resources = NULL;
    size_t len = filetree_calculateLength(tree);
    stbds_arrsetcap(resources, len);

    filetree_flattenToResourcesInner(tree, resources);

    return resources;
}
