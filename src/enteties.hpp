#pragma once

#include "initMeshes.hpp"
#include <SDL2/SDL.h>
#include <vulkan/vulkan.h>

class Player {
public:
  Mesh *playerMesh = nullptr;
  enum playerDirections { UP, DOWN, LEFT, RIGHT };
  float playerSpeed = 5.0f;

  const Uint8 *keyboardStateArray = SDL_GetKeyboardState(nullptr);
  void playerMovement(SDL_Event e);
  void addMesh(Mesh &playerMesh) { this->playerMesh = &playerMesh; }
  void movePlayer(playerDirections direction);
  void stopMovement();
};
