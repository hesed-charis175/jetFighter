#pragma once

#include "Plane.h"
#include "config.h"
#include <functional>
#include <glm/fwd.hpp>
struct CameraState {
  glm::vec3 pos;
  float dist = CAM_DIST_MIN;
  float orbitYaw = 0.f;
  float orbitPitch = 0.f;

  void update(const Plane &target, const Uint8 *keys, float dt,
              std::function<float(float, float)> terrainH);
  glm::mat4 view(const Plane &target) const;
};
