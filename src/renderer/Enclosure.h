#pragma once

#include "Shader.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

struct Enclosure {
private:
  Shader shader;
  GLuint vao, vbo;
  int vertCount = 0;
  float halfW, halfD, topY, botY;

public:
  Enclosure(float width, float depth, float altMax, float ground = -200.f);
  void draw(glm::mat4 vp);
};
