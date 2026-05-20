#include "PhysicsWorld.h"
#include "../renderer/City.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

static const glm::vec3 PLANE_PROBES[26] = {
    {0.0f, 0.0f, -14.65f}, {3.0f, -0.7f, -1.f},   {-3.f, -0.7f, -1.f},
    {3.0f, 0.7f, -1.f},    {-3.f, 0.7f, -1.f},    {-2.0f, 1.f, 10.f},
    {-2.0f, -0.3f, 10.f},  {2.0f, 1.f, 10.0f},    {2.0f, -0.3f, 10.0f},
    {0.0f, 6.5f, 10.f},    {9.45f, -0.8f, 4.5f},  {-9.45f, -0.8f, 4.5f},
    {9.45f, -0.8f, 6.5f},  {-9.45f, -0.8f, 6.5f}, {2.5f, -0.8f, 7.f},
    {-2.5f, -0.8f, 7.f},   {3.0f, -0.8f, -1.5f},  {-3.0f, -0.8f, -1.5f},
    {9.45f, -0.5f, 4.5f},  {-9.45f, -0.5f, 4.5f}, {-2.5f, -0.8f, 7.f},
    {2.5f, -0.8f, 7.f},    {3.f, -0.5f, -1.5f},   {-3.f, -0.5f, -1.5f},
    {9.45f, -0.8f, 6.5f},  {-9.45f, -0.8f, 6.5f},
};
void PhysicsWorld::step(float renderDt) {
  accumulator += renderDt;
  int steps = 0;
  while (accumulator >= FIXED_DT && steps < MAX_SUBSTEPS) {
    substep(FIXED_DT);
    accumulator -= FIXED_DT;
    ++steps;
  }
  if (steps == MAX_SUBSTEPS)
    accumulator = 0.f;
}

void PhysicsWorld::substep(float dt) {
  for (RigidBody *body : bodies) {
    if (!body->isActive)
      continue;

    body->updateInertiaTensor();
    body->forceAccum += body->persistentForce;
    body->torqueAccum += body->persistentTorque;
    body->forceAccum += glm::vec3(0, GRAVITY * body->mass, 0);

    body->linearVel += (body->forceAccum / body->mass) * dt;
    body->angularVel += body->inertiaTensorInv * body->torqueAccum * dt;
    body->angularVel *= glm::pow(0.80f, dt * PHYSICS_HZ);

    body->forceAccum = {0, 0, 0};
    body->torqueAccum = {0, 0, 0};

    std::vector<Contact> contacts;

    if (terrainHeight && terrainNormal)
      detectTerrainContacts(*body, contacts);

    for (const auto &plane : staticPlanes)
      detectPlaneContacts(*body, plane, contacts);

    for (const auto &box : staticBoxes)
      detectBoxContacts(*body, box, contacts);

    detectCityContacts(*body, contacts);

    if (!contacts.empty())
      resolveContacts(*body, contacts, dt);

    body->position += body->linearVel * dt;

    glm::quat wQuat(0.f, body->angularVel.x, body->angularVel.y,
                    body->angularVel.z);
    body->orientation += 0.5f * wQuat * body->orientation * dt;
    body->orientation = glm::normalize(body->orientation);
  }

  for (int i = 0; i < (int)bodies.size(); ++i) {
    for (int j = i + 1; j < (int)bodies.size(); ++j) {
      if (!bodies[i]->isActive || !bodies[j]->isActive)
        continue;
      Contact c;
      if (detectBodyBodyContact(*bodies[i], *bodies[j], c))
        resolveBodyBodyContact(*bodies[i], *bodies[j], c);
    }
  }
}

void PhysicsWorld::detectCityContacts(RigidBody &body,
                                      std::vector<Contact> &contacts) {
  if (!city)
    return;
  constexpr float SPHERE_R = 1.5f;
  for (const auto &local : PLANE_PROBES) {
    glm::vec3 world = body.position + body.orientation * local;
    city->queryContacts(world, SPHERE_R, contacts, 0.25f, 0.60f);
  }
}

void PhysicsWorld::detectTerrainContacts(RigidBody &body,
                                         std::vector<Contact> &contacts) {
  for (const auto &local : PLANE_PROBES) {
    glm::vec3 world = body.position + body.orientation * local;
    float th = terrainHeight(world.x, world.z);
    float pen = th - world.y;
    if (pen <= 0.f)
      continue;
    Contact c;
    c.point = world;
    c.normal = terrainNormal(world.x, world.z);
    c.penetration = pen;
    c.restitution = 0.10f;
    c.friction = 0.90f;
    contacts.push_back(c);
  }
}

void PhysicsWorld::detectPlaneContacts(RigidBody &body,
                                       const CollisionPlane &plane,
                                       std::vector<Contact> &contacts) {
  for (const auto &local : PLANE_PROBES) {
    glm::vec3 world = body.position + body.orientation * local;
    float dist = glm::dot(plane.normal, world) - plane.offset;
    if (dist >= 0.f)
      continue;
    Contact c;
    c.point = world;
    c.normal = plane.normal;
    c.penetration = -dist;
    c.restitution = plane.restitution;
    c.friction = plane.friction;
    contacts.push_back(c);
  }
}

void PhysicsWorld::resolveContacts(RigidBody &body,
                                   std::vector<Contact> &contacts, float) {
  constexpr int ITERATIONS = 20;
  for (int iter = 0; iter < ITERATIONS; ++iter) {
    for (Contact &c : contacts) {
      glm::vec3 r = c.point - body.position;
      glm::vec3 vContact = body.linearVel + glm::cross(body.angularVel, r);
      float vRel = glm::dot(vContact, c.normal);
      if (vRel >= 0.f)
        continue;

      glm::vec3 rXn = glm::cross(r, c.normal);
      float angFactor =
          glm::dot(c.normal, glm::cross(body.inertiaTensorInv * rXn, r));
      float invMass = 1.f / body.mass;
      float denom = invMass + angFactor;
      if (denom <= 1e-9f)
        continue;

      float jn = -(1.f + c.restitution) * vRel / denom;
      if (jn < 0.f)
        jn = 0.f;
      body.applyImpulse(c.normal * jn, c.point);

      glm::vec3 vAfterN = body.linearVel + glm::cross(body.angularVel, r);
      glm::vec3 vTangent = vAfterN - glm::dot(vAfterN, c.normal) * c.normal;
      float vTanLen = glm::length(vTangent);
      if (vTanLen > 1e-4f) {
        glm::vec3 tDir = -vTangent / vTanLen;
        glm::vec3 rXt = glm::cross(r, tDir);
        float denomT =
            invMass +
            glm::dot(tDir, glm::cross(body.inertiaTensorInv * rXt, r));
        if (denomT > 1e-9f) {
          float jt = glm::min(vTanLen / denomT, c.friction * jn);
          body.applyImpulse(tDir * jt, c.point);
        }
      }
      positionalCorrection(body, c);
    }
  }
}

void PhysicsWorld::positionalCorrection(RigidBody &body, const Contact &c) {
  constexpr float BETA = 0.5f, SLOP = 0.005f;
  body.position += c.normal * glm::max(c.penetration - SLOP, 0.f) * BETA;
}

static float obbSat(const RigidBody &a, const RigidBody &b, glm::vec3 axis,
                    float &pen) {
  if (glm::dot(axis, axis) < 1e-9f)
    return 1.f;
  axis = glm::normalize(axis);
  auto project = [&](const RigidBody &body) {
    float r = 0.f;
    r += glm::abs(glm::dot(body.axisX() * body.halfExtents.x, axis));
    r += glm::abs(glm::dot(body.axisY() * body.halfExtents.y, axis));
    r += glm::abs(glm::dot(body.axisZ() * body.halfExtents.z, axis));
    return r;
  };
  float rA = project(a), rB = project(b);
  pen = (rA + rB) - glm::abs(glm::dot(b.position - a.position, axis));
  return pen;
}

bool PhysicsWorld::detectBodyBodyContact(RigidBody &a, RigidBody &b,
                                         Contact &out) {
  if (glm::dot(b.position - a.position, b.position - a.position) >=
      160.f * 160.f)
    return false;

  constexpr float POINT_RADIUS = 2.5f;
  float minDist = FLT_MAX;
  glm::vec3 bestNormal(0, 1, 0), bestPoint(0);
  float bestPen = 0.f;

  for (const auto &la : PLANE_PROBES) {
    glm::vec3 wa = a.position + a.orientation * la;
    for (const auto &lb : PLANE_PROBES) {
      glm::vec3 wb = b.position + b.orientation * lb;
      glm::vec3 d = wb - wa;
      float dist = glm::length(d);
      if (dist < POINT_RADIUS * 2.f && dist < minDist) {
        minDist = dist;
        bestPen = POINT_RADIUS * 2.f - dist;
        bestNormal = dist > 1e-4f ? d / dist : glm::vec3(0, 1, 0);
        bestPoint = wa + bestNormal * POINT_RADIUS;
      }
    }
  }

  if (minDist == FLT_MAX)
    return false;
  out.normal = bestNormal;
  out.penetration = bestPen;
  out.point = bestPoint;
  out.restitution = 1.8f;
  out.friction = 0.05f;
  return true;
}

void PhysicsWorld::resolveBodyBodyContact(RigidBody &a, RigidBody &b,
                                          const Contact &c) {
  glm::vec3 vA = a.linearVel + glm::cross(a.angularVel, c.point - a.position);
  glm::vec3 vB = b.linearVel + glm::cross(b.angularVel, c.point - b.position);
  float vRel = glm::dot(vA - vB, c.normal);
  if (vRel >= 0.f)
    return;

  float jn = -(1.f + c.restitution) * vRel / (1.f / a.mass + 1.f / b.mass);
  glm::vec3 imp = c.normal * jn;
  a.applyImpulse(-imp, c.point);
  b.applyImpulse(imp, c.point);

  float corr = glm::max(c.penetration - 0.05f, 0.f) * 0.5f;
  a.position -= c.normal * corr * 0.5f;
  b.position += c.normal * corr * 0.5f;
}

void PhysicsWorld::detectBoxContacts(RigidBody &body, const StaticBox &box,
                                     std::vector<Contact> &contacts) {
  glm::vec3 boxCenter = (box.min + box.max) * 0.5f;
  glm::vec3 boxHalf = (box.max - box.min) * 0.5f;
  if (glm::length(body.position - boxCenter) > glm::length(boxHalf) + 10.f)
    return;

  for (const auto &local : PLANE_PROBES) {
    glm::vec3 w = body.position + body.orientation * local;
    if (w.x < box.min.x || w.x > box.max.x)
      continue;
    if (w.y < box.min.y || w.y > box.max.y)
      continue;
    if (w.z < box.min.z || w.z > box.max.z)
      continue;

    float depths[6] = {
        w.x - box.min.x, box.max.x - w.x, w.y - box.min.y,
        box.max.y - w.y, w.z - box.min.z, box.max.z - w.z,
    };
    glm::vec3 normals[6] = {{-1, 0, 0}, {1, 0, 0},  {0, -1, 0},
                            {0, 1, 0},  {0, 0, -1}, {0, 0, 1}};
    int best = 0;
    for (int i = 1; i < 6; i++)
      if (depths[i] < depths[best])
        best = i;

    Contact c;
    c.point = w;
    c.normal = normals[best];
    c.penetration = depths[best];
    c.restitution = box.restitution;
    c.friction = box.friction;
    contacts.push_back(c);
  }
}
