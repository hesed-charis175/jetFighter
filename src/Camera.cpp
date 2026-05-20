#include "Camera.h"
#include "Keys.h"

void CameraState::update(const Plane &target, const Uint8 *keys, float dt,
                         std::function<float(float, float)> terrainH) {
  glm::vec3 up = target.up();
  glm::vec3 right = target.right();
  glm::vec3 back = target.orientation * glm::vec3(0, 0, 1);

  float targetOrbitYaw = 0.f;
  float targetOrbitPitch = 0.f;
  if (keys[KEY_LOOK_BACKWARD])
    targetOrbitYaw = glm::radians(180.f);
  if (keys[KEY_LOOK_LEFT])
    targetOrbitYaw = -glm::radians(90.f);
  if (keys[KEY_LOOK_RIGHT])
    targetOrbitYaw = glm::radians(90.f);
  if (keys[KEY_LOOK_DOWN])
    targetOrbitPitch = -glm::radians(80.f);
  if (keys[KEY_LOOK_UP])
    targetOrbitPitch = glm::radians(80.f);

  bool yawActive =
      keys[KEY_LOOK_BACKWARD] || keys[KEY_LOOK_LEFT] || keys[KEY_LOOK_RIGHT];
  bool pitchActive = keys[KEY_LOOK_DOWN] || keys[KEY_LOOK_UP];

  float yawDelta = targetOrbitYaw - orbitYaw;
  if (yawDelta > glm::pi<float>())
    yawDelta -= glm::two_pi<float>();
  if (yawDelta < -glm::pi<float>())
    yawDelta += glm::two_pi<float>();
  orbitYaw += yawDelta * (yawActive ? 8.f : 2.5f) * dt;
  orbitPitch +=
      (targetOrbitPitch - orbitPitch) * (pitchActive ? 8.f : 2.5f) * dt;

  float speedT = (target.speed - SPEED_MIN) / (SPEED_MAX - SPEED_MIN);
  float distT = glm::clamp((speedT - 0.5f) / 0.5f, 0.f, 1.f);
  float targetDist = keys[KEY_THRUST_FORWARD]
                         ? glm::mix(CAM_DIST_MIN, CAM_DIST_MAX, distT)
                         : CAM_DIST_MIN;
  dist += (targetDist - dist) * ((targetDist > dist) ? 8.f : 2.5f) * dt;

  glm::quat qOrbitYaw = glm::angleAxis(orbitYaw, up);
  glm::quat qOrbitPitch = glm::angleAxis(orbitPitch, right);
  glm::vec3 orbitOffset = qOrbitPitch * (qOrbitYaw * (back * dist));
  pos = target.position + orbitOffset + up * CAM_HEIGHT;

  float groundY = terrainH(pos.x, pos.z) + 3.f;
  if (pos.y < groundY) {
    orbitPitch += (glm::radians(-60.f) - orbitPitch) * 10.f * dt;

    glm::quat qY = glm::angleAxis(orbitYaw, up);
    glm::quat qP = glm::angleAxis(orbitPitch, right);
    pos = target.position + qP * (qY * (back * dist)) + up * CAM_HEIGHT;

    pos.y = glm::max(pos.y, groundY);
  }
}

glm::mat4 CameraState::view(const Plane &target) const {
  float orbitMag = glm::abs(orbitYaw) + glm::abs(orbitPitch);
  float orbitBlend = glm::clamp(orbitMag / glm::radians(45.f), 0.f, 1.f);
  glm::vec3 lookAhead =
      target.position + target.nose() * 20.f + glm::vec3(0, 10, 0);
  glm::vec3 lookAtPlane = target.position + glm::vec3(0, 5.f, 0);
  glm::vec3 lookTarget = glm::mix(lookAhead, lookAtPlane, orbitBlend);
  return glm::lookAt(pos, lookTarget, target.up());
}
