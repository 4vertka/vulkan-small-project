#pragma once

#include <SDL2/SDL.h>
#include <glm/glm.hpp>

struct Camera2D {
  float cameraSpeed = 0.05f;
  glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 1.0f);

  void cameraMovement(SDL_Event event);
};
