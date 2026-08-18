#include "Terrain.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

Terrain::Terrain()
    : mtnShader("assets/shaders/terrain.vert", "assets/shaders/terrain.frag"),
      oceanShader("assets/shaders/ocean.vert", "assets/shaders/ocean.frag"),
      shadowShader("assets/shaders/shadowmap.vert",
                   "assets/shaders/shadowmap.frag") {
  buildMesh(256, 12000.f);
  buildHeightmap(12000.f); // bake once — was previously recomputed every
                           // vertex, every frame
  initShadowMap();
}
void Terrain::buildMesh(int N, float size) {
  float step = size / N;
  std::vector<float> verts;
  std::vector<unsigned> idx;

  verts.reserve((N + 1) * (N + 1) * 3);
  for (int z = 0; z <= N; z++)
    for (int x = 0; x <= N; x++) {
      verts.push_back((x - N / 2) * step);
      verts.push_back(0.f);
      verts.push_back((z - N / 2) * step);
    }

  idx.reserve(N * N * 6);
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
float Terrain::sampleHeightCache(float x, float z) const {
  float u = (x / worldSize + 0.5f) * (HEIGHT_RES - 1);
  float v = (z / worldSize + 0.5f) * (HEIGHT_RES - 1);
  u = glm::clamp(u, 0.f, (float)(HEIGHT_RES - 1));
  v = glm::clamp(v, 0.f, (float)(HEIGHT_RES - 1));

  int x0 = (int)u, z0 = (int)v;
  int x1 = glm::min(x0 + 1, HEIGHT_RES - 1);
  int z1 = glm::min(z0 + 1, HEIGHT_RES - 1);
  float fx = u - x0, fz = v - z0;

  float h00 = heightCache[z0 * HEIGHT_RES + x0];
  float h10 = heightCache[z0 * HEIGHT_RES + x1];
  float h01 = heightCache[z1 * HEIGHT_RES + x0];
  float h11 = heightCache[z1 * HEIGHT_RES + x1];

  float h0 = glm::mix(h00, h10, fx);
  float h1 = glm::mix(h01, h11, fx);
  return glm::mix(h0, h1, fz);
}
Terrain::~Terrain() {
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
  glDeleteBuffers(1, &ebo);
}

void Terrain::toggleMode() {
  mode = (mode == TerrainMode::Mountains) ? TerrainMode::Ocean
                                          : TerrainMode::Mountains;
  std::cout << (mode == TerrainMode::Mountains ? "Mountains\n" : "Ocean\n");
}

static float hash(glm::vec2 p) {
  p = glm::fract(p * glm::vec2(127.1f, 311.7f));
  p += glm::dot(p, p + 19.19f);
  return glm::fract(p.x * p.y);
}

static float noise(glm::vec2 p) {
  glm::vec2 i = glm::floor(p), f = glm::fract(p);
  f = f * f * (3.f - 2.f * f);
  return glm::mix(
      glm::mix(hash(i), hash(i + glm::vec2(1, 0)), f.x),
      glm::mix(hash(i + glm::vec2(0, 1)), hash(i + glm::vec2(1, 1)), f.x), f.y);
}

static float ridged(glm::vec2 p) {
  return 1.f - glm::abs(noise(p) * 2.f - 1.f);
}

static float fbm(glm::vec2 p) {
  float v = 0.f, a = 0.5f;
  for (int i = 0; i < 8; i++) {
    v += a * noise(p);
    p *= 2.03f;
    a *= 0.49f;
  }
  return v;
}

static float fbmRidged(glm::vec2 p) {
  float v = 0.f, a = 0.5f;
  for (int i = 0; i < 7; i++) {
    v += a * ridged(p);
    p *= 2.07f;
    a *= 0.48f;
  }
  return v;
}

static float terrainHeight(glm::vec2 xz) {
  glm::vec2 p = xz * 0.00035f;
  float continent = fbm(p * 0.18f) * 0.6f + 0.4f;
  float ridge = fbmRidged(p * 0.9f) * continent;
  float hills = fbm(p * 1.8f + glm::vec2(4.2f, 1.7f)) * 0.3f;
  float detail = fbm(p * 6.0f + glm::vec2(2.1f, 3.3f)) * 0.07f;
  float h = glm::mix(hills, ridge, glm::smoothstep(0.2f, 0.7f, ridge));
  h = h * continent + detail;
  h = glm::pow(h, 1.35f);
  return glm::max(h * 1100.f, -5.f);
}

float Terrain::heightAt(float x, float z) const {
  return sampleHeightCache(x, z);
}

glm::vec3 Terrain::normalAt(float x, float z) const {
  const float e = 3.f;
  float hL = sampleHeightCache(x - e, z);
  float hR = sampleHeightCache(x + e, z);
  float hD = sampleHeightCache(x, z - e);
  float hU = sampleHeightCache(x, z + e);
  return glm::normalize(glm::vec3(hL - hR, 2.f * e, hD - hU));
}
void Terrain::draw(const glm::mat4 &view, const glm::mat4 &proj,
                   const glm::vec3 &sunDir, float sunElev, float time,
                   const glm::mat4 &lightSpaceMat, GLuint shadowTex,
                   float planeAlt) {
  glm::mat4 mvp = proj * view;
  glm::mat4 bias(0.5f);
  bias[3] = glm::vec4(0.5f, 0.5f, 0.5f, 1.f);
  glm::mat4 shadowMatrix = bias * lightSpaceMat;

  Shader &sh = (mode == TerrainMode::Mountains) ? mtnShader : oceanShader;
  sh.use();
  sh.setMat4("uMVP", mvp);
  sh.setVec3("uSunDir", sunDir);
  sh.setFloat("uSunElev", sunElev);
  sh.setFloat("uPlaneAlt", planeAlt);
  sh.setMat4("uShadowMatrix", shadowMatrix);
  if (mode == TerrainMode::Ocean)
    sh.setFloat("uTime", time);

  if (mode == TerrainMode::Mountains) {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, heightmapTex);
    glUniform1i(glGetUniformLocation(sh.id, "uTerrainHeightMap"), 1);
    sh.setFloat("uWorldSize", worldSize);
  }
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, shadowTex);
  glUniform1i(glGetUniformLocation(sh.id, "uShadowMap"), 0);

  glBindVertexArray(vao);
  glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
  glBindVertexArray(0);
}
void Terrain::buildHeightmap(float size) {
  worldSize = size;
  heightCache.resize(HEIGHT_RES * HEIGHT_RES);
  for (int z = 0; z < HEIGHT_RES; z++) {
    for (int x = 0; x < HEIGHT_RES; x++) {
      float wx = (x / float(HEIGHT_RES - 1) - 0.5f) * size;
      float wz = (z / float(HEIGHT_RES - 1) - 0.5f) * size;
      heightCache[z * HEIGHT_RES + x] = terrainHeight(glm::vec2(wx, wz));
    }
  }

  glGenTextures(1, &heightmapTex);
  glBindTexture(GL_TEXTURE_2D, heightmapTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, HEIGHT_RES, HEIGHT_RES, 0, GL_RED,
               GL_FLOAT, heightCache.data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);
}

void Terrain::initShadowMap() {
  glGenFramebuffers(1, &shadowFBO);
  glGenTextures(1, &shadowTex);
  glBindTexture(GL_TEXTURE_2D, shadowTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SHADOW_RES, SHADOW_RES,
               0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,
                  GL_COMPARE_REF_TO_TEXTURE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  float border[] = {1, 1, 1, 1};
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
  glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                         shadowTex, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
