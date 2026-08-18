#pragma once
#include "Shader.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

enum class TerrainMode { Mountains, Ocean };

class Terrain {
public:
  Terrain();
  ~Terrain();
  void draw(const glm::mat4 &view, const glm::mat4 &proj,
            const glm::vec3 &sunDir, float sunElev, float time,
            const glm::mat4 &lightSpaceMat, GLuint shadowTex, float planeAlt);
  void toggleMode();
  float heightAt(float x, float z) const;
  glm::vec3 normalAt(float x, float z) const;

private:
  void buildMesh(int N, float size);
  void initShadowMap();
  void buildHeightmap(float size);
  float sampleHeightCache(float x, float z) const;

  GLuint shadowFBO, shadowTex;
  Shader shadowShader;
  static constexpr int SHADOW_RES = 2048;
  Shader mtnShader;
  Shader oceanShader;
  GLuint vao, vbo, ebo;
  int indexCount;
  TerrainMode mode = TerrainMode::Mountains;

  static constexpr int HEIGHT_RES = 512;
  float worldSize = 12000.f;
  std::vector<float> heightCache;
  GLuint heightmapTex;
};
