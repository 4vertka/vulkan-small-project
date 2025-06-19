#include <iostream>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#pragma once

namespace vkinit {

void transitionImage(VkCommandBuffer commandBuffer, VkImage image,
                     VkImageLayout oldLayout, VkImageLayout newLayout);

VkCommandBuffer beginSingleTimeCommands(VkCommandPool commandPool,
                                        VkDevice device);

void endSingleTimeCommands(VkCommandBuffer &commandBuffer,
                           VkQueue graphicsQueue, VkDevice device,
                           VkCommandPool commandPool);

void transitionImageLayout(VkImage image, VkFormat format,
                           VkImageLayout oldLayout, VkImageLayout newLayout,
                           VkCommandPool commandPool, VkDevice device,
                           VkQueue graphicsQueue);

}; // namespace vkinit
