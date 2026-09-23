// RASTERIZER.CPP, THE DECLARATION OF ALL THE METHODS DECLARED IN RASTERIZER.H
//FUTHERMORE, THIS CPP ADDS DEFINITIONS AND GIVES MEANING TO THESE METOHDS, 
//THIS CPP ALLOWS:
// 1. DRAWING 2D
// 2. DRAWING 3D
// 3. CAMERA-VIEWED 3D MESHES
//-------------------------------- 

#include "Rasterizer.h"
#include "Vectors.h"
#include "Mesh.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
constexpr float kNearClipDistance = 0.05f;

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

Vector3D ToVector3D(const Vertex& vertex)
{
    return Vector3D{vertex.x, vertex.y, vertex.z};
}

Vertex ToMeshVertex(const Vector3D& vector)
{
    return Vertex{vector.x, vector.y, vector.z};
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
    // coordinates. DrawMesh clips triangles against this plane before projection.
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

namespace
{
struct ClipVertex
{
    Vector3D position;
    float u = 0.0f;
    float v = 0.0f;
    float light = 1.0f;
    float highlight = 0.0f;
};

ClipVertex Interpolate(const ClipVertex& a, const ClipVertex& b, float t)
{
    return {{a.position.x + (b.position.x-a.position.x)*t,
             a.position.y + (b.position.y-a.position.y)*t,
             a.position.z + (b.position.z-a.position.z)*t},
            a.u+(b.u-a.u)*t, a.v+(b.v-a.v)*t, a.light+(b.light-a.light)*t,
            a.highlight+(b.highlight-a.highlight)*t};
}

template<typename Distance>
void ClipPolygon(std::vector<ClipVertex>& polygon, Distance distance)
{
    if (polygon.empty()) return;
    std::vector<ClipVertex> output;
    output.reserve(polygon.size()+1);
    ClipVertex previous = polygon.back();
    float previousDistance = distance(previous.position);
    for (const ClipVertex& current : polygon)
    {
        const float currentDistance = distance(current.position);
        const bool currentInside = currentDistance >= 0.0f;
        const bool previousInside = previousDistance >= 0.0f;
        if (currentInside != previousInside)
        {
            const float t = previousDistance / (previousDistance-currentDistance);
            output.push_back(Interpolate(previous, current, t));
        }
        if (currentInside) output.push_back(current);
        previous = current;
        previousDistance = currentDistance;
    }
    polygon = std::move(output);
}

// World-space soft key light. The same direction drives diffuse and highlights.
constexpr Vector3D kKeyLight = {-0.4056f, -0.7100f, -0.5750f};

float Lighting(Vector3D normal)
{
    const float length = Vectors::length(normal);
    if (!(length > 0.0f) || !std::isfinite(length)) return 0.4f;
    return 0.34f + 0.66f * std::max(0.0f, Vectors::DotProduct(normal, kKeyLight) / length);
}

float SpecularHighlight(Vector3D normal, Vector3D position, const Camera& camera, float shininess)
{
    const float normalLength = Vectors::length(normal);
    Vector3D view = Vectors::Subtraction(camera.position, position);
    const float viewLength = Vectors::length(view);
    if (!(normalLength > 0.0f) || !(viewLength > 0.0f) ||
        !std::isfinite(normalLength) || !std::isfinite(viewLength)) return 0.0f;
    normal = Vectors::Scalar(normal, 1.0f / normalLength);
    view = Vectors::Scalar(view, 1.0f / viewLength);
    const float lit = Vectors::DotProduct(normal, kKeyLight);
    // Faces remain two-sided, but their unlit/back-facing sides do not reflect
    // the key light. Explicit OBJ normals follow the same policy as diffuse.
    if (lit <= 0.0f || Vectors::DotProduct(normal, view) <= 0.0f) return 0.0f;
    Vector3D halfway = Vectors::Addition(kKeyLight, view);
    const float halfLength = Vectors::length(halfway);
    if (!(halfLength > 0.0f)) return 0.0f;
    halfway = Vectors::Scalar(halfway, 1.0f / halfLength);
    return lit * std::pow(std::clamp(Vectors::DotProduct(normal, halfway), 0.0f, 1.0f), shininess);
}

uint32_t Shade(uint32_t color, const std::array<float, 3>& diffuse, float light,
               const std::array<float, 3>& specular, float highlight)
{
    const auto channel = [&](unsigned shift, size_t index)
    {
        float value = float((color >> shift) & 255u) * diffuse[index] * light;
        // Restrict reflections to the remaining display range instead of
        // clipping large white patches into the authored diffuse texture.
        value += std::max(0.0f, 255.0f - value) * 0.55f * specular[index] * highlight;
        return static_cast<uint32_t>(std::clamp(value, 0.0f, 255.0f));
    };
    return 0xff000000u | (channel(16, 0) << 16) | (channel(8, 1) << 8) | channel(0, 2);
}

void FillProjected(Framebuffer& fb, const ClipVertex& a, const ClipVertex& b,
                   const ClipVertex& c, float focalLength, const Material* material,
                   bool textured, uint32_t fallbackColor)
{
    struct ScreenVertex { double x, y, inverseZ, uOverZ, vOverZ, lightOverZ, highlightOverZ; };
    const auto project = [&](const ClipVertex& vertex)
    {
        const double inverseZ = 1.0 / vertex.position.z;
        return ScreenVertex{fb.GetWidth()*0.5 + focalLength*vertex.position.x*inverseZ,
            fb.GetHeight()*0.5 + focalLength*vertex.position.y*inverseZ, inverseZ,
            vertex.u*inverseZ, vertex.v*inverseZ, vertex.light*inverseZ, vertex.highlight*inverseZ};
    };
    const ScreenVertex p[3] = {project(a), project(b), project(c)};
    const auto edge = [](const ScreenVertex& a, const ScreenVertex& b, double x, double y)
    {
        return (b.x-a.x)*(y-a.y) - (b.y-a.y)*(x-a.x);
    };
    const double area = edge(p[0], p[1], p[2].x, p[2].y);
    if (!std::isfinite(area) || std::abs(area) < 1e-10) return;
    // Side-plane clipping ensures these bounds stay representable as int.
    const int minX = static_cast<int>(std::max(0.0, std::floor(std::min({p[0].x,p[1].x,p[2].x}))));
    const int maxX = static_cast<int>(std::min(double(fb.GetWidth()-1), std::ceil(std::max({p[0].x,p[1].x,p[2].x}))));
    const int minY = static_cast<int>(std::max(0.0, std::floor(std::min({p[0].y,p[1].y,p[2].y}))));
    const int maxY = static_cast<int>(std::min(double(fb.GetHeight()-1), std::ceil(std::max({p[0].y,p[1].y,p[2].y}))));
    const std::array<float, 3> diffuse = material ? material->diffuse : std::array<float,3>{1,1,1};
    const std::array<float, 3> specular = material ? material->specular : std::array<float,3>{0,0,0};
    for (int y = minY; y <= maxY; ++y)
    {
        for (int x = minX; x <= maxX; ++x)
        {
            const double w0 = edge(p[1],p[2],x+0.5,y+0.5) / area;
            const double w1 = edge(p[2],p[0],x+0.5,y+0.5) / area;
            const double w2 = 1.0-w0-w1;
            if (w0 < -1e-9 || w1 < -1e-9 || w2 < -1e-9) continue;
            const double inverseZ = w0*p[0].inverseZ+w1*p[1].inverseZ+w2*p[2].inverseZ;
            if (!(inverseZ > 0.0)) continue;
            const float z = static_cast<float>(1.0 / inverseZ);
            const float light = static_cast<float>((w0*p[0].lightOverZ+w1*p[1].lightOverZ+w2*p[2].lightOverZ) / inverseZ);
            const float highlight = static_cast<float>((w0*p[0].highlightOverZ+w1*p[1].highlightOverZ+w2*p[2].highlightOverZ) / inverseZ);
            uint32_t base = material ? 0xffffffffu : fallbackColor;
            if (textured)
            {
                const float u = static_cast<float>((w0*p[0].uOverZ+w1*p[1].uOverZ+w2*p[2].uOverZ) / inverseZ);
                const float v = static_cast<float>((w0*p[0].vOverZ+w1*p[1].vOverZ+w2*p[2].vOverZ) / inverseZ);
                base = material->texture.Sample(u, v);
            }
            fb.SetPixelDepthUnchecked(x, y, z, Shade(base, diffuse, light, specular, highlight));
        }
    }
}
} // namespace

void Rasterizer::DrawMesh(Framebuffer& fb, const Mesh& mesh, const Camera& camera, uint32_t color)
{
    if (!(camera.focalLength > 0.0f) || !std::isfinite(camera.focalLength)) return;
    std::vector<Vector3D> cameraVertices;
    cameraVertices.reserve(mesh.vertices.size());
    for (const Vertex& vertex : mesh.vertices)
        cameraVertices.push_back(WorldToCamera(ToVector3D(vertex), camera));
    const float halfWidth = fb.GetWidth() / (2.0f*camera.focalLength);
    const float halfHeight = fb.GetHeight() / (2.0f*camera.focalLength);
    for (const Face& face : mesh.faces)
    {
        bool valid = true;
        for (uint32_t index : face.indices)
        {
            if (index >= mesh.vertices.size()) { valid = false; break; }
            const auto& p = cameraVertices[index];
            if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) { valid = false; break; }
        }
        if (!valid) continue;
        const Vector3D a = ToVector3D(mesh.vertices[face.indices[0]]);
        const Vector3D b = ToVector3D(mesh.vertices[face.indices[1]]);
        const Vector3D c = ToVector3D(mesh.vertices[face.indices[2]]);
        const Vector3D normal = Vectors::CrossProduct(Vectors::Subtraction(b,a), Vectors::Subtraction(c,a));
        if (!(Vectors::length(normal) > 1e-10f)) continue;
        const Material* material = face.materialIndex >= 0 && static_cast<size_t>(face.materialIndex) < mesh.materials.size()
            ? &mesh.materials[face.materialIndex] : nullptr;
        bool textured = material && !material->texture.Empty();
        for (uint32_t uvIndex : face.texcoordIndices)
            textured = textured && uvIndex < mesh.texcoords.size();
        const float flatLight = Lighting(normal);
        const bool reflective = material && std::any_of(material->specular.begin(), material->specular.end(),
            [](float channel) { return channel > 0.0f; });
        std::vector<ClipVertex> polygon;
        polygon.reserve(8);
        for (size_t i = 0; i < 3; ++i)
        {
            ClipVertex vertex{cameraVertices[face.indices[i]], 0, 0, flatLight};
            if (textured)
            {
                const auto& uv = mesh.texcoords[face.texcoordIndices[i]];
                vertex.u = uv.u;
                vertex.v = uv.v;
            }
            Vector3D shadingNormal = normal;
            if (face.normalIndices[i] < mesh.normals.size())
            {
                shadingNormal = ToVector3D(mesh.normals[face.normalIndices[i]]);
                vertex.light = Lighting(shadingNormal);
            }
            if (reflective)
                vertex.highlight = SpecularHighlight(shadingNormal,
                    ToVector3D(mesh.vertices[face.indices[i]]), camera, material->shininess);
            polygon.push_back(vertex);
        }
        // Clip camera-space positions and attributes before dividing by Z.
        ClipPolygon(polygon, [](Vector3D p) { return p.z-kNearClipDistance; });
        ClipPolygon(polygon, [=](Vector3D p) { return p.x+p.z*halfWidth; });
        ClipPolygon(polygon, [=](Vector3D p) { return p.z*halfWidth-p.x; });
        ClipPolygon(polygon, [=](Vector3D p) { return p.y+p.z*halfHeight; });
        ClipPolygon(polygon, [=](Vector3D p) { return p.z*halfHeight-p.y; });
        for (size_t i = 1; i+1 < polygon.size(); ++i)
            FillProjected(fb,polygon[0],polygon[i],polygon[i+1],camera.focalLength,material,textured,color);
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
            ToMeshVertex(Vectors::Addition(centerVector, Vector3D{0.0f, -size, 0.0f})),
            ToMeshVertex(Vectors::Addition(centerVector, Vector3D{-size, size, -size})),
            ToMeshVertex(Vectors::Addition(centerVector, Vector3D{size, size, -size})),
            ToMeshVertex(Vectors::Addition(centerVector, Vector3D{size, size, size})),
            ToMeshVertex(Vectors::Addition(centerVector, Vector3D{-size, size, size}))
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
            ToMeshVertex(Vectors::Addition(centerVector, Vector3D{-size, size, -size})),
            ToMeshVertex(Vectors::Addition(centerVector, Vector3D{size, size, -size})),
            ToMeshVertex(Vectors::Addition(centerVector, Vector3D{size, size, size})),
            ToMeshVertex(Vectors::Addition(centerVector, Vector3D{-size, size, size})),
            ToMeshVertex(Vectors::Addition(centerVector, Vector3D{-size, -size, -size})),
            ToMeshVertex(Vectors::Addition(centerVector, Vector3D{size, -size, -size})),
            ToMeshVertex(Vectors::Addition(centerVector, Vector3D{size, -size, size})),
            ToMeshVertex(Vectors::Addition(centerVector, Vector3D{-size, -size, size}))
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
    sphere.faces.reserve(static_cast<std::size_t>(sectorCount) * 2 * (stackCount - 1));

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
            sphere.vertices.push_back(ToMeshVertex(Vectors::Addition(centerVector, localVertex)));
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
                sphere.faces.push_back({
                    static_cast<uint32_t>(topLeft),
                    static_cast<uint32_t>(bottomLeft),
                    static_cast<uint32_t>(topRight)});
            }
            if (stack != stackCount - 1)
            {
                sphere.faces.push_back({
                    static_cast<uint32_t>(topRight),
                    static_cast<uint32_t>(bottomLeft),
                    static_cast<uint32_t>(bottomRight)});
            }
        }
    }


    DrawMesh(fb, sphere, camera, color);
}
