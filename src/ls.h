#pragma once

#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
    uint32_t crc;
    uint32_t offset;
    uint32_t size;
    uint16_t dtIndex;
    uint16_t padding;
} ls_entry_t;

typedef struct {
    uint16_t magic;
    uint16_t version;
    uint32_t numEntries;

    ls_entry_t *entries;
} ls_t;
#pragma pack(pop)

ls_t* ls_load(const char *filename);
void ls_free(ls_t *ls);
void printLsEntry(ls_entry_t *entry);
