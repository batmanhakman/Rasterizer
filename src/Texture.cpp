#include "Texture.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <iterator>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STBI_MAX_DIMENSIONS 8192
#include "stb_image.h"

bool Texture::Load(const std::filesystem::path& path, std::string& error)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
    {
        error = "Cannot open texture: " + path.u8string();
        return false;
    }
    const std::vector<unsigned char> bytes{
        std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    if (bytes.empty() || bytes.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
    {
        error = "Texture file is empty or too large: " + path.u8string();
        return false;
    }
    int loadedWidth = 0, loadedHeight = 0, channels = 0;
    unsigned char* rgba = stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()),
        &loadedWidth, &loadedHeight, &channels, 4);
    if (!rgba)
    {
        error = "Cannot decode PNG " + path.u8string() + ": " + stbi_failure_reason();
        return false;
    }
    std::vector<uint32_t> loaded(static_cast<size_t>(loadedWidth) * loadedHeight);
    for (size_t i = 0; i < loaded.size(); ++i)
    {
        loaded[i] = (uint32_t(rgba[4*i+3]) << 24) | (uint32_t(rgba[4*i]) << 16) |
                    (uint32_t(rgba[4*i+1]) << 8) | uint32_t(rgba[4*i+2]);
    }
    stbi_image_free(rgba);
    width = loadedWidth;
    height = loadedHeight;
    pixels = std::move(loaded);
    error.clear();
    return true;
}

uint32_t Texture::Sample(float u, float v) const
{
    if (Empty() || width <= 0 || height <= 0 || !std::isfinite(u) || !std::isfinite(v))
        return 0xffffffffu;
    // Keep exact endpoints on the edge texel (useful for texture atlases).
    if (u < 0.0f || u > 1.0f) u -= std::floor(u);
    if (v < 0.0f || v > 1.0f) v -= std::floor(v);
    const int x = std::clamp(static_cast<int>(u * width), 0, width - 1);
    const int y = std::clamp(static_cast<int>((1.0f - v) * height), 0, height - 1);
    return pixels[static_cast<size_t>(y) * width + x];
}
