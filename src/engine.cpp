#include "engine.hpp"
#include "camera.hpp"
#include "initializers.hpp"
#include "vertexData.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_hidapi.h>
#include <SDL2/SDL_joystick.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_vulkan.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <endian.h>
#include <functional>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vector_relational.hpp>
#include <immintrin.h>
#include <stdexcept>
#include <variant>
#include <vulkan/vulkan_core.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void VulkanEngine::initWindow() {
  SDL_Init(SDL_INIT_VIDEO);

  SDL_WindowFlags windowFlags =
      (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
  _window = SDL_CreateWindow("vulkan window", SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED, _windowExtent.width,
                             _windowExtent.height, windowFlags);
}

void VulkanEngine::initVulkan() {
  createInstanceAndPhysicalDeviceAndQueue();
  createSwapchain();
  createImageViews();
  createDescriptorSetLayout();
  createGraphicsPipeline();
  createCommandPool();
  // createVertexBuffer();
  // createIndexBuffer();
  // createTextureImage();
  // createTextureImageView();
  // createTextureSampler();

  createUniformBuffers();
  createDescriptorPool();

  createMap();

  createAllMeshes();

  // createDescriptorSet();
  createCommandBuffer();
  createSyncObject();
}

void VulkanEngine::mainLoop() {

  initImGUI();

  SDL_Event e;
  closeEngine = false;
  auto lastTime = std::chrono::high_resolution_clock::now();

  while (!closeEngine) {
    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime =
        std::chrono::duration<float>(currentTime - lastTime).count();
    lastTime = currentTime;

    updateMeshes(deltaTime);

    while (SDL_PollEvent(&e) != 0) {
      processInput(e);

      ImGui_ImplSDL2_ProcessEvent(&e);
    }

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Example bug");
    if (ImGui::Button("Camera Mode")) {
      _camereMode = true;
      _playerMode = false;
    }
    if (ImGui::Button("Player Mode")) {
      _camereMode = false;
      _playerMode = true;
    }
    if (ImGui::Button("Quit")) {
      closeEngine = true;
    }

    ImGui::End();
    ImGui::Render();

    drawFrame();
  }
}

void VulkanEngine::cleanup() {
  vkDeviceWaitIdle(_device);

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  cleanupSwapChain();

  for (auto &mesh : _meshes) {
    mesh.cleanup(_device);
  }
  _meshes.clear();

  if (_graphicsPipeline != VK_NULL_HANDLE) {
    vkDestroyPipeline(_device, _graphicsPipeline, nullptr);
    _graphicsPipeline = VK_NULL_HANDLE;
  }
  if (_pipelineLayout != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(_device, _pipelineLayout, nullptr);
    _pipelineLayout = VK_NULL_HANDLE;
  }

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (_uniformBuffers[i] != VK_NULL_HANDLE) {
      vkDestroyBuffer(_device, _uniformBuffers[i], nullptr);
      _uniformBuffers[i] = VK_NULL_HANDLE;
    }
    if (_uniformBufferMemory[i] != VK_NULL_HANDLE) {
      vkFreeMemory(_device, _uniformBufferMemory[i], nullptr);
      _uniformBufferMemory[i] = VK_NULL_HANDLE;
    }
  }

  if (_descriptorPool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(_device, _descriptorPool, nullptr);
    _descriptorPool = VK_NULL_HANDLE;
  }
  if (_descriptorSetLayout != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(_device, _descriptorSetLayout, nullptr);
    _descriptorSetLayout = VK_NULL_HANDLE;
  }

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (_imageAvailableSemaphores[i] != VK_NULL_HANDLE) {
      vkDestroySemaphore(_device, _imageAvailableSemaphores[i], nullptr);
      _imageAvailableSemaphores[i] = VK_NULL_HANDLE;
    }
    if (_inFlightFences[i] != VK_NULL_HANDLE) {
      vkDestroyFence(_device, _inFlightFences[i], nullptr);
      _inFlightFences[i] = VK_NULL_HANDLE;
    }
  }

  for (auto &semaphore : _renderFinishedSemaphores) {
    if (semaphore != VK_NULL_HANDLE) {
      vkDestroySemaphore(_device, semaphore, nullptr);
      semaphore = VK_NULL_HANDLE;
    }
  }

  if (_commandPool != VK_NULL_HANDLE) {
    vkDestroyCommandPool(_device, _commandPool, nullptr);
    _commandPool = VK_NULL_HANDLE;
  }

  if (_device != VK_NULL_HANDLE) {
    vkDestroyDevice(_device, nullptr);
    _device = VK_NULL_HANDLE;
  }

  if (_debugMessenger != VK_NULL_HANDLE) {
    DestroyDebugUtilsMessengerEXT(_instance, _debugMessenger, nullptr);
    _debugMessenger = VK_NULL_HANDLE;
  }

  if (_surface != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    _surface = VK_NULL_HANDLE;
  }

  if (_instance != VK_NULL_HANDLE) {
    vkDestroyInstance(_instance, nullptr);
    _instance = VK_NULL_HANDLE;
  }

  if (_window != nullptr) {
    SDL_DestroyWindow(_window);
    _window = nullptr;
  }

  SDL_Quit();
}

void VulkanEngine::createInstanceAndPhysicalDeviceAndQueue() {
  vkb::InstanceBuilder builder;
  auto instance_return = builder.set_app_name("vulkan engine")
                             .request_validation_layers()
                             .use_default_debug_messenger()
                             .require_api_version(1, 3, 0)
                             .build();
  if (!instance_return) {
    throw std::runtime_error("failed to create instance");
  }

  vkb::Instance final_instance = instance_return.value();

  _instance = final_instance.instance;
  _debugMessenger = final_instance.debug_messenger;

  SDL_Vulkan_CreateSurface(_window, _instance, &_surface);

  VkPhysicalDeviceVulkan13Features features13{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
  features13.dynamicRendering = true;
  features13.synchronization2 = true;

  VkPhysicalDeviceVulkan12Features features12{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
  features12.bufferDeviceAddress = true;
  features12.descriptorIndexing = true;

  VkPhysicalDeviceFeatures deviceFeatures{};
  deviceFeatures.samplerAnisotropy = VK_TRUE;

  // features13.pNext = &features12;

  vkb::PhysicalDeviceSelector selector{final_instance};
  vkb::PhysicalDevice physicalDeviceReturn =
      selector.set_minimum_version(1, 3)
          .set_required_features_13(features13)
          .set_required_features_12(features12)
          .set_required_features(deviceFeatures)
          .set_surface(_surface)
          .select()
          .value();

  if (!physicalDeviceReturn) {
    throw std::runtime_error("failed to select physical device");
  }

  vkb::DeviceBuilder deviceBuilder{physicalDeviceReturn};
  vkb::Device vkbDevice = deviceBuilder.build().value();

  _device = vkbDevice.device;
  _physicalDevice = vkbDevice.physical_device;

  _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
  _presentQueue = vkbDevice.get_queue(vkb::QueueType::present).value();
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

void VulkanEngine::createSwapchain() {

  vkb::SwapchainBuilder swapchainBuilder{_physicalDevice, _device, _surface};

  vkb::Swapchain swapchain_return =
      swapchainBuilder
          .set_desired_format(VkSurfaceFormatKHR{
              .format = VK_FORMAT_B8G8R8A8_SRGB,
              .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
          .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
          .set_desired_extent(_windowExtent.width, _windowExtent.height)
          .set_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
          .build()
          .value();

  if (!swapchain_return) {
    throw std::runtime_error("failed to create swapchain");
  }

  _swapchainExtent = swapchain_return.extent;
  _swapchain = swapchain_return.swapchain;
  _swapchainImages = swapchain_return.get_images().value();
  _swapchainImageViews = swapchain_return.get_image_views().value();
  _swapchainImageFormat = swapchain_return.image_format;

  std::cout << "swapchain created\n";
}

void VulkanEngine::createGraphicsPipeline() {

  auto vertShaderCode = readFile("../shaders/shader.vert.spv");
  auto fragShaderCode = readFile("../shaders/shader.frag.spv");

  VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
  VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
  vertShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
  fragShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                    fragShaderStageInfo};

  std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                               VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynamicState.pDynamicStates = dynamicStates.data();

  auto bindingDescription = vertexData::Vertex::getBindingDescription();
  auto attributeDescriptions = vertexData::Vertex::getAttributeDescriptions();

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributeDescriptions.size());
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)_swapchainExtent.width;
  viewport.height = (float)_swapchainExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = _swapchainExtent;

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f;
  rasterizer.depthBiasClamp = 0.0f;
  rasterizer.depthBiasSlopeFactor = 0.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f;
  multisampling.pSampleMask = nullptr;
  multisampling.alphaToCoverageEnable = VK_FALSE;
  multisampling.alphaToOneEnable = VK_FALSE;

  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f;
  colorBlending.blendConstants[1] = 0.0f;
  colorBlending.blendConstants[2] = 0.0f;
  colorBlending.blendConstants[3] = 0.0f;

  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(glm::mat4);

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &_descriptorSetLayout;
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

  if (vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr,
                             &_pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout");
  }

  VkFormat colorFormat = VK_FORMAT_B8G8R8A8_SRGB;

  VkPipelineRenderingCreateInfo renderingCreateInfo{};
  renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  renderingCreateInfo.colorAttachmentCount = 1;
  renderingCreateInfo.pColorAttachmentFormats = &colorFormat;
  renderingCreateInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
  renderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = _pipelineLayout;
  pipelineInfo.renderPass = VK_NULL_HANDLE;
  pipelineInfo.subpass = 0;

  pipelineInfo.pNext = &renderingCreateInfo;

  vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                            &_graphicsPipeline);

  vkDestroyShaderModule(_device, fragShaderModule, nullptr);
  vkDestroyShaderModule(_device, vertShaderModule, nullptr);
}

std::vector<char> VulkanEngine::readFile(const std::string &filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("failed to open file");
  }

  size_t fileSize = (size_t)file.tellg();
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);

  file.close();

  return buffer;
}

VkShaderModule VulkanEngine::createShaderModule(const std::vector<char> &code) {
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module");
  }

  return shaderModule;
}

void VulkanEngine::createCommandPool() {
  VkCommandPoolCreateInfo commandPoolInfo{};
  commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  commandPoolInfo.pNext = nullptr;
  commandPoolInfo.queueFamilyIndex = _graphicsQueueFamily;
  commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  if (vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_commandPool) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create command pool");
  }
}

void VulkanEngine::createCommandBuffer() {

  _commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = _commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)_commandBuffers.size();

  if (vkAllocateCommandBuffers(_device, &allocInfo, _commandBuffers.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers");
  }
}

void VulkanEngine::recordCommandBuffer(VkCommandBuffer commandBuffer,
                                       uint32_t imageIndex,
                                       uint32_t currentFrame) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0;
  beginInfo.pInheritanceInfo = nullptr;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer");
  }

  // vkinit::transitionImage(commandBuffer, _swapchainImages[imageIndex],
  //                         VK_IMAGE_LAYOUT_UNDEFINED,
  //                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  // vkinit::transitionImageLayout(_swapchainImages[imageIndex], , VkImageLayout
  // oldLayout, VkImageLayout newLayout, VkCommandPool commandPool, VkDevice
  // device, VkQueue graphicsQueue)

  vkinit::transitionImageLayout(_swapchainImages[imageIndex],
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                _commandPool, _device, _graphicsQueue);

  drawGeometry(commandBuffer, imageIndex, currentFrame);

  vkinit::transitionImageLayout(
      _swapchainImages[imageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, _commandPool, _device, _graphicsQueue);

  // vkinit::transitionImage(commandBuffer, _swapchainImages[imageIndex],
  //                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  //                         VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer");
  }
}

void VulkanEngine::drawFrame() {
  vkWaitForFences(_device, 1, &_inFlightFences[currentFrame], VK_TRUE,
                  UINT64_MAX);

  uint32_t imageIndex;
  VkResult result = vkAcquireNextImageKHR(
      _device, _swapchain, UINT64_MAX, _imageAvailableSemaphores[currentFrame],
      VK_NULL_HANDLE, &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain();
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image");
  }

  // updateUniformBuffer(currentFrame);

  vkResetFences(_device, 1, &_inFlightFences[currentFrame]);

  vkResetCommandBuffer(_commandBuffers[currentFrame], 0);

  recordCommandBuffer(_commandBuffers[currentFrame], imageIndex, currentFrame);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {_imageAvailableSemaphores[currentFrame]};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &_commandBuffers[currentFrame];

  VkSemaphore signalSemaphores[] = {_renderFinishedSemaphores[imageIndex]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  if (vkQueueSubmit(_graphicsQueue, 1, &submitInfo,
                    _inFlightFences[currentFrame]) != VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer");
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[] = {_swapchain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex;

  result = vkQueuePresentKHR(_presentQueue, &presentInfo);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      _resized) {
    _resized = false;
    recreateSwapChain();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image");
  }

  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanEngine::createSyncObject() {

  _imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  _renderFinishedSemaphores.resize(_swapchainImages.size());
  _inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(_device, &semaphoreInfo, nullptr,
                          &_imageAvailableSemaphores[i]) != VK_SUCCESS ||

        vkCreateFence(_device, &fenceInfo, nullptr, &_inFlightFences[i]) !=
            VK_SUCCESS) {

      throw std::runtime_error("failed to create semaphores");
    }
  }

  for (size_t i = 0; i < _swapchainImages.size(); i++) {
    if (vkCreateSemaphore(_device, &semaphoreInfo, nullptr,
                          &_renderFinishedSemaphores[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create rennder finished semaphore");
    }
  }
}

void VulkanEngine::drawGeometry(VkCommandBuffer commandBuffer,
                                uint32_t imageIndex, uint32_t currentFrame) {
  VkImageView currentImageView = _swapchainImageViews[imageIndex];

  VkRenderingAttachmentInfoKHR colorAttachmentInfo{};
  colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
  colorAttachmentInfo.imageView = currentImageView;
  colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  colorAttachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;
  colorAttachmentInfo.resolveImageView = VK_NULL_HANDLE;
  colorAttachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachmentInfo.clearValue.color = {0.0f, 0.0f, 1.0f, 1.0f};

  VkRenderingInfoKHR renderingInfo{};
  renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
  renderingInfo.renderArea.offset = {0, 0};
  renderingInfo.renderArea.extent = _swapchainExtent;
  renderingInfo.layerCount = 1;
  renderingInfo.viewMask = 0;
  renderingInfo.colorAttachmentCount = 1;
  renderingInfo.pColorAttachments = &colorAttachmentInfo;
  renderingInfo.pDepthAttachment = nullptr;
  renderingInfo.pStencilAttachment = nullptr;

  vkCmdBeginRendering(commandBuffer, &renderingInfo);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    _graphicsPipeline);

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)_swapchainExtent.width;
  viewport.height = (float)_swapchainExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = _swapchainExtent;
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  for (const auto &mesh : _meshes) {

    updateUniformBuffer(currentFrame);
    vkCmdPushConstants(commandBuffer, _pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4),
                       &mesh.transform);
    VkBuffer vertexBuffers[] = {mesh.vertexBuffer};
    VkDeviceSize offsets[] = {0};

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer, 0,
                         VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            _pipelineLayout, 0, 1, &mesh.descriptorSet, 0,
                            nullptr);

    vkCmdDrawIndexed(commandBuffer, mesh.indexCount, 1, 0, 0, 0);
  }

  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

  vkCmdEndRendering(commandBuffer);
}

void VulkanEngine::recreateSwapChain() {
  int width = 0, height = 0;

  SDL_GetWindowSize(_window, &width, &height);
  if (width <= 0 || height <= 0) {
    _resized = true;
  }

  vkDeviceWaitIdle(_device);

  cleanupSwapChain();

  createSwapchain();
  // createImageViews();
}

void VulkanEngine::cleanupSwapChain() {
  for (auto imageView : _swapchainImageViews) {
    vkDestroyImageView(_device, imageView, nullptr);
  }

  _swapchainImageViews.clear();

  vkDestroySwapchainKHR(_device, _swapchain, nullptr);
}

void VulkanEngine::createVertexBuffer() {

  VkDeviceSize bufferSize =
      sizeof(vertexData::vertices[0]) * vertexData::vertices.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingBuffer, stagingBufferMemory);

  void *data;
  vkMapMemory(_device, stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, vertexData::vertices.data(), (size_t)bufferSize);
  vkUnmapMemory(_device, stagingBufferMemory);

  createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _vertexBuffer, _vertexBufferMemory);

  copyBuffer(stagingBuffer, _vertexBuffer, bufferSize);

  vkDestroyBuffer(_device, stagingBuffer, nullptr);
  vkFreeMemory(_device, stagingBufferMemory, nullptr);
}

uint32_t VulkanEngine::findMemoryType(uint32_t typeFilter,
                                      VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
                                    properties) == properties) {
      return i;
    }
  }
  throw std::runtime_error("failed to find suitable memory type");
}

void VulkanEngine::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                VkMemoryPropertyFlags properties,
                                VkBuffer &buffer,
                                VkDeviceMemory &bufferMemory) {

  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to create buffer");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(_device, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
      findMemoryType(memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(_device, &allocInfo, nullptr, &bufferMemory) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate vertex buffer memory");
  }

  vkBindBufferMemory(_device, buffer, bufferMemory, 0);
}

void VulkanEngine::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer,
                              VkDeviceSize size) {

  VkCommandBuffer commandBuffer =
      vkinit::beginSingleTimeCommands(_commandPool, _device);

  VkBufferCopy copyRegion{};
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  vkinit::endSingleTimeCommands(commandBuffer, _graphicsQueue, _device,
                                _commandPool);
}

void VulkanEngine::createIndexBuffer() {
  VkDeviceSize bufferSize =
      sizeof(vertexData::indices[0]) * vertexData::indices.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingBuffer, stagingBufferMemory);

  void *data;
  vkMapMemory(_device, stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, vertexData::indices.data(), (size_t)bufferSize);
  vkUnmapMemory(_device, stagingBufferMemory);

  createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _indexBuffer, _indexBufferMemory);

  copyBuffer(stagingBuffer, _indexBuffer, bufferSize);

  vkDestroyBuffer(_device, stagingBuffer, nullptr);
  vkFreeMemory(_device, stagingBufferMemory, nullptr);
}

void VulkanEngine::createDescriptorSetLayout() {
  VkDescriptorSetLayoutBinding uboLayoutBinding{};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  uboLayoutBinding.pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutBinding samplerlayoutBinding{};
  samplerlayoutBinding.binding = 1;
  samplerlayoutBinding.descriptorCount = 1;
  samplerlayoutBinding.descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerlayoutBinding.pImmutableSamplers = nullptr;
  samplerlayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  std::array<VkDescriptorSetLayoutBinding, 2> binding = {uboLayoutBinding,
                                                         samplerlayoutBinding};

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(binding.size());
  layoutInfo.pBindings = binding.data();

  if (vkCreateDescriptorSetLayout(_device, &layoutInfo, nullptr,
                                  &_descriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout");
  }
}

void VulkanEngine::createUniformBuffers() {
  VkDeviceSize bufferSize = sizeof(UniformBufferObject);

  _uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  _uniformBufferMemory.resize(MAX_FRAMES_IN_FLIGHT);
  _uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 _uniformBuffers[i], _uniformBufferMemory[i]);

    vkMapMemory(_device, _uniformBufferMemory[i], 0, bufferSize, 0,
                &_uniformBuffersMapped[i]);
  }
}

void VulkanEngine::updateUniformBuffer(uint32_t currentImage) {
  static auto startTime = std::chrono::high_resolution_clock::now();

  auto currentTime = std::chrono::high_resolution_clock::now();
  float time = std::chrono::duration<float, std::chrono::seconds::period>(
                   currentTime - startTime)
                   .count();

  UniformBufferObject ubo{};
  // ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f),
  //                         glm::vec3(0.0f, 0.0f, 1.0f));

  // ubo.view =
  //     glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f),
  //                 glm::vec3(0.0f, 0.0f, 1.0f));

  // ubo.view = glm::lookAt(_cameraPos, _cameraPos + _cameraFront, _cameraUp);

  ubo.view = glm::translate(glm::mat4(1.0f), -_camera2d.cameraPosition);

  float aspect = _swapchainExtent.width / (float)_swapchainExtent.height;
  float orthoSize = 2.0f / _camera2d.cameraZoom;

  float worldWidth = 10.0f;
  float worldHeight = worldWidth / aspect;

  worldWidth /= _camera2d.cameraZoom;
  worldHeight /= _camera2d.cameraZoom;

  ubo.proj = glm::ortho(-worldWidth / 2, worldWidth / 2, -worldHeight / 2,
                        worldHeight / 2, -1.0f, 1.0f);

  // ubo.proj = glm::ortho(-orthoSize * aspect, orthoSize * aspect, -orthoSize,
  //                       orthoSize, -1.0f, 1.0f);

  // ubo.proj = glm::perspective(
  //     glm::radians(45.0f),
  //    _swapchainExtent.width / (float)_swapchainExtent.height, 0.1f, 10.0f);

  ubo.proj[1][1] *= -1;

  memcpy(_uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void VulkanEngine::createDescriptorPool() {

  uint32_t maxMashes = 100;
  uint32_t totalDescriptorSets = maxMashes * MAX_FRAMES_IN_FLIGHT;

  std::array<VkDescriptorPoolSize, 2> poolSizes{};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = totalDescriptorSets;
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount = maxMashes;

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = totalDescriptorSets + 1000;

  if (vkCreateDescriptorPool(_device, &poolInfo, nullptr, &_descriptorPool) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool");
  }
}

/*void VulkanEngine::createDescriptorSet() {
  std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                             _descriptorSetLayout);

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = _descriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  allocInfo.pSetLayouts = layouts.data();

  _descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

  if (vkAllocateDescriptorSets(_device, &allocInfo, _descriptorSets.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets");
  }

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = _uniformBuffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    // imageInfo.imageView = _textureImageView;
    // imageInfo.sampler = _textureSampler;

    std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = _descriptorSets[i];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = _descriptorSets[i];
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(_device,
                           static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);
  }
}*/

void VulkanEngine::createMesh(const std::vector<vertexData::Vertex> &vertices,
                              const std::vector<uint16_t> &indices,
                              const glm::mat4 &inittialTransform,
                              glm::vec3 position, const char *texturePath) {

  Mesh newMesh;
  newMesh.indexCount = static_cast<uint32_t>(indices.size());
  newMesh.transform = inittialTransform;

  newMesh.position = position;

  VkDeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();
  createBuffer(vertexBufferSize,
               VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                   VK_BUFFER_USAGE_TRANSFER_DST_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, newMesh.vertexBuffer,
               newMesh.vertexBufferMemory);

  VkDeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();
  createBuffer(indexBufferSize,
               VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                   VK_BUFFER_USAGE_TRANSFER_DST_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, newMesh.indexBuffer,
               newMesh.indexBufferMemory);

  uploadToBuffer(vertices.data(), vertexBufferSize, newMesh.vertexBuffer);
  uploadToBuffer(indices.data(), indexBufferSize, newMesh.indexBuffer);

  createTextureImage(texturePath, newMesh.textureImage,
                     newMesh.textureImageMemory);
  createTextureImageView(newMesh.textureImage, newMesh.textureImageView);
  createTextureSampler(newMesh.textureSampler);

  createMeshDescriptorSet(newMesh);

  _meshes.push_back(newMesh);
}

void VulkanEngine::uploadToBuffer(const void *data, VkDeviceSize size,
                                  VkBuffer dstBuffer) {
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingBuffer, stagingBufferMemory);

  void *mappedData;
  vkMapMemory(_device, stagingBufferMemory, 0, size, 0, &mappedData);
  memcpy(mappedData, data, (size_t)size);
  vkUnmapMemory(_device, stagingBufferMemory);

  copyBuffer(stagingBuffer, dstBuffer, size);

  vkDestroyBuffer(_device, stagingBuffer, nullptr);
  vkFreeMemory(_device, stagingBufferMemory, nullptr);
}

void VulkanEngine::createAllMeshes() {

  createMesh(vertexData::vertices, vertexData::indices,
             glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 1.0f, 0.0f)) *
                 glm::scale(glm::mat4(1.0f), glm::vec3(1.0f)),
             glm::vec3(-2.0f, 1.0f, 0.0f), "../textures/forest-2.png");

  createMesh(vertexData::vertices, vertexData::indices,
             glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, -1.0f, 0.0f)) *
                 glm::scale(glm::mat4(1.0f), glm::vec3(1.0f)),
             glm::vec3(3.0f, -1.0f, 0.0f),
             "../textures/statue-1275469_640.jpg");
}

void VulkanEngine::processInput(SDL_Event event) {

  switch (event.type) {
  case SDL_QUIT:
    closeEngine = true;
    break;
  }
  if (_camereMode) {
    _camera2d.cameraMovement(event);
  }
  if (_playerMode) {
    if (!_meshes.empty()) {
      auto &playerMesh = _meshes[0];
      _player2d.addMesh(playerMesh);
      _player2d.playerMovement(event);
    }
  }
}

void VulkanEngine::initImGUI() {
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  ImGui::StyleColorsDark();

  ImGui_ImplSDL2_InitForVulkan(_window);
  ImGui_ImplVulkan_InitInfo initInfo{};
  initInfo.Instance = _instance;
  initInfo.PhysicalDevice = _physicalDevice;
  initInfo.Device = _device;
  initInfo.Queue = _graphicsQueue;
  initInfo.QueueFamily = _graphicsQueueFamily;
  initInfo.DescriptorPool = _descriptorPool;
  initInfo.Subpass = 0;
  initInfo.MinImageCount = 3;
  initInfo.ImageCount = 3;
  initInfo.UseDynamicRendering = true;

  initInfo.PipelineRenderingCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
  initInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
  initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats =
      &_swapchainImageFormat;
  initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

  ImGui_ImplVulkan_Init(&initInfo);
}

void VulkanEngine::updateMeshes(float deltaTime) {
  for (auto &mesh : _meshes) {
    mesh.update(deltaTime);
  }
}

void VulkanEngine::createTextureImage(const char *filePath,
                                      VkImage &textureImage,
                                      VkDeviceMemory &textureImageMemory) {
  int texWidth, texHeight, texChannels;
  stbi_uc *pixels =
      stbi_load(filePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

  VkDeviceSize imageSize = texWidth * texHeight * 4;

  if (!pixels) {
    throw std::runtime_error("failed to load texture image");
  }

  VkBuffer staginBuffer;
  VkDeviceMemory stagingBufferMemory;

  createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               staginBuffer, stagingBufferMemory);

  void *data;
  vkMapMemory(_device, stagingBufferMemory, 0, imageSize, 0, &data);
  memcpy(data, pixels, static_cast<size_t>(imageSize));
  vkUnmapMemory(_device, stagingBufferMemory);

  stbi_image_free(pixels);

  createImage(
      texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

  vkinit::transitionImageLayout(textureImage, VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                _commandPool, _device, _graphicsQueue);

  copyBufferToImage(staginBuffer, textureImage, static_cast<uint32_t>(texWidth),
                    static_cast<uint32_t>(texHeight));

  vkinit::transitionImageLayout(textureImage,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                _commandPool, _device, _graphicsQueue);

  vkDestroyBuffer(_device, staginBuffer, nullptr);
  vkFreeMemory(_device, stagingBufferMemory, nullptr);
}

void VulkanEngine::createImage(uint32_t width, uint32_t height, VkFormat format,
                               VkImageTiling tiling, VkImageUsageFlags usage,
                               VkMemoryPropertyFlags properties,
                               VkImage &textureImage,
                               VkDeviceMemory &textureImageMemory) {

  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = usage;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.flags = 0;

  if (vkCreateImage(_device, &imageInfo, nullptr, &textureImage) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create image");
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(_device, textureImage, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
      findMemoryType(memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(_device, &allocInfo, nullptr, &textureImageMemory) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate image memory");
  }

  vkBindImageMemory(_device, textureImage, textureImageMemory, 0);
}

/*void VulkanEngine::transitionImageLayout(VkImage image, VkFormat format,
                                         VkImageLayout oldLayout,
                                         VkImageLayout newLayout) {
  VkCommandBuffer commandBuffer =
      vkinit::beginSingleTimeCommands(_commandPool, _device);

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    throw std::invalid_argument("unsupported layout transition!");
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0,
                       nullptr, 0, nullptr, 1, &barrier);

  vkinit::endSingleTimeCommands(commandBuffer, _graphicsQueue, _device,
                                _commandPool);
}*/

void VulkanEngine::copyBufferToImage(VkBuffer buffer, VkImage image,
                                     uint32_t width, uint32_t height) {

  VkCommandBuffer commandBuffer =
      vkinit::beginSingleTimeCommands(_commandPool, _device);

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};

  vkCmdCopyBufferToImage(commandBuffer, buffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  vkinit::endSingleTimeCommands(commandBuffer, _graphicsQueue, _device,
                                _commandPool);
}

void VulkanEngine::createTextureImageView(VkImage &textureImage,
                                          VkImageView &textureImageView) {
  textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB);
}

VkImageView VulkanEngine::createImageView(VkImage image, VkFormat format) {

  VkImageView imageView;

  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = format;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(_device, &viewInfo, nullptr, &imageView) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create texture image view");
  }
  return imageView;
}

void VulkanEngine::createImageViews() {
  _swapchainImageViews.resize(_swapchainImages.size());

  for (uint32_t i = 0; i < _swapchainImages.size(); i++) {
    _swapchainImageViews[i] =
        createImageView(_swapchainImages[i], _swapchainImageFormat);
  }
}

void VulkanEngine::createTextureSampler(VkSampler &textureSampler) {

  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(_physicalDevice, &properties);

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable = VK_TRUE;
  samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 0.0f;

  if (vkCreateSampler(_device, &samplerInfo, nullptr, &textureSampler) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create texture sampler");
  }
};

void VulkanEngine::createMeshDescriptorSet(Mesh &mesh) {

  if (_descriptorPool == VK_NULL_HANDLE) {
    throw std::runtime_error("Descriptor pool is VK_NULL_HANDLE");
  }
  if (_descriptorSetLayout == VK_NULL_HANDLE) {
    throw std::runtime_error("Descriptor set layout is VK_NULL_HANDLE");
  }
  if (mesh.textureImageView == VK_NULL_HANDLE) {
    throw std::runtime_error("Mesh texture image view is VK_NULL_HANDLE");
  }
  if (mesh.textureSampler == VK_NULL_HANDLE) {
    throw std::runtime_error("Mesh texture sampler is VK_NULL_HANDLE");
  }

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = _descriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &_descriptorSetLayout;

  VkResult result =
      vkAllocateDescriptorSets(_device, &allocInfo, &mesh.descriptorSet);
  if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate mesh descriptor set, error: " +
                             std::to_string(result));
  }

  VkDescriptorBufferInfo bufferInfo{};
  bufferInfo.buffer = _uniformBuffers[0];

  bufferInfo.offset = 0;
  bufferInfo.range = sizeof(UniformBufferObject);

  VkDescriptorImageInfo imageInfo{};
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView = mesh.textureImageView;
  imageInfo.sampler = mesh.textureSampler;

  std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

  descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[0].dstSet = mesh.descriptorSet;
  descriptorWrites[0].dstBinding = 0;
  descriptorWrites[0].dstArrayElement = 0;
  descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptorWrites[0].descriptorCount = 1;
  descriptorWrites[0].pBufferInfo = &bufferInfo;

  descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[1].dstSet = mesh.descriptorSet;
  descriptorWrites[1].dstBinding = 1;
  descriptorWrites[1].dstArrayElement = 0;
  descriptorWrites[1].descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrites[1].descriptorCount = 1;
  descriptorWrites[1].pImageInfo = &imageInfo;

  vkUpdateDescriptorSets(_device,
                         static_cast<uint32_t>(descriptorWrites.size()),
                         descriptorWrites.data(), 0, nullptr);
}

void VulkanEngine::createTilemapMesh(const Tilemap &tilemap,
                                     const char *texturePath) {

  std::vector<vertexData::Vertex> vertices;
  std::vector<uint16_t> indices;

  float tileSize = 1.0f;
  int atlasColums = 16;
  int atlasRows = 16;

  glm::vec3 tileColor(1.0f, 1.0f, 1.0f);

  for (int y = 0; y < tilemap.height; y++) {
    for (int x = 0; x < tilemap.width; x++) {
      Tile tile = tilemap.getTile(x, y);
      if (tile.type == 0)
        continue;

      float u1 = (float)((tile.type - 1) % atlasColums) / atlasColums;
      float v1 = (float)((tile.type - 1) / atlasColums) / atlasRows;
      float u2 = u1 + 1.0f / atlasColums;
      float v2 = v1 + 1.0f / atlasRows;

      glm::vec2 pos1(x * tileSize - 0.5f, y * tileSize - 0.5f);
      glm::vec2 pos2((x + 1) * tileSize - 0.5f, y * tileSize - 0.5f);
      glm::vec2 pos3((x + 1) * tileSize - 0.5f, (y + 1) * tileSize - 0.5f);
      glm::vec2 pos4(x * tileSize - 0.5f, (y + 1) * tileSize - 0.5f);

      uint16_t baseIndex = static_cast<uint16_t>(vertices.size());

      vertices.push_back({pos1, tileColor, {u1, v1}});
      vertices.push_back({pos2, tileColor, {u2, v1}});
      vertices.push_back({pos3, tileColor, {u2, v2}});
      vertices.push_back({pos4, tileColor, {u1, v2}});

      indices.push_back(baseIndex);
      indices.push_back(static_cast<uint16_t>(baseIndex + 1));
      indices.push_back(static_cast<uint16_t>(baseIndex + 2));
      indices.push_back(baseIndex);
      indices.push_back(static_cast<uint16_t>(baseIndex + 2));
      indices.push_back(static_cast<uint16_t>(baseIndex + 3));
    }
  }

  if (!vertices.empty()) {
    createMesh(vertices, indices, glm::mat4(1.0f), glm::vec3(0.0f),
               texturePath);
  }
}

void VulkanEngine::createMap() {
  Tilemap worldMap(10, 10);

  for (int y = 0; y < 50; y++) {
    for (int x = 0; x < 100; x++) {
      worldMap.setTile(x, y, 1);

      if (x > 30 && x < 70 && y > 10 && y < 20) {
        worldMap.setTile(x, y, 2);
      }
    }
  }

  createTilemapMesh(worldMap, "../textures/statue-1275469_640.jpg");
}
