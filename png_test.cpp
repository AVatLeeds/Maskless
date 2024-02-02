// Comile with g++ -Wall png_test.cpp -o png_test.exec -lpng
#include <iostream>

#include <png.h>

int main(int argc, char * argv[])
{
    std::cout << "Compiled with libpng version " << PNG_LIBPNG_VER_STRING << ". Using libpng " << png_libpng_ver << ".\n";
}