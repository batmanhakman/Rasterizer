#pragma once
#include "framebuffer.h"

struct Vertex2D
{
    int x;
    int y;
};

class Rasterizer
{
public:
    static void LineDraw(Framebuffer& fb, int x0, int y0, int x1, int y1, uint32_t color);
    static void TriangleDraw(Framebuffer& fb, const Vertex2D& p0, const Vertex2D& p1, const Vertex2D& p2, uint32_t color);
};
