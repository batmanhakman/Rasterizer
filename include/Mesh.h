#pragma once

#include "Texture.h"
#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

struct Vertex
{
    float x;
    float y;
    float z;
};

struct TexCoord
{
    float u;
    float v;
};

struct Material
{
    std::string name;
    std::array<float, 3> diffuse = {1.0f, 1.0f, 1.0f};
    Texture texture;
    std::array<float, 3> specular = {0.0f, 0.0f, 0.0f};
    float shininess = 32.0f;
};

struct Face
{
    // Keep indices first so existing untextured primitive initializers work.
    std::array<uint32_t, 3> indices;
    static constexpr uint32_t Missing = std::numeric_limits<uint32_t>::max();
    std::array<uint32_t, 3> texcoordIndices = {Missing, Missing, Missing};
    std::array<uint32_t, 3> normalIndices = {Missing, Missing, Missing};
    int materialIndex = -1;
};

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<Face> faces;
    std::vector<TexCoord> texcoords;
    std::vector<Vertex> normals;
    std::vector<Material> materials;
};
