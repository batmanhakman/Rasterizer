#include "framebuffer.h"
#include <algorithm>
#include <limits>
#include <stdexcept>

Framebuffer::Framebuffer(int width, int height) : width(width), height(height)
{
    if (width <= 0 || height <= 0 || width > 16384 || height > 16384)
        throw std::invalid_argument("Framebuffer dimensions must be between 1 and 16384");
    pixels.resize(static_cast<size_t>(width) * height);
    depth.resize(pixels.size());
    Clear(0xff000000u);
}

void Framebuffer::SetPixel(int x, int y, uint32_t color)
{
    if (x >= 0 && x < width && y >= 0 && y < height)
        SetPixelUnchecked(x, y, color);
}

void Framebuffer::SetPixelUnchecked(int x, int y, uint32_t color)
{
    pixels[static_cast<size_t>(y) * width + x] = color;
}

void Framebuffer::SetPixelDepthUnchecked(int x, int y, float z, uint32_t color)
{
    const size_t index = static_cast<size_t>(y) * width + x;
    if (z < depth[index])
    {
        depth[index] = z;
        pixels[index] = color;
    }
}

uint32_t Framebuffer::GetPixel(int x, int y) const
{
    return x >= 0 && x < width && y >= 0 && y < height
        ? pixels[static_cast<size_t>(y) * width + x] : 0;
}

void Framebuffer::Clear(uint32_t color)
{
    std::fill(pixels.begin(), pixels.end(), color);
    std::fill(depth.begin(), depth.end(), std::numeric_limits<float>::infinity());
}
