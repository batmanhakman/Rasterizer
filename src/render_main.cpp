#include "Scene.h"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
int Dimension(const char* value)
{
    std::size_t count = 0;
    const int result = std::stoi(value, &count);
    if (count != std::string(value).size() || result < 64 || result > 1920)
        throw std::runtime_error("Image dimensions must be integers between 64 and 1920.");
    return result;
}
float Number(const char* value)
{
    std::size_t count = 0;
    const float result = std::stof(value, &count);
    if (count != std::string(value).size() || !std::isfinite(result))
        throw std::runtime_error("Camera arguments must be finite numbers.");
    return result;
}
}

int RenderMain(int argc, char** argv)
{
    try
    {
        auto model = Scene::DefaultModelPath();
        std::filesystem::path output = "knight.ppm";
        int width = 1000, height = 760, positional = 0;
        float yaw = 0, pitch = 0, zoom = 0;
        for (int i = 1; i < argc; ++i)
        {
            const std::string option = argv[i];
            if (option == "--help" || option == "-h")
            {
                std::cout << "Usage: rasterizer_render [model.obj] [output.ppm] [--width N] [--height N]\n"
                    "                         [--yaw degrees] [--pitch degrees] [--zoom steps]\n"
                    "Defaults to the bundled knight. Camera arguments offset the initial view.\n";
                return 0;
            }
            if (option.rfind("--", 0) == 0)
            {
                if (++i == argc) throw std::runtime_error("Missing value for " + option);
                if (option == "--width") width = Dimension(argv[i]);
                else if (option == "--height") height = Dimension(argv[i]);
                else if (option == "--yaw") yaw = Number(argv[i]);
                else if (option == "--pitch") pitch = Number(argv[i]);
                else if (option == "--zoom") zoom = Number(argv[i]);
                else throw std::runtime_error("Unknown option: " + option);
            }
            else if (positional++ == 0) model = std::filesystem::u8path(option);
            else if (positional == 2) output = std::filesystem::u8path(option);
            else throw std::runtime_error("Too many positional arguments. Use --help for usage.");
        }
        Scene scene(width, height);
        std::string error;
        if (!scene.Load(model, error)) throw std::runtime_error(error);
        scene.Orbit(yaw * 0.01745329252f, pitch * 0.01745329252f);
        scene.Zoom(zoom);
        scene.Render();
        const auto& frame = scene.Frame();
        std::ofstream stream(output, std::ios::binary);
        if (!stream) throw std::runtime_error("Cannot create image: " + output.u8string());
        stream << "P6\n" << frame.GetWidth() << ' ' << frame.GetHeight() << "\n255\n";
        std::size_t visible = 0;
        for (int y = 0; y < frame.GetHeight(); ++y)
        {
            for (int x = 0; x < frame.GetWidth(); ++x)
            {
                const auto pixel = frame.GetPixel(x, y);
                const char rgb[]{static_cast<char>((pixel >> 16) & 255),
                    static_cast<char>((pixel >> 8) & 255), static_cast<char>(pixel & 255)};
                stream.write(rgb, 3);
                if ((pixel & 0xFFFFFFu) != 0x101827u) ++visible;
            }
        }
        stream.close();
        if (!stream) throw std::runtime_error("Could not finish writing image: " + output.u8string());
        if (visible == 0) throw std::runtime_error("The rendered frame contains no visible model pixels.");
        std::cout << "Rendered " << scene.Model().vertices.size() << " vertices, "
            << scene.Model().faces.size() << " triangles; " << visible << " visible pixels to "
            << output.u8string() << '\n';
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Rasterizer: " << error.what() << '\n';
        return 1;
    }
}

#ifdef _WIN32
int wmain(int argc, wchar_t** argv)
{
    // MSVC narrow argv uses the active code page; retain Unicode file paths.
    std::vector<std::string> utf8;
    std::vector<char*> arguments;
    utf8.reserve(argc);
    arguments.reserve(argc);
    for (int i = 0; i < argc; ++i) utf8.push_back(std::filesystem::path(argv[i]).u8string());
    for (auto& argument : utf8) arguments.push_back(argument.data());
    return RenderMain(argc, arguments.data());
}
#else
int main(int argc, char** argv) { return RenderMain(argc, argv); }
#endif
