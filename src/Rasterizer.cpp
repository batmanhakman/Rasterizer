#include "Rasterizer.h"
#include <cmath>

void Rasterizer::LineDraw(Framebuffer& fb, int x0, int y0, int x1, int y1, uint32_t color)
{
    // Distance formula (dx = delta of x and dy = delta of y)
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);

    int sx;
    int sy;

    if (x0 < x1)
    {
        sx = 1;
    }
    else
    {
        sx = -1;
    }

    
    if (y0 < y1)
    {
        sy = 1;
    }
    else
    {
        sy = -1;
    }

    // IMPLEMENT Initail Error Term (err)


    // FINAL Implement Pixel-Stepping Loop
}