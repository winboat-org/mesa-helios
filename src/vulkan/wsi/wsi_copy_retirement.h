/* SPDX-License-Identifier: MIT */
#ifndef WSI_COPY_RETIREMENT_H
#define WSI_COPY_RETIREMENT_H

#include <stdbool.h>
#include <stdint.h>
#include <vulkan/vulkan_core.h>

/* The caller keeps the helper device and its same-thread, fixed submission
 * token alive throughout this call. No callback may flush or replace it.
 * Cancellation stops presentation; only a successful copy wait releases the
 * source. A bounded cancellation drain must never turn a timeout into success.
 */
struct wsi_copy_retirement_ops {
   VkResult (*presentation_status)(void *data);
   bool (*device_lost)(void *data);
   int32_t (*wait_copy)(void *data, uint32_t timeout_us);
   uint64_t (*now_ns)(void *data);
   void (*pending)(void *data);
};

struct wsi_copy_retirement_result {
   VkResult status;
   bool completed;
   bool pending;
   bool cancelled;
   bool drain_expired;
};

static inline struct wsi_copy_retirement_result
wsi_copy_retire(const struct wsi_copy_retirement_ops *ops, void *data,
                uint32_t wait_slice_us, uint64_t cancel_drain_ns)
{
   struct wsi_copy_retirement_result out = {VK_SUCCESS, false, false, false, false};
   uint64_t cancelled_at = 0;
   bool waited = false;
   int32_t copy_result = 1;

   for (;;) {
      const VkResult status = ops->presentation_status(data);
      if (status == VK_ERROR_OUT_OF_DATE_KHR ||
          status == VK_ERROR_SURFACE_LOST_KHR) {
         if (!out.cancelled) {
            out.cancelled = true;
            out.status = status;
            cancelled_at = ops->now_ns(data);
         }
      } else if (status != VK_SUCCESS) {
         out.status = status;
         return out;
      }

      /* Check on both sides of every wait. Failure cleanup may notify a CPU
       * fence without proving that the GPU read completed. */
      if (ops->device_lost(data)) {
         out.status = VK_ERROR_DEVICE_LOST;
         return out;
      }
      if (waited && copy_result != 1) {
         if (copy_result == 0)
            out.completed = true;
         else
            out.status = VK_ERROR_DEVICE_LOST;
         return out;
      }

      uint32_t slice_us = wait_slice_us;
      if (out.cancelled) {
         const uint64_t elapsed = ops->now_ns(data) - cancelled_at;
         if (waited && elapsed >= cancel_drain_ns) {
            out.drain_expired = true;
            return out;
         }
         const uint64_t remaining = elapsed < cancel_drain_ns
            ? cancel_drain_ns - elapsed : 0;
         const uint64_t remaining_us = remaining / 1000 + (remaining % 1000 != 0);
         if (remaining_us < slice_us)
            slice_us = (uint32_t)remaining_us;
         /* Still query once if cancellation was already expired: an already
          * completed copy is safe to release, unlike a timed-out wait. */
      }
      copy_result = ops->wait_copy(data, slice_us);
      waited = true;
      if (copy_result == 1 && !out.pending) {
         out.pending = true;
         ops->pending(data);
      }
   }
}

#endif
