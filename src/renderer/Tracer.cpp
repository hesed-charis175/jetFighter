#include "Tracer.h"
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

Tracer::Tracer()
    : shader("assets/shaders/tracer.vert", "assets/shaders/tracer.frag") {
  rounds.resize(MAX_ROUNDS);

  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, MAX_ROUNDS * 7 * sizeof(float), nullptr,
               GL_STREAM_DRAW);

  constexpr GLsizei stride = 7 * sizeof(float);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
  glVertexAttribDivisor(0, 1);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                        (void *)(3 * sizeof(float)));
  glVertexAttribDivisor(1, 1);

  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride,
                        (void *)(6 * sizeof(float)));
  glVertexAttribDivisor(2, 1);

  glBindVertexArray(0);
}

Tracer::~Tracer() {
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
}

void Tracer::fire(const glm::vec3 &planePos, const glm::mat4 &modelMat,
                  const glm::quat &ori, float planeSpeed) {
  float roundSpeed =
      glm::clamp(planeSpeed * 2.f, SPEED_MAX * 2.f, SPEED_MAX * 4.f);

  glm::vec3 noseDir = glm::normalize(ori * glm::vec3(0, 0, -1));
  glm::vec3 right = glm::normalize(ori * glm::vec3(1, 0, 0));
  glm::vec3 down = glm::normalize(ori * glm::vec3(0, -1, 0));

  glm::vec3 ports[2] = {
      planePos - noseDir * 10.5f - right * 2.9f + down * 1.5f,
      planePos - noseDir * 10.5f + right * 2.9f + down * 1.5f,
  };

  for (auto &port : ports) {
    for (auto &r : rounds) {
      if (!r.active) {
        r.pos = port;
        r.dir = noseDir;
        r.speed = roundSpeed;
        r.life = 0.f;
        r.active = true;
        break;
      }
    }
  }
}
void Tracer::update(float dt) {
  fireTimer += dt;
  for (auto &r : rounds) {
    if (!r.active)
      continue;
    r.pos += r.dir * r.speed * dt;
    r.life += dt / LIFETIME;
    if (r.life >= 1.f)
      r.active = false;
  }
}

void Tracer::draw(const glm::mat4 &vp, const glm::vec3 &camPos) {
  std::vector<float> buf;
  buf.reserve(MAX_ROUNDS * 7);
  int count = 0;
  for (auto &r : rounds) {
    if (!r.active)
      continue;
    buf.push_back(r.pos.x);
    buf.push_back(r.pos.y);
    buf.push_back(r.pos.z);
    buf.push_back(r.dir.x);
    buf.push_back(r.dir.y);
    buf.push_back(r.dir.z);
    buf.push_back(r.life);
    ++count;
  }
  if (count == 0)
    return;

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0, buf.size() * sizeof(float), buf.data());

  shader.use();
  glUniformMatrix4fv(glGetUniformLocation(shader.id, "uVP"), 1, GL_FALSE,
                     glm::value_ptr(vp));
  glUniform3fv(glGetUniformLocation(shader.id, "uCamPos"), 1,
               glm::value_ptr(camPos));

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glDepthMask(GL_FALSE);
  glDisable(GL_CULL_FACE);

  glBindVertexArray(vao);
  glDrawArraysInstanced(GL_TRIANGLES, 0, 6, count);
  glBindVertexArray(0);

  glDepthMask(GL_TRUE);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_BLEND);
}
