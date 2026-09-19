/* SPDX-License-Identifier: MIT
 * Execute the real buffer-blit recorder with a CPU-only Vulkan dispatch.
 */
#include "wsi_common_private.h"
#include "vk_alloc.h"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static unsigned stage;
static bool external;

static VKAPI_ATTR VkResult VKAPI_CALL
allocate_commands(VkDevice device, const VkCommandBufferAllocateInfo *info, VkCommandBuffer *out)
{
   CHECK(info->commandBufferCount == 1);
   *out = (VkCommandBuffer)(uintptr_t)1;
   return VK_SUCCESS;
}
static VKAPI_ATTR VkResult VKAPI_CALL
begin_commands(VkCommandBuffer command, const VkCommandBufferBeginInfo *info)
{
   CHECK(stage == 0);
   return VK_SUCCESS;
}
static VKAPI_ATTR VkResult VKAPI_CALL
end_commands(VkCommandBuffer command)
{
   CHECK(stage == (external ? 4 : 3));
   return VK_SUCCESS;
}
static VKAPI_ATTR VkResult VKAPI_CALL
name_object(VkDevice device, const VkDebugUtilsObjectNameInfoEXT *info)
{
   return VK_SUCCESS;
}
static VKAPI_ATTR void VKAPI_CALL
copy_image(VkCommandBuffer command, VkImage image, VkImageLayout layout,
           VkBuffer buffer, uint32_t count, const VkBufferImageCopy *regions)
{
   CHECK(stage++ == 1 && layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && count == 1);
   CHECK(regions->imageExtent.width == 941 && regions->imageExtent.height == 1030);
}
static VKAPI_ATTR void VKAPI_CALL
barrier(VkCommandBuffer command, VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage,
        VkDependencyFlags flags, uint32_t memory_count, const VkMemoryBarrier *memory,
        uint32_t buffer_count, const VkBufferMemoryBarrier *buffers,
        uint32_t image_count, const VkImageMemoryBarrier *images)
{
   if (external && stage == 2) {
      CHECK(image_count == 0 && !images && buffer_count == 1);
      CHECK(src_stage == VK_PIPELINE_STAGE_TRANSFER_BIT);
      CHECK(dst_stage == VK_PIPELINE_STAGE_HOST_BIT);
      CHECK(buffers->dstAccessMask == VK_ACCESS_HOST_READ_BIT);
      stage++;
      return;
   }
   CHECK(image_count == 1);
   CHECK(images->subresourceRange.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT);
   CHECK(images->subresourceRange.levelCount == 1 && images->subresourceRange.layerCount == 1);
   if (stage++ == 0) {
      CHECK(!buffer_count);
      CHECK(images->oldLayout == (external ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR));
      CHECK(images->newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
      CHECK(images->srcQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED);
      CHECK(images->dstQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED);
   } else {
      CHECK(stage == (external ? 4 : 3));
      CHECK(buffer_count == (external ? 0 : 1));
      if (external) {
         CHECK(!(src_stage & VK_PIPELINE_STAGE_HOST_BIT));
         CHECK(!(dst_stage & VK_PIPELINE_STAGE_HOST_BIT));
         CHECK(dst_stage == VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
      }
      CHECK(images->oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
      CHECK(images->newLayout == (external ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR));
      CHECK(images->srcQueueFamilyIndex == (external ? 0 : VK_QUEUE_FAMILY_IGNORED));
      CHECK(images->dstQueueFamilyIndex == (external ? VK_QUEUE_FAMILY_EXTERNAL : VK_QUEUE_FAMILY_IGNORED));
      CHECK(images->srcAccessMask == VK_ACCESS_TRANSFER_READ_BIT && !images->dstAccessMask);
      if (!external)
         CHECK(buffers->dstAccessMask == VK_ACCESS_HOST_READ_BIT);
   }
}
int main(void)
{
   const struct wsi_device wsi = {
      .queue_family_count = 1, .AllocateCommandBuffers = allocate_commands,
      .BeginCommandBuffer = begin_commands, .EndCommandBuffer = end_commands,
      .SetDebugUtilsObjectNameEXT = name_object,
      .CmdPipelineBarrier = barrier, .CmdCopyImageToBuffer = copy_image,
   };
   VkCommandPool pool = (VkCommandPool)(uintptr_t)1;
   struct wsi_swapchain chain = { .wsi = &wsi, .blit.type = WSI_SWAPCHAIN_BUFFER_BLIT, .cmd_pools = &pool };
   chain.alloc = *vk_default_allocator();
   struct wsi_image_info info = {
      .image_type = WSI_IMAGE_TYPE_CPU, .linear_stride = 3764,
      .create.format = VK_FORMAT_B8G8R8A8_SRGB, .create.extent = {941, 1030, 1},
   };
   for (unsigned mode = 0; mode < 2; mode++) {
      external = info.wsi.helios_external_blit_src = mode;
      stage = 0;
      struct wsi_image image = { .image = (VkImage)(uintptr_t)2 };
      CHECK(wsi_finish_create_blit_context(&chain, &info, &image) == VK_SUCCESS);
      CHECK(stage == (external ? 4 : 3));
      vk_free(&chain.alloc, image.blit.cmd_buffers);
   }
   puts("PASS: real WSI recorder releases the source only after its fallback read");
}
