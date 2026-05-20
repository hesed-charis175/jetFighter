#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

class Shader {
public:
  GLuint id;
  Shader(const char *vertPath, const char *fragPath);
  void use() const;
  void setMat4(const std::string &name, const glm::mat4 &m) const;
  void setVec3(const std::string &name, const glm::vec3 &v) const;
  void setFloat(const std::string &name, float v) const;
};
