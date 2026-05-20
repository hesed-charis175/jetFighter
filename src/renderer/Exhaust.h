#pragma once
#include "Shader.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

struct ExhaustRay {
  glm::vec3 origin;
  glm::vec3 dir;
  float size;
  float length;
  float thrust;
};

class Exhaust {
public:
  Exhaust(const std::vector<glm::vec3> &nozzleOffsets);
  ~Exhaust();

  void update(const glm::mat4 &modelMat, float thrustFraction, float dt);
  void draw(const glm::mat4 &view, const glm::mat4 &proj,
            const glm::vec3 &camPos);

private:
  std::vector<glm::vec3> nozzles;
  std::vector<ExhaustRay> rays;
  Shader shader;
  GLuint vao = 0, vbo = 0;

  glm::vec3 lastPlaneRight{1.f, 0.f, 0.f};

  static constexpr float EX_SIZE = 2.4f;
  static constexpr float EX_LENGTH = 8.0f;
};
