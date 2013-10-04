#ifndef MileagePlusWallet_palettes_h
#define MileagePlusWallet_palettes_h

#include <stdlib.h>

/*
 * Get a palette of N colors from an image.
 * 
 * This function expects the pixel data to be in RGBA8888 format. It will fill the `colors`
 * array with no more than `*numColors` colors. After this function is complete, `numColors`
 * will point to the real number of colors that were placed in the `colors` array.
 */
void get_palette(const uint8_t *pixels, size_t numPixels, uint8_t *colors, size_t *numColors);

/*
 * Get the most important color from an image.
 *
 * This is a highly subjective thing and this function will mostly be useful if you need a quick 
 * representative color from an image or as an example to be modified for your specific needs.
 */
void get_dominant_color(const uint8_t *pixels, size_t numPixels, uint8_t *color);

#endif
