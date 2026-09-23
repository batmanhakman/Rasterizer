#include "Scene.h"
#include "ObjLoader.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#endif

namespace
{
std::filesystem::path ExecutableDirectory()
{
#ifdef _WIN32
    std::vector<wchar_t> buffer(32768);
    const DWORD count = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (count == 0 || count == buffer.size())
        throw std::runtime_error("Cannot locate the executable directory.");
    return std::filesystem::path(std::wstring(buffer.data(), count)).parent_path();
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buffer(size);
    if (_NSGetExecutablePath(buffer.data(), &size) != 0)
        throw std::runtime_error("Cannot locate the executable directory.");
    return std::filesystem::weakly_canonical(buffer.data()).parent_path();
#else
    std::vector<char> buffer(32768);
    const auto count = readlink("/proc/self/exe", buffer.data(), buffer.size());
    if (count <= 0 || static_cast<std::size_t>(count) == buffer.size())
        throw std::runtime_error("Cannot locate the executable directory.");
    return std::filesystem::path(std::string(buffer.data(), count)).parent_path();
#endif
}
}

Scene::Scene(int width, int height) { Resize(width, height); }

std::filesystem::path Scene::DefaultModelPath()
{
    return ExecutableDirectory() / "assets" / "knight" / "knight.obj";
}

bool Scene::Load(const std::filesystem::path& path, std::string& error)
{
    Mesh loaded;
    if (!ObjLoader::Load(path, loaded, error)) return false;
    Vertex lo{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    Vertex hi{-lo.x, -lo.y, -lo.z};
    for (const auto& v : loaded.vertices)
    {
        lo.x = std::min(lo.x, v.x); lo.y = std::min(lo.y, v.y); lo.z = std::min(lo.z, v.z);
        hi.x = std::max(hi.x, v.x); hi.y = std::max(hi.y, v.y); hi.z = std::max(hi.z, v.z);
    }
    const Vertex center{(lo.x + hi.x) * 0.5f, (lo.y + hi.y) * 0.5f, (lo.z + hi.z) * 0.5f};
    // OBJ assets use Y-up. The existing rasterizer's camera uses Y-down.
    for (auto& v : loaded.vertices)
    {
        v.x -= center.x;
        v.y = center.y - v.y;
        v.z -= center.z;
    }
    for (auto& n : loaded.normals) n.y = -n.y;
    // A reflection changes winding; keep outward normals and corner data aligned.
    for (auto& face : loaded.faces)
    {
        std::swap(face.indices[1], face.indices[2]);
        std::swap(face.texcoordIndices[1], face.texcoordIndices[2]);
        std::swap(face.normalIndices[1], face.normalIndices[2]);
    }
    const float dx = hi.x - lo.x, dy = hi.y - lo.y, dz = hi.z - lo.z;
    radius = std::max(0.01f, 0.5f * std::sqrt(dx * dx + dy * dy + dz * dz));
    mesh = std::move(loaded);
    Reset();
    return true;
}

void Scene::Resize(int width, int height)
{
    if (width <= 0 || height <= 0) return;
    // Bound software rendering memory on very large/high-DPI desktops.
    const float scale = std::min(1.0f, 1920.0f / static_cast<float>(std::max(width, height)));
    width = std::max(1, static_cast<int>(width * scale));
    height = std::max(1, static_cast<int>(height * scale));
    if (!framebuffer || width != framebuffer->GetWidth() || height != framebuffer->GetHeight())
        framebuffer = std::make_unique<Framebuffer>(width, height);
}

void Scene::Reset()
{
    yaw = 0.32f;
    pitch = -0.10f;
    distance = radius * 2.45f;
}

void Scene::Update(const SceneInput& input, float seconds)
{
    seconds = std::clamp(seconds, 0.0f, 0.1f);
    Orbit((static_cast<int>(input.right) - static_cast<int>(input.left)) * seconds * 1.2f,
          (static_cast<int>(input.down) - static_cast<int>(input.up)) * seconds * 1.2f);
    Zoom((static_cast<int>(input.zoomIn) - static_cast<int>(input.zoomOut)) * seconds * 6.0f);
}

void Scene::Orbit(float horizontal, float vertical)
{
    yaw = std::remainder(yaw + horizontal, 6.283185307f);
    pitch = std::clamp(pitch + vertical, -1.35f, 1.35f);
}

void Scene::Zoom(float steps)
{
    distance = std::clamp(distance * std::exp(-steps * 0.12f), radius * 1.15f, radius * 9.0f);
}

void Scene::Render()
{
    framebuffer->Clear(0xFF101827u);
    const float cp = std::cos(pitch);
    const Camera camera{
        Vector3D{-std::sin(yaw) * cp * distance, std::sin(pitch) * distance, -std::cos(yaw) * cp * distance},
        yaw, pitch, std::min(framebuffer->GetWidth(), framebuffer->GetHeight()) * 1.03f};
    Rasterizer::DrawMesh(*framebuffer, mesh, camera, 0xFFD1D6DEu);
}
