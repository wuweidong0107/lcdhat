#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <linux/fb.h>
#include <string.h>
#include <sys/ioctl.h>
#include <wchar.h>
#include <math.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

static unsigned char *fb_mem;
static int screen_bytes;
static int line_bytes;
static int pixel_bytes;
static struct fb_var_screeninfo var;
static struct fb_fix_screeninfo fix;

static int fontsize;
static int fontcolor;
static FT_Library    library;
static FT_Face       face;
static FT_GlyphSlot  slot;
static FT_Matrix     matrix;                 /* transformation matrix */
static FT_Vector     pen;                    /* untransformed origin  */
static FT_Error      error;

static void fb_draw_pixel(int x, int y, int color)
{
    unsigned char *pixel_8 = fb_mem + y * line_bytes + x * pixel_bytes;
    unsigned short *pixel_16 = (unsigned short *)(pixel_8);
    unsigned int *pixel_32 = (unsigned int *)(pixel_8);
    int red,green,blue;
    
    switch(var.bits_per_pixel) {
        case 8: {
            *pixel_8 = color;
            break;
        }
        case 16: {
            // rgb:5bit 6bit 5bit 
            red   = (color >> 16) & 0xff;
            green = (color >> 8) & 0xff;
            blue  = (color >> 0) & 0xff;
            color = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3);
            *pixel_16 = color;
            break;
        }
        case 32: {
            // rgb:8bit 8bit 8bit
            *pixel_32 = color;
            break;
        }
        default: {
            fprintf(stderr, "unsupported bpp %d\n", var.bits_per_pixel);
        }
    }
}

static void fb_draw_bitmap(FT_Bitmap*  bitmap, FT_Int x, FT_Int y, int color)
{
    FT_Int  i, j, p, q;
    FT_Int  x_max = x + bitmap->width;
    FT_Int  y_max = y + bitmap->rows;


    for (i = x, p = 0; i < x_max; i++, p++) {
        for (j = y, q = 0; j < y_max; j++, q++) {
            if (i < 0 || j < 0 || i >= (FT_Int)var.xres || j >= (FT_Int)var.yres)
                continue;

            if (bitmap->buffer[q * bitmap->width + p] != 0)
                fb_draw_pixel(i, j, color);
            else
                fb_draw_pixel(i, j, 0);
        }
    }
}

void fb_exit()
{
    FT_Done_Face(face);
    FT_Done_FreeType(library);
}

void fb_ft_print(const char *str, int line, int size, int color)
{
    unsigned int n = 0;

    if (size > 0) {
        fontsize = size;
        FT_Set_Pixel_Sizes(face, fontsize, 0);
    }

    if (line < 1 && (unsigned int )line*fontsize > var.yres)
        return;

    /* start at (0,40) relative to the upper left corner  */
    pen.x = 0 * 64;
    pen.y = (var.yres- fontsize*line) * 64;

    for (n = 0; n < strlen(str); n++) {
        if (str[n] == '\n') {
            line++;
            pen.x = 0 * 64;
            pen.y = (var.yres - fontsize*line) * 64;
            continue;
        }

        FT_Set_Transform(face, &matrix, &pen);

        error = FT_Load_Char(face, str[n], FT_LOAD_RENDER);
        if (error)
            continue;                 /* ignore errors */
        
        /* now, draw to our target surface (convert position) */
        fb_draw_bitmap( &slot->bitmap, slot->bitmap_left, var.yres - slot->bitmap_top, color);

        /* increment pen position */
        if ((unsigned int )(slot->bitmap_left + fontsize) >= var.xres) {
            line++;
            pen.x = 0 * 64;
            pen.y = (var.yres - fontsize*line) * 64;
        } else {
            pen.x += slot->advance.x;
            pen.y += slot->advance.y;
        }
    }
}

void fb_clear()
{
    memset(fb_mem, 0, screen_bytes);
}

int fb_ft_init(int fd, const char *font, int size, int color)
{
    double angle = 0;
    
    if (ioctl(fd, FBIOGET_VSCREENINFO, &var) == -1) {
        fprintf(stderr, "fail to FBIOGET_VSCREENINFO\n");
        return -1;
    }
    if (ioctl(fd, FBIOGET_FSCREENINFO, &fix) == -1) {
        fprintf(stderr, "fail to FBIOGET_VSCREENINFO\n");
        return -1;
    }
    
    screen_bytes = var.xres * var.yres * var.bits_per_pixel / 8;
    line_bytes = var.xres * var.bits_per_pixel / 8;
    pixel_bytes = var.bits_per_pixel / 8;
    
    if ((fb_mem = mmap(NULL, screen_bytes, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED) {
        fprintf(stderr, "fail to mmap\n");
        return -1;
    }

    // clear fb
    memset(fb_mem, 0, screen_bytes);

    FT_Init_FreeType(&library);              /* initialize library */
    if (FT_New_Face(library, font, 0, &face) < 0) {
        fprintf(stderr, "fail to FT_New_Face()\n");
        return -1;
    }

    fontsize = size;
    fontcolor = color;
    FT_Set_Pixel_Sizes(face, fontsize, 0);
    slot = face->glyph;

    /* set up matrix */
    matrix.xx = (FT_Fixed)( cos( angle ) * 0x10000L );
    matrix.xy = (FT_Fixed)(-sin( angle ) * 0x10000L );
    matrix.yx = (FT_Fixed)( sin( angle ) * 0x10000L );
    matrix.yy = (FT_Fixed)( cos( angle ) * 0x10000L );

    return 0;
}

