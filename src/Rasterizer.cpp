// RASTERIZER.CPP, THE DECLARATION OF ALL THE METHODS DECLARED IN RASTERIZER.H
//FUTHERMORE, THIS CPP ADDS DEFINITIONS AND GIVES MEANING TO THESE METOHDS, 
//THIS CPP ALLOWS:
// 1. DRAWING 2D
// 2. DRAWING 3D
// 3. CAMERA-VIEWED 3D MESHES
//-------------------------------- 

#include "Rasterizer.h"
#include "Vectors.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
constexpr float kNearClipDistance = 5.0f;

Vector3D WorldToCamera(const Vector3D& worldPosition, const Camera& camera)
{
    // First express the point relative to the camera. Then project it onto
    // the camera's right, down, and forward axes to obtain camera space.
    const Vector3D relative = Vectors::Subtraction(worldPosition, camera.position);
    const float yawSine = std::sin(camera.yaw);
    const float yawCosine = std::cos(camera.yaw);
    const float pitchSine = std::sin(camera.pitch);
    const float pitchCosine = std::cos(camera.pitch);

    const Vector3D right = {yawCosine, 0.0f, -yawSine};
    const Vector3D forward = {
        yawSine * pitchCosine,
        -pitchSine,
        yawCosine * pitchCosine};
    const Vector3D down = Vectors::CrossProduct(forward, right);

    return Vector3D{
        Vectors::DotProduct(relative, right),
        Vectors::DotProduct(relative, down),
        Vectors::DotProduct(relative, forward)};
}

Vertex3D ToVertex3D(const Vector3D& vector)
{
    return Vertex3D{vector.x, vector.y, vector.z};
}
} // namespace

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
    // center and therefore smaller. Keeping a small near distance prevents a
    // vertex almost at the camera from producing impractically huge screen
    // coordinates. Full near-plane triangle clipping can replace this later.
    if (vertex.z <= kNearClipDistance)
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

void Rasterizer::DrawMesh(
    Framebuffer& fb,
    const Mesh& mesh,
    const Camera& camera,
    uint32_t color)
{
    std::vector<Vertex2D> projectedVertices(mesh.vertices.size());
    std::vector<float> cameraDepths(mesh.vertices.size());

    // Transform every world-space Vector3D into the camera's coordinate
    // system before using the existing perspective projection routine.
    for (std::size_t i = 0; i < mesh.vertices.size(); ++i)
    {
        const Vector3D cameraSpaceVertex = WorldToCamera(mesh.vertices[i], camera);
        cameraDepths[i] = cameraSpaceVertex.z;
        if (!ProjectVertex(
                fb,
                ToVertex3D(cameraSpaceVertex),
                camera.focalLength,
                projectedVertices[i]))
        {
            // This retains the previous behavior: a model is not rendered
            // unless all of its vertices are in front of the camera.
            return;
        }
    }

    // The framebuffer has no depth buffer. Draw the triangles that are
    // farther from the camera first, so nearer faces paint over them rather
    // than appearing to be see-through. Average depth is sufficient for the
    // non-intersecting cube and pyramid; a z-buffer is the robust next step
    // for intersecting geometry.
    std::vector<TriangleIndices> sortedTriangles;
    sortedTriangles.reserve(mesh.triangles.size());
    for (const TriangleIndices& triangle : mesh.triangles)
    {
        if (triangle.first < projectedVertices.size() &&
            triangle.second < projectedVertices.size() &&
            triangle.third < projectedVertices.size())
        {
            sortedTriangles.push_back(triangle);
        }
    }

    std::sort(
        sortedTriangles.begin(),
        sortedTriangles.end(),
        [&](const TriangleIndices& left, const TriangleIndices& right)
        {
            const float leftDepth =
                (cameraDepths[left.first] + cameraDepths[left.second] + cameraDepths[left.third]) / 3.0f;
            const float rightDepth =
                (cameraDepths[right.first] + cameraDepths[right.second] + cameraDepths[right.third]) / 3.0f;
            return leftDepth > rightDepth;
        });

    for (const TriangleIndices& triangle : sortedTriangles)
    {
        // TO-DO
        // LIGHTING DEBUG NOTE:
        // A face that looks transparent is usually receiving brightness 0 and
        // therefore being drawn black against the black background. Before
        // changing projection code, temporarily use a small ambient minimum
        // (for example, 0.1) to prove the triangle is still being rasterized.
        // If it reappears, inspect its winding: CrossProduct(B - A, C - A)
        // must point outward. The cube's face indices below are now wound
        // consistently for lighting; use that same order for sphere faces.
        // LIGHTING ROADMAP (implement this before calling TriangleFill):
        // 1. Fetch the three *world-space* vertices from mesh.vertices using
        //    triangle.first, triangle.second, and triangle.third. Do lighting
        //    in world space; projected 2D coordinates no longer describe a
        //    face's real orientation.
        const Vector3D& vertexA = mesh.vertices[triangle.first];
        const Vector3D& vertexB = mesh.vertices[triangle.second];
        const Vector3D& vertexC = mesh.vertices[triangle.third];
        // 2. Form two edges from the first vertex to the other two. Their
        //    cross product is the face normal. Normalize that normal by
        //    dividing it by its Vectors::length result. The triangle winding
        //    determines which way it faces: reverse the cross-product order
        //    (or the triangle indices) if a lit face is unexpectedly dark.
        const Vector3D edge1 = Vectors::Subtraction(vertexB, vertexA);
        const Vector3D edge2 = Vectors::Subtraction(vertexC, vertexA);

        const Vector3D faceNormal = Vectors::CrossProduct(edge1, edge2);
        const float faceNormalLength = Vectors::length(faceNormal);
        const Vector3D toCamera = Vectors::Subtraction(camera.position, vertexA);

        // Closed, outward-wound meshes do not need their back faces drawn.
        // Culling them before normalization, lighting, and rasterization
        // removes roughly half of a sphere's triangle workload.
        if (faceNormalLength == 0.0f || Vectors::DotProduct(faceNormal, toCamera) <= 0.0f)
        {
            continue;
        }

        // Normalized Normal
        const Vector3D normalizedfaceNormal = {
            faceNormal.x / faceNormalLength,
            faceNormal.y / faceNormalLength,
            faceNormal.z / faceNormalLength
        };


        // 3. Store a point-light position (for example, as a Vector3D near
        //    the camera). Subtract the first face vertex from that position to
        //    get a direction *toward* the light, then normalize it as well.
        const Vector3D lightPoint = camera.position;
        const Vector3D lightDirection = Vectors::Subtraction(lightPoint, vertexA);

        const float lightDirectionLength = Vectors::length(lightDirection);
        const Vector3D normalizedlightDirection = {
            lightDirection.x / lightDirectionLength,
            lightDirection.y / lightDirectionLength,
            lightDirection.z / lightDirectionLength
        };
        // 4. The dot product of the normalized normal and light direction is
        //    the Lambert brightness: 1 means directly lit, 0 means sideways,
        //    and negative means the light is behind the face. Clamp it to at
        //    least zero, or to a small ambient value so dark faces remain
        //    visible. Optional: multiply it by distance falloff from the
        //    point light for a more realistic result.
        float brightness = Vectors::DotProduct(normalizedfaceNormal, normalizedlightDirection);

        if(brightness < 0)
        {
            brightness = 0;
        };

        float ambientMinimum = std::max(0.1f, brightness);
        // 5. Multiply the base color's red, green, and blue channels by that
        //    brightness, clamp each channel to [0, 255], rebuild a pixel with
        //    framebuffer.color(...), and pass that shaded color below. Keep
        //    alpha unchanged. Recalculate per triangle for flat lighting.

        uint32_t shadedColor = fb.color(255 * ambientMinimum, 0, 0, 255);

        TriangleFill(
            fb,
            projectedVertices[triangle.first],
            projectedVertices[triangle.second],
            projectedVertices[triangle.third],
            shadedColor);
    }
}

void Rasterizer::Pyramid3DDraw(
    Framebuffer& fb,
    const Vertex3D& center,
    float size,
    const Camera& camera,
    uint32_t color)
{
    const Vector3D centerVector = {center.x, center.y, center.z};
    const Mesh pyramid = {
        {
            Vectors::Addition(centerVector, Vector3D{0.0f, -size, 0.0f}),
            Vectors::Addition(centerVector, Vector3D{-size, size, -size}),
            Vectors::Addition(centerVector, Vector3D{size, size, -size}),
            Vectors::Addition(centerVector, Vector3D{size, size, size}),
            Vectors::Addition(centerVector, Vector3D{-size, size, size})
        },
        {
            {0, 1, 2},
            {0, 2, 3},
            {0, 3, 4},
            {0, 4, 1}
        }
    };

    DrawMesh(fb, pyramid, camera, color);
}

void Rasterizer::CubeRaw3DDraw(
    Framebuffer& fb,
    const Vertex3D& center,
    int halfSize,
    const Camera& camera,
    uint32_t color)
{
    const float size = static_cast<float>(halfSize);
    const Vector3D centerVector = {center.x, center.y, center.z};
    const Mesh cube = {
        {
            Vectors::Addition(centerVector, Vector3D{-size, size, -size}),
            Vectors::Addition(centerVector, Vector3D{size, size, -size}),
            Vectors::Addition(centerVector, Vector3D{size, size, size}),
            Vectors::Addition(centerVector, Vector3D{-size, size, size}),
            Vectors::Addition(centerVector, Vector3D{-size, -size, -size}),
            Vectors::Addition(centerVector, Vector3D{size, -size, -size}),
            Vectors::Addition(centerVector, Vector3D{size, -size, size}),
            Vectors::Addition(centerVector, Vector3D{-size, -size, size})
        },
        {
            {0, 2, 1}, {0, 3, 2}, // top
            {4, 5, 6}, {4, 6, 7}, // bottom
            {0, 1, 5}, {0, 5, 4}, // front
            {3, 6, 2}, {3, 7, 6}, // back
            {0, 7, 3}, {0, 4, 7}, // left
            {1, 2, 6}, {1, 6, 5}  // right
        }
    };

    DrawMesh(fb, cube, camera, color);
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

    if (minX > maxX || minY > maxY)
    {
        return;
    }

    const auto edge = [](const Vertex2D& a, const Vertex2D& b, int x, int y)
    {
        return (x - a.x) * (b.y - a.y) - (y - a.y) * (b.x - a.x);
    };

    for (int y = minY; y <= maxY; y++)
    {
        // Intersect this scanline with the three edges first. This avoids
        // visiting the large empty parts of a triangle's bounding rectangle,
        // which become especially expensive when a nearby mesh is magnified.
        float leftIntersection = std::numeric_limits<float>::infinity();
        float rightIntersection = -std::numeric_limits<float>::infinity();
        const auto includeEdgeIntersection = [&](const Vertex2D& a, const Vertex2D& b)
        {
            if (a.y == b.y)
            {
                if (y == a.y)
                {
                    leftIntersection = std::min(leftIntersection, static_cast<float>(std::min(a.x, b.x)));
                    rightIntersection = std::max(rightIntersection, static_cast<float>(std::max(a.x, b.x)));
                }
                return;
            }

            const int edgeMinY = std::min(a.y, b.y);
            const int edgeMaxY = std::max(a.y, b.y);
            if (y < edgeMinY || y > edgeMaxY)
            {
                return;
            }

            const float progress = static_cast<float>(y - a.y) / static_cast<float>(b.y - a.y);
            const float x = a.x + progress * static_cast<float>(b.x - a.x);
            leftIntersection = std::min(leftIntersection, x);
            rightIntersection = std::max(rightIntersection, x);
        };

        includeEdgeIntersection(p0, p1);
        includeEdgeIntersection(p1, p2);
        includeEdgeIntersection(p2, p0);

        if (leftIntersection > rightIntersection)
        {
            continue;
        }

        const int startX = std::max(minX, static_cast<int>(std::floor(leftIntersection)));
        const int endX = std::min(maxX, static_cast<int>(std::ceil(rightIntersection)));
        for (int x = startX; x <= endX; x++)
        {
            const int e0 = edge(p0, p1, x, y);
            const int e1 = edge(p1, p2, x, y);
            const int e2 = edge(p2, p0, x, y);

            // All three edge tests must have the same sign for a pixel to be
            // inside a consistently wound triangle.
            if ((e0 >= 0 && e1 >= 0 && e2 >= 0) ||
                (e0 <= 0 && e1 <= 0 && e2 <= 0))
            {
                fb.SetPixelUnchecked(x, y, color);
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


void Rasterizer::SphereRaw3D(
    Framebuffer& fb,
    const Vertex3D& center,
    int radius,
    int sectorCount,
    int stackCount,
    const Camera& camera,
    uint32_t color)
{
    // A sphere needs at least three sectors around its middle and two vertical
    // sections from pole to pole. Avoid generating invalid or empty meshes.
    if (radius <= 0 || sectorCount < 3 || stackCount < 2)
    {
        return;
    }

    const float pi = 3.14159265358979323846f;
    const float sphereRadius = static_cast<float>(radius);
    const Vector3D centerVector = {center.x, center.y, center.z};
    Mesh sphere;
    sphere.vertices.reserve(static_cast<std::size_t>(stackCount + 1) * (sectorCount + 1));
    sphere.triangles.reserve(static_cast<std::size_t>(sectorCount) * 2 * (stackCount - 1));

    // Generate latitude rings. The duplicate vertex at sectorCount closes the
    // seam between longitude 0 and longitude 2*pi.
    for (int stack = 0; stack <= stackCount; ++stack)
    {
        const float latitude = -pi / 2.0f + pi * static_cast<float>(stack) / stackCount;
        const float ringRadius = sphereRadius * std::cos(latitude);
        const float y = sphereRadius * std::sin(latitude);

        for (int sector = 0; sector <= sectorCount; ++sector)
        {
            const float longitude = 2.0f * pi * static_cast<float>(sector) / sectorCount;
            const Vector3D localVertex = {
                ringRadius * std::cos(longitude),
                y,
                ringRadius * std::sin(longitude)};
            sphere.vertices.push_back(Vectors::Addition(centerVector, localVertex));
        }
    }

    const auto vertexIndex = [sectorCount](int stack, int sector)
    {
        return static_cast<std::size_t>(stack * (sectorCount + 1) + sector);
    };

    // Split each latitude/longitude cell into two outward-facing triangles.
    // One triangle is omitted at each pole because the duplicated pole points
    // would otherwise create a degenerate triangle with a zero-length normal.
    for (int stack = 0; stack < stackCount; ++stack)
    {
        for (int sector = 0; sector < sectorCount; ++sector)
        {
            const std::size_t topLeft = vertexIndex(stack, sector);
            const std::size_t topRight = vertexIndex(stack, sector + 1);
            const std::size_t bottomLeft = vertexIndex(stack + 1, sector);
            const std::size_t bottomRight = vertexIndex(stack + 1, sector + 1);

            if (stack != 0)
            {
                sphere.triangles.push_back({topLeft, bottomLeft, topRight});
            }
            if (stack != stackCount - 1)
            {
                sphere.triangles.push_back({topRight, bottomLeft, bottomRight});
            }
        }
    }


    DrawMesh(fb, sphere, camera, color);
}
