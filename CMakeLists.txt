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

add_executable(jetfighter ${SOURCES})

target_compile_definitions(jetfighter PRIVATE GLM_ENABLE_EXPERIMENTAL)

target_link_libraries(jetfighter
    SDL2::SDL2
    OpenGL::GL
    GLEW::GLEW
    assimp::assimp
    glm::glm
    SDL2_mixer::SDL2_mixer
)
