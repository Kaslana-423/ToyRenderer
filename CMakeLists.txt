cmake_minimum_required(VERSION 3.20)

project(MyRenderer VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Keep third-party projects small and focused on what this demo needs.
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_INSTALL OFF CACHE BOOL "" FORCE)

# Build a small static Assimp with the scene and character formats used here.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_MMD_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)

foreach(dependency glfw glm stb assimp imgui)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/${dependency}")
        message(FATAL_ERROR
            "Missing external/${dependency}. Please restore the vendored dependencies."
        )
    endif()
endforeach()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad/src/gl.c" OR
   NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad/include/glad/gl.h")
    message(FATAL_ERROR "Missing the generated OpenGL loader under third_party/glad.")
endif()

add_subdirectory(external/glfw)
add_subdirectory(external/glm)
add_subdirectory(external/assimp)

# Compile the pre-generated OpenGL 3.3 Core loader. Keeping generated sources
# in the main repository means normal builds do not require Python or Jinja2.
add_library(glad_gl_core_33 STATIC third_party/glad/src/gl.c)
target_include_directories(glad_gl_core_33 PUBLIC third_party/glad/include)

# stb is header-only. It is ready for later LearnOpenGL texture-loading chapters.
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/external/stb")

add_library(imgui STATIC
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends"
)
target_link_libraries(imgui PUBLIC glfw)

add_executable(MyRenderer
    src/DebugUI.cpp
    src/main.cpp
    src/Renderer.cpp
    src/Scene.cpp
    src/stb_image.cpp
)
target_compile_definitions(MyRenderer PRIVATE GLFW_INCLUDE_NONE)
target_include_directories(MyRenderer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(MyRenderer PRIVATE
    glfw
    glad_gl_core_33
    glm::glm
    assimp::assimp
    stb
    imgui
)

if(MSVC)
    target_compile_options(MyRenderer PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(MyRenderer PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Copy only the shaders and models used by the current scene.
add_custom_command(TARGET MyRenderer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:MyRenderer>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/lighting.vs"
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/lighting.fs"
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/shadow_depth.vs"
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/shadow_depth.fs"
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/ssao_geometry.vs"
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/ssao_geometry.fs"
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/ssao.fs"
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/ssao_blur.fs"
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/hdr.vs"
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/hdr.fs"
        "$<TARGET_FILE_DIR:MyRenderer>/shaders"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:MyRenderer>/res/model/pmxHouse"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/res/model/pmxHouse"
        "$<TARGET_FILE_DIR:MyRenderer>/res/model/pmxHouse"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:MyRenderer>/res/model/lisa"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/res/model/lisa"
        "$<TARGET_FILE_DIR:MyRenderer>/res/model/lisa"
    COMMENT "Copying renderer shaders and scene assets"
)

# Make the green Run button in a generated Visual Studio solution launch the demo.
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT MyRenderer)
set_target_properties(MyRenderer PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:MyRenderer>"
)
