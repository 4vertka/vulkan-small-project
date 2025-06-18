#include "./enteties.hpp"
#include <SDL2/SDL_events.h>

void Player::playerMovement(SDL_Event event) {

  if (!playerMesh)
    return;

  if (event.type == SDL_KEYDOWN) {
    switch (event.key.keysym.sym) {
    case SDLK_w:
      movePlayer(UP);
      break;
    case SDLK_s:
      movePlayer(DOWN);
      break;
    case SDLK_a:
      movePlayer(LEFT);
      break;
    case SDLK_d:
      movePlayer(RIGHT);
      break;
    }
  } else if (event.type == SDL_KEYUP) {
    switch (event.key.keysym.sym) {
    case SDLK_w:
    case SDLK_s:
      playerMesh->velocity.y = 0;
      break;
    case SDLK_a:
    case SDLK_d:
      playerMesh->velocity.x = 0;
      break;
    }
  }
}

void Player::movePlayer(playerDirections direction) {
  if (direction == UP) {
    playerMesh->velocity.y = playerSpeed;
  }
  if (direction == DOWN) {
    playerMesh->velocity.y = -playerSpeed;
  }
  if (direction == LEFT) {
    playerMesh->velocity.x = -playerSpeed;
  }
  if (direction == RIGHT) {
    playerMesh->velocity.x = playerSpeed;
  }
}

void Player::stopMovement() {
  if (playerMesh) {
    playerMesh->velocity = glm::vec3(0.0f);
  }
}
