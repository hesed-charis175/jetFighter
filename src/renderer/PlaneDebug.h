#pragma once
#include <GL/glew.h>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class PlaneDebug {
public:
  PlaneDebug();
  ~PlaneDebug();

  void draw(const glm::vec3 &pos, const glm::quat &orientation,
            const glm::mat4 &vp);

private:
  GLuint mVAO = 0, mVBO = 0, mProg = 0;

  static const glm::vec3 RED_PTS[10];
  static const glm::vec3 GREEN_PTS[16];
  static constexpr int TOTAL_PTS = 26;
};
