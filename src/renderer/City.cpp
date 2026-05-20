#include "City.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <numeric>

static glm::vec3 closestPtTriangle(const glm::vec3 &p, const glm::vec3 &a,
                                   const glm::vec3 &b, const glm::vec3 &c) {
  glm::vec3 ab = b - a, ac = c - a, ap = p - a;
  float d1 = glm::dot(ab, ap), d2 = glm::dot(ac, ap);
  if (d1 <= 0.f && d2 <= 0.f)
    return a;
  glm::vec3 bp = p - b;
  float d3 = glm::dot(ab, bp), d4 = glm::dot(ac, bp);
  if (d3 >= 0.f && d4 <= d3)
    return b;
  float vc = d1 * d4 - d3 * d2;
  if (vc <= 0.f && d1 >= 0.f && d3 <= 0.f)
    return a + ab * (d1 / (d1 - d3));
  glm::vec3 cp = p - c;
  float d5 = glm::dot(ab, cp), d6 = glm::dot(ac, cp);
  if (d6 >= 0.f && d5 <= d6)
    return c;
  float vb = d5 * d2 - d1 * d6;
  if (vb <= 0.f && d2 >= 0.f && d6 <= 0.f)
    return a + ac * (d2 / (d2 - d6));
  float va = d3 * d6 - d5 * d4;
  if (va <= 0.f && (d4 - d3) >= 0.f && (d5 - d6) >= 0.f) {
    float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
    return b + (c - b) * w;
  }
  float denom = 1.f / (va + vb + vc);
  return a + ab * (vb * denom) + ac * (vc * denom);
}

City::City(const std::string &path, Shader &shader, PhysicsWorld &physics,
           glm::vec3 worldPos, float scale)
    : mModel(path, shader, false), mShader(shader), mWorldPos(worldPos),
      mScale(scale) {
  float cx = (MODEL_MIN_X + MODEL_MAX_X) * 0.5f;
  float cz = (MODEL_MIN_Z + MODEL_MAX_Z) * 0.5f;
  mModelMat =
      glm::translate(glm::mat4(1.f), worldPos) *
      glm::scale(glm::mat4(1.f), glm::vec3(scale)) *
      glm::translate(glm::mat4(1.f), glm::vec3(-cx, -MODEL_MIN_Y - 7.f, -cz));

  mWMin = glm::vec3(1e30f);
  mWMax = glm::vec3(-1e30f);
  for (const auto &[mn, mx] : mModel.meshAABBs) {
    glm::vec3 cs[8] = {
        {mn.x, mn.y, mn.z}, {mx.x, mn.y, mn.z}, {mn.x, mx.y, mn.z},
        {mx.x, mx.y, mn.z}, {mn.x, mn.y, mx.z}, {mx.x, mn.y, mx.z},
        {mn.x, mx.y, mx.z}, {mx.x, mx.y, mx.z},
    };
    for (auto &c : cs) {
      glm::vec3 w = glm::vec3(mModelMat * glm::vec4(c, 1.f));
      mWMin = glm::min(mWMin, w);
      mWMax = glm::max(mWMax, w);
    }
  }

  mHeightGrid.assign(HGX * HGZ, 0.f);
  for (const auto &[mn, mx] : mModel.meshAABBs) {
    glm::vec3 cs[8] = {
        {mn.x, mn.y, mn.z}, {mx.x, mn.y, mn.z}, {mn.x, mx.y, mn.z},
        {mx.x, mx.y, mn.z}, {mn.x, mn.y, mx.z}, {mx.x, mn.y, mx.z},
        {mn.x, mx.y, mx.z}, {mx.x, mx.y, mx.z},
    };
    glm::vec3 wMin(1e30f), wMax(-1e30f);
    for (auto &c : cs) {
      glm::vec3 w = glm::vec3(mModelMat * glm::vec4(c, 1.f));
      wMin = glm::min(wMin, w);
      wMax = glm::max(wMax, w);
    }
    if ((wMax.y - wMin.y) < 2.f)
      continue;
    float rx0 = (wMin.x - mWMin.x) / (mWMax.x - mWMin.x);
    float rx1 = (wMax.x - mWMin.x) / (mWMax.x - mWMin.x);
    float rz0 = (wMin.z - mWMin.z) / (mWMax.z - mWMin.z);
    float rz1 = (wMax.z - mWMin.z) / (mWMax.z - mWMin.z);
    int gx0 = glm::clamp((int)(rx0 * HGX), 0, HGX - 1);
    int gx1 = glm::clamp((int)(rx1 * HGX), 0, HGX - 1);
    int gz0 = glm::clamp((int)(rz0 * HGZ), 0, HGZ - 1);
    int gz1 = glm::clamp((int)(rz1 * HGZ), 0, HGZ - 1);
    for (int gz = gz0; gz <= gz1; ++gz)
      for (int gx = gx0; gx <= gx1; ++gx)
        mHeightGrid[gz * HGX + gx] =
            glm::max(mHeightGrid[gz * HGX + gx], wMax.y);
  }

  for (const auto &tri : mModel.triangles) {
    CityTri ct;
    for (int i = 0; i < 3; ++i)
      ct.v[i] = glm::vec3(mModelMat * glm::vec4(tri[i], 1.f));
    glm::vec3 e1 = ct.v[1] - ct.v[0];
    glm::vec3 e2 = ct.v[2] - ct.v[0];
    glm::vec3 n = glm::cross(e1, e2);
    float nl = glm::length(n);
    if (nl < 1e-9f)
      continue;
    ct.normal = n / nl;
    ct.planeD = glm::dot(ct.normal, ct.v[0]);
    mTris.push_back(ct);
  }

  buildBVH();
}

void City::computeAABB(const std::vector<int> &idx, int begin, int end,
                       glm::vec3 &outMin, glm::vec3 &outMax) const {
  outMin = glm::vec3(1e30f);
  outMax = glm::vec3(-1e30f);
  for (int i = begin; i < end; ++i) {
    const CityTri &t = mTris[idx[i]];
    for (int k = 0; k < 3; ++k) {
      outMin = glm::min(outMin, t.v[k]);
      outMax = glm::max(outMax, t.v[k]);
    }
  }
}

int City::buildNode(std::vector<int> &triIdx, int begin, int end) {
  int nodeIdx = (int)mBVH.size();
  mBVH.emplace_back();
  BVHNode &node = mBVH[nodeIdx];
  computeAABB(triIdx, begin, end, node.aabbMin, node.aabbMax);
  int count = end - begin;
  if (count <= LEAF_TRI_MAX) {
    node.triStart = begin;
    node.triCount = count;
    return nodeIdx;
  }
  glm::vec3 extent = node.aabbMax - node.aabbMin;
  int axis = 0;
  if (extent.y > extent.x)
    axis = 1;
  if (extent.z > extent[axis])
    axis = 2;
  int mid = (begin + end) / 2;
  std::nth_element(triIdx.begin() + begin, triIdx.begin() + mid,
                   triIdx.begin() + end, [&](int a, int b) {
                     float ca = (mTris[a].v[0][axis] + mTris[a].v[1][axis] +
                                 mTris[a].v[2][axis]) /
                                3.f;
                     float cb = (mTris[b].v[0][axis] + mTris[b].v[1][axis] +
                                 mTris[b].v[2][axis]) /
                                3.f;
                     return ca < cb;
                   });
  int leftIdx = buildNode(triIdx, begin, mid);
  int rightIdx = buildNode(triIdx, mid, end);
  mBVH[nodeIdx].left = leftIdx;
  mBVH[nodeIdx].right = rightIdx;
  mBVH[nodeIdx].triCount = 0;
  return nodeIdx;
}

void City::buildBVH() {
  if (mTris.empty())
    return;
  std::vector<int> idx(mTris.size());
  std::iota(idx.begin(), idx.end(), 0);
  mBVH.reserve(mTris.size() * 2);
  mBVHRoot = buildNode(idx, 0, (int)idx.size());
  std::vector<CityTri> sorted;
  sorted.reserve(mTris.size());
  for (int i : idx)
    sorted.push_back(mTris[i]);
  mTris = std::move(sorted);
}

bool City::sphereTriContact(const glm::vec3 &center, float radius,
                            const CityTri &tri, Contact &out, float restitution,
                            float friction) const {
  glm::vec3 closest = closestPtTriangle(center, tri.v[0], tri.v[1], tri.v[2]);
  glm::vec3 d = center - closest;
  float dist2 = glm::dot(d, d);
  if (dist2 >= radius * radius)
    return false;
  float dist = std::sqrt(dist2);
  out.point = closest;
  out.penetration = radius - dist;
  if (dist > 1e-5f)
    out.normal = d / dist;
  else
    out.normal = (glm::dot(tri.normal, center - tri.v[0]) >= 0.f) ? tri.normal
                                                                  : -tri.normal;
  out.restitution = restitution;
  out.friction = friction;
  return true;
}

void City::queryNode(int nodeIdx, const glm::vec3 &pos, float radius,
                     std::vector<Contact> &contacts, float restitution,
                     float friction) const {
  if (nodeIdx < 0)
    return;
  const BVHNode &node = mBVH[nodeIdx];
  glm::vec3 closest = glm::clamp(pos, node.aabbMin, node.aabbMax);
  glm::vec3 diff = pos - closest;
  if (glm::dot(diff, diff) > radius * radius)
    return;
  if (node.triCount > 0) {
    for (int i = node.triStart; i < node.triStart + node.triCount; ++i) {
      Contact c;
      if (sphereTriContact(pos, radius, mTris[i], c, restitution, friction))
        contacts.push_back(c);
    }
  } else {
    queryNode(node.left, pos, radius, contacts, restitution, friction);
    queryNode(node.right, pos, radius, contacts, restitution, friction);
  }
}

void City::queryContacts(const glm::vec3 &pos, float radius,
                         std::vector<Contact> &contacts, float restitution,
                         float friction) const {
  if (mBVHRoot < 0)
    return;
  queryNode(mBVHRoot, pos, radius, contacts, restitution, friction);
}

void City::drawDepth(Shader &depthShader) {
  mModel.drawDepth(mModelMat, depthShader);
}

void City::draw(const glm::mat4 &view, const glm::mat4 &proj,
                const glm::vec3 &sunDir, const glm::mat4 &lightSpaceMat,
                GLuint shadowTex) {
  mModel.draw(mModelMat, view, proj, sunDir, lightSpaceMat, shadowTex);
}

float City::heightAt(float wx, float wz) const {
  if (wx < mWMin.x || wx > mWMax.x || wz < mWMin.z || wz > mWMax.z)
    return 0.f;
  float tx = (wx - mWMin.x) / (mWMax.x - mWMin.x);
  float tz = (wz - mWMin.z) / (mWMax.z - mWMin.z);
  int gx = glm::clamp((int)(tx * HGX), 0, HGX - 1);
  int gz = glm::clamp((int)(tz * HGZ), 0, HGZ - 1);
  return mHeightGrid[gz * HGX + gx];
}
