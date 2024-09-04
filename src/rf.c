#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <zlib.h>

#include "vendor/stb_ds.h"

#include "rf.h"

typedef struct {
    char *key;
    uint32_t value;
} string_table_entry_t;

resource_t* loadResourcesFromRFFile(const char *filename)
{
    resource_t *resources = NULL;

    rf_header_t header = { 0 };
    uint8_t *uncompressedData = NULL;
    char **extensions = NULL;
    rf_entry_t *entries = NULL;
    char *stringsData = NULL;

    // uncompress data from file
    {
        FILE *fd = fopen(filename, "rb");
        assert(fd && "Cannot open file");

        fseek(fd, 0, SEEK_END);
        size_t dataSize = ftell(fd);
        fseek(fd, 0, SEEK_SET);
        uint8_t *compressedData = (uint8_t*)malloc(dataSize);

        fread(compressedData, dataSize, 1, fd);
        fclose(fd);

        memcpy(&header, compressedData, sizeof(header));

        uncompressedData = (uint8_t*)calloc(1, header.sizeUncompressed);

        // NOTE: `uncompress` mutates this, but is a different size
        // so use a new variable to avoid clobbering other header fields.
        uint64_t destLen = header.sizeUncompressed;
        uncompress(
            uncompressedData, &destLen, // dest
            compressedData+header.headerSize, header.sizeCompressed // src
        );

        free(compressedData);

        entries = (rf_entry_t*)(uncompressedData);
        stringsData = (char*)uncompressedData + header.stringBlockOffset - header.headerSize + 4;
    }

    // build extension list
    {
        uint8_t *addr = uncompressedData + header.stringBlockOffset - header.headerSize;
        uint32_t numStringSections = *(uint32_t*)(addr);
        addr += 4;

        // seek past strings
        addr += numStringSections * 0x2000;
        uint32_t numExtensions = *(uint32_t*)addr;
        addr += 4;

        for (int i = 0; i < numExtensions; ++i) {
            uint32_t extIdx = *(uint32_t*)(addr + i * 4);
            char *ext = strdup(stringsData + extIdx);
            stbds_arrput(extensions, ext);
        }
    }

    for (int i = 0; i < header.numEntries; ++i) {
        rf_entry_t *entry = &entries[i];
        resource_t res = { 0 };

        uint32_t strOffset = entry->nameInfo & 0x000FFFFF;
        uint32_t extensionIdx = entry->nameInfo >> 24;

        res.packOffset = entry->packOffset;
        res.filename = NULL;
        res.sizeCompressed = entry->sizeCompressed;
        res.sizeUncompressed = entry->sizeUncompressed;
        res.timestamp = entry->timestamp;
        res.flags = entry->flags;

        const char *extension = extensions[extensionIdx];
        const char *name;
        if ((entry->nameInfo & 0x00800000)) {
            uint16_t ref = *(uint16_t*)(stringsData + strOffset);
            uint16_t len = (ref & 0x1F) + 4;
            // ????? this is not very intuitive.
            uint16_t off = (ref & 0xE0) >> 6 << 8 | (ref >> 8);

            // slice first part
            char *s = strdup((stringsData + strOffset) - off);
            s[len] = '\0';

            char buf[1024] = { 0 };
            sprintf(buf, "%s%s", s, stringsData + strOffset + 2);
            free(s);
            name = buf;
        } else {
            name = stringsData + strOffset;
        }

        res.filename = (char*)calloc(1, strlen(name) + strlen(extension) + 1);
        sprintf(res.filename, "%s%s", name, extension);

        stbds_arrput(resources, res);
    }

    // Free temporary data
    size_t numExtensions = stbds_arrlenu(extensions);
    for (int i = 0; i < numExtensions; ++i) {
        free(extensions[i]);
    }
    stbds_arrfree(extensions);

    free(uncompressedData);

    return resources;
}

void saveResourcesToRFFile(resource_t *resources, const char *filename)
{
    size_t numResources = stbds_arrlenu(resources);
    rf_entry_t *entriesOut = (rf_entry_t*)calloc(numResources, sizeof(*entriesOut));

    size_t stringsSize = 0;
    string_table_entry_t *stringMap = NULL;
    sh_new_strdup(stringMap);

    for (int resourceIdx = 0; resourceIdx < numResources; ++resourceIdx) {
        resource_t *res = &resources[resourceIdx];
        uint32_t strOffset = 0;
        uint32_t extensionIdx = 0;

        string_table_entry_t *strKv = stbds_shgetp_null(stringMap, res->filename);
        if (strKv) {
            strOffset = strKv->value;
        } else {
            strOffset = stringsSize;
            stbds_shput(stringMap, res->filename, strOffset);
            stringsSize += strlen(res->filename) + 1;
        }

        rf_entry_t *entry = &entriesOut[resourceIdx];
        entry->packOffset = res->packOffset;
        entry->nameInfo = (strOffset & 0x000FFFFF) | (extensionIdx << 24);
        entry->sizeCompressed = res->sizeCompressed;
        entry->sizeUncompressed = res->sizeUncompressed;
        entry->timestamp = res->timestamp;
        entry->flags = res->flags;
    }

    // FIXME: this file is just a hack to avoid buffering
    // but it is helpful for debugging as well at the moment.
    FILE *decompressedFileOut = fopen("tmp_decomp.bin", "w+b");
    assert(decompressedFileOut);
    
    fwrite(entriesOut, sizeof(rf_entry_t), numResources, decompressedFileOut);
    
    // Align to 0x80.
    {
        size_t padSize = (0x80 - (ftell(decompressedFileOut) % 0x80)) % 0x80;
        uint8_t *bbPadBuff = (uint8_t*)malloc(padSize);
        memset(bbPadBuff, 0xBB, padSize);
        fwrite(bbPadBuff, padSize, 1, decompressedFileOut);
        free(bbPadBuff);
    }

    // NOTE: strings are in blocks of 0x2000 bytes, so this chunk
    // is necessarily aligned to 0x2000.
    size_t numStrings = stbds_shlenu(stringMap);
    size_t stringsPadSize = (0x2000 - ((stringsSize) % 0x2000)) % 0x2000;
    size_t stringChunkSize = stringsSize + stringsPadSize;

    uint32_t numStringSections = (stringChunkSize / 0x2000);
    fwrite(&numStringSections, 4, 1, decompressedFileOut);

    for (int i = 0; i < numStrings; ++i) {
        string_table_entry_t *str = &stringMap[i];
        fwrite(str->key, strlen(str->key)+1, 1, decompressedFileOut);
    }

    uint8_t *padBuf = (uint8_t*)calloc(1, stringsPadSize);
    fwrite(padBuf, stringsPadSize, 1, decompressedFileOut);
    free(padBuf);

    // NOTE: extensions are dumb and pointless. we don't use that nonsense.
    uint32_t numExtensions = 1;
    fwrite(&numExtensions, 4, 1, decompressedFileOut);
    uint32_t nullExt = 0;
    fwrite(&nullExt, 4, 1, decompressedFileOut);

    // Again, align to 0x80
    {
        size_t padSize = (0x80 - (ftell(decompressedFileOut) % 0x80)) % 0x80;
        uint8_t *bbPadBuff = (uint8_t*)malloc(padSize);
        memset(bbPadBuff, 0xBB, padSize);
        fwrite(bbPadBuff, padSize, 1, decompressedFileOut);
        free(bbPadBuff);
    }

    //// Write header + compressed data
    rf_header_t header = { 0 };
    memset(&header, 0xAA, sizeof(header)); // not as stupid as it looks
    header.magic[0] = 'R';
    header.magic[1] = 'F';
    header.version = 6; // I GUESS. p sure it isn't read.
    header.headerSize = sizeof(header);
    header.padding = 0;
    header.entriesBlockOffset = header.headerSize;
    header.entriesBlockSize = numResources * sizeof(rf_entry_t);
    header.timestamp = 0;
    header.sizeCompressed = 0;
    header.sizeUncompressed = ftell(decompressedFileOut);
    header.stringBlockOffset = header.entriesBlockOffset + header.entriesBlockSize;
    header.stringBlockOffset += (0x80 - (header.stringBlockOffset % 0x80)) % 80;
    header.stringBlockSize = stringsSize;
    header.numEntries = numResources;

    uint8_t *uncompressedData = (uint8_t*)malloc(header.sizeUncompressed);
    fseek(decompressedFileOut, 0, SEEK_SET);
    fread(uncompressedData, header.sizeUncompressed, 1, decompressedFileOut);
    fclose(decompressedFileOut);

    free(entriesOut);

    uint64_t compressedDataSize = header.sizeUncompressed;
    uint8_t *compressedData = (uint8_t*)malloc(compressedDataSize);

    compress(
        compressedData, &compressedDataSize,
        uncompressedData, header.sizeUncompressed
    );

    free(uncompressedData);

    header.sizeCompressed = compressedDataSize;

    // Finally, create the actual resource file.
    FILE *finalOut = fopen(filename, "wb");
    fwrite(&header, sizeof(header), 1, finalOut);
    fwrite(compressedData, compressedDataSize, 1, finalOut);
    free(compressedData);
    fclose(finalOut);
}

void freeResources(resource_t *resources)
{
    size_t numResources = stbds_arrlenu(resources);

    for (int i = 0; i < numResources; ++i) {
        resource_t *res = &resources[i];
        free(res->filename);
    }

    stbds_arrfree(resources);
}

resource_t *resource_clone(resource_t *res)
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
