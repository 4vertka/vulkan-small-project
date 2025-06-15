#include "../vk-bootstrap/src/VkBootstrap.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_vulkan.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

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

  VkCommandBuffer _commandBuffer;
  void createCommandBuffer();
};
