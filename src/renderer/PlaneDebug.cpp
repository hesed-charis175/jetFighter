#include "PlaneDebug.h"
#include <glm/gtc/type_ptr.hpp>
#include <vector>

const glm::vec3 PlaneDebug::RED_PTS[10] = {
    {0.0f, 0.0f, -14.65f}, {3.0f, -0.7f, -1.f}, {-3.f, -0.7f, -1.f},
    {3.0f, 0.7f, -1.f},    {-3.f, 0.7f, -1.f},  {-2.0f, 1.f, 10.f},
    {-2.0f, -0.3f, 10.f},  {2.0f, 1.f, 10.0f},  {2.0f, -0.3f, 10.0f},
    {0.0f, 6.5f, 10.f},
};
const glm::vec3 PlaneDebug::GREEN_PTS[16] = {
    {9.45f, -0.8f, 4.5f},  {-9.45f, -0.8f, 4.5f}, {9.45f, -0.8f, 6.5f},
    {-9.45f, -0.8f, 6.5f}, {2.5f, -0.8f, 7.f},    {-2.5f, -0.8f, 7.f},
    {3.0f, -0.8f, -1.5f},  {-3.0f, -0.8f, -1.5f}, {9.45f, -0.5f, 4.5f},
    {-9.45f, -0.5f, 4.5f}, {-2.5f, -0.8f, 7.f},   {2.5f, -0.8f, 7.f},
    {3.f, -0.5f, -1.5f},   {-3.f, -0.5f, -1.5f},  {9.45f, -0.8f, 6.5f},
    {-9.45f, -0.8f, 6.5f},
};
static GLuint compileShader(GLenum type, const char *src) {
  GLuint s = glCreateShader(type);
  glShaderSource(s, 1, &src, nullptr);
  glCompileShader(s);
  return s;
}

PlaneDebug::PlaneDebug() {
  const char *vsrc = R"(
        #version 330 core
        layout(location=0) in vec3 aPos;
        layout(location=1) in vec3 aColor;
        uniform mat4 uVP;
        out vec3 vColor;
        void main() {
            vColor = aColor;
            gl_Position = uVP * vec4(aPos, 1.0);
            gl_PointSize = 8.0;
        }
    )";
  const char *fsrc = R"(
        #version 330 core
        in vec3 vColor;
        out vec4 FragColor;
        void main() { FragColor = vec4(vColor, 1.0); }
    )";

  GLuint vs = compileShader(GL_VERTEX_SHADER, vsrc);
  GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsrc);
  mProg = glCreateProgram();
  glAttachShader(mProg, vs);
  glAttachShader(mProg, fs);
  glLinkProgram(mProg);
  glDeleteShader(vs);
  glDeleteShader(fs);

  glGenVertexArrays(1, &mVAO);
  glGenBuffers(1, &mVBO);
  glBindVertexArray(mVAO);
  glBindBuffer(GL_ARRAY_BUFFER, mVBO);
  glBufferData(GL_ARRAY_BUFFER, 26 * 6 * sizeof(float), nullptr,
               GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glBindVertexArray(0);
}

PlaneDebug::~PlaneDebug() {
  glDeleteVertexArrays(1, &mVAO);
  glDeleteBuffers(1, &mVBO);
  glDeleteProgram(mProg);
}

void PlaneDebug::draw(const glm::vec3 &pos, const glm::quat &orientation,
                      const glm::mat4 &vp) {
  float data[26 * 6];
  int idx = 0;
  for (int i = 0; i < 10; ++i) {
    glm::vec3 world = pos + orientation * RED_PTS[i];
    data[idx++] = world.x;
    data[idx++] = world.y;
    data[idx++] = world.z;
    data[idx++] = 1;
    data[idx++] = 0;
    data[idx++] = 0;
  }
  for (int i = 0; i < 16; ++i) {
    glm::vec3 world = pos + orientation * GREEN_PTS[i];
    data[idx++] = world.x;
    data[idx++] = world.y;
    data[idx++] = world.z;
    data[idx++] = 0;
    data[idx++] = 1;
    data[idx++] = 0;
  }
  glBindBuffer(GL_ARRAY_BUFFER, mVBO);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(data), data);

  glUseProgram(mProg);
  glUniformMatrix4fv(glGetUniformLocation(mProg, "uVP"), 1, GL_FALSE,
                     glm::value_ptr(vp));
  glEnable(GL_PROGRAM_POINT_SIZE);
  glDisable(GL_DEPTH_TEST);
  glBindVertexArray(mVAO);
  glDrawArrays(GL_POINTS, 0, 26);
  glBindVertexArray(0);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_PROGRAM_POINT_SIZE);
}
