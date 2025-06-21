#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

struct Mesh {
  VkBuffer vertexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
  VkBuffer indexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
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

  bool plyerMesh = false;

  void update(float deltaTime) {
    position += velocity * deltaTime;
    updateTransform();
  }

  void updateTransform() {
    transform = glm::translate(glm::mat4(1.0f), position) *
                glm::rotate(glm::mat4(1.0f), rotation, glm::vec3(0, 0, 1)) *
                glm::scale(glm::mat4(1.0f), scale);
  }

  void cleanup(VkDevice device) {
    if (textureImageView != VK_NULL_HANDLE) {
      vkDestroyImageView(device, textureImageView, nullptr);
      textureImageView = VK_NULL_HANDLE;
    }
    if (textureImage != VK_NULL_HANDLE) {
      vkDestroyImage(device, textureImage, nullptr);
      textureImage = VK_NULL_HANDLE;
    }
    if (textureImageMemory != VK_NULL_HANDLE) {
      vkFreeMemory(device, textureImageMemory, nullptr);
      textureImageMemory = VK_NULL_HANDLE;
    }
    if (textureSampler != VK_NULL_HANDLE) {
      vkDestroySampler(device, textureSampler, nullptr);
      textureSampler = VK_NULL_HANDLE;
    }
    if (vertexBuffer != VK_NULL_HANDLE) {
      vkDestroyBuffer(device, vertexBuffer, nullptr);
      vertexBuffer = VK_NULL_HANDLE;
    }
    if (vertexBufferMemory != VK_NULL_HANDLE) {
      vkFreeMemory(device, vertexBufferMemory, nullptr);
      vertexBufferMemory = VK_NULL_HANDLE;
    }
    if (indexBuffer != VK_NULL_HANDLE) {
      vkDestroyBuffer(device, indexBuffer, nullptr);
      indexBuffer = VK_NULL_HANDLE;
    }
    if (indexBufferMemory != VK_NULL_HANDLE) {
      vkFreeMemory(device, indexBufferMemory, nullptr);
      indexBufferMemory = VK_NULL_HANDLE;
    }
  }
};

struct Tile {
  uint16_t type;
};

struct Tilemap {
public:
  std::vector<std::vector<Tile>> tiles;
  int width;
  int height;

  Tilemap(int w, int h) : width(w), height(h) {
    tiles.resize(height, std::vector<Tile>(width));

    for (auto &row : tiles) {
      for (auto &tile : row) {
        tile.type = 0;
      }
    }
  }

  void setTile(int x, int y, uint16_t type) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
      tiles[y][x].type = type;
    }
  }

  Tile getTile(int x, int y) const {
    if (x >= 0 && x < width && y >= 0 && y < height) {
      return tiles[y][x];
    }

    return Tile{0};
  }
};
