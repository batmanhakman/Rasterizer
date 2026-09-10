#pragma once
#include "framebuffer.h"


class Rasterizer
{
    public:
     static void LineDraw(Framebuffer& fb, int x0, int y0,int x1, int y1, uint32_t color);

}

