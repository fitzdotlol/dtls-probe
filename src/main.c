#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <raylib.h>
#include <raymath.h>

#define STB_DS_IMPLEMENTATION
#include "vendor/stb_ds.h"

#include "rf.h"
#include "patchlist.h"
#include "filetree.h"
#include "explorer.h"

// Construct full paths from hierarchy & filenames. not particularly efficient.
void fillTreePaths(filetree_node_t *node)
{
    node->path = strdup(node->filename ? node->filename : "");
    filetree_node_t *n = node;

    while (n->parent && n->parent->filename) {
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

    // TODO: I'm not sure if the len check is actually needed here.
    // if len == 0, then there's likely a corresponding difference
    // in the resource that should be checked instead.

    if (len != 0 && !(node->res->flags & RES_FLAG_DIR)) {
        // FIXME: base dir (data/, data(us_en)/), should be stored
        // in the tree somewhere. Somewhere? near the root, I imagine lol.
        const char *path = TextFormat("data/%s", node->path);
        patchlist_append(patchlist, path);
    }

    size_t numChildren = stbds_arrlenu(node->children);
    for (int i = 0; i < numChildren; ++i) {
        filetree_node_t *child = node->children[i];
        writeTreePathsToPatchlist(child, patchlist);
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

    // FIXME: this was the old strategy. rather than clobber the (still useful)
    // original resources, it would be prudent to keep the trees separate until
    // the time of export.
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

    return 0;
}
