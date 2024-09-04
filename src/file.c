#include "file.h"
#include <stdint.h>
#include <string.h>

void swapEndian16(int16_t *num)
{
    *num = (*num >> 8) | (*num << 8);
}

void swapEndianU16(uint16_t *num)
{
    *num = (*num >> 8) | (*num << 8);
}

void swapEndian32(int *num)
{
    *num = ((*num>>24) & 0x000000FF)
        | ((*num<<8) & 0x00FF0000)
        | ((*num>>8) & 0x0000FF00)
        | ((*num<<24) & 0xFF000000);
}

void file_skip(filereader_t *file, size_t len)
{
    file->ptr += len;
}

void file_align(filereader_t *file)
{
    file->ptr += (4 - (file->ptr % 4));
}

int16_t file_readInt16(filereader_t *file)
{
    int16_t value = *(int16_t*)(file->data + file->ptr);

    // FIXME: should also take host endianness into account.
    if (file->endian == ENDIAN_BIG) {
        swapEndian16(&value);
    }

    file->ptr += sizeof(value);

    return value;
}

uint16_t file_readUInt16(filereader_t *file)
{
    uint16_t value = *(uint16_t*)(file->data + file->ptr);

    // FIXME: should also take host endianness into account.
    if (file->endian == ENDIAN_BIG) {
        swapEndianU16(&value);
    }

    file->ptr += sizeof(value);

    return value;
}

int32_t file_readInt32(filereader_t *file)
{
    int32_t value = *(int32_t*)(file->data + file->ptr);

    // FIXME: should also take host endianness into account.
    if (file->endian == ENDIAN_BIG) {
        swapEndian32(&value);
    }

    file->ptr += sizeof(value);

    return value;
}

float file_readFloat(filereader_t *file)
{
    float value = *(float*)(file->data + file->ptr);

    // FIXME: should also take host endianness into account.
    if (file->endian == ENDIAN_BIG) {
        swapEndian32((int32_t*)&value);
    }

    file->ptr += sizeof(value);

    return value;
}

// FIXME: uhh this should actually use the length...
char* file_readString(filereader_t *file, size_t len)
{
    char *str = strdup((char*)file->data+file->ptr);
    file->ptr += len;

    return str;
}

char* file_readCString(filereader_t *file)
{
    char *str = strdup((char*)file->data+file->ptr);
    file->ptr += (strlen(str) + 1);

    return str;
}
