#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "vendor/stb_ds.h"

#include "patchlist.h"

patchlist_t* patchlist_loadFromFile(const char *filename)
{
    patchlist_t *patchlist = (patchlist_t*)calloc(1, sizeof(*patchlist));

    FILE *file = fopen(filename, "rb");
    fread(&patchlist->header, sizeof(patchlist->header), 1, file);

    fseek(file, 0, SEEK_END);
    size_t dataSize = ftell(file) - sizeof(patchlist->header);
    fseek(file, sizeof(patchlist->header), SEEK_SET);

    if (dataSize != patchlist->header.numFiles * PATCHLIST_ENTRY_LEN) {
        assert(0 && "[patchlist] incorrect file count in header");
    }

    char *data = (char*)malloc(dataSize);
    fread(data, dataSize, 1, file);
    fclose(file);

    for (int i = 0; i < patchlist->header.numFiles; ++i) {
        char *name = data + (i * PATCHLIST_ENTRY_LEN);
        stbds_arrput(patchlist->files, strdup(name));
    }

    free(data);

    return patchlist;
}

// NOTE: mutates (read: updates) header.numFiles
void patchlist_saveToFile(patchlist_t *patchlist, const char *filename)
{
    patchlist->header.numFiles = stbds_arrlenu(patchlist->files);

    uint8_t pad[PATCHLIST_ENTRY_LEN] = { 0 };

    FILE *file = fopen(filename, "wb");
    fwrite(&patchlist->header, sizeof(patchlist->header), 1, file);

    for (int i = 0; i < patchlist->header.numFiles; ++i) {
        size_t len = strlen(patchlist->files[i]);
        fwrite(patchlist->files[i], len, 1, file);
        fwrite(pad, PATCHLIST_ENTRY_LEN-len, 1, file);
    }

    fclose(file);
}

void patchlist_free(patchlist_t *patchlist)
{
    size_t numFiles = stbds_arrlenu(patchlist->files);
    for (int i = 0; i < numFiles; ++i) {
        free(patchlist->files[i]);
    }

    stbds_arrfree(patchlist->files);

    free(patchlist);
}

void patchlist_append(patchlist_t *patchlist, const char *str)
{
    stbds_arrput(patchlist->files, strdup(str));
}
