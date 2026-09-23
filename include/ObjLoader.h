#pragma once
#include "Mesh.h"
#include <filesystem>
#include <string>

namespace ObjLoader
{
// Load positions, UVs, normals, triangulated convex polygons, and MTL diffuse
// colors/PNG textures. Failure leaves mesh unchanged and gives a diagnostic.
bool Load(const std::filesystem::path& path, Mesh& mesh, std::string& error);
// Compatibility entry points; use Load for material/UV-aware imports.
bool LoadVertices(const std::string& path, Mesh& mesh);
bool LoadFaces(const std::string& path, Mesh& mesh);
}
