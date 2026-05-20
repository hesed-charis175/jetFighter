#pragma once
#include "Shader.h"
#include <GL/glew.h>
#include <array>
#include <atomic>
#include <glm/glm.hpp>
#include <string>
#include <thread>
#include <vector>

struct Mesh {
  GLuint vao, vbo, ebo;
  int indexCount;
  bool isMerged = false;
  GLuint diffuseTex = 0;
  glm::vec4 baseColor{0.7f, 0.72f, 0.75f, 1.f};
};

class Model {
public:
  Model(const std::string &path, Shader &shader, bool mergeMeshes = false);
  ~Model();
  void draw(const glm::mat4 &modelMatrix, const glm::mat4 &view,
            const glm::mat4 &proj, const glm::vec3 &sunDir,
            const glm::mat4 &lightSpaceMat, GLuint shadowMapTex);
  void drawDepth(const glm::mat4 &modelMatrix, Shader &depthShader);

  std::vector<std::pair<glm::vec3, glm::vec3>> meshAABBs;

  std::vector<std::array<glm::vec3, 3>> triangles;

private:
  void processNode(const struct aiNode *node, const struct aiScene *scene);
  Mesh processMesh(const struct aiMesh *mesh, const struct aiScene *scene);
  GLuint loadEmbeddedTexture(const struct aiTexture *tex);

  void mergeMeshesIntoOne(const aiScene *scene);
  std::vector<Mesh> meshes;
  std::vector<GLuint> textures;
  Shader &shader;
};
