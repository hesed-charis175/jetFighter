#pragma once
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

static constexpr float SPEED_MIN = 22.f;
static constexpr float SPEED_MAX = 600.f;
static constexpr float THRUST_ACC = 150.f;
static constexpr float DRAG_DEC = 60.f;
static constexpr float BRAKE_DEC = 160.f;

static constexpr float YAW_ACC = 0.05f;
static constexpr float YAW_MAX = 0.15f;
static constexpr float PITCH_ACC = 1.6f;
static constexpr float PITCH_MAX = 2.2f;
static constexpr float ANG_DAMPING = 2.3f;

static constexpr float BANK_RATE = 0.8f;
static constexpr float BANK_MAX = glm::radians(15.f);

static constexpr float CAM_DIST_MIN = 24.f;
static constexpr float CAM_DIST_MAX = 34.f;
static constexpr float CAM_HEIGHT = 7.f;
static constexpr float PLANE_SCALE = 1.1f;
static constexpr float FOV_BASE = 70.f;
static constexpr float FOV_MAX = 80.f;

static constexpr float ENCLOSURE_SIZE = 7500.f;
static constexpr float ALTITUDE_MAX = 8000.0f;
