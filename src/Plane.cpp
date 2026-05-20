#include "Plane.h"

Plane::Plane(glm::vec3 pos, float headingDeg, bool player)
    : isPlayerControlled(player), position(pos),
      orientation(glm::quat(glm::vec3(0.f, glm::radians(headingDeg), 0.f))),
      speed(200.f) {}

void Plane::updateAero(const PlaneInput &in, float dt) {
  if (in.thrust > 0.f)
    speed += THRUST_ACC * in.thrust * dt;
  else if (in.thrust < 0.f)
    speed -= (DRAG_DEC + BRAKE_DEC) * (-in.thrust) * dt;
  else
    speed -= DRAG_DEC * dt;
  speed = glm::clamp(speed, SPEED_MIN, SPEED_MAX);
  thrustFraction = glm::clamp(in.thrust, 0.f, 1.f);

  if (glm::abs(in.yaw) > 0.01f)
    yawRate += YAW_ACC * in.yaw * dt;
  else
    yawRate -= yawRate * ANG_DAMPING * dt;
  yawRate = glm::clamp(yawRate, -YAW_MAX, YAW_MAX);

  if (glm::abs(in.pitch) > 0.01f)
    pitchRate += PITCH_ACC * in.pitch * dt;
  else
    pitchRate -= pitchRate * ANG_DAMPING * dt;
  pitchRate = glm::clamp(pitchRate, -PITCH_MAX, PITCH_MAX);

  float targetBank = (yawRate / YAW_MAX) * BANK_MAX;
  bankAngle += (targetBank - bankAngle) * BANK_RATE * dt;
}

void Plane::rebuildModelMat() {
  glm::quat bankRot =
      glm::angleAxis(bankAngle, orientation * glm::vec3(0, 0, 1));
  visualOrientation = glm::normalize(bankRot * orientation);
  glm::quat fixRot = glm::angleAxis(glm::radians(-90.f), glm::vec3(1, 0, 0));
  modelMat = glm::translate(glm::mat4(1.f), position) *
             glm::toMat4(visualOrientation) * glm::toMat4(fixRot) *
             glm::scale(glm::mat4(1.f), glm::vec3(PLANE_SCALE));
}
