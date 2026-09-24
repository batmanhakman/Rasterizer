#pragma once
#include "Rasterizer.h"
#include <filesystem>
#include <memory>
#include <string>

struct SceneInput
{
    bool left = false, right = false, up = false, down = false;
    bool zoomIn = false, zoomOut = false;
};

// Shared by both native windows and the command-line renderer.
class Scene
{
public:
    Scene(int width = 1000, int height = 760);
    bool Load(const std::filesystem::path& path, std::string& error);
    void Resize(int width, int height);
    void Update(const SceneInput& input, float seconds);
    void Orbit(float horizontal, float vertical);
    void Zoom(float steps);
    void Reset();
    void Render();
    const Framebuffer& Frame() const { return *framebuffer; }
    const Mesh& Model() const { return mesh; }
    static std::filesystem::path DefaultModelPath();

private:
    std::unique_ptr<Framebuffer> framebuffer;
    Mesh mesh;
    float radius = 1.0f, yaw = 0.32f, pitch = -0.10f, distance = 2.45f;
};
