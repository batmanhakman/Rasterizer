#include "Rasterizer.h"
#include <cmath>

// COMMENT!!!!!

void Rasterizer::LineDraw(Framebuffer& fb, int x0, int y0, int x1, int y1, uint32_t color)
{
    // Distance formula (dx = delta of x and dy = delta of y)
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);

    int sx;
    int sy;

    if (x0 < x1)
    {
        sx = 1; // line moves right
    }
    else
    {
        sx = -1; // line moves left
    }

    
    if (y0 < y1)
    {
        sy = 1; // line moves up
    }
    else
    {
        sy = -1; // line moves down
    }

    //sometimes the line can drift away from the ideal mathematical line, so we use error term to try and fix it. Should it move horizontally and/or vertically?
    int err = dx - dy;


    while(true)
    {
        // Set the pixels.
        fb.SetPixel(x0,y0, color);
        if(x0 == x1 && y0 == y1)
        {
            // When all the pixels are set, break the loop
            break;
        }
            // evaluate both X and Y step conditions independently.
            int e2 = 2*err;

            if(e2 > -dy) // do we need horizontal step?
            {
                err = err - dy;
                x0 = x0 + sx;
            }
        
            if(e2 < dx ) // do we need vertical step?
            {
                err = err + dx;
                y0 = y0 + sy;
            }    
    }   
}
