#include "../vk-bootstrap/src/VkBootstrap.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_vulkan.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "./initializers.hpp"
#include "./vertexData.hpp"

struct Mesh {
  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
  VkBuffer indexBuffer;
  VkDeviceMemory indexBufferMemory;
  uint16_t indexCount;
};

struct UniformBufferObject {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

class VulkanEngine {

public:
  void run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
  }

private:
  void initWindow();
  void initVulkan();
  void mainLoop();
  void cleanup();

  SDL_Window *_window;
  VkExtent2D _windowExtent{1700, 900};
  bool closeEngine = false;

  const int MAX_FRAMES_IN_FLIGHT = 2;

  VkDebugUtilsMessengerEXT _debugMessenger;
  void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                     VkDebugUtilsMessengerEXT debugMessenger,
                                     const VkAllocationCallbacks *pAllocator);
  VkSurfaceKHR _surface;

  // instance
  VkInstance _instance;
  void createInstanceAndPhysicalDeviceAndQueue();

  // device
  VkPhysicalDevice _physicalDevice{VK_NULL_HANDLE};
  VkDevice _device;
  void pickPhysicalDevice();

  VkQueue _graphicsQueue;
  VkQueue _presentQueue;
  uint32_t _graphicsQueueFamily;

  void createSwapchain();
  VkSwapchainKHR _swapchain;
  VkFormat _swapchainImageFormat;
  std::vector<VkImage> _swapchainImages;
  std::vector<VkImageView> _swapchainImageViews;
  VkExtent2D _swapchainExtent;

  VkPipelineLayout _pipelineLayout;
  void createGraphicsPipeline();
  VkShaderModule createShaderModule(const std::vector<char> &code);
  static std::vector<char> readFile(const std::string &filename);

  VkRenderingAttachmentInfoKHR createRenderingAttachmentInfo();
  VkRenderingInfoKHR
  createRenderingInfo(VkRenderingAttachmentInfoKHR &colorAttachmentInfo);
  VkPipeline _graphicsPipeline;

  VkCommandPool _commandPool;
  void createCommandPool();

  void createCommandBuffer();
  void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex,
                           uint32_t currentFrame);

  void drawFrame();

  void createSyncObject();
  void drawGeometry(VkCommandBuffer commandBuffer, uint32_t imageIndex,
                    uint32_t currentFrame);

  std::vector<VkCommandBuffer> _commandBuffers;
  std::vector<VkSemaphore> _imageAvailableSemaphores;
  std::vector<VkSemaphore> _renderFinishedSemaphores;
  std::vector<VkFence> _inFlightFences;

  bool _resized = false;

  uint32_t currentFrame = 0;

  void recreateSwapChain();
  void cleanupSwapChain();
  void createVertexBuffer();
  void createIndexBuffer();
  uint32_t findMemoryType(uint32_t typeFilter,
                          VkMemoryPropertyFlags properties);
  VkBuffer _vertexBuffer;
  VkDeviceMemory _vertexBufferMemory;
  VkBuffer _indexBuffer;
  VkDeviceMemory _indexBufferMemory;

  std::vector<VkBuffer> _uniformBuffers;
  std::vector<VkDeviceMemory> _uniformBufferMemory;
  std::vector<void *> _uniformBuffersMapped;

  std::vector<VkDescriptorSet> _descriptorSets;

  void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties, VkBuffer &buffer,
                    VkDeviceMemory &bufferMemory);

  void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

  VkDescriptorSetLayout _descriptorSetLayout;
  VkDescriptorPool _descriptorPool;
  void createDescriptorSetLayout();
  void createDescriptorPool();
  void createDescriptorSet();

  void createUniformBuffers();

  void updateUniformBuffer(uint32_t currentImage);

  std::vector<Mesh> _meshes;
  void createMesh(const std::vector<vertexData::Vertex> &vertices,
                  const std::vector<uint16_t> &indices);

  void uploadToBuffer(const void *data, VkDeviceSize size, VkBuffer dstBuffer);
};
