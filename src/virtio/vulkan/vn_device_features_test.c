/* SPDX-License-Identifier: MIT
 * CPU-only regression: no renderer or Vulkan driver is loaded.
 */
#include "vn_device_features.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); exit(1); } } while (0)

struct allocator_state { unsigned calls, fail_at, live; };

static VKAPI_ATTR void *VKAPI_CALL
allocate(void *data, size_t size, size_t alignment, VkSystemAllocationScope scope)
{
   struct allocator_state *s = data;
   CHECK(alignment <= 8 && scope == VK_SYSTEM_ALLOCATION_SCOPE_COMMAND);
   if (++s->calls == s->fail_at)
      return NULL;
   void *p = malloc(size);
   if (p)
      s->live++;
   return p;
}

static VKAPI_ATTR void VKAPI_CALL
release(void *data, void *p)
{
   struct allocator_state *s = data;
   CHECK(s->live);
   s->live--;
   free(p);
}

int main(void)
{
   struct allocator_state state = {0};
   const VkAllocationCallbacks alloc = {
      .pUserData = &state, .pfnAllocation = allocate, .pfnFree = release,
   };
   VkBaseOutStructure *out = NULL;
   CHECK(vn_device_features_enable_timeline(&alloc, NULL, &out) == VK_SUCCESS);
   CHECK(out->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES);
   CHECK(((VkPhysicalDeviceTimelineSemaphoreFeatures *)out)->timelineSemaphore);
   CHECK(!out->pNext);
   vn_device_features_free(&alloc, out);
   CHECK(!state.live);

   VkPhysicalDeviceVulkan12Features v12 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
      .samplerMirrorClampToEdge = VK_TRUE, .timelineSemaphore = VK_FALSE,
   };
   VkPhysicalDeviceVulkan13Features v13 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
      .pNext = &v12, .synchronization2 = VK_FALSE,
   };
   VkPhysicalDevice physical = VK_NULL_HANDLE;
   VkDeviceGroupDeviceCreateInfo group = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO,
      .pNext = &v13, .physicalDeviceCount = 1, .pPhysicalDevices = &physical,
   };
   VkPhysicalDeviceFeatures2 features = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
      .pNext = &group, .features.robustBufferAccess = VK_TRUE,
   };
   const VkPhysicalDeviceVulkan12Features saved12 = v12;
   const VkPhysicalDeviceVulkan13Features saved13 = v13;
   const VkDeviceGroupDeviceCreateInfo saved_group = group;
   const VkPhysicalDeviceFeatures2 saved_features = features;
   CHECK(vn_device_features_enable_timeline(&alloc, &features, &out) == VK_SUCCESS);
   CHECK(((VkPhysicalDeviceFeatures2 *)out)->features.robustBufferAccess);
   CHECK(((VkDeviceGroupDeviceCreateInfo *)out->pNext)->pPhysicalDevices == &physical);
   CHECK(!((VkPhysicalDeviceVulkan13Features *)out->pNext->pNext)->synchronization2);
   VkPhysicalDeviceVulkan12Features *copy12 = (void *)out->pNext->pNext->pNext;
   CHECK(copy12->timelineSemaphore && copy12->samplerMirrorClampToEdge && !copy12->pNext);
   CHECK(!memcmp(&v12, &saved12, sizeof(v12)));
   CHECK(!memcmp(&v13, &saved13, sizeof(v13)));
   CHECK(!memcmp(&group, &saved_group, sizeof(group)));
   CHECK(!memcmp(&features, &saved_features, sizeof(features)));
   vn_device_features_free(&alloc, out);
   CHECK(!state.live);

   VkPhysicalDeviceTimelineSemaphoreFeatures timeline = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES,
   };
   CHECK(vn_device_features_enable_timeline(&alloc, &timeline, &out) == VK_SUCCESS);
   CHECK(((VkPhysicalDeviceTimelineSemaphoreFeatures *)out)->timelineSemaphore);
   CHECK(!out->pNext && !timeline.timelineSemaphore);
   vn_device_features_free(&alloc, out);

   VkBaseInStructure unknown = { .sType = VK_STRUCTURE_TYPE_MAX_ENUM, .pNext = (void *)&timeline };
   CHECK(vn_device_features_enable_timeline(&alloc, &unknown, &out) == VK_SUCCESS);
   CHECK(out->sType == timeline.sType && !out->pNext);
   vn_device_features_free(&alloc, out);

   /* Fail each allocation of both an existing-feature and appended-feature chain. */
   for (unsigned failure = 1; failure <= 4; failure++) {
      state.calls = 0; state.fail_at = failure;
      out = (void *)1;
      CHECK(vn_device_features_enable_timeline(&alloc, &features, &out) == VK_ERROR_OUT_OF_HOST_MEMORY);
      CHECK(!out && !state.live);
   }
   VkPhysicalDeviceFeatures2 no_timeline = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
   for (unsigned failure = 1; failure <= 2; failure++) {
      state.calls = 0; state.fail_at = failure;
      CHECK(vn_device_features_enable_timeline(&alloc, &no_timeline, &out) == VK_ERROR_OUT_OF_HOST_MEMORY);
      CHECK(!out && !state.live);
   }
   puts("PASS host timeline chain: empty, promoted, standalone, unknown, immutable, OOM unwind");
   return 0;
}
