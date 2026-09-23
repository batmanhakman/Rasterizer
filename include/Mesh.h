#pragma once

#include <array>
#include <cstdint>
#include <vector>

struct Vertex
{
    float x;
    float y;
    float z;
};

struct Face
{
    std::array<uint32_t, 3> indices;
};

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<Face> faces;
};
