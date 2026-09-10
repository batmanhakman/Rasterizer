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