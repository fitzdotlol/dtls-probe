#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <raylib.h>
#include <raymath.h>
#include <time.h>

#define STB_DS_IMPLEMENTATION
#include "vendor/stb_ds.h"

#include "rf.h"
#include "patchlist.h"
#include "filetree.h"
#include "explorer.h"

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
        res->filename = node->filename;

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
    // TODO: lol
    printf("WARNING filetree_free still not implemented\n");
}

resource_t *cloneResource(resource_t *res)
{
    resource_t *newRes = (resource_t*)malloc(sizeof(*newRes));
    newRes->packOffset = res->packOffset;
    newRes->filename = strdup(res->filename);
    newRes->sizeCompressed = res->sizeCompressed;
    newRes->sizeUncompressed = res->sizeUncompressed;
    newRes->timestamp = res->timestamp;
    newRes->flags = res->flags;

    return newRes;
}

filetree_node_t* filetree_fromResources(resource_t *resources)
{
    int lastDepth = 0;
    filetree_node_t *prevNode;

    size_t numResources = stbds_arrlenu(resources);
    for (int i = 0; i < numResources; ++i) {
        resource_t *res = cloneResource(&resources[i]);
        // FIXME: probably pull depth out of flags.
        // this isn't terribly expensive, but what's the point?
        int depth = res->flags & 0xff;

        filetree_node_t *node = (filetree_node_t*)calloc(1, sizeof(*node));
        node->parent = NULL;
        node->children = NULL;
        node->path = NULL;
        node->filename = res->filename;
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
    root->parent = NULL;
    root->children = NULL;
    root->path = NULL;
    // TODO: can this just be null? I'd love to just remove all of these
    // as this is zeroed anyway.
    root->filename = "";
    root->res = NULL;

    preRoot->parent = root;
    stbds_arrput(root->children, preRoot);

    return root;
}

filetree_node_t* filetree_fromRFFile(const char *filename)
{
    resource_t *resources = loadResourcesFromRFFile(filename);
    filetree_node_t *tree = filetree_fromResources(resources);
    freeResources(resources);

    return tree;
}

// Reconstruct full paths from hierarchy & filenames.
// not particularly efficient.
void fillTreePaths(filetree_node_t *node)
{
    node->path = strdup(node->filename);
    filetree_node_t *n = node;

    while (n->parent) {
        n = n->parent;

        char *oldPath = node->path;

        node->path = strdup(TextFormat("%s%s", n->filename, oldPath));
        free(oldPath);
    }

    size_t numChildren = stbds_arrlenu(node->children);
    for (int i = 0; i < numChildren; ++i) {
        fillTreePaths(node->children[i]);
    }
}

void writeTreePathsToPatchlist(filetree_node_t *node, patchlist_t *patchlist)
{
    size_t len = strlen(node->path);

    if (len == 0) {
        
    } else if (node->path[len-1] == '/') {
    // FIXME: maybe shoud check for folder flag instead

    } else {
        // HACK:
        const char *path = TextFormat("data/%s", node->path);
        patchlist_append(patchlist, path);
    }

    size_t numChildren = stbds_arrlenu(node->children);
    for (int i = 0; i < numChildren; ++i) {
        filetree_node_t *child = node->children[i];
        writeTreePathsToPatchlist(child, patchlist);
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
    resource_t *resources = NULL;
    size_t len = filetree_calculateLength(tree);

    stbds_arrsetcap(resources, len);

    filetree_flattenToResourcesInner(tree, resources);

    return resources;
}

void concatResources(resource_t *dest, resource_t *src)
{
    size_t srcLen = stbds_arrlenu(src);
    for (int i = 0; i < srcLen; ++i) {
        stbds_arrput(dest, src[i]);
    }
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
        destNode->res = cloneResource(srcNode->res);
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
            printf("<<< can't insert %s yet!\n", srcChild->path);
        }
    }
}

#define GAME_CONTENT_PATH "/home/fitz/cemu_games/Super Smash Bros for Wii U [Game] [0005000010144f00]/content/"
#define UPDATE_CONTENT_PATH "/home/fitz/cemu_games/Super Smash Bros for Wii U [Update] [0005000e10144f00]/content/patch/"
#define MOD_WORKSPACE_PATH "/home/fitz/s4workspace/"
#define MOD_CONTENT_PATH "/home/fitz/.local/share/Cemu/graphicPacks/SuperSmashBrosVice/content/patch/"

int main(int argc, char **argv)
{
    filetree_node_t *resFileTree = filetree_fromRFFile(UPDATE_CONTENT_PATH "resource(us_en)");
    filetree_node_t *localFileTree = filetree_fromWorkspacePath(MOD_WORKSPACE_PATH);

    // NOTE: populate `path` fields
    fillTreePaths(resFileTree);
    fillTreePaths(localFileTree);

    // filetree_merge(resFileTree, localFileTree);

    {
        patchlist_t *patchlist = patchlist_loadFromFile(UPDATE_CONTENT_PATH "patchlist");
        writeTreePathsToPatchlist(localFileTree, patchlist);
        patchlist_saveToFile(patchlist, MOD_CONTENT_PATH "patchlist");
        patchlist_free(patchlist);
    }

    {
        resource_t *newResources = filetree_flattenToResources(resFileTree);
        saveResourcesToRFFile(newResources, MOD_CONTENT_PATH "resource(us_en)");
        freeResources(newResources);
    }

    startExplorerWindow(resFileTree);

    filetree_free(resFileTree);
    filetree_free(localFileTree);

    printf("Done 1.\n");

    // Now test our new file...
    {
        resource_t *res2 = loadResourcesFromRFFile(MOD_CONTENT_PATH "resource(us_en)");
        freeResources(res2);
    }

    printf("Done 2.\n");

    return 0;
}
