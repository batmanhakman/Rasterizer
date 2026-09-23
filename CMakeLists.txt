cmake_minimum_required(VERSION 3.20)
project(Rasterizer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
# A release ZIP runs without a separate Visual C++ redistributable install.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_library(rasterizer_core
    src/framebuffer.cpp src/Rasterizer.cpp src/Vectors.cpp
    src/ObjLoader.cpp src/Texture.cpp src/Scene.cpp)
target_include_directories(rasterizer_core PUBLIC include PRIVATE third_party)
if(MSVC)
    target_compile_options(rasterizer_core PRIVATE /W4 /utf-8)
    target_compile_definitions(rasterizer_core PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE)
else()
    target_compile_options(rasterizer_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(rasterizer_render src/render_main.cpp)
target_link_libraries(rasterizer_render PRIVATE rasterizer_core)

if(WIN32)
    add_executable(Rasterizer WIN32 main.cpp src/window.cpp)
    target_link_libraries(Rasterizer PRIVATE rasterizer_core user32 gdi32)
elseif(APPLE)
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 17)
    set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
    add_executable(Rasterizer main.cpp src/macos.mm)
    target_compile_options(Rasterizer PRIVATE "$<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>")
    target_link_libraries(Rasterizer PRIVATE rasterizer_core "-framework Cocoa")
else()
    message(STATUS "Native window requires Windows or macOS; building headless renderer and tests.")
endif()

# All targets share a per-configuration output directory. One copy target avoids
# racing concurrent asset copies during parallel builds.
add_custom_target(rasterizer_assets ALL
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets" "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/assets"
    COMMENT "Copying knight model and textures next to the executables")
add_dependencies(rasterizer_render rasterizer_assets)
if(TARGET Rasterizer)
    add_dependencies(Rasterizer rasterizer_assets)
    install(TARGETS Rasterizer RUNTIME DESTINATION .)
    set_property(TARGET Rasterizer PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Rasterizer>")
endif()
install(TARGETS rasterizer_render RUNTIME DESTINATION .)
install(DIRECTORY assets/ DESTINATION assets)
install(DIRECTORY source/ DESTINATION source FILES_MATCHING PATTERN "*.blend" PATTERN "*.md")
install(FILES README.md DESTINATION .)
install(FILES docs/knight-preview.png docs/knight-blender.png DESTINATION docs)
install(FILES third_party/README.md DESTINATION third_party)
install(FILES third_party/LICENSE.stb DESTINATION licenses OPTIONAL)

include(CTest)
if(BUILD_TESTING)
    add_executable(rasterizer_tests tests/core_tests.cpp)
    target_link_libraries(rasterizer_tests PRIVATE rasterizer_core)
    add_dependencies(rasterizer_tests rasterizer_assets)
    add_test(NAME core COMMAND rasterizer_tests "${CMAKE_CURRENT_SOURCE_DIR}/assets/knight/knight.obj")
    # CTest runs from the build root, deliberately outside the executable folder.
    add_test(NAME render_bundled COMMAND rasterizer_render)
    add_test(NAME render_back COMMAND rasterizer_render
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/knight/knight.obj" "${CMAKE_BINARY_DIR}/knight-back.ppm" --yaw 180)
endif()

set(CPACK_GENERATOR ZIP)
set(CPACK_PACKAGE_NAME Rasterizer)
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "Rasterizer-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
include(CPack)
