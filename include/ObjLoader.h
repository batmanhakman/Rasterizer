#pragma once

#include "Mesh.h"

#include <string>

namespace ObjLoader
{
// Loads only vertex-position lines from an OBJ file.
// Returns false when the file cannot be opened or a vertex line is malformed.
bool LoadVertices(const std::string& path, Mesh& mesh);
bool LoadFaces(const std::string& path, Mesh& mesh);
}
