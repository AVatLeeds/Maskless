#ifndef PNG_READER_H
#define PNG_READER_H

#include <cstdint>
#include <cstdio>

#include <png.h>

class PNG
{
    public:
    PNG();
    ~PNG();

    int open(char * path);
    unsigned int width();
    unsigned int height();
    unsigned int row_bytes();

    uint8_t **  data_ptr = NULL;

    private:
    FILE * file_ptr = NULL;
    png_struct * png_read_struct_ptr = NULL;
    png_info * png_info_ptr = NULL;
};

#endif