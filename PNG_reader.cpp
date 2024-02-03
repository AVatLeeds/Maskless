#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include <png.h>

#include "PNG_reader.h"


int PNG::open(char *path)
{
    file_ptr = fopen(path, "rb");
    if (file_ptr == NULL)
    {
        std::cerr << "Error: Failed to open test image file.\n";
        return -1;
    }

    const size_t header_length_bytes = 8;
    const png_byte png_header_buffer[header_length_bytes] = {0};
    size_t length_read = fread((void *)(&png_header_buffer), sizeof(png_byte), header_length_bytes, file_ptr);
    if (length_read != header_length_bytes)
    {
        std::cerr << "Error: Failed to read " << header_length_bytes << " bytes from start of file.\n";
        return -1;
    }
    
    int isnt_png = png_sig_cmp(png_header_buffer, 0, header_length_bytes);
    if (isnt_png)
    {
        std::cerr << "Error: File doesn't seem to be a PNG.\n";
        return -1;
    }

    png_read_struct_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_read_struct_ptr == NULL)
    {
        std::cerr << "Error: Failed to create PNG read structure.\n";
        return -1;
    }

    png_info_ptr = png_create_info_struct(png_read_struct_ptr);
    if (png_info_ptr == NULL)
    {
        png_destroy_read_struct(&png_read_struct_ptr, NULL, NULL);
        std::cerr << "Error: Failed to create PNG info structure.\n";
        return -1;
    }

    png_init_io(png_read_struct_ptr, file_ptr);

    png_set_sig_bytes(png_read_struct_ptr, header_length_bytes);

    png_read_png(png_read_struct_ptr, png_info_ptr, PNG_TRANSFORM_SCALE_16 | PNG_TRANSFORM_STRIP_ALPHA | PNG_TRANSFORM_BGR, NULL);

    data_ptr = png_get_rows(png_read_struct_ptr, png_info_ptr);
}

unsigned int PNG::width()
{
    return png_get_image_width(png_read_struct_ptr, png_info_ptr);
}

unsigned int PNG::height()
{
    return png_get_image_height(png_read_struct_ptr, png_info_ptr);
}

unsigned int PNG::row_bytes()
{
    return png_get_rowbytes(png_read_struct_ptr, png_info_ptr);
}

PNG::~PNG()
{
    free(data_ptr);
    png_destroy_read_struct(&png_read_struct_ptr, &png_info_ptr, NULL);
    fclose(file_ptr);
}