#pragma once
#include "../physics/PhysicsWorld.h"
#include "Model.h"
#include "Shader.h"
#include <GL/glew.h>
#include <array>
#include <glm/glm.hpp>
#include <vector>

struct BVHNode {
  glm::vec3 aabbMin, aabbMax;
  int left = -1;
  int right = -1;
  int triStart = 0;
  int triCount = 0;
};

struct CityTri {
  glm::vec3 v[3];
  glm::vec3 normal;
  float planeD;
};

class City {
public:
  City(const std::string &path, Shader &shader, PhysicsWorld &physics,
       glm::vec3 worldPos, float scale = 1.0f);

  void draw(const glm::mat4 &view, const glm::mat4 &proj,
            const glm::vec3 &sunDir, const glm::mat4 &lightSpaceMat,
            GLuint shadowTex);
  void drawDepth(Shader &depthShader);
  glm::mat4 modelMat() const { return mModelMat; }

  float heightAt(float wx, float wz) const;

  void queryContacts(const glm::vec3 &pos, float radius,
                     std::vector<Contact> &contacts, float restitution = 0.25f,
                     float friction = 0.60f) const;

private:
  void buildBVH();
  int buildNode(std::vector<int> &triIdx, int begin, int end);
  void computeAABB(const std::vector<int> &idx, int begin, int end,
                   glm::vec3 &outMin, glm::vec3 &outMax) const;

  void queryNode(int nodeIdx, const glm::vec3 &pos, float radius,
                 std::vector<Contact> &contacts, float restitution,
                 float friction) const;

  bool sphereTriContact(const glm::vec3 &center, float radius,
                        const CityTri &tri, Contact &out, float restitution,
                        float friction) const;

  static constexpr int LEAF_TRI_MAX = 8;

  Model mModel;
  Shader &mShader;
  glm::mat4 mModelMat;
  glm::vec3 mWorldPos;
  float mScale;

  std::vector<CityTri> mTris;
  std::vector<BVHNode> mBVH;
  int mBVHRoot = -1;

  static constexpr int HGX = 128;
  static constexpr int HGZ = 128;
  std::vector<float> mHeightGrid;
  glm::vec3 mWMin{0.f}, mWMax{0.f};

  static constexpr float MODEL_MIN_X = -4.7f;
  static constexpr float MODEL_MAX_X = 1460.7f;
  static constexpr float MODEL_MIN_Z = -1830.7f;
  static constexpr float MODEL_MAX_Z = 559.2f;
  static constexpr float MODEL_MIN_Y = -13.8f;
};
