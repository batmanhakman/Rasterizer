#include "ObjLoader.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace
{
std::string Trim(std::string value)
{
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    value = value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
        value = value.substr(1, value.size() - 2);
    return value;
}

bool ReadFloat(std::istringstream& input, float& value)
{
    std::string token;
    if (!(input >> token)) return false;
    try
    {
        size_t end = 0;
        value = std::stof(token, &end);
        return end == token.size() && std::isfinite(value);
    }
    catch (const std::exception&) { return false; }
}

bool ResolveIndex(const std::string& token, size_t count, uint32_t& result)
{
    if (token.empty()) return false;
    try
    {
        size_t end = 0;
        const long long raw = std::stoll(token, &end);
        if (end != token.size() || raw == 0) return false;
        const long long index = raw > 0 ? raw - 1 : static_cast<long long>(count) + raw;
        if (index < 0 || static_cast<unsigned long long>(index) >= count ||
            static_cast<unsigned long long>(index) >= Face::Missing) return false;
        result = static_cast<uint32_t>(index);
        return true;
    }
    catch (const std::exception&) { return false; }
}

struct Corner
{
    uint32_t position = Face::Missing;
    uint32_t uv = Face::Missing;
    uint32_t normal = Face::Missing;
};

bool ReadCorner(const std::string& token, const Mesh& mesh, Corner& corner)
{
    const size_t first = token.find('/');
    if (!ResolveIndex(token.substr(0, first), mesh.vertices.size(), corner.position)) return false;
    if (first == std::string::npos) return true;
    const size_t second = token.find('/', first + 1);
    const std::string uv = token.substr(first + 1,
        second == std::string::npos ? second : second - first - 1);
    if (!uv.empty() && !ResolveIndex(uv, mesh.texcoords.size(), corner.uv)) return false;
    if (second == std::string::npos) return !uv.empty();
    if (token.find('/', second + 1) != std::string::npos) return false;
    return ResolveIndex(token.substr(second + 1), mesh.normals.size(), corner.normal);
}

using MaterialNames = std::unordered_map<std::string, int>;
int MaterialIndex(const std::string& name, Mesh& mesh, MaterialNames& names)
{
    const auto found = names.find(name);
    if (found != names.end()) return found->second;
    const int index = static_cast<int>(mesh.materials.size());
    Material material;
    material.name = name;
    mesh.materials.push_back(std::move(material));
    names.emplace(name, index);
    return index;
}

bool ReadMaterialLibrary(const std::filesystem::path& path, Mesh& mesh,
                         MaterialNames& names, std::string& error)
{
    std::ifstream input(path);
    if (!input)
    {
        error = "Cannot open material library: " + path.u8string();
        return false;
    }
    std::string line;
    size_t lineNumber = 0;
    int current = -1;
    while (std::getline(input, line))
    {
        ++lineNumber;
        line = line.substr(0, line.find('#'));
        std::istringstream fields(line);
        std::string kind;
        fields >> kind;
        const auto fail = [&](const std::string& detail)
        {
            error = path.u8string() + ":" + std::to_string(lineNumber) + ": " + detail;
            return false;
        };
        if (kind == "newmtl")
        {
            std::string name;
            std::getline(fields, name);
            name = Trim(name);
            if (name.empty()) return fail("newmtl needs a material name");
            current = MaterialIndex(name, mesh, names);
        }
        else if (kind == "Kd")
        {
            if (current < 0) return fail("Kd appears before newmtl");
            for (float& channel : mesh.materials[current].diffuse)
            {
                if (!ReadFloat(fields, channel)) return fail("Kd needs three finite numbers");
                channel = std::clamp(channel, 0.0f, 1.0f);
            }
        }
        else if (kind == "Ks")
        {
            if (current < 0) return fail("Ks appears before newmtl");
            for (float& channel : mesh.materials[current].specular)
            {
                if (!ReadFloat(fields, channel)) return fail("Ks needs three finite numbers");
                channel = std::clamp(channel, 0.0f, 1.0f);
            }
        }
        else if (kind == "Ns")
        {
            if (current < 0) return fail("Ns appears before newmtl");
            float& exponent = mesh.materials[current].shininess;
            if (!ReadFloat(fields, exponent)) return fail("Ns needs a finite number");
            exponent = std::clamp(exponent, 1.0f, 1000.0f);
        }
        else if (kind == "map_Kd")
        {
            if (current < 0) return fail("map_Kd appears before newmtl");
            std::string filename;
            std::getline(fields, filename);
            filename = Trim(filename);
            if (filename.empty()) return fail("map_Kd needs a PNG filename");
            if (filename.front() == '-') return fail("map_Kd options are not supported; use a plain PNG path");
            std::replace(filename.begin(), filename.end(), '\\', '/');
            std::string textureError;
            if (!mesh.materials[current].texture.Load(path.parent_path() / std::filesystem::u8path(filename), textureError))
                return fail(textureError);
        }
    }
    return true;
}
} // namespace

bool ObjLoader::Load(const std::filesystem::path& path, Mesh& mesh, std::string& error)
{
    std::ifstream input(path);
    if (!input)
    {
        error = "Cannot open OBJ: " + path.u8string();
        return false;
    }
    Mesh loaded;
    MaterialNames materialNames;
    std::string line;
    size_t lineNumber = 0;
    int materialIndex = -1;
    while (std::getline(input, line))
    {
        ++lineNumber;
        if (lineNumber == 1 && line.compare(0, 3, "\xef\xbb\xbf") == 0) line.erase(0, 3);
        line = line.substr(0, line.find('#'));
        std::istringstream fields(line);
        std::string kind;
        fields >> kind;
        const auto fail = [&](const std::string& detail)
        {
            error = path.u8string() + ":" + std::to_string(lineNumber) + ": " + detail;
            return false;
        };
        if (kind == "v" || kind == "vn")
        {
            Vertex value{};
            if (!ReadFloat(fields, value.x) || !ReadFloat(fields, value.y) || !ReadFloat(fields, value.z))
                return fail(kind + " needs three finite numbers");
            (kind == "v" ? loaded.vertices : loaded.normals).push_back(value);
        }
        else if (kind == "vt")
        {
            TexCoord value{};
            if (!ReadFloat(fields, value.u)) return fail("vt needs a finite u coordinate");
            fields >> std::ws;
            if (!fields.eof() && !ReadFloat(fields, value.v)) return fail("vt has an invalid v coordinate");
            loaded.texcoords.push_back(value);
        }
        else if (kind == "f")
        {
            std::vector<Corner> corners;
            std::string token;
            while (fields >> token)
            {
                Corner corner;
                if (!ReadCorner(token, loaded, corner)) return fail("invalid or out-of-range face index: " + token);
                corners.push_back(corner);
            }
            if (corners.size() < 3) return fail("a face needs at least three corners");
            // OBJ convex polygons are triangulated as a fan. UV seams remain
            // per-corner, so sharing positions does not merge texture seams.
            for (size_t i = 1; i + 1 < corners.size(); ++i)
            {
                Face face{};
                const Corner triangle[3] = {corners[0], corners[i], corners[i+1]};
                for (size_t j = 0; j < 3; ++j)
                {
                    face.indices[j] = triangle[j].position;
                    face.texcoordIndices[j] = triangle[j].uv;
                    face.normalIndices[j] = triangle[j].normal;
                }
                face.materialIndex = materialIndex;
                loaded.faces.push_back(face);
            }
        }
        else if (kind == "usemtl")
        {
            std::string name;
            std::getline(fields, name);
            name = Trim(name);
            if (name.empty()) return fail("usemtl needs a material name");
            materialIndex = MaterialIndex(name, loaded, materialNames);
        }
        else if (kind == "mtllib")
        {
            std::string filename;
            std::getline(fields, filename);
            filename = Trim(filename);
            if (filename.empty()) return fail("mtllib needs a filename");
            std::replace(filename.begin(), filename.end(), '\\', '/');
            const auto fullPath = path.parent_path() / std::filesystem::u8path(filename);
            // A filename containing spaces is preferred when it exists;
            // otherwise OBJ also permits several libraries on one line.
            if (std::filesystem::exists(fullPath))
            {
                if (!ReadMaterialLibrary(fullPath, loaded, materialNames, error)) return false;
            }
            else
            {
                std::istringstream filenames(filename);
                while (filenames >> filename)
                    if (!ReadMaterialLibrary(path.parent_path() / std::filesystem::u8path(filename), loaded, materialNames, error)) return false;
            }
        }
    }
    if (loaded.vertices.empty() || loaded.faces.empty())
    {
        error = path.u8string() + ": OBJ must contain vertices and faces";
        return false;
    }
    mesh = std::move(loaded);
    error.clear();
    return true;
}

bool ObjLoader::LoadVertices(const std::string& path, Mesh& mesh)
{
    std::ifstream input(std::filesystem::u8path(path));
    if (!input) return false;
    std::vector<Vertex> vertices;
    std::string line;
    while (std::getline(input, line))
    {
        std::istringstream fields(line);
        std::string kind;
        fields >> kind;
        if (kind != "v") continue;
        Vertex vertex{};
        if (!ReadFloat(fields, vertex.x) || !ReadFloat(fields, vertex.y) || !ReadFloat(fields, vertex.z)) return false;
        vertices.push_back(vertex);
    }
    mesh.vertices = std::move(vertices);
    return true;
}

bool ObjLoader::LoadFaces(const std::string& path, Mesh& mesh)
{
    std::string error;
    return Load(std::filesystem::u8path(path), mesh, error);
}
