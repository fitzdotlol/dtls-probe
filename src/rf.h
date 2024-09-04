#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    RES_FLAG_UNK_100  = 0x0100,
    RES_FLAG_DIR      = 0x0200,
    RES_FLAG_PACKED   = 0x0400,
    RES_FLAG_NO_LOC   = 0x0800,
    RES_FLAG_UNK_1000 = 0x1000,
    RES_FLAG_DEBUG    = 0x2000,
    RES_FLAG_OVERRIDE = 0x4000,
    RES_FLAG_UNK_8000 = 0x8000,
} resource_flag_t;

#pragma pack(push, 1)
typedef struct {
    char magic[2];
    uint16_t version;
    uint32_t headerSize;
    uint32_t padding;

    uint32_t entriesBlockOffset;
    uint32_t entriesBlockSize;
    uint32_t timestamp;
    uint32_t sizeCompressed;
    uint32_t sizeUncompressed;

    uint32_t stringBlockOffset;
    uint32_t stringBlockSize;
    uint32_t numEntries;

    uint32_t aaPad[0x15];
} rf_header_t;

typedef struct {
    uint32_t packOffset;
    uint32_t nameInfo;
    uint32_t sizeCompressed;
    uint32_t sizeUncompressed;
    uint32_t timestamp;
    uint32_t flags;
} rf_entry_t;
#pragma pack(pop)

typedef struct {
    uint32_t packOffset;
    char *filename;
    uint32_t sizeCompressed;
    uint32_t sizeUncompressed;
    uint32_t timestamp;
    resource_flag_t flags;
} resource_t;

resource_t* loadResourcesFromRFFile(const char *filename);
void saveResourcesToRFFile(resource_t *resources, const char *filename);
void freeResources(resource_t *resources);
