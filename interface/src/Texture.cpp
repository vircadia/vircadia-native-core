//
//  Texture.cpp
//  interface
//
//  Added by Yoz on 11/5/12.
//
//  Code lifted from http://en.wikibooks.org/wiki/OpenGL_Programming/Intermediate/Textures

#include "Texture.h"

#include "InterfaceConfig.h"
#include <lodepng.h>
#include <vector>
#include <cstdio>

#define TEXTURE_LOAD_ERROR 0

/**
 * Read a given filename as a PNG texture, and set 
   it as the current default OpenGL texture.
 * @param[in]   filename    Relative path to PNG file
 * @return  Zero for success.
 */

int load_png_as_texture(char* filename)
{
    std::vector<unsigned char> image;
    // width and height will be read from the file at the start
    // and loaded into these vars
    unsigned int width = 1, height = 1;
    unsigned error = lodepng::decode(image, width, height, filename);
    if (error) {
        std::cout << "Error loading texture" << std::endl;
        return (int) error;
    }

    // Make some OpenGL properties better for 2D and enable alpha channel.
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    
    if(glGetError() != GL_NO_ERROR)
    {
        std::cout << "Error initing GL" << std::endl;
        return 1;
    }
    
    // Texture size must be power of two for the primitive OpenGL version this is written for. Find next power of two.
    size_t u2 = 1; while(u2 < width) u2 *= 2;
    size_t v2 = 1; while(v2 < height) v2 *= 2;
    // Ratio for power of two version compared to actual version, to render the non power of two image with proper size.
    // double u3 = (double)width / u2;
    // double v3 = (double)height / v2;
    
    // Make power of two version of the image.
    std::vector<unsigned char> image2(u2 * v2 * 4);
    for(size_t y = 0; y < height; y++)
        for(size_t x = 0; x < width; x++)
            for(size_t c = 0; c < 4; c++)
            {
                image2[4 * u2 * y + 4 * x + c] = image[4 * width * y + 4 * x + c];
            }
    
    // Enable the texture for OpenGL.
    glEnable(GL_TEXTURE_2D);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //GL_NEAREST = no smoothing
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, u2, v2, 0, GL_RGBA, GL_UNSIGNED_BYTE, &image2[0]);

    return 0;
}

