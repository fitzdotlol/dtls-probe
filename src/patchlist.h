#pragma once

#include <stdint.h>

#define PATCHLIST_ENTRY_LEN (0x80)

typedef struct {
    uint32_t magic;
    uint32_t numFiles;
    
    uint8_t dontKnowDontCare[0x78];
} patchlist_header_t;

typedef struct {
    patchlist_header_t header;
    char **files;
} patchlist_t;

patchlist_t* patchlist_loadFromFile(const char *filename);
void patchlist_saveToFile(patchlist_t *patchlist, const char *filename);
void patchlist_free(patchlist_t *patchlist);

void patchlist_append(patchlist_t *patchlist, const char *str);
