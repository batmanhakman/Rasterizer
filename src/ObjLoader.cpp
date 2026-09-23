#include "ObjLoader.h"

#include <fstream>
#include <sstream>

namespace ObjLoader
{
bool LoadVertices(const std::string& path, Mesh& mesh)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        return false;
    }

    // Loading replaces the mesh's existing positions. Faces are deliberately
    // untouched because this first loader step does not parse face data.
    mesh.vertices.clear();

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream lineStream(line);
        std::string type;
        lineStream >> type;

        // OBJ lines that are not vertex positions are intentionally ignored.
        if (type != "v")
        {
            continue;
        }

        Vertex vertex{};
        if (!(lineStream >> vertex.x >> vertex.y >> vertex.z))
        {
            return false;
        }

        mesh.vertices.push_back(vertex);
    }

    return true;
}

bool LoadFaces(const std::string& path, Mesh& mesh)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        return false;
    }

    mesh.faces.clear();

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream lineStream(line);
        std::string type;
        lineStream >> type;

        if (type != "f")
        {
            continue;
        }

        Face face{};
        for (std::size_t i = 0; i < face.indices.size(); ++i)
        {
            std::string token;
            if (!(lineStream >> token))
            {
                return false;
            }

            // Face tokens may be "vertex", "vertex/uv", or
            // "vertex/uv/normal". This loader uses only the vertex index.
            const std::size_t slash = token.find('/');
            const std::string vertexIndex = token.substr(0, slash);
            try
            {
                const unsigned long index = std::stoul(vertexIndex);
                if (index == 0)
                {
                    return false;
                }
                face.indices[i] = static_cast<uint32_t>(index - 1);
            }
            catch (const std::invalid_argument&)
            {
                return false;
            }
            catch (const std::out_of_range&)
            {
                return false;
            }
        }

        mesh.faces.push_back(face);
    }

    return true;
}
}
