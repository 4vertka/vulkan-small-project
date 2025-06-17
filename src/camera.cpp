#include "./camera.hpp"

void Camera2D::cameraMovement(SDL_Event event) {

  switch (event.type) {
  case SDL_KEYDOWN:
    switch (event.key.keysym.sym) {
    case SDLK_w:
      cameraPosition.y -= cameraSpeed;
      break;
    case SDLK_s:
      cameraPosition.y += cameraSpeed;
      break;
    case SDLK_a:
      cameraPosition.x += cameraSpeed;
      break;
    case SDLK_d:
      cameraPosition.x -= cameraSpeed;
      break;
    }
  }
}
