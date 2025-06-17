#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_stdinc.h>
#include <glm/glm.hpp>

struct Camera2D {

  enum CameraMovement {
    moveLeft,
    moveRight,
    moveUp,
    moveDown,
  };
  enum CameraZoom { zoomIn, zoomOut };

  float cameraSpeed = 0.05f;
  glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 1.0f);
  float cameraZoom = 1.0f;
  const Uint8 *keyboardStateArray = SDL_GetKeyboardState(nullptr);

  void cameraMovement(SDL_Event event);
  void moveCamera(CameraMovement movementDirection);
  void moveZoom(CameraZoom cameraZoom);
};
