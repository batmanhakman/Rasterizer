#include "ObjLoader.h"
#include "Rasterizer.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace
{
void Require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}
void Write(const std::filesystem::path& path, const std::string& text)
{
    std::ofstream file(path);
    file << text;
    Require(bool(file), "Failed to write test fixture");
}
struct TestDirectory
{
    std::filesystem::path path = std::filesystem::temp_directory_path() /
        ("rasterizer-test-" + std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    TestDirectory() { std::filesystem::create_directories(path); }
    ~TestDirectory() { std::error_code ignored; std::filesystem::remove_all(path, ignored); }
};
const uint32_t background = 0xff101828u;
const Camera camera = {{0,0,0},0,0,32};
size_t Painted(const Framebuffer& fb)
{
    return static_cast<size_t>(std::count_if(fb.GetBuffer(), fb.GetBuffer()+fb.GetWidth()*fb.GetHeight(),
        [](uint32_t pixel) { return pixel != background; }));
}

void TestObjAndTexture(const std::filesystem::path& directory)
{
    const unsigned char png[] = {137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,2,0,0,0,2,8,6,0,0,0,114,182,13,36,0,0,0,20,73,68,65,84,120,156,99,248,207,192,240,31,12,129,52,16,48,252,7,0,71,202,8,248,139,78,67,133,0,0,0,0,73,69,78,68,174,66,96,130};
    std::ofstream textureFile(directory / "quadrants.png", std::ios::binary);
    textureFile.write(reinterpret_cast<const char*>(png), sizeof(png));
    textureFile.close();
    Write(directory / "colors.mtl", "newmtl enamel\nKd 0.8 0.9 1\nmap_Kd quadrants.png\n");
    const std::string geometry = "v -1 -1 2\nv 1 -1 2\nv 1 1 2\nv -1 1 2\n"
        "vt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\nvn 0 0 -1\n";
    Write(directory / "quad.obj", "mtllib colors.mtl\n" + geometry +
        "usemtl enamel\nf -4/-1/-1 -3/-2/-1 -2/-3/-1 -1/-4/-1 # quad with UV seam\n");
    Mesh mesh;
    std::string error;
    if (!ObjLoader::Load(directory / "quad.obj", mesh, error)) throw std::runtime_error(error);
    Require(mesh.vertices.size()==4 && mesh.faces.size()==2, "OBJ quad must triangulate to two triangles");
    Require(mesh.faces[0].indices[0]==0 && mesh.faces[0].texcoordIndices[0]==3,
        "Negative UV indices must remain independent from position indices");
    Require(mesh.faces[0].normalIndices[0]==0, "Negative normal index was not loaded");
    Require(mesh.materials.size()==1 && mesh.faces[0].materialIndex==0, "MTL assignment missing");
    Require(mesh.materials[0].diffuse[0]==0.8f, "MTL Kd missing");
    const Texture& texture = mesh.materials[0].texture;
    Require(texture.width==2 && texture.height==2, "PNG dimensions incorrect");
    Require(texture.Sample(0,1)==0xffff0000u && texture.Sample(1,1)==0xff00ff00u &&
            texture.Sample(0,0)==0xff0000ffu && texture.Sample(1,0)==0xffffff00u,
            "PNG channels or OBJ bottom-left V convention incorrect");
    Framebuffer fb(64,64);
    fb.Clear(background);
    Rasterizer::DrawMesh(fb, mesh, camera, 0xffffffffu);
    Require(Painted(fb)>800, "Loaded textured quad failed to render");
    const uint32_t upperLeft=fb.GetPixel(20,20), lowerRight=fb.GetPixel(42,42);
    Require(((upperLeft>>16)&255)>0 && ((upperLeft>>8)&255)==0 && (upperLeft&255)==0,
        "Per-corner UVs not applied to loaded texture");
    Require((lowerRight&255)==0 && ((lowerRight>>16)&255)>0 && ((lowerRight>>8)&255)>0,
        "Asymmetric texture orientation changed during rasterization");

    for (const std::string& bad : {"f 0 2 3", "f 1 2 99", "f -99 2 3", "f 1x 2 3",
         "f 1/99 2/1 3/1", "f 1//99 2//1 3//1", "f 1 2", "f 1/ 2/1 3/1", "v nan 0 0"})
    {
        Write(directory / "bad.obj", geometry + bad + "\n");
        Require(!ObjLoader::Load(directory / "bad.obj", mesh, error), "Malformed OBJ accepted");
        Require(!error.empty() && mesh.faces.size()==2, "Failed load must preserve mesh and give diagnostic");
    }
    Write(directory / "missing.obj", "mtllib missing.mtl\n"+geometry+"f 1 2 3\n");
    Require(!ObjLoader::Load(directory / "missing.obj", mesh, error), "Missing MTL was ignored");
    Write(directory / "broken.mtl", "newmtl x\nmap_Kd absent.png\n");
    Write(directory / "broken.obj", "mtllib broken.mtl\n"+geometry+"f 1 2 3\n");
    Require(!ObjLoader::Load(directory / "broken.obj", mesh, error), "Missing texture was ignored");
}

void TestMetalHighlights(const std::filesystem::path& directory)
{
    Write(directory / "surfaces.mtl",
        "newmtl metal\nKd .18 .18 .18\nKs .8 .6 .3\nNs 48\n"
        "newmtl cloth\nKd .18 .18 .18\nKs 0 0 0\nNs 96\n"
        "newmtl matte\nKd .18 .18 .18\n"
        "newmtl limits\nKs -1 2 .5\nNs 2000\n");
    const std::string geometry = "v -.8 -.8 3\nv .8 -.8 3\nv 0 .8 3\n"
        "vn -.4056 -.7100 -1.575\nusemtl metal\nf 1//1 2//1 3//1\n";
    Write(directory / "surfaces.obj", "mtllib surfaces.mtl\n" + geometry);
    Mesh mesh;
    std::string error;
    if (!ObjLoader::Load(directory / "surfaces.obj", mesh, error)) throw std::runtime_error(error);
    Require(mesh.materials[0].specular[0] == .8f && mesh.materials[0].shininess == 48,
        "MTL Ks/Ns were not loaded");
    Require(mesh.materials[2].specular == std::array<float,3>{0,0,0} && mesh.materials[2].shininess == 32,
        "Materials without Ks must stay matte");
    Require(mesh.materials[3].specular == std::array<float,3>{0,1,.5f} && mesh.materials[3].shininess == 1000,
        "Out-of-range finite MTL reflection values were not clamped");
    Framebuffer metal(64,64), cloth(64,64), matte(64,64), turnedMetal(64,64), turnedCloth(64,64);
    metal.Clear(background); cloth.Clear(background); matte.Clear(background);
    turnedMetal.Clear(background); turnedCloth.Clear(background);
    Rasterizer::DrawMesh(metal, mesh, camera, 0xffffffffu);
    const Camera angled = {{2,0,0}, -std::atan2(2.0f,3.0f), 0, 32};
    Rasterizer::DrawMesh(turnedMetal, mesh, angled, 0xffffffffu);
    mesh.faces[0].materialIndex = 1;
    Rasterizer::DrawMesh(cloth, mesh, camera, 0xffffffffu);
    Rasterizer::DrawMesh(turnedCloth, mesh, angled, 0xffffffffu);
    mesh.faces[0].materialIndex = 2;
    Rasterizer::DrawMesh(matte, mesh, camera, 0xffffffffu);
    const auto red = [](uint32_t pixel) { return (pixel >> 16) & 255u; };
    Require(red(metal.GetPixel(32,32)) > red(cloth.GetPixel(32,32)) + 20,
        "Metal did not catch the key-light highlight");
    Require(red(metal.GetPixel(32,32)) > red(turnedMetal.GetPixel(32,32)) + 15,
        "Metal highlights do not respond to view direction");
    Require(cloth.GetPixel(32,32) == turnedCloth.GetPixel(32,32),
        "Matte cloth acquired view-dependent highlights");
    Require(std::equal(cloth.GetBuffer(), cloth.GetBuffer()+4096, matte.GetBuffer()),
        "Explicit zero Ks must preserve default diffuse-only rendering");
    Require(red(metal.GetPixel(32,32)) < 200, "Metal highlight overwhelms its diffuse appearance");
    for (const std::string& invalid : {"Ks nan 0 0", "Ks .5 inf 0", "Ks .5", "Ns nan", "Ns inf"})
    {
        Write(directory / "surfaces.mtl", "newmtl metal\n" + invalid + "\n");
        Require(!ObjLoader::Load(directory / "surfaces.obj", mesh, error),
            "Malformed specular material values were accepted");
    }
}
Mesh OverlappingTriangles()
{
    Mesh mesh;
    mesh.vertices={{-.75f,-.75f,1},{3.75f,-3.75f,5},{-3.75f,3.75f,5},
                   {-2.325f,-2.325f,3.1f},{2.325f,-2.325f,3.1f},{-2.325f,2.325f,3.1f}};
    mesh.faces={{0,1,2},{3,4,5}};
    Material red, green;
    red.diffuse={1,0,0}; green.diffuse={0,1,0};
    mesh.materials={red,green};
    mesh.faces[0].materialIndex=0; mesh.faces[1].materialIndex=1;
    return mesh;
}

void TestDepth()
{
    Mesh mesh=OverlappingTriangles();
    Framebuffer first(64,64), second(64,64);
    first.Clear(background); second.Clear(background);
    Rasterizer::DrawMesh(first,mesh,camera,0xffffffffu);
    std::reverse(mesh.faces.begin(),mesh.faces.end());
    Rasterizer::DrawMesh(second,mesh,camera,0xffffffffu);
    Require(std::equal(first.GetBuffer(),first.GetBuffer()+4096,second.GetBuffer()),
        "Occlusion must be independent of face order");
    Require((first.GetPixel(18,18)&0x00ff0000u)>0 && (first.GetPixel(18,18)&0x0000ff00u)==0,
        "Near red triangle did not occlude green");
    Require((first.GetPixel(48,12)&0x0000ff00u)>0 && (first.GetPixel(48,12)&0x00ff0000u)==0,
        "Intersecting green triangle did not occlude red");
    first.Clear(background);
    mesh.faces={mesh.faces[0]}; // green at z=3.1, farther than the previous red pixels
    Rasterizer::DrawMesh(first,mesh,camera,0xffffffffu);
    Require((first.GetPixel(18,18)&0x0000ff00u)>0, "Clear failed to reset the depth buffer");

    first.Clear(background); second.Clear(background);
    mesh=OverlappingTriangles();
    Mesh other=mesh;
    other.faces={mesh.faces[1]}; mesh.faces={mesh.faces[0]};
    Rasterizer::DrawMesh(first,mesh,camera,0xffffffffu);
    Rasterizer::DrawMesh(first,other,camera,0xffffffffu);
    Rasterizer::DrawMesh(second,other,camera,0xffffffffu);
    Rasterizer::DrawMesh(second,mesh,camera,0xffffffffu);
    Require(std::equal(first.GetBuffer(),first.GetBuffer()+4096,second.GetBuffer()),
        "Depth buffer must persist across DrawMesh calls in one frame");
}

void TestPerspectiveAndClipping()
{
    Mesh mesh;
    mesh.vertices={{-.5f,-.5f,1},{2,-2,4},{-.5f,.5f,1}};
    mesh.faces={{0,1,2}};
    mesh.texcoords={{0,.5f},{1,.5f},{0,.5f}};
    mesh.faces[0].texcoordIndices={0,1,2}; mesh.faces[0].materialIndex=0;
    Material material;
    material.texture.width=2; material.texture.height=1;
    material.texture.pixels={0xffff0000u,0xff00ff00u};
    mesh.materials={material};
    Framebuffer fb(64,64);
    fb.Clear(background);
    Rasterizer::DrawMesh(fb,mesh,camera,0xffffffffu);
    Require((fb.GetPixel(36,20)&0x00ff0000u)>0 && (fb.GetPixel(36,20)&0x0000ff00u)==0,
        "UVs are affine rather than perspective-correct");
    Require((fb.GetPixel(44,17)&0x0000ff00u)>0, "Texture is not sampled across the triangle");

    mesh.materials.clear(); mesh.faces[0].materialIndex=-1;
    mesh.vertices={{0,-.08f,-.1f},{.6f,.5f,1},{-.6f,.5f,1},{0,0,-20}};
    fb.Clear(background);
    Rasterizer::DrawMesh(fb,mesh,camera,0xffff0000u);
    Require(Painted(fb)>100, "Near-plane crossing triangle disappeared instead of clipping");
    mesh.vertices[0].z=-1; mesh.vertices[1].z=-1; mesh.vertices[2].z=-1;
    fb.Clear(background);
    Rasterizer::DrawMesh(fb,mesh,camera,0xffffffffu);
    Require(Painted(fb)==0, "Fully behind-camera triangle was drawn");
    mesh.vertices={{-.5f,-.5f,1},{.5f,-.5f,1},{0,.5f,1},{0,0,-20}};
    Rasterizer::DrawMesh(fb,mesh,camera,0xffff0000u);
    Require(Painted(fb)>100, "Unused behind-camera vertex caused the visible mesh to disappear");
}
} // namespace

int main(int argc, char** argv)
{
    try
    {
        TestDirectory directory;
        TestObjAndTexture(directory.path);
        TestDepth();
        TestMetalHighlights(directory.path);
        TestPerspectiveAndClipping();
        if (argc>1)
        {
            Mesh knight;
            std::string error;
            if (!ObjLoader::Load(std::filesystem::u8path(argv[1]),knight,error)) throw std::runtime_error(error);
            Require(knight.faces.size()>100 && !knight.texcoords.empty(), "Knight mesh is missing geometry or UVs");
            Require(std::any_of(knight.materials.begin(),knight.materials.end(),
                [](const Material& material) { return !material.texture.Empty(); }), "Knight PNG texture was not loaded");
            for (const Face& face : knight.faces)
            {
                Require(face.materialIndex>=0 && static_cast<size_t>(face.materialIndex)<knight.materials.size(),
                    "Knight face is missing its material");
                for (uint32_t uv : face.texcoordIndices) Require(uv<knight.texcoords.size(), "Knight face is missing a UV");
            }
            std::cout << "Knight integration: " << knight.vertices.size() << " vertices, " << knight.faces.size() << " triangles\n";
        }
        std::cout << "All renderer core tests passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
