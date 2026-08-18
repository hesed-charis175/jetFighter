#pragma once

#include <GL/glew.h>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

struct RadarBlip {
  glm::vec3 worldPos;
  glm::vec3 color;
  float size;
};

class RadarSystem {
private:
  float screenWidth = 1200.0f;
  float screenHeight = 800.0f;
  float radarRadiusPixels = 100.0f;
  float maxDetectionRange = 2500.0f;
  glm::vec2 radarCenterScreen = glm::vec2(1070.0f, 130.0f);

  // Landmarks, set once via SetLandmarks() — not passed every frame.
  glm::vec3 cityCenter = glm::vec3(0.f);
  float cityRadius = 0.f;
  float enclosureHalfSize = 0.f;
  float altitudeCeilingY = -1e9f; // sentinel "unset"

  // Altitude ladder layout
  float ladderX = 55.0f;
  float ladderHalfHeightPx = 130.0f;
  float ladderHalfRangeMeters = 1500.0f;

  GLuint shaderProgram = 0;
  GLuint circleVAO = 0, circleVBO = 0;
  GLuint dynamicVAO = 0, dynamicVBO = 0;

  int circleVertexCount = 36;

  const char *vertShaderSrc = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        uniform mat4 uProjection;
        void main() {
            gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
        }
    )";

  const char *fragShaderSrc = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec4 uColor;
        void main() {
            FragColor = uColor;
        }
    )";

  GLuint compileShader(GLenum type, const char *src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    return shader;
  }

  void initGL() {
    if (shaderProgram != 0)
      return;

    GLuint vert = compileShader(GL_VERTEX_SHADER, vertShaderSrc);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragShaderSrc);
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vert);
    glAttachShader(shaderProgram, frag);
    glLinkProgram(shaderProgram);
    glDeleteShader(vert);
    glDeleteShader(frag);

    std::vector<float> circleVertices;
    circleVertices.push_back(0.0f);
    circleVertices.push_back(0.0f);
    for (int i = 0; i <= circleVertexCount; ++i) {
      float angle = i * (2.0f * 3.14159265f / circleVertexCount);
      circleVertices.push_back(std::cos(angle));
      circleVertices.push_back(std::sin(angle));
    }

    glGenVertexArrays(1, &circleVAO);
    glGenBuffers(1, &circleVBO);
    glBindVertexArray(circleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, circleVBO);
    glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(float),
                 circleVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);

    glGenVertexArrays(1, &dynamicVAO);
    glGenBuffers(1, &dynamicVBO);
    glBindVertexArray(dynamicVAO);
    glBindBuffer(GL_ARRAY_BUFFER, dynamicVBO);
    glBufferData(GL_ARRAY_BUFFER, 4096 * sizeof(float), nullptr,
                 GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
  }

  void drawLines(const std::vector<float> &vertices, const glm::vec4 &color,
                 GLenum mode = GL_LINES, float lineWidth = 1.0f) {
    if (vertices.empty())
      return;
    glLineWidth(lineWidth);
    glUniform4fv(glGetUniformLocation(shaderProgram, "uColor"), 1,
                 glm::value_ptr(color));
    glBindVertexArray(dynamicVAO);
    glBindBuffer(GL_ARRAY_BUFFER, dynamicVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float),
                    vertices.data());
    glDrawArrays(mode, 0, vertices.size() / 2);
  }

  // Projects world space position onto radar screen position and handles border
  // clamping
  bool WorldToRadar(const glm::vec3 &worldPos, const glm::vec3 &playerPos,
                    const glm::quat &playerOrientation, glm::vec2 &outScreenPos,
                    bool &outIsClamped) {
    glm::vec3 relPos = worldPos - playerPos;
    glm::vec3 playerForward = playerOrientation * glm::vec3(0, 0, -1);
    playerForward.y = 0.0f;
    if (glm::length(playerForward) > 0.0001f)
      playerForward = glm::normalize(playerForward);
    else
      playerForward = glm::vec3(0, 0, -1);

    float headingAngle = std::atan2(-playerForward.x, -playerForward.z);
    float cosH = std::cos(-headingAngle);
    float sinH = std::sin(-headingAngle);

    float rotX = relPos.x * cosH - relPos.z * sinH;
    float rotZ = relPos.x * sinH + relPos.z * cosH;

    float normX = rotX / maxDetectionRange;
    float normZ = -rotZ / maxDetectionRange;

    glm::vec2 normVec(normX, normZ);
    float dist = glm::length(normVec);

    if (dist > 1.0f) {
      normVec = normVec / dist;
      outIsClamped = true;
    } else {
      outIsClamped = false;
    }

    outScreenPos = radarCenterScreen + normVec * radarRadiusPixels;
    return true;
  }

  // Lights up a wedge on the outer ring in the direction of a nearby
  // enclosure wall — replaces the old always-clamped corner dots, which
  // were almost never useful since the enclosure is bigger than radar range.
  void DrawWallWarnings(const glm::vec3 &playerPos,
                        const glm::quat &playerOrientation) {
    if (enclosureHalfSize <= 0.f)
      return;

    glm::vec3 playerForward = playerOrientation * glm::vec3(0, 0, -1);
    playerForward.y = 0.0f;
    if (glm::length(playerForward) > 0.0001f)
      playerForward = glm::normalize(playerForward);
    else
      playerForward = glm::vec3(0, 0, -1);
    float headingAngle = std::atan2(-playerForward.x, -playerForward.z);
    float cosH = std::cos(-headingAngle);
    float sinH = std::sin(-headingAngle);

    struct Wall {
      float dist, dx, dz;
    };
    Wall walls[4] = {
        {enclosureHalfSize - playerPos.x, 1.f, 0.f},
        {playerPos.x + enclosureHalfSize, -1.f, 0.f},
        {enclosureHalfSize - playerPos.z, 0.f, 1.f},
        {playerPos.z + enclosureHalfSize, 0.f, -1.f},
    };

    constexpr float WARN_RANGE = 1200.0f;
    for (auto &w : walls) {
      if (w.dist > WARN_RANGE || w.dist < 0.f)
        continue;
      float intensity = 1.0f - glm::clamp(w.dist / WARN_RANGE, 0.0f, 1.0f);

      float rotX = w.dx * cosH - w.dz * sinH;
      float rotZ = w.dx * sinH + w.dz * cosH;
      float baseAngle = std::atan2(-rotZ, rotX);

      const float arcHalf = glm::radians(16.0f);
      const int segs = 8;
      std::vector<float> wedge;
      wedge.push_back(radarCenterScreen.x);
      wedge.push_back(radarCenterScreen.y);
      for (int s = 0; s <= segs; ++s) {
        float a = baseAngle - arcHalf + (2.f * arcHalf) * (s / float(segs));
        wedge.push_back(radarCenterScreen.x + std::cos(a) * radarRadiusPixels);
        wedge.push_back(radarCenterScreen.y + std::sin(a) * radarRadiusPixels);
      }
      drawLines(wedge, glm::vec4(1.0f, 0.15f, 0.1f, intensity * 0.55f),
                GL_TRIANGLE_FAN);
    }
  }

  // Draws the city as a filled, ringed disc scaled to its real radius,
  // instead of a single point.
  void DrawCityMarker(const glm::vec3 &playerPos,
                      const glm::quat &playerOrientation) {
    if (cityRadius <= 0.f)
      return;

    glm::vec2 screenPos;
    bool clamped;
    WorldToRadar(cityCenter, playerPos, playerOrientation, screenPos, clamped);

    float worldToPixel = radarRadiusPixels / maxDetectionRange;
    float pixelRadius =
        glm::clamp(cityRadius * worldToPixel, 4.0f, radarRadiusPixels * 0.9f);
    if (clamped)
      pixelRadius = glm::min(pixelRadius, 10.0f);

    const int segs = 20;
    std::vector<float> disc;
    disc.push_back(screenPos.x);
    disc.push_back(screenPos.y);
    for (int s = 0; s <= segs; ++s) {
      float a = (2.0f * 3.14159265f) * (s / float(segs));
      disc.push_back(screenPos.x + std::cos(a) * pixelRadius);
      disc.push_back(screenPos.y + std::sin(a) * pixelRadius);
    }
    drawLines(disc, glm::vec4(0.15f, 0.55f, 1.0f, 0.35f), GL_TRIANGLE_FAN);

    std::vector<float> ring;
    for (int s = 0; s <= segs; ++s) {
      float a = (2.0f * 3.14159265f) * (s / float(segs));
      ring.push_back(screenPos.x + std::cos(a) * pixelRadius);
      ring.push_back(screenPos.y + std::sin(a) * pixelRadius);
    }
    drawLines(ring, glm::vec4(0.3f, 0.75f, 1.0f, 0.9f), GL_LINE_LOOP, 1.5f);
  }

  // Vertical altitude tape: player always centered, contacts / ground /
  // ceiling shown as ticks at their relative height.
  void DrawAltitudeLadder(const glm::vec3 &playerPos, float terrainYBelowPlayer,
                          const std::vector<RadarBlip> &blips) {
    float halfH = ladderHalfHeightPx;
    float cx = ladderX;
    float cy = screenHeight * 0.5f;

    auto deltaToY = [&](float delta, bool &clamped) {
      float t = glm::clamp(delta / ladderHalfRangeMeters, -1.0f, 1.0f);
      clamped = std::abs(delta) > ladderHalfRangeMeters;
      return cy + t * halfH;
    };

    std::vector<float> spine = {cx, cy - halfH, cx, cy + halfH};
    drawLines(spine, glm::vec4(1, 1, 1, 0.35f), GL_LINES, 1.5f);

    std::vector<float> minorTicks, majorTicks;
    const float tickStep = 250.0f;
    int steps = (int)(ladderHalfRangeMeters / tickStep);
    for (int i = -steps; i <= steps; ++i) {
      float delta = i * tickStep;
      bool clamped;
      float y = deltaToY(delta, clamped);
      bool major = (i % 4 == 0); // every 1000m
      float len = major ? 10.0f : 5.0f;
      auto &buf = major ? majorTicks : minorTicks;
      buf.push_back(cx);
      buf.push_back(y);
      buf.push_back(cx + len);
      buf.push_back(y);
    }
    drawLines(minorTicks, glm::vec4(1, 1, 1, 0.25f), GL_LINES, 1.0f);
    drawLines(majorTicks, glm::vec4(1, 1, 1, 0.55f), GL_LINES, 1.5f);

    // Ground reference
    {
      bool clamped;
      float delta = terrainYBelowPlayer - playerPos.y;
      float y = deltaToY(delta, clamped);
      if (!clamped) {
        std::vector<float> ground = {cx - 4.f, y, cx + 16.f, y};
        drawLines(ground, glm::vec4(0.55f, 0.4f, 0.2f, 0.9f), GL_LINES, 3.0f);
      }
    }

    // Ceiling reference
    if (altitudeCeilingY > -1e8f) {
      bool clamped;
      float delta = altitudeCeilingY - playerPos.y;
      float y = deltaToY(delta, clamped);
      if (!clamped) {
        std::vector<float> ceil = {cx - 4.f, y, cx + 16.f, y};
        drawLines(ceil, glm::vec4(1.0f, 0.2f, 0.2f, 0.9f), GL_LINES, 3.0f);
      }
    }

    // Contacts (reuses the same colors as the round radar, so a contact is
    // easy to correlate between the two widgets)
    for (const auto &blip : blips) {
      bool clamped;
      float delta = blip.worldPos.y - playerPos.y;
      float y = deltaToY(delta, clamped);
      float yy = glm::clamp(y, cy - halfH, cy + halfH);
      std::vector<float> tri;
      if (!clamped) {
        tri = {cx + 6.f, yy, cx + 16.f, yy - 4.f, cx + 16.f, yy + 4.f};
      } else {
        // pinned to the edge, nudged further out to read as "off-scale"
        float dir = (delta > 0.f) ? -3.f : 3.f;
        tri = {cx + 6.f,       yy,        cx + 16.f,
               yy + dir - 4.f, cx + 16.f, yy + dir + 4.f};
      }
      drawLines(tri, glm::vec4(blip.color, 0.95f), GL_TRIANGLES);
    }

    // Player marker — always dead center
    std::vector<float> playerMark = {cx - 10.f, cy,       cx + 2.f,
                                     cy - 5.f,  cx + 2.f, cy + 5.f};
    drawLines(playerMark, glm::vec4(1.0f, 1.0f, 0.2f, 1.0f), GL_TRIANGLES);
  }

public:
  RadarSystem(float w = 1200.0f, float h = 800.0f, float radius = 100.0f,
              float range = 2500.0f)
      : screenWidth(w), screenHeight(h), radarRadiusPixels(radius),
        maxDetectionRange(range) {
    SetScreenSize(w, h);
  }

  void SetScreenSize(float w, float h) {
    screenWidth = w;
    screenHeight = h;
    radarCenterScreen =
        glm::vec2(w - radarRadiusPixels - 30.0f, radarRadiusPixels + 30.0f);
  }

  // Call once after setup — these don't change frame to frame.
  void SetLandmarks(const glm::vec3 &cityCenterIn, float cityRadiusIn,
                    float enclosureHalfSizeIn, float altitudeCeilingIn) {
    cityCenter = cityCenterIn;
    cityRadius = cityRadiusIn;
    enclosureHalfSize = enclosureHalfSizeIn;
    altitudeCeilingY = altitudeCeilingIn;
  }

  void RenderRadar(const glm::vec3 &playerPos,
                   const glm::quat &playerOrientation,
                   float terrainYBelowPlayer,
                   const std::vector<RadarBlip> &blips) {
    initGL();

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);

    glUseProgram(shaderProgram);
    glm::mat4 proj =
        glm::ortho(0.0f, screenWidth, 0.0f, screenHeight, -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uProjection"), 1,
                       GL_FALSE, glm::value_ptr(proj));

    // ---- Round radar background ----
    glBindVertexArray(circleVAO);
    glm::mat4 bgModel =
        glm::translate(glm::mat4(1.0f), glm::vec3(radarCenterScreen, 0.0f));
    bgModel = glm::scale(bgModel,
                         glm::vec3(radarRadiusPixels, radarRadiusPixels, 1.0f));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uProjection"), 1,
                       GL_FALSE, glm::value_ptr(proj * bgModel));
    glUniform4f(glGetUniformLocation(shaderProgram, "uColor"), 0.02f, 0.12f,
                0.05f, 0.65f);
    glDrawArrays(GL_TRIANGLE_FAN, 0, circleVertexCount + 2);

    glLineWidth(2.0f);
    glUniform4f(glGetUniformLocation(shaderProgram, "uColor"), 0.0f, 1.0f, 0.3f,
                0.8f);
    glDrawArrays(GL_LINE_LOOP, 1, circleVertexCount);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uProjection"), 1,
                       GL_FALSE, glm::value_ptr(proj));

    // ---- Landmarks ----
    DrawWallWarnings(playerPos, playerOrientation);
    DrawCityMarker(playerPos, playerOrientation);

    // ---- Crosshair grid ----
    std::vector<float> gridLines = {radarCenterScreen.x - radarRadiusPixels,
                                    radarCenterScreen.y,
                                    radarCenterScreen.x + radarRadiusPixels,
                                    radarCenterScreen.y,
                                    radarCenterScreen.x,
                                    radarCenterScreen.y - radarRadiusPixels,
                                    radarCenterScreen.x,
                                    radarCenterScreen.y + radarRadiusPixels};
    drawLines(gridLines, glm::vec4(1.0f, 1.0f, 1.0f, 0.15f), GL_LINES, 1.0f);

    // ---- Center player marker ----
    std::vector<float> playerTri = {
        radarCenterScreen.x,        radarCenterScreen.y + 6.0f,
        radarCenterScreen.x - 4.0f, radarCenterScreen.y - 4.0f,
        radarCenterScreen.x + 4.0f, radarCenterScreen.y - 4.0f};
    drawLines(playerTri, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), GL_TRIANGLES);

    // ---- Dynamic contacts (other planes) ----
    for (const auto &blip : blips) {
      glm::vec2 groundPos;
      bool isClamped = false;

      if (WorldToRadar(blip.worldPos, playerPos, playerOrientation, groundPos,
                       isClamped)) {

        float altitudeDelta = blip.worldPos.y - playerPos.y;
        float altitudeLineHeight =
            isClamped ? 0.0f : glm::clamp(altitudeDelta * 0.05f, -30.0f, 30.0f);

        glm::vec2 targetPos = groundPos + glm::vec2(0.0f, altitudeLineHeight);

        // Stem alpha communicates "how far off your altitude" at a glance,
        // even without reading the exact offset.
        float altMag = glm::clamp(std::abs(altitudeDelta) / 600.0f, 0.0f, 1.0f);

        if (!isClamped) {
          float gSize = 1.5f;
          std::vector<float> groundDot = {
              groundPos.x - gSize, groundPos.y - gSize, groundPos.x + gSize,
              groundPos.y - gSize, groundPos.x + gSize, groundPos.y + gSize,
              groundPos.x - gSize, groundPos.y - gSize, groundPos.x + gSize,
              groundPos.y + gSize, groundPos.x - gSize, groundPos.y + gSize};
          drawLines(groundDot, glm::vec4(blip.color, 0.4f), GL_TRIANGLES);

          std::vector<float> stem = {groundPos.x, groundPos.y, targetPos.x,
                                     targetPos.y};
          drawLines(stem, glm::vec4(blip.color, 0.35f + altMag * 0.5f),
                    GL_LINES, 1.5f);
        }

        float s = blip.size * 0.5f;
        std::vector<float> blipQuad = {
            targetPos.x - s, targetPos.y - s, targetPos.x + s, targetPos.y - s,
            targetPos.x + s, targetPos.y + s, targetPos.x - s, targetPos.y - s,
            targetPos.x + s, targetPos.y + s, targetPos.x - s, targetPos.y + s};
        drawLines(blipQuad, glm::vec4(blip.color, 1.0f), GL_TRIANGLES);
      }
    }

    // ---- Altitude ladder (second widget) ----
    DrawAltitudeLadder(playerPos, terrainYBelowPlayer, blips);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
    glUseProgram(0);
  }
};
