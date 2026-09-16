// RASTERIZER.CPP, THE DECLARATION OF ALL THE METHODS DECLARED IN RASTERIZER.H
//FUTHERMORE, THIS CPP ADDS DEFINITIONS AND GIVES MEANING TO THESE METOHDS, 
//THIS CPP ALLOWS:
// 1. DRAWING 2D
// 2. DRAWING 3D
// 3. SPINNING
//-------------------------------- 

#include "Rasterizer.h"
#include <algorithm>
#include <cmath>

// Basic line drawing, draws a line when called
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



// Make a square get drawn
void Rasterizer::SquareDraw(
    Framebuffer& fb, const Vertex2D& p0, const Vertex2D& p1, const Vertex2D& p2, const Vertex2D& p3, uint32_t color
)
{
    // for loop to fill the square
    for (int y = p0.y; y <= p2.y; y++)
    {
        for (int x = p0.x; x <= p2.x; x++)
        {
            fb.SetPixel(x, y, color);
        }
    }
    // Draw lines to connect the squares
    LineDraw(fb, p0.x, p0.y, p1.x, p1.y, color);
    LineDraw(fb, p1.x, p1.y, p2.x, p2.y, color);
    LineDraw(fb, p2.x, p2.y, p3.x, p3.y, color);
    LineDraw(fb, p3.x, p3.y, p0.x, p0.y, color);
}
// Rotated Square
void Rasterizer::RotatedSquareDraw(
    Framebuffer& fb,
    const Vertex2D& center,
    int halfSize,
    float angle,
    uint32_t color)
{
    // Keep the square's corners relative to its center. Using halfSize means
    // the full side length is halfSize * 2.
    const float cosine = std::cos(angle);
    const float sine = std::sin(angle);
    const float localX[4] = {-static_cast<float>(halfSize), static_cast<float>(halfSize),
                             static_cast<float>(halfSize), -static_cast<float>(halfSize)};
    const float localY[4] = {-static_cast<float>(halfSize), -static_cast<float>(halfSize),
                             static_cast<float>(halfSize), static_cast<float>(halfSize)};
    float verticesX[4];
    float verticesY[4];

    // The rotated square can only affect pixels inside this bounding box.
    // Starting with the framebuffer limits and expanding the box for each corner.
    int minX = fb.GetWidth();
    int maxX = 0;
    int minY = fb.GetHeight();
    int maxY = 0;

    for (int i = 0; i < 4; i++)
    {
        // Rotates a corner around (0, 0), then moves it to the square's center.
        // The angle is measured in radians.
        verticesX[i] = center.x + localX[i] * cosine - localY[i] * sine;
        verticesY[i] = center.y + localX[i] * sine + localY[i] * cosine;
        minX = std::min(minX, static_cast<int>(std::floor(verticesX[i])));
        maxX = std::max(maxX, static_cast<int>(std::ceil(verticesX[i])));
        minY = std::min(minY, static_cast<int>(std::floor(verticesY[i])));
        maxY = std::max(maxY, static_cast<int>(std::ceil(verticesY[i])));
    }
    // Fills the square
    for (int y = minY; y <= maxY; y++)
    {
        for (int x = minX; x <= maxX; x++)
        {
            bool inside = true;
            float previousCross = 0.0f;

            // Test the pixel against all four edges. A pixel is inside a
            // convex polygon when every edge sees it on the same side.
            for (int i = 0; i < 4; i++)
            {
                int next = (i + 1) % 4;
                float edgeX = verticesX[next] - verticesX[i];
                float edgeY = verticesY[next] - verticesY[i];
                float pointX = static_cast<float>(x) - verticesX[i];
                float pointY = static_cast<float>(y) - verticesY[i];
                float cross = edgeX * pointY - edgeY * pointX;

                // Store the first edge's side, then reject pixels that switch
                // sides on a later edge.
                if (i == 0)
                {
                    previousCross = cross;
                }
                else if ((previousCross < 0.0f && cross > 0.0f) ||
                         (previousCross > 0.0f && cross < 0.0f))
                {
                    inside = false;
                    break;
                }
            }

            if (inside)
            {
                fb.SetPixel(x, y, color);
            }
        }
    }

    // Draw the edges after filling so the rotated outline stays crisp.
    for (int i = 0; i < 4; i++)
    {
        int next = (i + 1) % 4;
        LineDraw(
            fb,
            static_cast<int>(std::lround(verticesX[i])),
            static_cast<int>(std::lround(verticesY[i])),
            static_cast<int>(std::lround(verticesX[next])),
            static_cast<int>(std::lround(verticesY[next])),
            color);
    }
}

// Draw a filled triangle rotated around its center.
void Rasterizer::RotatedTriangleDraw(
    Framebuffer& fb,
    const Vertex2D& center,
    int size,
    float angle,
    uint32_t color)
{
    const float sine = std::sin(angle);
    const float cosine = std::cos(angle);

    // These vertices describe an upright triangle around a local origin.
    const Vertex2D localTop = {0, -size};
    const Vertex2D localLeft = {-size, size};
    const Vertex2D localRight = {size, size};

    // Rotate a local vertex and translate it to the requested screen center.
    const auto rotateVertex = [&](const Vertex2D& vertex)
    {
        // Formula: rotatedX = x * cos(angle) - y * sin(angle).
        const float rotatedX = vertex.x * cosine - vertex.y * sine;
        // The y formula adds the second term.
        const float rotatedY = vertex.x * sine + vertex.y * cosine;

        return Vertex2D{
            center.x + static_cast<int>(std::lround(rotatedX)),
            center.y + static_cast<int>(std::lround(rotatedY))};
    };

    const Vertex2D rotatedTop = rotateVertex(localTop);
    const Vertex2D rotatedLeft = rotateVertex(localLeft);
    const Vertex2D rotatedRight = rotateVertex(localRight);

    // Reuse the existing triangle renderer with the rotated vertices.
    TriangleDraw(fb, rotatedTop, rotatedLeft, rotatedRight, color);
}

Vertex3D Rasterizer::RotateY(const Vertex3D& vertex, float angle)
{
    // A Y-axis rotation changes x and z while leaving y unchanged.
    const float sine = std::sin(angle);
    const float cosine = std::cos(angle);

    return Vertex3D{
        vertex.x * cosine - vertex.z * sine,
        vertex.y,
        vertex.x * sine + vertex.z * cosine};
}

bool Rasterizer::ProjectVertex(
    const Framebuffer& fb,
    const Vertex3D& vertex,
    float focalLength,
    Vertex2D& projected)
{
    // Perspective divides by depth: farther points appear closer to the
    // center and therefore smaller. z must be positive and non-zero.
    if (vertex.z <= 0.0f)
    {
        return false;
    }

    projected.x = fb.GetWidth() / 2 +
        static_cast<int>(std::lround(focalLength * vertex.x / vertex.z));
    projected.y = fb.GetHeight() / 2 +
        static_cast<int>(std::lround(focalLength * vertex.y / vertex.z));
    return true;
}

void Rasterizer::Triangle3DDraw(
    Framebuffer& fb,
    const Vertex3D& p0,
    const Vertex3D& p1,
    const Vertex3D& p2,
    float angle,
    float focalLength,
    uint32_t color)
{
    // Rotate each model-space vertex before projecting it to the screen.
    const Vertex3D rotated0 = RotateY(p0, angle);
    const Vertex3D rotated1 = RotateY(p1, angle);
    const Vertex3D rotated2 = RotateY(p2, angle);

    Vertex2D projected0;
    Vertex2D projected1;
    Vertex2D projected2;

    // Do not draw a triangle if any vertex is at or behind the camera.
    if (!ProjectVertex(fb, rotated0, focalLength, projected0) ||
        !ProjectVertex(fb, rotated1, focalLength, projected1) ||
        !ProjectVertex(fb, rotated2, focalLength, projected2))
    {
        return;
    }

    // Reuse the filled 2D renderer after the 3D-to-2D projection.
    TriangleFill(fb, projected0, projected1, projected2, color);
}

void Rasterizer::Pyramid3DDraw(
    Framebuffer& fb,
    const Vertex3D& center,
    float size,
    float angle,
    float focalLength,
    uint32_t color)
{
    const float sine = std::sin(angle);
    const float cosine = std::cos(angle);

    // Build a pyramid in local 3D space. The apex is above the square base.
    const Vertex3D localApex = {0.0f, -size, 0.0f};
    const Vertex3D localFrontLeft = {-size, size, -size};
    const Vertex3D localFrontRight = {size, size, -size};
    const Vertex3D localBackRight = {size, size, size};
    const Vertex3D localBackLeft = {-size, size, size};

    // Rotate around Y and then move the model to its world/camera position.
    const auto transform = [&](const Vertex3D& vertex)
    {
        return Vertex3D{
            center.x + vertex.x * cosine - vertex.z * sine,
            center.y + vertex.y,
            center.z + vertex.x * sine + vertex.z * cosine};
    };

    const Vertex3D apex = transform(localApex);
    const Vertex3D frontLeft = transform(localFrontLeft);
    const Vertex3D frontRight = transform(localFrontRight);
    const Vertex3D backRight = transform(localBackRight);
    const Vertex3D backLeft = transform(localBackLeft);

    Vertex2D projectedApex;
    Vertex2D projectedFrontLeft;
    Vertex2D projectedFrontRight;
    Vertex2D projectedBackRight;
    Vertex2D projectedBackLeft;

    if (!ProjectVertex(fb, apex, focalLength, projectedApex) ||
        !ProjectVertex(fb, frontLeft, focalLength, projectedFrontLeft) ||
        !ProjectVertex(fb, frontRight, focalLength, projectedFrontRight) ||
        !ProjectVertex(fb, backRight, focalLength, projectedBackRight) ||
        !ProjectVertex(fb, backLeft, focalLength, projectedBackLeft))
    {
        return;
    }

    // A pyramid has four triangular side faces. Drawing each face filled makes
    // the depth visible; the base is omitted because it points downward.
    TriangleFill(fb, projectedApex, projectedFrontLeft, projectedFrontRight, color);
    TriangleFill(fb, projectedApex, projectedFrontRight, projectedBackRight, color);
    TriangleFill(fb, projectedApex, projectedBackRight, projectedBackLeft, color);
    TriangleFill(fb, projectedApex, projectedBackLeft, projectedFrontLeft, color);
}
// Draws a 3D cube that spins
void Rasterizer::CubeRaw3DDraw(
    Framebuffer& fb,
    const Vertex3D& center,
    int halfSize,
    float angle,
    float focalLenght,
    uint32_t color
)
{
    const float sine = std::sin(angle);
    const float cosine = std::cos(angle);

    // All cube corners are local offsets from the cube center. The transform
    // below adds the center exactly once after rotating each corner.
    const float size = static_cast<float>(halfSize);
    const Vertex3D topFrontLeft = {-size, size, -size};
    const Vertex3D topFrontRight = {size, size, -size};
    const Vertex3D topBackRight = {size, size, size};
    const Vertex3D topBackLeft = {-size, size, size};
    const Vertex3D bottomFrontLeft = {-size, -size, -size};
    const Vertex3D bottomFrontRight = {size, -size, -size};
    const Vertex3D bottomBackRight = {size, -size, size};
    const Vertex3D bottomBackLeft = {-size, -size, size};

    // These aliases describe the same eight corners from the side-face
    // perspective, making the face definitions below easier to read.
    const Vertex3D rightTopFront = topFrontRight;
    const Vertex3D rightTopBack = topBackRight;
    const Vertex3D rightBottomBack = bottomBackRight;
    const Vertex3D rightBottomFront = bottomFrontRight;
    const Vertex3D leftTopFront = topFrontLeft;
    const Vertex3D leftTopBack = topBackLeft;
    const Vertex3D leftBottomBack = bottomBackLeft;
    const Vertex3D leftBottomFront = bottomFrontLeft;

    // Rotate around Y
    const auto Transform = [&](const Vertex3D& vertex)
    {
        return Vertex3D
        {
            center.x + vertex.x * cosine - vertex.z * sine,
            center.y + vertex.y,
            center.z + vertex.x * sine + vertex.z * cosine
        };
    };

    // Transform those 3D to 2D
    const Vertex3D transformedTopFrontLeft = Transform(topFrontLeft);
    const Vertex3D transformedTopFrontRight = Transform(topFrontRight);
    const Vertex3D transformedTopBackRight = Transform(topBackRight);
    const Vertex3D transformedTopBackLeft = Transform(topBackLeft);

    const Vertex3D transformedBottomFrontLeft = Transform(bottomFrontLeft);
    const Vertex3D transformedBottomFrontRight = Transform(bottomFrontRight);
    const Vertex3D transformedBottomBackRight = Transform(bottomBackRight);
    const Vertex3D transformedBottomBackLeft = Transform(bottomBackLeft);

    const Vertex3D transformedRightTopFront = Transform(rightTopFront);
    const Vertex3D transformedRightTopBack = Transform(rightTopBack);
    const Vertex3D transformedRightBottomBack = Transform(rightBottomBack);
    const Vertex3D transformedRightBottomFront = Transform(rightBottomFront);

    const Vertex3D transformedLeftTopFront = Transform(leftTopFront);
    const Vertex3D transformedLeftTopBack = Transform(leftTopBack);
    const Vertex3D transformedLeftBottomBack = Transform(leftBottomBack);
    const Vertex3D transformedLeftBottomFront = Transform(leftBottomFront);

    // These are the 2D Vertex, projected is for the ProjectVertex
    Vertex2D projectedTopFrontLeft;
    Vertex2D projectedTopFrontRight;
    Vertex2D projectedTopBackRight;
    Vertex2D projectedTopBackLeft;
    Vertex2D projectedBottomFrontLeft;
    Vertex2D projectedBottomFrontRight;
    Vertex2D projectedBottomBackRight;
    Vertex2D projectedBottomBackLeft;

    Vertex2D projectedRightTopFront;
    Vertex2D projectedRightTopBack;
    Vertex2D projectedRightBottomBack;
    Vertex2D projectedRightBottomFront;

    Vertex2D projectedLeftTopFront;
    Vertex2D projectedLeftTopBack;
    Vertex2D projectedLeftBottomBack;
    Vertex2D projectedLeftBottomFront;

    // Every cube vertex must be in front of the camera before it can be
    // projected. The output Vertex2D receives the screen coordinates, Project Vertex takes a 3D Vertex,
    // and turns it to 2D while still retaining the depth of the 3D
    if (!ProjectVertex(fb, transformedTopFrontLeft, focalLenght, projectedTopFrontLeft) ||
        !ProjectVertex(fb, transformedTopFrontRight, focalLenght, projectedTopFrontRight) ||
        !ProjectVertex(fb, transformedTopBackRight, focalLenght, projectedTopBackRight) ||
        !ProjectVertex(fb, transformedTopBackLeft, focalLenght, projectedTopBackLeft) ||
        !ProjectVertex(fb, transformedBottomFrontLeft, focalLenght, projectedBottomFrontLeft) ||
        !ProjectVertex(fb, transformedBottomFrontRight, focalLenght, projectedBottomFrontRight) ||
        !ProjectVertex(fb, transformedBottomBackRight, focalLenght, projectedBottomBackRight) ||
        !ProjectVertex(fb, transformedBottomBackLeft, focalLenght, projectedBottomBackLeft) ||
        !ProjectVertex(fb, transformedRightTopFront, focalLenght, projectedRightTopFront) ||
        !ProjectVertex(fb, transformedRightTopBack, focalLenght, projectedRightTopBack) ||
        !ProjectVertex(fb, transformedRightBottomBack, focalLenght, projectedRightBottomBack) ||
        !ProjectVertex(fb, transformedRightBottomFront, focalLenght, projectedRightBottomFront) ||
        !ProjectVertex(fb, transformedLeftTopFront, focalLenght, projectedLeftTopFront) ||
        !ProjectVertex(fb, transformedLeftTopBack, focalLenght, projectedLeftTopBack) ||
        !ProjectVertex(fb, transformedLeftBottomBack, focalLenght, projectedLeftBottomBack) ||
        !ProjectVertex(fb, transformedLeftBottomFront, focalLenght, projectedLeftBottomFront))
    {
        return;
    }

    // Each cube face is a quadrilateral split into two triangles.
    // Top face.
    TriangleFill(fb, projectedTopFrontLeft, projectedTopFrontRight,
                 projectedTopBackRight, color);
    TriangleFill(fb, projectedTopFrontLeft, projectedTopBackRight,
                 projectedTopBackLeft, color);

    // Bottom face.
    TriangleFill(fb, projectedBottomFrontLeft, projectedBottomFrontRight,
                 projectedBottomBackRight, color);
    TriangleFill(fb, projectedBottomFrontLeft, projectedBottomBackRight,
                 projectedBottomBackLeft, color);

    // Front face.
    TriangleFill(fb, projectedTopFrontLeft, projectedTopFrontRight,
                 projectedBottomFrontRight, color);
    TriangleFill(fb, projectedTopFrontLeft, projectedBottomFrontRight,
                 projectedBottomFrontLeft, color);

    // Back face.
    TriangleFill(fb, projectedTopBackLeft, projectedTopBackRight,
                 projectedBottomBackRight, color);
    TriangleFill(fb, projectedTopBackLeft, projectedBottomBackRight,
                 projectedBottomBackLeft, color);

    // Left face.
    TriangleFill(fb, projectedTopFrontLeft, projectedTopBackLeft,
                 projectedBottomBackLeft, color);
    TriangleFill(fb, projectedTopFrontLeft, projectedBottomBackLeft,
                 projectedBottomFrontLeft, color);

    // Right face.
    TriangleFill(fb, projectedTopFrontRight, projectedTopBackRight,
                 projectedBottomBackRight, color);
    TriangleFill(fb, projectedTopFrontRight, projectedBottomBackRight,
                 projectedBottomFrontRight, color);
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

void Rasterizer::TriangleFill(
    Framebuffer& fb,
    const Vertex2D& p0,
    const Vertex2D& p1,
    const Vertex2D& p2,
    uint32_t color)
{
    // Restrict the pixel scan to the triangle's bounding rectangle.
    const int minX = std::max(0, std::min({p0.x, p1.x, p2.x}));
    const int maxX = std::min(fb.GetWidth() - 1, std::max({p0.x, p1.x, p2.x}));
    const int minY = std::max(0, std::min({p0.y, p1.y, p2.y}));
    const int maxY = std::min(fb.GetHeight() - 1, std::max({p0.y, p1.y, p2.y}));

    const auto edge = [](const Vertex2D& a, const Vertex2D& b, int x, int y)
    {
        return (x - a.x) * (b.y - a.y) - (y - a.y) * (b.x - a.x);
    };

    for (int y = minY; y <= maxY; y++)
    {
        for (int x = minX; x <= maxX; x++)
        {
            const int e0 = edge(p0, p1, x, y);
            const int e1 = edge(p1, p2, x, y);
            const int e2 = edge(p2, p0, x, y);

            // All three edge tests must have the same sign for a pixel to be
            // inside a consistently wound triangle.
            if ((e0 >= 0 && e1 >= 0 && e2 >= 0) ||
                (e0 <= 0 && e1 <= 0 && e2 <= 0))
            {
                fb.SetPixel(x, y, color);
            }
        }
    }
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
