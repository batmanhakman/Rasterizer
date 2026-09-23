#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct Texture
{
    int width = 0;
    int height = 0;
    // Packed AARRGGBB pixels, top row first.
    std::vector<uint32_t> pixels;

    bool Load(const std::filesystem::path& path, std::string& error);
    bool Empty() const { return pixels.empty(); }
    // OBJ UVs start at the bottom-left. Coordinates outside [0,1] repeat.
    uint32_t Sample(float u, float v) const;
};
