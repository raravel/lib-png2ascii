#pragma once

typedef struct png_minimal_info {
	png_uint_32 width;
	png_uint_32 height;
	png_bytepp buf;
} png_minfo_t;

typedef struct _png_rgba_pixel {
	png_byte red;
	png_byte green;
	png_byte blue;
	png_byte alpha;
} png_rgba_pixel_t;

typedef struct _ascii_info {
	int width;
	int wsize;
	int height;
	char **buf;
	png_rgba_pixel_t **color;
} ascii_info_t;

png_minfo_t* read_png( char *file );
void free_png_minfo( png_minfo_t *minfo );
ascii_info_t* png2ascii( png_minfo_t *minfo );
void free_ascii_info( ascii_info_t *info );