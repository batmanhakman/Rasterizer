#include <iostream>
#include "framebuffer.h"

Framebuffer::Framebuffer(int width, int height)
{   
    // saving width and height parameters
    this->width = width;
    this->height = height;

    // allocate the 1d pixel array
    pixels = new uint32_t[width * height];
}

Framebuffer::~Framebuffer()
{   
    // pixels needs cleaning up as it does not go away once the framebuffer is over
    delete[] pixels;
    // reset pixels to nothing
    pixels = nullptr;
}

void Framebuffer::SetPixel(int x, int y, uint32_t color)
{   
    if (x >= 0 && x < width && y >= 0 && y < height)
    {
        // calculate 1D index
        int index = (y * width) + x;
        // write color to memory
        pixels[index] = color;
    }
}


uint32_t* Framebuffer::GetBuffer() const
{
    return pixels;
}

uint32_t Framebuffer::GetPixel(int x, int y) const
{
    if (x >= 0 && x < width && y >= 0 && y < height)
    {
        // calculate 1D Array
        int index = (y * width) + x;

        // return pixel value
        return pixels[index];
    }

    // if out of bounds, return default color
    return 0;
} 

void Framebuffer::Clear(uint32_t color)
{
    // calculate total number of pixels.
    int totalPixels = width * height;

    // starts at pixel 0 and clears all pixels until it reaches the value of totalPixels.
    for (int i = 0; i < totalPixels; i++)
    {
        pixels[i] = color;
    }
}