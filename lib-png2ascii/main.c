#include <stdio.h>
#include <png.h>

int main( void )
{
	png_structp png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
	printf( "Hello World!\n" );
	return 0;
}