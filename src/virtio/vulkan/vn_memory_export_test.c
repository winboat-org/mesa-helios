/* SPDX-License-Identifier: MIT
 * Exercise the allocation contract for the real internal mapping export.
 * No Vulkan driver or GPU is used.
 */
#include "vn_memory_export.h"
#include "vn_physical_device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); exit(1); } } while (0)

int main(void)
{
   VkMemoryAllocateFlagsInfo flags = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
      .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
   };
   VkMemoryAllocateInfo info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .pNext = &flags,
      .allocationSize = 4097, .memoryTypeIndex = 4,
   };
   const VkMemoryAllocateInfo saved = info;
   VkMemoryAllocateInfo local;
   VkExportMemoryAllocateInfo export;
   CHECK(vn_memory_export_for_cpu_mapping(&info, &local, &export) == VK_SUCCESS);
   CHECK(!memcmp(&info, &saved, sizeof(info)));
   CHECK(local.allocationSize == 8192 && local.memoryTypeIndex == 4);
   CHECK(local.pNext == &export && export.pNext == &flags);
   CHECK(export.handleTypes == VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT);
   struct vn_physical_device physical = {
      .external_memory = {
         .renderer_handle_type = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT,
         .opaque_fd_mapping = true,
      },
   };
   /* The actual buffer/image selector must agree with the allocation export,
    * while an explicit DMA_BUF resource retains the scanout handle type. */
   CHECK(vn_renderer_handle_type_for_guest(&physical, 0) == export.handleTypes);
   CHECK(vn_renderer_handle_type_for_guest(
      &physical, VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT) ==
         VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT);
   physical.external_memory.opaque_fd_mapping = false;
   CHECK(vn_renderer_handle_type_for_guest(&physical, 0) ==
         VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT);
   CHECK(flags.flags == VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT && !flags.pNext);

   /* Dedicated resource identity must survive the mapping export; padding
    * still satisfies its memory requirement, including sub-page buffers. */
   VkMemoryDedicatedAllocateInfo dedicated = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,
      .pNext = &flags, .buffer = (VkBuffer)(uintptr_t)1,
   };
   info.pNext = &dedicated;
   info.allocationSize = 256;
   CHECK(vn_memory_export_for_cpu_mapping(&info, &local, &export) == VK_SUCCESS);
   CHECK(local.allocationSize == 4096 && export.pNext == &dedicated);
   CHECK(dedicated.buffer == (VkBuffer)(uintptr_t)1);
   dedicated.buffer = VK_NULL_HANDLE;
   dedicated.image = (VkImage)(uintptr_t)2;
   CHECK(vn_memory_export_for_cpu_mapping(&info, &local, &export) == VK_SUCCESS);
   CHECK(local.allocationSize == 4096 && export.pNext == &dedicated);
   CHECK(dedicated.image == (VkImage)(uintptr_t)2);
   dedicated.image = VK_NULL_HANDLE;
   CHECK(vn_memory_export_for_cpu_mapping(&info, &local, &export) == VK_SUCCESS);
   CHECK(local.allocationSize == 4096);

   info.pNext = NULL;
   info.allocationSize = UINT64_MAX;
   CHECK(vn_memory_export_for_cpu_mapping(&info, &local, &export) == VK_ERROR_OUT_OF_DEVICE_MEMORY);
   info.allocationSize = 4096;
   VkExportMemoryAllocateInfo existing = {
      .sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO,
      .pNext = &flags,
      .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT,
   };
   info.pNext = &existing;
   CHECK(vn_memory_export_for_cpu_mapping(&info, &local, &export) == VK_SUCCESS);
   CHECK(local.pNext == &existing && existing.pNext == &flags);
   CHECK(local.allocationSize == 4096);
   existing.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
   CHECK(vn_memory_export_for_cpu_mapping(&info, &local, &export) == VK_ERROR_INVALID_EXTERNAL_HANDLE);
   CHECK(existing.handleTypes == VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT);
   puts("PASS opaque mapping allocation: size, dedicated, chain, overflow, no mixed export");
   return 0;
}
