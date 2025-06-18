#include "./camera.hpp"

void Camera2D::cameraMovement(SDL_Event event) {

  if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
    if (keyboardStateArray[SDL_SCANCODE_W] &&
        !(keyboardStateArray[SDL_SCANCODE_S])) {
      moveCamera(moveUp);
    } else if (!keyboardStateArray[SDL_SCANCODE_W] &&
               keyboardStateArray[SDL_SCANCODE_S]) {
      moveCamera(moveDown);
    }
    if (keyboardStateArray[SDL_SCANCODE_D] &&
        !keyboardStateArray[SDL_SCANCODE_A]) {
      moveCamera(moveRight);
    } else if (!keyboardStateArray[SDL_SCANCODE_D] &&
               keyboardStateArray[SDL_SCANCODE_A]) {
      moveCamera(moveLeft);
    }
  }

  /*switch (event.type) {
  case SDL_KEYDOWN:
    switch (event.key.keysym.sym) {
    case SDLK_w:
      moveCamera(moveUp);
      break;
    case SDLK_s:
      moveCamera(moveDown);
      break;
    case SDLK_a:
      moveCamera(moveLeft);
      break;
    case SDLK_d:
      moveCamera(moveRight);
      break;
    case SDLK_q:
      moveZoom(zoomIn);
      break;
    case SDLK_e:
      moveZoom(zoomOut);
      break;
    }
  }*/
}

void Camera2D::moveCamera(CameraMovement movementDirection) {
  if (movementDirection == moveLeft) {
    cameraPosition.x -= cameraSpeed;
  }
  if (movementDirection == moveRight) {
    cameraPosition.x += cameraSpeed;
  }
  if (movementDirection == moveUp) {
    cameraPosition.y += cameraSpeed;
  }
  if (movementDirection == moveDown) {
    cameraPosition.y -= cameraSpeed;
  }
}

void Camera2D::moveZoom(CameraZoom cameraZoom) {
  if (this->cameraZoom >= 2.0f)
    this->cameraZoom = 2.0f;
  if (this->cameraZoom <= 0.5f)
    this->cameraZoom = 0.5f;
  if (cameraZoom == zoomIn)
    this->cameraZoom += 0.1f;
  if (cameraZoom == zoomOut)
    this->cameraZoom -= 0.1f;
}
