#include "Ground.h"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

Ground::Ground()
    : shader("assets/shaders/ground.vert", "assets/shaders/ground.frag") {
  const int N = 100;
  const float size = 2000.f;
  const float step = size / N;

  std::vector<float> verts;
  std::vector<unsigned> idx;

  for (int z = 0; z <= N; z++)
    for (int x = 0; x <= N; x++) {
      verts.push_back((x - N / 2) * step);
      verts.push_back(0.f);
      verts.push_back((z - N / 2) * step);
    }

  for (int z = 0; z < N; z++)
    for (int x = 0; x < N; x++) {
      unsigned tl = z * (N + 1) + x;
      idx.push_back(tl);
      idx.push_back(tl + 1);
      idx.push_back(tl + N + 1);
      idx.push_back(tl + 1);
      idx.push_back(tl + N + 2);
      idx.push_back(tl + N + 1);
    }

  indexCount = (int)idx.size();

  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(),
               GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned),
               idx.data(), GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  glBindVertexArray(0);
}

Ground::~Ground() {
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
  glDeleteBuffers(1, &ebo);
}

void Ground::draw(const glm::mat4 &view, const glm::mat4 &proj) {
  shader.use();
  shader.setMat4("uMVP", proj * view);
  glBindVertexArray(vao);
  glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
  glBindVertexArray(0);
}
