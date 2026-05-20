#include "Ocean.h"
#include "FFT.hpp"
#include <cmath>
#include <cstdlib>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

static double gaussian() {
  double v1, v2, s;
  do {
    v1 = (rand() % 201 - 100) / 100.0;
    v2 = (rand() % 201 - 100) / 100.0;
    s = v1 * v1 + v2 * v2;
  } while (s >= 1.0 || s == 0.0);
  return v1 * sqrt(-log(s) / s);
}

static double phillips(int nx, int ny, int N, float lx, float ly,
                       float windSpeed, float amplitude) {
  const double g = 9.81;
  double kx = (2.0 * M_PI * nx) / lx;
  double ky = (2.0 * M_PI * ny) / ly;
  double k2 = kx * kx + ky * ky;
  if (k2 == 0.0)
    return 0.0;
  double L2 = pow(windSpeed * windSpeed / g, 2.0);
  double minW2 = 0.0001;
  double result = amplitude * exp(-1.0 / (k2 * L2)) * exp(-k2 * minW2) *
                  pow(kx * kx / k2, 2) // wind alignment
                  / (k2 * k2);
  return result;
}

Ocean::Ocean(int p_N, float p_lx, float p_ly, float windSpeed, float amplitude)
    : N(p_N), N1(p_N + 1), lx(p_lx), ly(p_ly),
      shader("assets/shaders/ocean.vert", "assets/shaders/ocean.frag") {
  srand(42);

  auto make2d = [&](int a, int b) {
    return std::vector<std::vector<double>>(a, std::vector<double>(b, 0.0));
  };
  h0R = make2d(N1, N1);
  h0I = make2d(N1, N1);
  HR = make2d(N1, N1);
  HI = make2d(N1, N1);
  hr = make2d(N, N1);
  hi = make2d(N, N1);

  buildSpectrum(windSpeed, amplitude);

  fftRow.reserve(N);
  fftCol.reserve(N);
  for (int i = 0; i < N; i++)
    fftRow.push_back(new FFT(N, &HR[i], &HI[i]));
  for (int i = 0; i < N; i++)
    fftCol.push_back(new FFT(N, &hr[i], &hi[i]));

  heightData.resize(N * N, 0.f);
  glGenTextures(1, &heightTex);
  glBindTexture(GL_TEXTURE_2D, heightTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, N, N, 0, GL_RED, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  buildMesh();
}

Ocean::~Ocean() {
  for (auto f : fftRow)
    delete f;
  for (auto f : fftCol)
    delete f;
  glDeleteTextures(1, &heightTex);
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
  glDeleteBuffers(1, &ebo);
}

void Ocean::buildSpectrum(float windSpeed, float amplitude) {
  for (int x = 0; x <= N; x++) {
    int nx = x - N / 2;
    for (int y = 0; y <= N; y++) {
      int ny = y - N / 2;
      double p = sqrt(phillips(nx, ny, N, lx, ly, windSpeed, amplitude) / 2.0);
      h0R[x][y] = p * gaussian();
      h0I[x][y] = p * gaussian();
    }
  }
}

void Ocean::computeFFT(float t) {
  for (int x = 0; x < N; x++) {
    int nx = x - N / 2;
    for (int y = 0; y <= N; y++) {
      int ny = y - N / 2;
      double kx = (2.0 * M_PI * nx) / lx;
      double ky = (2.0 * M_PI * ny) / ly;
      double k = sqrt(kx * kx + ky * ky);
      double w = sqrt(9.81 * k + 0.0001);
      double A = w * t;
      double cA = cos(A), sA = sin(A);
      double r0 = h0R[x][y], i0 = h0I[x][y];
      double r0c = h0R[N - x][N - y], i0c = h0I[N - x][N - y];
      HR[x][y] = r0 * cA - i0 * sA + r0c * cA + i0c * sA;
      HI[x][y] = r0 * sA + i0 * cA - r0c * sA + i0c * cA;
    }
    fftRow[x]->reverse();
  }
  for (int y = 0; y < N; y++) {
    for (int x = 0; x < N; x++) {
      hr[y][x] = HR[x][y];
      hi[y][x] = HI[x][y];
    }
    fftCol[y]->reverse();
  }
}

void Ocean::uploadHeightmap() {
  const float scale = 0.012f;
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float h = (float)(pow(-1, x + y) * hr[y][x]) * scale;
      heightData[y * N + x] = h;
    }
  glBindTexture(GL_TEXTURE_2D, heightTex);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, N, N, GL_RED, GL_FLOAT,
                  heightData.data());
}

void Ocean::update(float t) {
  computeFFT(t);
  uploadHeightmap();
}

void Ocean::buildMesh() {
  int M = 128;
  float step = lx / M;

  std::vector<float> verts;
  std::vector<unsigned> idx;

  for (int z = 0; z <= M; z++)
    for (int x = 0; x <= M; x++) {
      float px = x * step;
      float pz = z * step;
      verts.push_back(px);
      verts.push_back(0.f);
      verts.push_back(pz);
      verts.push_back(px / lx);
      verts.push_back(pz / ly);
    }

  for (int z = 0; z < M; z++)
    for (int x = 0; x < M; x++) {
      unsigned tl = z * (M + 1) + x;
      idx.push_back(tl);
      idx.push_back(tl + 1);
      idx.push_back(tl + M + 1);
      idx.push_back(tl + 1);
      idx.push_back(tl + M + 2);
      idx.push_back(tl + M + 1);
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
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glBindVertexArray(0);
}

void Ocean::draw(const glm::mat4 &view, const glm::mat4 &proj,
                 const glm::vec3 &sunDir, float sunElev, float time) {
  shader.use();
  shader.setVec3("uSunDir", sunDir);
  shader.setFloat("uSunElev", sunElev);
  shader.setFloat("uTime", time);
  shader.setFloat("uLx", lx);
  shader.setFloat("uLy", ly);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, heightTex);
  glUniform1i(glGetUniformLocation(shader.id, "uHeightmap"), 0);

  glBindVertexArray(vao);

  int half = TILES / 2;
  for (int tz = -half; tz <= half; tz++)
    for (int tx = -half; tx <= half; tx++) {
      glm::mat4 model =
          glm::translate(glm::mat4(1.f), glm::vec3(tx * lx - lx * 0.5f, 0.f,
                                                   tz * ly - ly * 0.5f));
      shader.setMat4("uMVP", proj * view * model);
      shader.setMat4("uModel", model);
      glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    }

  glBindVertexArray(0);
}
