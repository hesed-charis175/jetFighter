#include "Shader.h"
#include <fstream>
#include <iostream>
#include <sstream>

static std::string readFile(const char *path) {
  std::ifstream f(path);
  if (!f) {
    std::cerr << "Cannot open shader: " << path << "\n";
    return "";
  }
  std::stringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

static GLuint compile(GLenum type, const std::string &src) {
  GLuint s = glCreateShader(type);
  const char *c = src.c_str();
  glShaderSource(s, 1, &c, nullptr);
  glCompileShader(s);
  GLint ok;
  glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[512];
    glGetShaderInfoLog(s, 512, nullptr, log);
    std::cerr << "Shader error:\n" << log << "\n";
  }
  return s;
}

Shader::Shader(const char *vertPath, const char *fragPath) {
  auto vsrc = readFile(vertPath);
  auto fsrc = readFile(fragPath);
  GLuint vs = compile(GL_VERTEX_SHADER, vsrc);
  GLuint fs = compile(GL_FRAGMENT_SHADER, fsrc);
  id = glCreateProgram();
  glAttachShader(id, vs);
  glAttachShader(id, fs);
  glLinkProgram(id);
  glDeleteShader(vs);
  glDeleteShader(fs);
}

void Shader::use() const { glUseProgram(id); }
void Shader::setMat4(const std::string &n, const glm::mat4 &m) const {
  glUniformMatrix4fv(glGetUniformLocation(id, n.c_str()), 1, GL_FALSE,
                     glm::value_ptr(m));
}
void Shader::setVec3(const std::string &n, const glm::vec3 &v) const {
  glUniform3fv(glGetUniformLocation(id, n.c_str()), 1, glm::value_ptr(v));
}
void Shader::setFloat(const std::string &n, float v) const {
  glUniform1f(glGetUniformLocation(id, n.c_str()), v);
}
