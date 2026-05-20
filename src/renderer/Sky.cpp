#include "Sky.h"
#include <glm/gtc/matrix_transform.hpp>

static const float verts[] = {
    -1, 1,  -1, -1, -1, -1, 1,  -1, -1, 1,  -1, -1, 1,  1,  -1, -1, 1,  -1,
    -1, -1, 1,  -1, -1, -1, -1, 1,  -1, -1, 1,  -1, -1, 1,  1,  -1, -1, 1,
    1,  -1, -1, 1,  -1, 1,  1,  1,  1,  1,  1,  1,  1,  1,  -1, 1,  -1, -1,
    -1, -1, 1,  -1, 1,  1,  1,  1,  1,  1,  1,  1,  1,  -1, 1,  -1, -1, 1,
    -1, 1,  -1, 1,  1,  -1, 1,  1,  1,  1,  1,  1,  -1, 1,  1,  -1, 1,  -1,
    -1, -1, -1, -1, -1, 1,  1,  -1, 1,  1,  -1, 1,  1,  -1, -1, -1, -1, -1,
};

Sky::Sky() : shader("assets/shaders/sky.vert", "assets/shaders/sky.frag") {
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  glBindVertexArray(0);
}

Sky::~Sky() {
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
}

void Sky::draw(const glm::mat4 &view, const glm::mat4 &proj,
               const glm::vec3 &sunDir, float time) {
  glDepthFunc(GL_LEQUAL);
  shader.use();

  glm::mat4 v = glm::mat4(glm::mat3(view));
  shader.setMat4("uVP", proj * v);
  shader.setVec3("uSunDir", sunDir);
  shader.setFloat("uTime", time);

  glBindVertexArray(vao);
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glBindVertexArray(0);
  glDepthFunc(GL_LESS);
}
