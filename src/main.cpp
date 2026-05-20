#include "Camera.h"
#include "GameWindow.h"
#include "Keys.h"
#include "Plane.h"
#include "config.h"
#include "physics/PhysicsWorld.h"
#include "physics/PlaneBody.h"
#include "renderer/City.h"
#include "renderer/Enclosure.h"
#include "renderer/Exhaust.h"
#include "renderer/Model.h"
#include "renderer/Ocean.h"
#include "renderer/PlaneDebug.h"
#include "renderer/Shader.h"
#include "renderer/Sky.h"
#include "renderer/Terrain.h"
#include "renderer/Tracer.h"
#include <SDL2/SDL_video.h>
#include <iostream>

PlaneInput playerInput(const Uint8 *keys) {
  PlaneInput in;
  if (keys[KEY_THRUST_FORWARD])
    in.thrust = 1.f;
  else if (keys[KEY_STOP_THRUST])
    in.thrust = -1.f;
  if (keys[KEY_TURN_LEFT])
    in.yaw = 1.f;
  else if (keys[KEY_TURN_RIGHT])
    in.yaw = -1.f;
  if (keys[KEY_ROLL_UP])
    in.pitch = 1.f;
  else if (keys[KEY_ROLL_DOWN])
    in.pitch = -1.f;
  return in;
}

PlaneInput aiInput(const Plane &, float) { return {}; }

int main() {
  Window window(1200, 800, true);
  Sky sky;
  Terrain terrain;
  Ocean ocean(128, 3500.f, 3500.f);

  Shader modelShader("assets/shaders/model.vert", "assets/shaders/model.frag");
  Model rafale("assets/models/dassault_rafale.glb", modelShader);

  PhysicsWorld physics;

  PlaneDebug planeDebug;
  float cityTerrainY = terrain.heightAt(0.f, 0.f);
  Shader cityShader("assets/shaders/model.vert", "assets/shaders/model.frag");
  City city("assets/models/city/source/city.glb", cityShader, physics,
            glm::vec3(0.f, cityTerrainY, 0.f), 1.0f);
  physics.city = &city;
  Enclosure enclosure(ENCLOSURE_SIZE, ENCLOSURE_SIZE, ALTITUDE_MAX);
  {
    physics.addStaticPlane({glm::vec3(0, -1, 0), -ALTITUDE_MAX, 0.75f, 0.05f});
    physics.addStaticPlane(
        {glm::vec3(1, 0, 0), -(ENCLOSURE_SIZE * .5f), 0.75f, 0.05f});
    physics.addStaticPlane(
        {glm::vec3(-1, 0, 0), -(ENCLOSURE_SIZE * .5f), 0.75f, 0.05f});
    physics.addStaticPlane(
        {glm::vec3(0, 0, 1), -(ENCLOSURE_SIZE * .5f), 0.75f, 0.05f});
    physics.addStaticPlane(
        {glm::vec3(0, 0, -1), -(ENCLOSURE_SIZE * .5f), 0.75f, 0.05f});
  }
  physics.terrainHeight = [&](float x, float z) -> float {
    return terrain.heightAt(x, z);
  };
  physics.terrainNormal = [&](float x, float z) -> glm::vec3 {
    return terrain.normalAt(x, z);
  };

  physics.addStaticPlane({glm::vec3(0, 1, 0), -200.f, 0.05f, 0.5f});

  static constexpr int SHADOW_RES = 2048;
  GLuint shadowFBO, shadowTex;
  glGenFramebuffers(1, &shadowFBO);
  glGenTextures(1, &shadowTex);
  glBindTexture(GL_TEXTURE_2D, shadowTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SHADOW_RES, SHADOW_RES,
               0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  float borderCol[] = {1, 1, 1, 1};
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderCol);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,
                  GL_COMPARE_REF_TO_TEXTURE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
  glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                         shadowTex, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  Shader shadowShader("assets/shaders/shadow.vert",
                      "assets/shaders/shadow.frag");

  static constexpr int NUM_PLANES = 4;
  std::array<Plane, NUM_PLANES> planes = {
      Plane({0.f, 200.f, 0.f}, 180.f, true),
      Plane({-80.f, 200.f, 120.f}, 180.f, false),
      Plane({80.f, 200.f, 120.f}, 180.f, false),
      Plane({0.f, 200.f, 240.f}, 180.f, false),
  };

  std::array<PlaneBody *, NUM_PLANES> planeBodies;
  for (int i = 0; i < NUM_PLANES; ++i) {
    auto *pb = new PlaneBody(planes[i].position, (i == 0) ? 180.f : 180.f);
    planeBodies[i] = pb;
    physics.addBody(&pb->body);
  }

  const std::vector<glm::vec3> rafaleNozzles = {
      glm::vec3(-0.75f, -0.25f, 4.8f),
      glm::vec3(0.75f, -0.25f, 4.8f),
  };
  std::array<Exhaust, NUM_PLANES> exhausts = {
      Exhaust(rafaleNozzles),
      Exhaust(rafaleNozzles),
      Exhaust(rafaleNozzles),
      Exhaust(rafaleNozzles),
  };

  Tracer tracer;
  float traceFireTimer = 0.f;
  constexpr float TRACER_FIRE_RATE = 0.06f;
  CameraState cam;
  cam.pos = planes[0].position - glm::vec3(0, 0, -CAM_DIST_MIN);

  bool running = true;
  Uint64 prev = SDL_GetTicks64();
  GLuint shadowFBO2 = 0, shadowTex2 = 0;
  glGenFramebuffers(1, &shadowFBO2);
  glGenTextures(1, &shadowTex2);
  glBindTexture(GL_TEXTURE_2D, shadowTex2);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 4096, 4096, 0,
               GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  float bc[] = {1, 1, 1, 1};
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, bc);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,
                  GL_COMPARE_REF_TO_TEXTURE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
  glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO2);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                         shadowTex2, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  while (running) {
    Uint64 now = SDL_GetTicks64();
    float dt = glm::clamp((now - prev) / 1000.f, 0.f, 0.05f);
    prev = now;
    float time = now / 1000.f;

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT)
        running = false;
      if (e.type == SDL_KEYDOWN && !e.key.repeat)
        if (e.key.keysym.sym == SDLK_ESCAPE)
          running = false;
    }
    const Uint8 *keys = SDL_GetKeyboardState(nullptr);

    for (int i = 0; i < NUM_PLANES; ++i) {
      PlaneInput in = planes[i].isPlayerControlled ? playerInput(keys)
                                                   : aiInput(planes[i], time);

      planes[i].updateAero(in, dt);

      planeBodies[i]->pushAeroForces(planes[i].speed, planes[i].thrustFraction,
                                     planes[i].yawRate, planes[i].pitchRate,
                                     dt);

      glm::quat bankRot =
          glm::angleAxis(planes[i].bankAngle,
                         planeBodies[i]->body.orientation * glm::vec3(0, 0, 1));
      planeBodies[i]->body.orientation =
          glm::normalize(bankRot * planes[i].orientation);
    }

    physics.step(dt);
    traceFireTimer += dt;
    if (keys[SDL_SCANCODE_KP_7] && traceFireTimer >= TRACER_FIRE_RATE) {
      traceFireTimer = 0.f;
      tracer.fire(planes[0].position, planes[0].modelMat, planes[0].orientation,
                  planes[0].speed);
    }
    tracer.update(dt);
    for (int i = 0; i < NUM_PLANES; ++i) {
      float newSpeed =
          planeBodies[i]->pullState(planes[i].position, planes[i].orientation);
      planes[i].speed = glm::clamp(newSpeed, SPEED_MIN, SPEED_MAX);
      planes[i].rebuildModelMat();

      if (i == 0 && false) {
        std::cout << "[player] " << planeBodies[0]->speedKMH() << " km/h\n";
      }
    }

    cam.update(planes[0], keys, dt,
               [&](float x, float z) { return terrain.heightAt(x, z); });

    float sunAngle = time * (glm::pi<float>() / 300.f);
    float sunElev = sin(sunAngle);
    glm::vec3 sunDir = glm::normalize(glm::vec3(cos(sunAngle), sunElev, 0.f));

    float terrainBelow =
        terrain.heightAt(planes[0].position.x, planes[0].position.z);
    float altitude = planes[0].position.y - terrainBelow;

    glm::vec3 lightUp = glm::abs(glm::dot(sunDir, glm::vec3(0, 1, 0))) > 0.99f
                            ? glm::vec3(1, 0, 0)
                            : glm::vec3(0, 1, 0);

    float footprint = glm::clamp(altitude * 0.35f, 20.f, 600.f);
    glm::vec3 sc1(planes[0].position.x, terrainBelow, planes[0].position.z);
    glm::mat4 lightView =
        glm::lookAt(sc1 + sunDir * (altitude + 200.f), sc1, lightUp);
    glm::mat4 lightProj = glm::ortho(-footprint, footprint, -footprint,
                                     footprint, 0.1f, altitude + 600.f);
    glm::mat4 lightSpaceMat = lightProj * lightView;

    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glViewport(0, 0, SHADOW_RES, SHADOW_RES);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.f, 4.f);
    shadowShader.use();
    for (int i = 0; i < NUM_PLANES; ++i) {
      shadowShader.setMat4("uLightSpaceMVP",
                           lightSpaceMat * planes[i].modelMat);
      rafale.drawDepth(planes[i].modelMat, shadowShader);
    }
    glDisable(GL_POLYGON_OFFSET_FILL);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glm::vec3 sc2(0.f, cityTerrainY, 0.f);
    glm::mat4 lightView2 = glm::lookAt(sc2 + sunDir * 4000.f, sc2, lightUp);
    glm::mat4 lightProj2 =
        glm::ortho(-1200.f, 1200.f, -1400.f, 1400.f, 0.1f, 8000.f);
    glm::mat4 lightSpaceMat2 = lightProj2 * lightView2;

    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO2);
    glViewport(0, 0, 4096, 4096);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.f, 4.f);
    shadowShader.use();
    for (int i = 0; i < NUM_PLANES; ++i) {
      shadowShader.setMat4("uLightSpaceMVP",
                           lightSpaceMat2 * planes[i].modelMat);
      rafale.drawDepth(planes[i].modelMat, shadowShader);
    }
    shadowShader.setMat4("uLightSpaceMVP", lightSpaceMat2 * city.modelMat());
    city.drawDepth(shadowShader);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (window.getSize().h == 0)
      window.size.h = 1;

    glm::mat4 view = cam.view(planes[0]);
    float speedT = (planes[0].speed - SPEED_MIN) / (SPEED_MAX - SPEED_MIN);
    float fov = glm::mix(FOV_BASE, FOV_MAX, glm::smoothstep(0.f, 1.f, speedT));
    glm::mat4 proj = glm::perspective(
        glm::radians(fov), (float)window.size.w / window.size.h, 0.5f, 40000.f);

    glViewport(0, 0, window.size.w, window.size.h);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    sky.draw(view, proj, sunDir, time);
    ocean.update(time);
    terrain.draw(view, proj, sunDir, sunElev, time, lightSpaceMat, shadowTex,
                 altitude);
    ocean.draw(view, proj, sunDir, sunElev, time);
    enclosure.draw(proj * view);
    for (int i = 0; i < NUM_PLANES; ++i) {
      rafale.draw(planes[i].modelMat, view, proj, sunDir, lightSpaceMat,
                  shadowTex);
      exhausts[i].update(planes[i].modelMat, planes[i].thrustFraction, dt);
      exhausts[i].draw(view, proj, cam.pos);
    }

    city.draw(view, proj, sunDir, lightSpaceMat2, shadowTex2);
    tracer.draw(proj * view, cam.pos);

    SDL_GL_SwapWindow(window.win);
  }

  for (int i = 0; i < NUM_PLANES; ++i)
    delete planeBodies[i];

  return 0;
}
