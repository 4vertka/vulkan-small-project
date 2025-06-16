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
  void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

  void drawFrame();

  void createSyncObject();
  void drawGeometry(VkCommandBuffer commandBuffer, uint32_t imageIndex);

  std::vector<VkCommandBuffer> _commandBuffers;
  std::vector<VkSemaphore> _imageAvailableSemaphores;
  std::vector<VkSemaphore> _renderFinishedSemaphores;
  std::vector<VkFence> _inFlightFences;

  bool _resized = false;

  uint32_t currentFrame = 0;

  void recreateSwapChain();
  void cleanupSwapChain();
};
