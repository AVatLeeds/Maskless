// Comile with g++ -Wall png_test.cpp -o png_test.exec -lpng
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <stdlib.h>
#include <stdint.h>

#include <png.h>

int main(int argc, char * argv[])
{
    std::cout << "Compiled with libpng version " << PNG_LIBPNG_VER_STRING << ". Using libpng " << png_libpng_ver << ".\n";

    FILE * file_ptr = fopen("./test_image.png", "rb");
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
    }
    
    int isnt_png = png_sig_cmp(png_header_buffer, 0, header_length_bytes);
    if (isnt_png)
    {
        std::cerr << "Error: File doesn't seem to be a PNG.\n";
        return -1;
    }

    png_struct * png_struct_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_struct_ptr == NULL)
    {
        std::cerr << "Error: Failed to create PNG read structure.\n";
        return -1;
    }

    png_info * png_info_ptr = png_create_info_struct(png_struct_ptr);
    if (png_info_ptr == NULL)
    {
        png_destroy_read_struct(&png_struct_ptr, NULL, NULL);
        std::cerr << "Error: Failed to create PNG info structure.\n";
        return -1;
    }

    png_init_io(png_struct_ptr, file_ptr);

    png_set_sig_bytes(png_struct_ptr, header_length_bytes);

    png_read_png(png_struct_ptr, png_info_ptr, PNG_TRANSFORM_SCALE_16 | PNG_TRANSFORM_STRIP_ALPHA, NULL);

    uint32_t width = png_get_image_width(png_struct_ptr, png_info_ptr);
    uint32_t height = png_get_image_height(png_struct_ptr, png_info_ptr);

    std::cout << "Image has dimensions " << width << " x " << height << std::endl;
    return 0;
}