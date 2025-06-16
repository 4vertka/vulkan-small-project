#pragma once

#include <array>
#include <exception>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace vertexData {

struct Vertex {
  glm::vec2 position;
  glm::vec3 color;

  static VkVertexInputBindingDescription getBindingDescription();

  static std::array<VkVertexInputAttributeDescription, 2>
  getAttributeDescriptions();
};

const std::vector<Vertex> vertices = {{{0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}},
                                      {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
                                      {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}};

}; // namespace vertexData
