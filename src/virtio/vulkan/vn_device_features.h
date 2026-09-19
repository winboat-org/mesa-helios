/* SPDX-License-Identifier: MIT */
#ifndef VN_DEVICE_FEATURES_H
#define VN_DEVICE_FEATURES_H

#include <stdbool.h>
#include "vk_alloc.h"
#include "vk_enum_to_str.h"

static inline void
vn_device_features_free(const VkAllocationCallbacks *alloc,
                        VkBaseOutStructure *chain)
{
   while (chain) {
      VkBaseOutStructure *next = chain->pNext;
      vk_free(alloc, chain);
      chain = next;
   }
}

/* WSI needs a host timeline even when the app did not enable one. Copy the
 * renderer create chain, never mutate the app's chain or enabled_features.
 * Preserve unrelated fields and avoid duplicating a promoted feature struct.
 * Nested arrays/pointers remain app-owned until synchronous CreateDevice ends.
 */
static inline VkResult
vn_device_features_enable_timeline(const VkAllocationCallbacks *alloc,
                                   const void *app_chain,
                                   VkBaseOutStructure **out_chain)
{
   VkBaseOutStructure *head = NULL;
   VkBaseOutStructure **tail = &head;
   bool found = false;
   *out_chain = NULL;

   for (const VkBaseInStructure *item = app_chain; item; item = item->pNext) {
      const size_t size = vk_structure_type_size(item);
      /* Vulkan ignores unknown pNext types, as does the wire encoder. */
      if (!size)
         continue;

      VkBaseOutStructure *copy =
         vk_alloc(alloc, size, 8, VK_SYSTEM_ALLOCATION_SCOPE_COMMAND);
      if (!copy)
         goto oom;
      memcpy(copy, item, size);
      copy->pNext = NULL;
      *tail = copy;
      tail = &copy->pNext;

      if (copy->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES) {
         ((VkPhysicalDeviceVulkan12Features *)copy)->timelineSemaphore = VK_TRUE;
         found = true;
      } else if (copy->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES) {
         ((VkPhysicalDeviceTimelineSemaphoreFeatures *)copy)->timelineSemaphore = VK_TRUE;
         found = true;
      }
   }

   if (!found) {
      VkPhysicalDeviceTimelineSemaphoreFeatures *timeline =
         vk_zalloc(alloc, sizeof(*timeline), 8,
                   VK_SYSTEM_ALLOCATION_SCOPE_COMMAND);
      if (!timeline)
         goto oom;
      timeline->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
      timeline->timelineSemaphore = VK_TRUE;
      *tail = (VkBaseOutStructure *)timeline;
   }

   *out_chain = head;
   return VK_SUCCESS;

oom:
   vn_device_features_free(alloc, head);
   return VK_ERROR_OUT_OF_HOST_MEMORY;
}

#endif
