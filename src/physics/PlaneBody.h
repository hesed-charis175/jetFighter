#pragma once

#include "PhysicsWorld.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

struct Plane;

class PlaneBody {
public:
  RigidBody body;

  explicit PlaneBody(glm::vec3 pos, float headingDeg) {
    body.position = pos;
    body.orientation = glm::quat(glm::vec3(0.f, glm::radians(headingDeg), 0.f));

    glm::vec3 nose = body.orientation * glm::vec3(0, 0, -1);
    body.linearVel = glm::vec3(0.f);

    body.mass = 9000.f;
    body.halfExtents = {5.45f, 1.1f, 7.65f};
    body.updateInertiaTensor();
  }

  void pushAeroForces(float speed, float thrustFrac, float yawRate,
                      float pitchRate, float dt) {
    glm::vec3 noseDir = glm::normalize(body.orientation * glm::vec3(0, 0, -1));
    glm::vec3 localUp = glm::normalize(body.orientation * glm::vec3(0, 1, 0));
    glm::vec3 localRight =
        glm::normalize(body.orientation * glm::vec3(1, 0, 0));
    body.persistentForce = {0, 0, 0};
    body.persistentTorque = {0, 0, 0};
    glm::vec3 fwdVel = noseDir * glm::dot(body.linearVel, noseDir);
    glm::vec3 sideVel = body.linearVel - fwdVel;

    float lateralDrag = glm::clamp(speed / 60.f, 1.0f, 6.0f);
    float verticalDrag = glm::clamp(speed / 120.f, 0.5f, 3.0f);

    glm::vec3 sideSlip = localRight * glm::dot(sideVel, localRight);
    glm::vec3 verticalSlip = localUp * glm::dot(sideVel, localUp);

    body.persistentForce -= sideSlip * body.mass * lateralDrag;
    body.persistentForce -= verticalSlip * body.mass * verticalDrag;
    constexpr float MAX_THRUST = 350'000.f;
    float thrust = thrustFrac * MAX_THRUST;
    body.persistentForce += noseDir * thrust;

    constexpr float RHO = 1.225f;
    constexpr float S_WING = 45.7f;
    constexpr float CL = 0.30f;
    float lift = 0.5f * RHO * speed * speed * S_WING * CL;
    body.persistentForce += localUp * lift;

    constexpr float CD = 0.03f;
    float drag = 0.5f * RHO * speed * speed * S_WING * CD;
    glm::vec3 velDir = glm::length(body.linearVel) > 0.1f
                           ? glm::normalize(body.linearVel)
                           : noseDir;
    body.persistentForce -= velDir * drag;

    constexpr float YAW_DAMP = 10'000.f;
    constexpr float PITCH_DAMP = 15'000.f;
    constexpr float CONTROL_AUTHORITY = 180'000.f;
    float wYaw = glm::dot(body.angularVel, localUp);
    float wPitch = glm::dot(body.angularVel, localRight);
    float speedFactor = glm::clamp(speed / 80.f, 0.5f, 4.0f);
    body.persistentTorque -= localUp * (wYaw * YAW_DAMP * speedFactor);
    body.persistentTorque -= localRight * (wPitch * PITCH_DAMP * speedFactor);
    body.persistentTorque += localUp * (yawRate * 500000.f * speedFactor);
    body.persistentTorque += localRight * (pitchRate * 600000.f * speedFactor);
    glm::vec3 lateralErr = glm::cross(noseDir, velDir);
    body.persistentTorque -= lateralErr * CONTROL_AUTHORITY * speedFactor;

    constexpr float MAX_ANG_VEL = 1.2f;
    float angSpeed = glm::length(body.angularVel);
    if (angSpeed > MAX_ANG_VEL)
      body.angularVel = (body.angularVel / angSpeed) * MAX_ANG_VEL;
  }
  float pullState(glm::vec3 &outPosition, glm::quat &outOrientation) const {
    outPosition = body.position;
    outOrientation = body.orientation;
    return glm::length(body.linearVel);
  }
  float speedKMH() const { return body.speedKMH(); }
  float speedMS() const { return body.speedMS(); }
  glm::vec3 vel() const { return body.linearVel; }
};
