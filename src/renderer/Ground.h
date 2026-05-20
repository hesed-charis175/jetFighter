#pragma once
#include "Shader.h"
#include <GL/glew.h>
#include <glm/glm.hpp>

class Ground {
public:
  Ground();
  ~Ground();
  void draw(const glm::mat4 &view, const glm::mat4 &proj);

private:
  Shader shader;
  GLuint vao, vbo, ebo;
  int indexCount;
};
