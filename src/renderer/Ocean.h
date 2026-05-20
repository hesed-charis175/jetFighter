#pragma once
#include "Shader.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

class Ocean {
public:
  Ocean(int N = 128, float lx = 1000.f, float ly = 1000.f,
        float windSpeed = 26.f, float amplitude = 3e-5f);
  ~Ocean();

  void update(float timeSeconds);
  void draw(const glm::mat4 &view, const glm::mat4 &proj,
            const glm::vec3 &sunDir, float sunElev, float time);

private:
  void buildMesh();
  void buildSpectrum(float windSpeed, float amplitude);
  void computeFFT(float t);
  void uploadHeightmap();

  int N, N1;
  float lx, ly;

  std::vector<std::vector<double>> h0R, h0I;
  std::vector<std::vector<double>> HR, HI, hr, hi;
  std::vector<class FFT *> fftRow, fftCol;
  GLuint heightTex;
  std::vector<float> heightData;

  GLuint vao, vbo, ebo;
  int indexCount;

  static constexpr int TILES = 7;

  Shader shader;
};
