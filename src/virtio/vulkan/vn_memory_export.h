/* SPDX-License-Identifier: MIT */
#ifndef VN_MEMORY_EXPORT_H
#define VN_MEMORY_EXPORT_H

#include "vk_util.h"

/* The Helios aperture maps whole 4 KiB pages. Preserve the renderer's padding
 * for an internal opaque export, whose Vulkan mapper must import exactly the
 * allocation size sent here. Dedicated allocations retain their resource
 * chain; the padded size still covers its queried memory requirement.
 */
static inline VkResult
vn_memory_export_for_cpu_mapping(const VkMemoryAllocateInfo *info,
                                 VkMemoryAllocateInfo *local,
                                 VkExportMemoryAllocateInfo *export)
{
   const VkExportMemoryAllocateInfo *existing =
      vk_find_struct_const(info->pNext, EXPORT_MEMORY_ALLOCATE_INFO);
   if (existing &&
       existing->handleTypes != VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT)
      return VK_ERROR_INVALID_EXTERNAL_HANDLE;
   *local = *info;
   const VkDeviceSize mask = 4095;
   if (local->allocationSize > UINT64_MAX - mask)
      return VK_ERROR_OUT_OF_DEVICE_MEMORY;
   local->allocationSize = (local->allocationSize + mask) & ~mask;
   *export = (VkExportMemoryAllocateInfo){
      .sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO,
      .pNext = info->pNext,
      .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT,
   };
   /* A caller-supplied zero-handle export was normalized by the allocation
    * chain fixup. Do not append a second structure of the same sType. */
   if (!existing)
      local->pNext = export;
   return VK_SUCCESS;
}

#endif
