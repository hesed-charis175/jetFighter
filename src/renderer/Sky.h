#pragma once
#include "Shader.h"
#include <GL/glew.h>
#include <glm/glm.hpp>

class Sky {
public:
  Sky();
  ~Sky();
  void draw(const glm::mat4 &view, const glm::mat4 &proj,
            const glm::vec3 &sunDir, float time);

private:
  Shader shader;
  GLuint vao, vbo;
};
