#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.h>

struct Mesh {
  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
  VkBuffer indexBuffer;
  VkDeviceMemory indexBufferMemory;
  uint16_t indexCount;

  VkDescriptorSet descriptorSet;

  VkImage textureImage;
  VkImageView textureImageView;
  VkSampler textureSampler;
  VkDeviceMemory textureImageMemory;

  glm::mat4 transform;
  glm::vec3 position = glm::vec3(0.0f);
  glm::vec3 velocity = glm::vec3(0.0f);
  float rotation = 0.0f;
  glm::vec3 scale = glm::vec3(1.0f);

  void update(float deltaTime) {
    position += velocity * deltaTime;
    updateTransform();
  }

  void updateTransform() {
    transform = glm::translate(glm::mat4(1.0f), position) *
                glm::rotate(glm::mat4(1.0f), rotation, glm::vec3(0, 0, 1)) *
                glm::scale(glm::mat4(1.0f), scale);
  }
};
