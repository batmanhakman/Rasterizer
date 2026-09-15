#pragma once
#include <cstdint>

class Framebuffer
{
    public:
    Framebuffer(int width, int height );

    // clean up memory
    ~Framebuffer();

    // get dimensions
    int GetWidth() const {return width; }
    int GetHeight() const {return height; }

    // Opposite of RGBA to get desiered colors, that just how a mac works.
    uint32_t color(uint32_t r, uint32_t g, uint32_t b, uint32_t a) const
    {
        return  ((a & 0xffu) << 24) |
                ((r & 0xffu) << 16) |
                ((g & 0xffu) << 8)  |
                (b & 0xffu);
    };


    // Get the pixels and color and store it
    uint32_t GetPixel(int x, int y) const;
    
    uint32_t* GetBuffer() const;

    // Set the pixels
    void SetPixel(int x, int y, uint32_t color);

    // clear colors
    void Clear(uint32_t color);


    private:
    int width;
    int height;
    uint32_t* pixels;
};