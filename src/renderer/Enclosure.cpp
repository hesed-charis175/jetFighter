#include "Enclosure.h"

Enclosure::Enclosure(float width, float depth, float altMax, float ground)
    : halfW(width * .5f), halfD(depth * .5f), topY(altMax), botY(ground),
      shader("assets/shaders/enclosure.vert", "assets/shaders/enclosure.frag") {
  constexpr float CELL = 200.f;
  float uW = halfW * 2.f / CELL, uD = halfD * 2.f / CELL,
        uH = (topY - botY) / CELL;

  std::vector<float> buf;
  auto push = [&](glm::vec3 p, glm::vec2 uv) {
    buf.insert(buf.end(), {p.x, p.y, p.z, uv.x, uv.y});
  };
  auto quad = [&](glm::vec3 a, glm::vec2 ua, glm::vec3 b, glm::vec2 ub,
                  glm::vec3 c, glm::vec2 uc, glm::vec3 d, glm::vec2 ud) {
    push(a, ua);
    push(b, ub);
    push(c, uc);
    push(a, ua);
    push(c, uc);
    push(d, ud);
  };

  float x0 = -halfW, x1 = halfW, y0 = botY, y1 = topY, z0 = -halfD, z1 = halfD;

  quad({x0, y1, z0}, {0, 0}, {x1, y1, z0}, {uW, 0}, {x1, y1, z1}, {uW, uD},
       {x0, y1, z1}, {0, uD});
  quad({x0, y0, z0}, {0, 0}, {x0, y0, z1}, {uD, 0}, {x0, y1, z1}, {uD, uH},
       {x0, y1, z0}, {0, uH});
  quad({x1, y0, z1}, {0, 0}, {x1, y0, z0}, {uD, 0}, {x1, y1, z0}, {uD, uH},
       {x1, y1, z1}, {0, uH});
  quad({x0, y0, z0}, {0, 0}, {x1, y0, z0}, {uW, 0}, {x1, y1, z0}, {uW, uH},
       {x0, y1, z0}, {0, uH});
  quad({x1, y0, z1}, {0, 0}, {x0, y0, z1}, {uW, 0}, {x0, y1, z1}, {uW, uH},
       {x1, y1, z1}, {0, uH});

  vertCount = (int)(buf.size() / 5);
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, buf.size() * sizeof(float), buf.data(),
               GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glBindVertexArray(0);
}

void Enclosure::draw(glm::mat4 vp) {
  shader.use();

  shader.setMat4("uVP", vp);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDepthMask(GL_FALSE);
  glDisable(GL_CULL_FACE);
  glBindVertexArray(vao);
  glDrawArrays(GL_TRIANGLES, 0, vertCount);
  glBindVertexArray(0);
  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);
}
