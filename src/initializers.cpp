#include "./initializers.hpp"
#include <vulkan/vulkan_core.h>

void vkinit::transitionImage(VkCommandBuffer commandBuffer, VkImage image,
                             VkImageLayout oldLayout, VkImageLayout newLayout) {

  const VkImageMemoryBarrier2 imageMemoryBarrier2{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
      .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
      .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
      .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
      .dstAccessMask =
          VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
      .oldLayout = oldLayout,
      .newLayout = newLayout,
      .image = image,
      .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = VK_REMAINING_MIP_LEVELS,
                        .baseArrayLayer = 0,
                        .layerCount = VK_REMAINING_ARRAY_LAYERS}};

  VkDependencyInfo dependencyInfo{};
  dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  dependencyInfo.pNext = nullptr;
  dependencyInfo.imageMemoryBarrierCount = 1;
  dependencyInfo.pImageMemoryBarriers = &imageMemoryBarrier2;

  // vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
  //                      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0,
  //                      nullptr, 0, nullptr, 1, &image_memory_barrier);
  vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
}

VkImageSubresourceRange
vkinit::imagSubresourceRange(VkImageAspectFlags aspectMask) {

  VkImageSubresourceRange subImage{};
  subImage.aspectMask = aspectMask;
  subImage.baseMipLevel = 0;
  subImage.levelCount = VK_REMAINING_MIP_LEVELS;
  subImage.baseArrayLayer = 0;
  subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;

  return subImage;
}
