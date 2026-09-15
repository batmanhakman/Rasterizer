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



// Make the square get drawn
void Rasterizer::SquareDraw(
    Framebuffer& fb, const Vertex2D& p0, const Vertex2D& p1, const Vertex2D& p2, const Vertex2D& p3, uint32_t color
)
{
    for (int y = p0.y; y <= p2.y; y++)
    {
        for (int x = p0.x; x <= p2.x; x++)
        {
            fb.SetPixel(x, y, color);
        }
    }
    LineDraw(fb, p0.x, p0.y, p1.x, p1.y, color);
    LineDraw(fb, p1.x, p1.y, p2.x, p2.y, color);
    LineDraw(fb, p2.x, p2.y, p3.x, p3.y, color);
    LineDraw(fb, p3.x, p3.y, p0.x, p0.y, color);
}

// Make the circle get drawn
void Rasterizer::TriangleDraw(
    Framebuffer& fb,
    const Vertex2D& p0,
    const Vertex2D& p1,
    const Vertex2D& p2,
    uint32_t color)
{
    // Draw the three edges, closing the triangle by connecting p2 back to p0.
    LineDraw(fb, p0.x, p0.y, p1.x, p1.y, color);
    LineDraw(fb, p1.x, p1.y, p2.x, p2.y, color);
    LineDraw(fb, p2.x, p2.y, p0.x, p0.y, color);
}

// Draw a filled circle.
void Rasterizer::CircleDraw(
    Framebuffer& fb,
    const Vertex2D& Center,
    int radius,
    uint32_t color
)
{
    int squaredRadius = radius * radius;

    for (int y = Center.y - radius; y <= Center.y + radius; y++)
    {
        for (int x = Center.x - radius; x <= Center.x + radius; x++)
        {
            int dx = x - Center.x;
            int dy = y - Center.y;

            int distanceSquared = dx * dx + dy * dy;

            if (distanceSquared <= squaredRadius)
            {
                fb.SetPixel(x, y, color);
            }
        }
    }
}
