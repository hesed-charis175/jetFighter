#include "Exhaust.h"
#include <glm/gtc/type_ptr.hpp>

Exhaust::Exhaust(const std::vector<glm::vec3> &nozzleOffsets)
    : nozzles(nozzleOffsets),
      shader("assets/shaders/exhaust.vert", "assets/shaders/exhaust.frag") {
  rays.resize(nozzles.size());

  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, nozzles.size() * 9 * sizeof(float), nullptr,
               GL_STREAM_DRAW);

  constexpr GLsizei stride = 9 * sizeof(float);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
  glVertexAttribDivisor(0, 1);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, stride,
                        (void *)(3 * sizeof(float)));
  glVertexAttribDivisor(1, 1);

  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride,
                        (void *)(4 * sizeof(float)));
  glVertexAttribDivisor(2, 1);

  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride,
                        (void *)(7 * sizeof(float)));
  glVertexAttribDivisor(3, 1);

  glEnableVertexAttribArray(4);
  glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride,
                        (void *)(8 * sizeof(float)));
  glVertexAttribDivisor(4, 1);

  glBindVertexArray(0);
}

Exhaust::~Exhaust() {
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
}

void Exhaust::update(const glm::mat4 &modelMat, float thrustFraction,
                     float dt) {
  (void)dt;

  lastPlaneRight =
      glm::normalize(glm::vec3(modelMat * glm::vec4(1.f, 0.f, 0.f, 0.f)));

  glm::vec3 dir =
      glm::normalize(glm::vec3(modelMat * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f)));

  for (int i = 0; i < (int)nozzles.size(); i++) {
    glm::vec3 worldNozzle = glm::vec3(modelMat * glm::vec4(nozzles[i], 1.f));
    glm::vec3 localOffset = glm::vec3(0.f, -6.0f, -4.0f);
    worldNozzle += glm::vec3(modelMat * glm::vec4(localOffset, 0.f));
    rays[i].origin = worldNozzle;
    rays[i].dir = dir;
    rays[i].size = EX_SIZE;
    rays[i].length = EX_LENGTH * (0.6f + thrustFraction * 0.4f);
    rays[i].thrust = thrustFraction;
  }
}

void Exhaust::draw(const glm::mat4 &view, const glm::mat4 &proj,
                   const glm::vec3 &camPos) {
  if (rays.empty())
    return;

  std::vector<float> buf;
  buf.reserve(rays.size() * 9);
  for (auto &r : rays) {
    buf.push_back(r.origin.x);
    buf.push_back(r.origin.y);
    buf.push_back(r.origin.z);
    buf.push_back(r.size);
    buf.push_back(r.dir.x);
    buf.push_back(r.dir.y);
    buf.push_back(r.dir.z);
    buf.push_back(r.length);
    buf.push_back(r.thrust);
  }

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0, buf.size() * sizeof(float), buf.data());

  shader.use();
  shader.setMat4("uVP", proj * view);
  shader.setVec3("uCamPos", camPos);
  shader.setVec3("uPlaneRight", lastPlaneRight);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glDepthMask(GL_FALSE);

  glBindVertexArray(vao);
  glDrawArraysInstanced(GL_TRIANGLES, 0, 18, (GLsizei)rays.size());
  glBindVertexArray(0);

  glDepthMask(GL_TRUE);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_BLEND);
}
