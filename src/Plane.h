#pragma once

#include "config.h"

struct PlaneInput {
  float thrust = 0.f;
  float yaw = 0.f;
  float pitch = 0.f;
};

struct Plane {
  bool isPlayerControlled = false;
  glm::vec3 position;
  glm::quat orientation;
  float speed;

  float yawRate = 0.f;
  float pitchRate = 0.f;
  float bankAngle = 0.f;

  glm::mat4 modelMat = glm::mat4(1.f);
  glm::quat visualOrientation;
  float thrustFraction = 0.f;

  Plane(glm::vec3 pos, float headingDeg, bool player);

  void updateAero(const PlaneInput &in, float dt);
  void rebuildModelMat();
  glm::vec3 nose() const { return orientation * glm::vec3(0, 0, -1); }
  glm::vec3 up() const { return orientation * glm::vec3(0, 1, 0); }
  glm::vec3 right() const { return orientation * glm::vec3(1, 0, 0); };
};
