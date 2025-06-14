#include "engine.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_vulkan.h>
#include <stdexcept>
#include <variant>
#include <vulkan/vulkan_core.h>

void VulkanEngine::initWindow() {
  SDL_Init(SDL_INIT_VIDEO);

  SDL_WindowFlags windowFlags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);
  _window = SDL_CreateWindow("vulkan window", SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED, _windowExtent.width,
                             _windowExtent.height, windowFlags);
}

void VulkanEngine::initVulkan() { createInstanceAndPhysicalDeviceAndQueue(); }

void VulkanEngine::mainLoop() {
  SDL_Event e;
  closeEngine = false;
  while (!closeEngine) {
    while (SDL_PollEvent(&e) != 0) {
      if (e.type == SDL_QUIT) {
        closeEngine = true;
      }
    }
  }
}

void VulkanEngine::cleanup() {
  vkDestroyDevice(_device, nullptr);

  DestroyDebugUtilsMessengerEXT(_instance, _debugMessenger, nullptr);

  vkDestroySurfaceKHR(_instance, _surface, nullptr);

  vkDestroyInstance(_instance, nullptr);

  SDL_DestroyWindow(_window);
}

void VulkanEngine::createInstanceAndPhysicalDeviceAndQueue() {
  vkb::InstanceBuilder builder;
  auto instance_return = builder.set_app_name("vulkan engine")
                             .request_validation_layers()
                             .use_default_debug_messenger()
                             .build();
  if (!instance_return) {
    throw std::runtime_error("failed to create instance");
  }

  vkb::Instance final_instance = instance_return.value();

  _instance = final_instance.instance;
  _debugMessenger = final_instance.debug_messenger;

  SDL_Vulkan_CreateSurface(_window, _instance, &_surface);

  vkb::PhysicalDeviceSelector selector{final_instance};
  auto physicalDeviceReturn =
      selector.set_minimum_version(1, 3).set_surface(_surface).select();

  if (!physicalDeviceReturn) {
    throw std::runtime_error("failed to select physical device");
  }

  vkb::DeviceBuilder deviceBuilder{physicalDeviceReturn.value()};
  vkb::Device vkbDevice = deviceBuilder.build().value();

  _device = vkbDevice.device;
  _physicalDevice = vkbDevice.physical_device;

  _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
  _graphicsQueueFamily =
      vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
}

void VulkanEngine::DestroyDebugUtilsMessengerEXT(
    VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks *pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}
