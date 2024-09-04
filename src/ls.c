#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "stb_ds.h"

#include "ls.h"
#include "file.h"

ls_t* ls_load(const char *filename)
{
    FILE *fd = fopen(filename, "rb");
    assert(fd && "Cannot open file");

    fseek(fd, 0, SEEK_END);
    size_t dataSize = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    uint8_t *data_ = (uint8_t*)malloc(dataSize);

    filereader_t file = {
        .data = data_,
        .ptr = 0,
        .endian = ENDIAN_LITTLE,
    };

    fread(file.data, dataSize, 1, fd);
    fclose(fd);

    ls_t *ls = (ls_t*)calloc(1, sizeof(*ls));
    ls->magic = file_readUInt16(&file);
    ls->version = file_readUInt16(&file);
    ls->numEntries = file_readInt32(&file);

    for (int i = 0; i < ls->numEntries; ++i) {
        ls_entry_t entry = {
            .crc = file_readInt32(&file),
            .offset = file_readInt32(&file),
            .size = file_readInt32(&file),
            .dtIndex = file_readUInt16(&file),
            .padding = file_readUInt16(&file),
        };

        stbds_arrput(ls->entries, entry);
    }

    free(data_);

    return ls;
}

void ls_free(ls_t *ls)
{
    stbds_arrfree(ls->entries);
    free(ls);
}

void ls_entry_print(ls_entry_t *entry)
{
    printf("ls_entry_t {\n");
    printf("    .crc = 0x%04X,\n", entry->crc);
    printf("    .offset = 0x%04X,\n", entry->offset);
    printf("    .size = 0x%04X,\n", entry->size);
    printf("    .dtIndex = 0x%04X,\n", entry->dtIndex);
    printf("    .padding = 0x%04X,\n", entry->padding);
    printf("}\n");
}
