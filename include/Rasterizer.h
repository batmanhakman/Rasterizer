#pragma once
#include "framebuffer.h"
#include "Mesh.h"
#include "Vectors.h"

#include <cstddef>
#include <vector>


struct Vertex2D
{
    int x;
    int y;
};

struct Vertex3D
{
    float x;
    float y;
    float z;
};

// A triangle refers to three entries in Mesh::vertices. Keeping the vertex
// data and face topology separate lets every 3D model use the same renderer.
// struct TriangleIndices
// {
//     std::size_t first;
//     std::size_t second;
//     std::size_t third;
// }

// The camera looks along +Z when yaw and pitch are zero. Position is in the
// same world-space units as Mesh vertices.
struct Camera
{
    Vector3D position;
    float yaw;
    float pitch;
    float focalLength;
};

class Rasterizer
{
public:
    // Draw a line
    static void LineDraw(Framebuffer& fb, int x0, int y0, int x1, int y1, uint32_t color);
    // Draw a Triangle
    static void TriangleDraw(Framebuffer& fb, const Vertex2D& p0, const Vertex2D& p1, const Vertex2D& p2, uint32_t color);
    // Fill a 2D triangle using a pixel-by-pixel inside test.
    static void TriangleFill(Framebuffer& fb, const Vertex2D& p0, const Vertex2D& p1, const Vertex2D& p2, uint32_t color);
    // Rotating Triangle
    static void RotatedTriangleDraw(Framebuffer& fb, const Vertex2D& center, int size, float angle, uint32_t color);
    // Rotate a 3D point around the Y axis. The angle is in radians.
    static Vertex3D RotateY(const Vertex3D& vertex, float angle);
    // Project a 3D point onto the framebuffer using perspective.
    static bool ProjectVertex(const Framebuffer& fb, const Vertex3D& vertex, float focalLength, Vertex2D& projected);
    // Rotate, project, and draw a 3D triangle.
    static void Triangle3DDraw(
        Framebuffer& fb,
        const Vertex3D& p0,
        const Vertex3D& p1,
        const Vertex3D& p2,
        float angle,
        float focalLength,
        uint32_t color);
    // Transform a world-space mesh into camera space, project it, and fill
    // every indexed triangle in its declared order.
    static void DrawMesh(
        Framebuffer& fb,
        const Mesh& mesh,
        const Camera& camera,
        uint32_t color);
    // Draw a stationary four-sided pyramid through the generic mesh path.
    static void Pyramid3DDraw(
        Framebuffer& fb,
        const Vertex3D& center,
        float size,
        const Camera& camera,
        uint32_t color);
    // Draw a stationary cube through the generic mesh path.
    static void CubeRaw3DDraw(
        Framebuffer& fb,
        const Vertex3D& center,
        int halfSize,
        const Camera& camera,
        uint32_t color);
    static void SphereRaw3D(
        Framebuffer& fb,
        const Vertex3D& center,
        int radius,
        int sectorCount,
        int stackCount,
        const Camera& camera,
        uint32_t color
    );
    // Draw a square
    static void SquareDraw(Framebuffer& fb, const Vertex2D& p0, const Vertex2D& p1, const Vertex2D& p2, const Vertex2D& p3, uint32_t color);
    // Draw a filled square rotated around its center. The angle is in radians.
    static void RotatedSquareDraw(Framebuffer& fb, const Vertex2D& center, int halfSize, float angle, uint32_t color);
    // Draw a circle
    static void CircleDraw(Framebuffer& fb, const Vertex2D& Center, int radius, uint32_t color);
};
