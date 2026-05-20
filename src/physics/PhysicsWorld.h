#pragma once

#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vector>

using TerrainHeightFn = std::function<float(float x, float z)>;
using TerrainNormalFn = std::function<glm::vec3(float x, float z)>;

class City;

struct Contact {
  glm::vec3 point;
  glm::vec3 normal;
  float penetration;
  float restitution;
  float friction;
};

struct RigidBody {
  float mass = 10000.f;
  glm::mat3 inertiaTensorInv;

  glm::vec3 position = {0, 1200, 0};
  glm::quat orientation = glm::quat(1, 0, 0, 0);
  glm::vec3 linearVel = {0, 0, -200};
  glm::vec3 angularVel = {0, 0, 0};

  glm::vec3 forceAccum = {0, 0, 0};
  glm::vec3 torqueAccum = {0, 0, 0};

  glm::vec3 halfExtents = {5.45f, 2.65f, 7.65f};
  glm::vec3 persistentForce = {0, 0, 0};
  glm::vec3 persistentTorque = {0, 0, 0};
  bool isActive = true;

  float speedMS() const { return glm::length(linearVel); }
  float speedKMH() const { return speedMS() * 3.6f; }

  void applyImpulse(glm::vec3 impulse, glm::vec3 worldPoint) {
    linearVel += impulse / mass;
    glm::vec3 r = worldPoint - position;
    angularVel += inertiaTensorInv * glm::cross(r, impulse);
  }

  void updateInertiaTensor() {
    float a = halfExtents.x * 2.f, b = halfExtents.y * 2.f,
          c = halfExtents.z * 2.f;
    float ix = (1.f / 12.f) * mass * (b * b + c * c);
    float iy = (1.f / 12.f) * mass * (a * a + c * c);
    float iz = (1.f / 12.f) * mass * (a * a + b * b);
    glm::mat3 localInvI =
        glm::mat3(1.f / ix, 0, 0, 0, 1.f / iy, 0, 0, 0, 1.f / iz);
    glm::mat3 R = glm::mat3_cast(orientation);
    inertiaTensorInv = R * localInvI * glm::transpose(R);
  }

  glm::vec3 axisX() const {
    return glm::normalize(orientation * glm::vec3(1, 0, 0));
  }
  glm::vec3 axisY() const {
    return glm::normalize(orientation * glm::vec3(0, 1, 0));
  }
  glm::vec3 axisZ() const {
    return glm::normalize(orientation * glm::vec3(0, 0, 1));
  }

  glm::vec3 support(glm::vec3 d) const {
    glm::vec3 p = position;
    p += axisX() * (glm::dot(d, axisX()) >= 0 ? halfExtents.x : -halfExtents.x);
    p += axisY() * (glm::dot(d, axisY()) >= 0 ? halfExtents.y : -halfExtents.y);
    p += axisZ() * (glm::dot(d, axisZ()) >= 0 ? halfExtents.z : -halfExtents.z);
    return p;
  }
};

struct StaticBox {
  glm::vec3 min, max;
  float restitution = 0.3f;
  float friction = 0.6f;
};

struct CollisionPlane {
  glm::vec3 normal;
  float offset;
  float restitution = 0.18f;
  float friction = 0.40f;
};

class PhysicsWorld {
public:
  static constexpr float PHYSICS_HZ = 200.f;
  static constexpr float FIXED_DT = 1.f / PHYSICS_HZ;
  static constexpr int MAX_SUBSTEPS = 10;
  static constexpr float GRAVITY = -11.81f;

  static constexpr float CITY_QUERY_RADIUS = 9.0f;

  TerrainHeightFn terrainHeight;
  TerrainNormalFn terrainNormal;

  City *city = nullptr;

  std::vector<CollisionPlane> staticPlanes;
  std::vector<RigidBody *> bodies;
  std::vector<StaticBox> staticBoxes;

  float accumulator = 0.f;

  void addBody(RigidBody *b) { bodies.push_back(b); }
  void addStaticPlane(CollisionPlane p) { staticPlanes.push_back(p); }
  void addStaticBox(StaticBox b) { staticBoxes.push_back(b); }

  void step(float dt);

private:
  void substep(float dt);

  void detectTerrainContacts(RigidBody &body, std::vector<Contact> &contacts);
  void detectPlaneContacts(RigidBody &body, const CollisionPlane &plane,
                           std::vector<Contact> &contacts);
  void detectBoxContacts(RigidBody &body, const StaticBox &box,
                         std::vector<Contact> &contacts);

  void detectCityContacts(RigidBody &body, std::vector<Contact> &contacts);

  bool detectBodyBodyContact(RigidBody &a, RigidBody &b, Contact &out);
  void resolveBodyBodyContact(RigidBody &a, RigidBody &b, const Contact &c);
  void resolveContacts(RigidBody &body, std::vector<Contact> &contacts,
                       float dt);
  void positionalCorrection(RigidBody &body, const Contact &c);
};
