#pragma once

#include <stdint.h>
#include <stdio.h>

typedef enum {
    ENDIAN_BIG,
    ENDIAN_LITTLE
} file_endian_t;

typedef struct {
    uint8_t *data;
    size_t ptr;
    file_endian_t endian;
} filereader_t;

void swapEndian16(int16_t *num);
void swapEndian32(int32_t *num);

void file_skip(filereader_t *file, size_t len);
void file_align(filereader_t *file);

int16_t file_readInt16(filereader_t *file);
uint16_t file_readUInt16(filereader_t *file);
int32_t file_readInt32(filereader_t *file);
float file_readFloat(filereader_t *file);
char* file_readString(filereader_t *file, size_t len);
char* file_readCString(filereader_t *file);
