#pragma once
#include <cstdint>
#include <vector>

class Framebuffer
{
public:
    Framebuffer(int width, int height);
    int GetWidth() const { return width; }
    int GetHeight() const { return height; }
    // The same packed AARRGGBB format is used on every platform.
    uint32_t color(uint32_t r, uint32_t g, uint32_t b, uint32_t a) const
    {
        return ((a & 0xffu) << 24) | ((r & 0xffu) << 16) |
               ((g & 0xffu) << 8) | (b & 0xffu);
    }
    uint32_t GetPixel(int x, int y) const;
    uint32_t* GetBuffer() const { return const_cast<uint32_t*>(pixels.data()); }
    void SetPixel(int x, int y, uint32_t color);
    void SetPixelUnchecked(int x, int y, uint32_t color);
    // Coordinates must already be clipped to the framebuffer.
    void SetPixelDepthUnchecked(int x, int y, float z, uint32_t color);
    void Clear(uint32_t color);
private:
    int width;
    int height;
    std::vector<uint32_t> pixels;
    std::vector<float> depth;
};
