cmake_minimum_required(VERSION 3.20)
project(JetFighter)
set(CMAKE_CXX_STANDARD 20)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(assimp REQUIRED)
find_package(glm REQUIRED)
find_package(SDL2_mixer REQUIRED)


file(GLOB_RECURSE SOURCES src/*.cpp)

add_executable(rjetfighter ${SOURCES})

target_compile_definitions(rjetfighter PRIVATE GLM_ENABLE_EXPERIMENTAL)

target_link_libraries(rjetfighter
    SDL2::SDL2
    OpenGL::GL
    GLEW::GLEW
    assimp::assimp
    glm::glm
    SDL2_mixer::SDL2_mixer
)
