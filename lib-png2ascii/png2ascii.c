#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <Shlwapi.h>
#include <png.h>
#include "png2ascii.h"


const char PREFIX_ASCII[] = "#,.0123456789:;@ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz$";

png_minfo_t* read_png( char *file )
{
	png_byte signature[8] = { 0, };
	png_minfo_t *pinfo = NULL;

	if ( PathFileExistsA( file ) ) {
		FILE *fp = fopen( file, "rb" );
		if ( !fp ) {
			printf( "Can not file open. [%s]\n", file );
			return NULL;
		}
		
		/* signature read, check png signature */
		fread( signature, 1, sizeof( signature ), fp );
		if ( png_sig_cmp( signature, 0, sizeof( signature )) != 0 ) {
			printf("File [%s] is not png.\n", file);
			fclose( fp );
			return NULL;
		}


		/* init png struct pointer */
		png_structp png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );

		if ( !png_ptr ) {
			printf( "png_create_read_struct faild\n" );
			fclose( fp );
			return NULL;
		}

		png_infop info_ptr = png_create_info_struct( png_ptr );

		if ( !info_ptr ) {
			printf( "png_create_info_struct faild\n" );
			png_destroy_read_struct( &png_ptr, NULL, NULL );
			fclose( fp );
			return NULL;
		}

		png_infop end_info = png_create_info_struct( png_ptr );
		if ( !end_info ) {
			printf( "png_create_info_struct [end_info]\n" );
			png_destroy_read_struct( &png_ptr, &info_ptr, NULL );
		}

		if ( setjmp( png_jmpbuf( png_ptr ) ) ) {
			printf( "Error during init_io\n" );
			png_destroy_read_struct( &png_ptr, &info_ptr, &end_info );
			fclose( fp );
			return NULL;
		}

		/* reading png file info IHDR */
		png_init_io( png_ptr, fp );
		png_set_sig_bytes( png_ptr, sizeof( signature ) );
		png_read_info( png_ptr, info_ptr );


		png_uint_32 width = png_get_image_width( png_ptr, info_ptr );
		png_uint_32 height = png_get_image_height( png_ptr, info_ptr );
		png_byte color_type = png_get_color_type( png_ptr, info_ptr );
		png_byte bit_depth = png_get_bit_depth( png_ptr, info_ptr );
		int pass = png_set_interlace_handling( png_ptr );
		png_read_update_info( png_ptr, info_ptr );

		if ( png_get_color_type( png_ptr, info_ptr ) != PNG_COLOR_TYPE_RGBA ) {
			printf( "File color type must be PNG_COLOR_TYPE_RGBA\n" );
			png_destroy_read_struct( &png_ptr, &info_ptr, &end_info );
			fclose( fp );
			return NULL;
		}


		/* create minimal info struct */
		pinfo = (png_minfo_t*)malloc( sizeof( png_minfo_t ) );
		pinfo->width = width;
		pinfo->height = height;
		pinfo->buf = (png_bytepp)png_malloc( png_ptr, sizeof( png_bytep ) * height );

		/* reading image data */
		if ( setjmp( png_jmpbuf( png_ptr ) ) ) {
			printf( "Error during read image\n" );
			png_destroy_read_struct( &png_ptr, &info_ptr, &end_info );
			fclose( fp );
			return NULL;
		}

		png_size_t wsize = width * sizeof( png_rgba_pixel_t );
		for ( int y=0; y < height; y++ ) {
			pinfo->buf[y] = (png_bytep)png_malloc(png_ptr, png_get_rowbytes(png_ptr, info_ptr));
		}
		//png_set_rows( png_ptr, info_ptr, pinfo->buf );
		png_read_image( png_ptr, pinfo->buf );
		png_read_end( png_ptr, end_info );

		/* free pointer */
		png_destroy_read_struct( &png_ptr, &info_ptr, &end_info );
		fclose( fp );

		return pinfo;
	} else {
		printf( "No such file [%s]\n", file );
	}
	return NULL;
}

void free_png_minfo( png_minfo_t *minfo )
{
	if ( minfo ) {
		if ( minfo->buf ) {
			for ( int y = 0; y < minfo->height; y++ ) {
				if ( minfo->buf[y] ) {
					free( minfo->buf[y] );
				}
			}
			free( minfo->buf );
		}
		free( minfo );
	}
}

ascii_info_t* png2ascii( png_minfo_t *minfo )
{
	if ( minfo == NULL ) {
		printf( "Info struct is null.\n" );
		return NULL;
	}

	ascii_info_t *info = (ascii_info_t*)malloc( sizeof( ascii_info_t ) );
	info->width = minfo->width;
	info->wsize = ( ( minfo->width * 2 ) );
	info->height = minfo->height;
	info->buf = (char**)malloc( sizeof( char* ) * minfo->height );
	info->color = (png_rgba_pixel_t**)malloc( sizeof( png_rgba_pixel_t* ) * minfo->height );

	for ( int y = 0; y < minfo->height; y++ ) {
		png_bytep row = minfo->buf[y];
		info->buf[y] = (char*)malloc( sizeof( char ) * info->wsize );
		info->color[y] = (png_rgba_pixel_t*)malloc( sizeof( png_rgba_pixel_t ) * info->width);
		for ( int x = 0; x < minfo->width; x++ ) {
			png_rgba_pixel_t pixel;
			memcpy( &pixel, ( row +(x * sizeof( png_rgba_pixel_t )) ), sizeof( png_rgba_pixel_t ) );
			if ( pixel.alpha == 0 ) {
				snprintf( info->buf[y]+(x*2), ( info->wsize - (x*2)), "  " );
			} else {
				png_byte grey = ( pixel.red + pixel.green + pixel.blue ) / 3; // average
				char c = PREFIX_ASCII[grey * ( strlen( PREFIX_ASCII ) - 1 ) / 256];
				snprintf( info->buf[y] + ( x * 2 ), ( info->wsize - ( x * 2 ) ), "%c%c", c, c );
			}
			memcpy(&info->color[y][x], &pixel, sizeof(png_rgba_pixel_t));
		}
		info->buf[y][info->wsize - 1] = '\0';
	}

	return info;
}

void free_ascii_info( ascii_info_t *info ) 
{
	if ( info ) {
		if ( info->buf ) {
			for ( int y = 0; y < info->height; y++ ) {
				if ( info->buf[y] ) {
					free( info->buf[y] );
				}
			}
			free( info->buf );
		}
		free( info );
	}
}

#if 0
void test_print( ascii_info_t *info )
{
	for ( int y = 0; y < info->height; y++ ) {
		for ( int x = 0; x < info->width; x++ ) {
			png_rgba_pixel_t p = info->color[y][x];
			printf( "\033[38;2;%d;%d;%dm", p.red, p.green, p.blue ); \
			printf( "%c%c", info->buf[y][(x*2)], info->buf[y][(x*2) + 1] );
		}
		printf( "\n" );
	}
}

int main( void )
{
	png_minfo_t *minfo = read_png( "adventurer-attack1-00.png" );
	if ( minfo ) {
		ascii_info_t *ascii = png2ascii( minfo );
		test_print( ascii );
		free_ascii_info( ascii );
	}
	free_png_minfo( minfo );
	return 0;
}
#endif