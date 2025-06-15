#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#pragma once

namespace vkinit {

void transitionImage(VkCommandBuffer commandBuffer, VkImage image,
                     VkImageLayout oldLayout, VkImageLayout newLayout);

VkImageSubresourceRange imagSubresourceRange(VkImageAspectFlags aspectMask);

}; // namespace vkinit
