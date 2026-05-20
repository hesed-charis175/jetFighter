#pragma once
#include "Shader.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

struct TracerRound {
  glm::vec3 pos;
  glm::vec3 dir;
  float speed = 0.f;
  float life = 0.f;
  bool active = false;
};

class Tracer {
public:
  Tracer();
  ~Tracer();

  void fire(const glm::vec3 &planePos, const glm::mat4 &modelMat,
            const glm::quat &ori, float planeSpeed);
  void update(float dt);
  void draw(const glm::mat4 &vp, const glm::vec3 &camPos);

private:
  static constexpr int MAX_ROUNDS = 256;
  static constexpr float ROUND_SPEED = 900.f;
  static constexpr float LIFETIME = 1.8f;
  static constexpr float FIRE_RATE = 0.06f;
  static constexpr float SPEED_MAX = 250.f;
  std::vector<TracerRound> rounds;
  float fireTimer = 0.f;

  Shader shader;
  GLuint vao = 0, vbo = 0;
};
