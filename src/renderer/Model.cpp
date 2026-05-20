#define STB_IMAGE_IMPLEMENTATION
#include "Model.h"
#include "stb_image.h"
#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/pbrmaterial.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

Model::Model(const std::string &path, Shader &shader, bool mergeMeshes)
    : shader(shader) {
  Assimp::Importer importer;
  importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE,
                              aiPrimitiveType_POINT | aiPrimitiveType_LINE);
  importer.SetPropertyBool(AI_CONFIG_IMPORT_NO_SKELETON_MESHES, true);

  unsigned int flags = aiProcess_Triangulate | aiProcess_FlipUVs;
  if (!mergeMeshes)
    flags |= aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices;

  const aiScene *scene = importer.ReadFile(path, flags);

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) {
    std::cerr << "Assimp error: " << importer.GetErrorString() << "\n";
    return;
  }

  if (mergeMeshes) {
    mergeMeshesIntoOne(scene);
  } else {
    processNode(scene->mRootNode, scene);
  }
}

void Model::mergeMeshesIntoOne(const aiScene *scene) {
  std::vector<float> verts;
  std::vector<unsigned> indices;
  unsigned vertOffset = 0;

  for (unsigned mi = 0; mi < scene->mNumMeshes; ++mi) {
    const aiMesh *mesh = scene->mMeshes[mi];

    glm::vec3 col(0.75f, 0.73f, 0.70f);
    if (mesh->mMaterialIndex < scene->mNumMaterials) {
      aiMaterial *mat = scene->mMaterials[mesh->mMaterialIndex];
      aiColor4D c;
      if (AI_SUCCESS == mat->Get(AI_MATKEY_BASE_COLOR, c))
        col = glm::vec3(c.r, c.g, c.b);
      else if (AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_DIFFUSE, c))
        col = glm::vec3(c.r, c.g, c.b);
    }

    for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
      verts.push_back(mesh->mVertices[i].x);
      verts.push_back(mesh->mVertices[i].y);
      verts.push_back(mesh->mVertices[i].z);
      if (mesh->HasNormals()) {
        verts.push_back(mesh->mNormals[i].x);
        verts.push_back(mesh->mNormals[i].y);
        verts.push_back(mesh->mNormals[i].z);
      } else {
        verts.push_back(0.f);
        verts.push_back(1.f);
        verts.push_back(0.f);
      }
      if (mesh->mTextureCoords[0]) {
        verts.push_back(mesh->mTextureCoords[0][i].x);
        verts.push_back(mesh->mTextureCoords[0][i].y);
      } else {
        verts.push_back(0.f);
        verts.push_back(0.f);
      }
      verts.push_back(col.r);
      verts.push_back(col.g);
      verts.push_back(col.b);
    }

    for (unsigned f = 0; f < mesh->mNumFaces; ++f)
      for (unsigned j = 0; j < mesh->mFaces[f].mNumIndices; ++j)
        indices.push_back(mesh->mFaces[f].mIndices[j] + vertOffset);

    vertOffset += mesh->mNumVertices;
  }

  Mesh out{};
  out.indexCount = (int)indices.size();
  out.baseColor = glm::vec4(1.f);
  out.isMerged = true;

  glGenVertexArrays(1, &out.vao);
  glGenBuffers(1, &out.vbo);
  glGenBuffers(1, &out.ebo);
  glBindVertexArray(out.vao);
  glBindBuffer(GL_ARRAY_BUFFER, out.vbo);
  glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(),
               GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out.ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned),
               indices.data(), GL_STATIC_DRAW);

  constexpr GLsizei stride = 11 * sizeof(float);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
                        (void *)(6 * sizeof(float)));
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride,
                        (void *)(8 * sizeof(float)));
  glBindVertexArray(0);
  meshes.push_back(out);
}

Model::~Model() {
  for (auto &m : meshes) {
    glDeleteVertexArrays(1, &m.vao);
    glDeleteBuffers(1, &m.vbo);
    glDeleteBuffers(1, &m.ebo);
  }
  for (auto t : textures)
    glDeleteTextures(1, &t);
}

void Model::processNode(const aiNode *node, const aiScene *scene) {
  for (unsigned i = 0; i < node->mNumMeshes; i++)
    meshes.push_back(processMesh(scene->mMeshes[node->mMeshes[i]], scene));
  for (unsigned i = 0; i < node->mNumChildren; i++)
    processNode(node->mChildren[i], scene);
}

GLuint Model::loadEmbeddedTexture(const aiTexture *tex) {
  GLuint id = 0;
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);

  int w, h, ch;
  unsigned char *data = nullptr;

  if (tex->mHeight == 0) {
    data = stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(tex->pcData),
                                 tex->mWidth, &w, &h, &ch, 4);
  } else {
    w = tex->mWidth;
    h = tex->mHeight;
    data = new unsigned char[w * h * 4];
    for (int i = 0; i < w * h; i++) {
      data[i * 4 + 0] = tex->pcData[i].r;
      data[i * 4 + 1] = tex->pcData[i].g;
      data[i * 4 + 2] = tex->pcData[i].b;
      data[i * 4 + 3] = tex->pcData[i].a;
    }
  }

  if (data) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 data);
    glGenerateMipmap(GL_TEXTURE_2D);
    if (tex->mHeight == 0)
      stbi_image_free(data);
    else
      delete[] data;
  }

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  textures.push_back(id);
  return id;
}

Mesh Model::processMesh(const aiMesh *mesh, const aiScene *scene) {
  std::vector<float> verts;
  verts.reserve(mesh->mNumVertices * 8);

  for (unsigned i = 0; i < mesh->mNumVertices; i++) {
    verts.push_back(mesh->mVertices[i].x);
    verts.push_back(mesh->mVertices[i].y);
    verts.push_back(mesh->mVertices[i].z);
    if (mesh->HasNormals()) {
      verts.push_back(mesh->mNormals[i].x);
      verts.push_back(mesh->mNormals[i].y);
      verts.push_back(mesh->mNormals[i].z);
    } else {
      verts.push_back(0.f);
      verts.push_back(1.f);
      verts.push_back(0.f);
    }
    if (mesh->mTextureCoords[0]) {
      verts.push_back(mesh->mTextureCoords[0][i].x);
      verts.push_back(mesh->mTextureCoords[0][i].y);
    } else {
      verts.push_back(0.f);
      verts.push_back(0.f);
    }
  }

  std::vector<unsigned> indices;
  indices.reserve(mesh->mNumFaces * 3);
  for (unsigned i = 0; i < mesh->mNumFaces; i++)
    for (unsigned j = 0; j < mesh->mFaces[i].mNumIndices; j++)
      indices.push_back(mesh->mFaces[i].mIndices[j]);

  for (unsigned i = 0; i < mesh->mNumFaces; i++) {
    if (mesh->mFaces[i].mNumIndices != 3)
      continue;
    unsigned i0 = mesh->mFaces[i].mIndices[0];
    unsigned i1 = mesh->mFaces[i].mIndices[1];
    unsigned i2 = mesh->mFaces[i].mIndices[2];
    triangles.push_back({
        glm::vec3(mesh->mVertices[i0].x, mesh->mVertices[i0].y,
                  mesh->mVertices[i0].z),
        glm::vec3(mesh->mVertices[i1].x, mesh->mVertices[i1].y,
                  mesh->mVertices[i1].z),
        glm::vec3(mesh->mVertices[i2].x, mesh->mVertices[i2].y,
                  mesh->mVertices[i2].z),
    });
  }

  Mesh out{};
  out.indexCount = (int)indices.size();

  if (mesh->mMaterialIndex >= 0) {
    aiMaterial *mat = scene->mMaterials[mesh->mMaterialIndex];
    aiString texPath;
    if (mat->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_TEXTURE,
                        &texPath) == AI_SUCCESS ||
        mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
      if (texPath.data[0] == '*') {
        int idx = atoi(texPath.data + 1);
        if (idx < (int)scene->mNumTextures)
          out.diffuseTex = loadEmbeddedTexture(scene->mTextures[idx]);
      }
    }
    if (out.diffuseTex == 0) {
      aiColor4D c;
      if (AI_SUCCESS ==
          mat->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_FACTOR, c))
        out.baseColor = glm::vec4(c.r, c.g, c.b, c.a);
      else if (AI_SUCCESS == mat->Get(AI_MATKEY_BASE_COLOR, c))
        out.baseColor = glm::vec4(c.r, c.g, c.b, c.a);
      else if (AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_DIFFUSE, c))
        out.baseColor = glm::vec4(c.r, c.g, c.b, c.a);
    }
  }

  glm::vec3 aabbMin(1e30f), aabbMax(-1e30f);
  for (unsigned i = 0; i < mesh->mNumVertices; i++) {
    glm::vec3 v(mesh->mVertices[i].x, mesh->mVertices[i].y,
                mesh->mVertices[i].z);
    aabbMin = glm::min(aabbMin, v);
    aabbMax = glm::max(aabbMax, v);
  }
  meshAABBs.push_back({aabbMin, aabbMax});

  glGenVertexArrays(1, &out.vao);
  glGenBuffers(1, &out.vbo);
  glGenBuffers(1, &out.ebo);
  glBindVertexArray(out.vao);
  glBindBuffer(GL_ARRAY_BUFFER, out.vbo);
  glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(),
               GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out.ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned),
               indices.data(), GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void *)(6 * sizeof(float)));
  glBindVertexArray(0);
  return out;
}

void Model::draw(const glm::mat4 &modelMatrix, const glm::mat4 &view,
                 const glm::mat4 &proj, const glm::vec3 &sunDir,
                 const glm::mat4 &lightSpaceMat, GLuint shadowMapTex) {
  shader.use();
  shader.setMat4("uModel", modelMatrix);
  shader.setMat4("uMVP", proj * view * modelMatrix);
  shader.setMat4("uLightSpaceMat", lightSpaceMat);
  shader.setVec3("uSunDir", sunDir);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, shadowMapTex);
  glUniform1i(glGetUniformLocation(shader.id, "uShadowMap"), 1);
  glUniform1f(glGetUniformLocation(shader.id, "uShadowStrength"), 0.65f);

  for (auto &m : meshes) {
    glUniform1i(glGetUniformLocation(shader.id, "uUseVertColor"),
                m.isMerged ? 1 : 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shadowMapTex);
    if (m.diffuseTex) {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, m.diffuseTex);
      glUniform1i(glGetUniformLocation(shader.id, "uDiffuse"), 0);
      glUniform1i(glGetUniformLocation(shader.id, "uHasTex"), 1);
    } else {
      glUniform1i(glGetUniformLocation(shader.id, "uHasTex"), 0);
      shader.setVec3("uBaseColor", glm::vec3(m.baseColor));
    }
    glBindVertexArray(m.vao);
    glDrawElements(GL_TRIANGLES, m.indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
  }
}

void Model::drawDepth(const glm::mat4 &modelMatrix, Shader &depthShader) {
  depthShader.use();
  for (auto &m : meshes) {
    glBindVertexArray(m.vao);
    glDrawElements(GL_TRIANGLES, m.indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
  }
}
