/* SPDX-License-Identifier: MIT
 * CPU-only test of the real allocation function using fake Vulkan dispatch.
 */
#include "wsi_common_private.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); exit(1); } } while (0)

static VkDeviceSize required_size, allocated_size;
static unsigned allocation_calls, shm_size;
static bool shm_fails;
static char host_storage;

static VKAPI_ATTR VkResult VKAPI_CALL
create_buffer(VkDevice dev, const VkBufferCreateInfo *info,
              const VkAllocationCallbacks *alloc, VkBuffer *out)
{
   CHECK(info->size == 3876920 && info->usage == VK_BUFFER_USAGE_TRANSFER_DST_BIT);
   *out = (VkBuffer)(uintptr_t)1;
   return VK_SUCCESS;
}

static VKAPI_ATTR void VKAPI_CALL
buffer_requirements(VkDevice dev, VkBuffer buffer, VkMemoryRequirements *reqs)
{
   *reqs = (VkMemoryRequirements){ .size = required_size, .alignment = 64, .memoryTypeBits = 1 };
}

static VKAPI_ATTR void VKAPI_CALL
image_requirements(VkDevice dev, VkImage image, VkMemoryRequirements *reqs)
{
   *reqs = (VkMemoryRequirements){ .size = 4096, .alignment = 64, .memoryTypeBits = 1 };
}

static VKAPI_ATTR VkResult VKAPI_CALL
allocate_memory(VkDevice dev, const VkMemoryAllocateInfo *info,
                const VkAllocationCallbacks *alloc, VkDeviceMemory *out)
{
   if (!allocation_calls++) {
      allocated_size = info->allocationSize;
      CHECK(allocated_size == required_size);
   } else {
      CHECK(info->allocationSize == 4096);
   }
   *out = (VkDeviceMemory)(uintptr_t)allocation_calls;
   return VK_SUCCESS;
}

static VKAPI_ATTR VkResult VKAPI_CALL
bind_buffer(VkDevice dev, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize offset)
{
   CHECK(!offset && allocated_size >= required_size);
   return VK_SUCCESS;
}

static uint32_t
select_type(const struct wsi_device *wsi, uint32_t bits)
{
   CHECK(bits == 1);
   return 0;
}

static uint8_t *
allocate_shm(struct wsi_image *image, unsigned size)
{
   shm_size = size;
   return shm_fails ? NULL : (uint8_t *)&host_storage;
}

int main(void)
{
   const struct wsi_device wsi = {
      .CreateBuffer = create_buffer, .GetBufferMemoryRequirements = buffer_requirements,
      .GetImageMemoryRequirements = image_requirements,
      .AllocateMemory = allocate_memory, .BindBufferMemory = bind_buffer,
   };
   const struct wsi_swapchain chain = {
      .wsi = &wsi, .blit.type = WSI_SWAPCHAIN_BUFFER_BLIT,
   };
   struct wsi_image_info info = {
      .image_type = WSI_IMAGE_TYPE_CPU, .linear_size = 3876920, .linear_stride = 3764,
      .select_image_memory_type = select_type, .select_blit_dst_memory_type = select_type,
   };
   for (unsigned test = 0; test < 4; test++) {
      required_size = test ? 3876928 : 3876920;
      allocation_calls = shm_size = 0;
      info.alloc_shm = test >= 2 ? allocate_shm : NULL;
      shm_fails = test == 3;
      struct wsi_image image = {0};
      CHECK(wsi_create_buffer_blit_context(&chain, &info, &image, 0) == VK_SUCCESS);
      CHECK(allocation_calls == 2);
      CHECK(!info.alloc_shm || shm_size == required_size);
      CHECK(image.sizes[0] == info.linear_size && image.row_pitches[0] == info.linear_stride);
   }
   required_size = (VkDeviceSize)UINT32_MAX + 1;
   allocation_calls = shm_size = 0;
   struct wsi_image image = {0};
   CHECK(wsi_create_buffer_blit_context(&chain, &info, &image, 0) == VK_ERROR_OUT_OF_HOST_MEMORY);
   CHECK(!allocation_calls && !shm_size);
   puts("PASS WSI allocation: exact, padded, shared, fallback, no truncation; pixel layout unchanged");
   return 0;
}
