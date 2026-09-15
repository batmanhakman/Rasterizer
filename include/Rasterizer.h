#pragma once
#include "framebuffer.h"
#include <cmath>

struct Vertex2D
{
    int x;
    int y;
};


class Rasterizer
{
public:
    // Draw a line
    static void LineDraw(Framebuffer& fb, int x0, int y0, int x1, int y1, uint32_t color);
    // Draw a Triangle
    static void TriangleDraw(Framebuffer& fb, const Vertex2D& p0, const Vertex2D& p1, const Vertex2D& p2, uint32_t color);
    // Draw a square
    static void SquareDraw(Framebuffer& fb, const Vertex2D& p0, const Vertex2D& p1, const Vertex2D& p2, const Vertex2D& p3, uint32_t color);
    // Draw a circle
    static void CircleDraw(Framebuffer& fb, const Vertex2D& Center, int radius, uint32_t color);
};
