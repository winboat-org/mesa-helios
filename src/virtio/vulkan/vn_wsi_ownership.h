/* SPDX-License-Identifier: MIT
 * Pure PRESENT-layout/ownership mapping, shared with the CPU regression test.
 */
#ifndef VN_WSI_OWNERSHIP_H
#define VN_WSI_OWNERSHIP_H

#include <stdbool.h>
#include <vulkan/vulkan_core.h>

struct vn_cmd_fix_image_memory_barrier_result {
   bool availability_op_needed;
   bool visibility_op_needed;
   bool external_acquire_unmodified;
};

static inline struct vn_cmd_fix_image_memory_barrier_result
vn_wsi_fix_image_memory_barrier(bool prime_blit_src, bool helios_external_src,
                                VkSharingMode sharing_mode, uint32_t pool_qfi,
                                VkImageLayout *old_layout, VkImageLayout *new_layout,
                                uint32_t *src_qfi, uint32_t *dst_qfi)
{
   struct vn_cmd_fix_image_memory_barrier_result result = {
      .availability_op_needed = true, .visibility_op_needed = true,
   };
   if (*old_layout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR &&
       *new_layout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
      return result;

   /* Ordinary prime sources never leave the producer's Vulkan instance.
    * Helios's source retains that ownership when the app enters PRESENT,
    * because WSI must still read it for the fallback buffer. WSI then releases
    * it explicitly; the next app transition out of PRESENT must acquire it.
    */
   const bool prime_local = prime_blit_src &&
      (!helios_external_src || *old_layout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
   if (prime_local || *old_layout == *new_layout) {
      if (*old_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
         *old_layout = VK_IMAGE_LAYOUT_GENERAL;
      if (*new_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
         *new_layout = VK_IMAGE_LAYOUT_GENERAL;
      return result;
   }

   const uint32_t external_qfi = helios_external_src
      ? VK_QUEUE_FAMILY_EXTERNAL : VK_QUEUE_FAMILY_FOREIGN_EXT;
   if (*old_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
      *old_layout = VK_IMAGE_LAYOUT_GENERAL;
      result.availability_op_needed = false;
      result.external_acquire_unmodified = true;
      if (sharing_mode == VK_SHARING_MODE_CONCURRENT) {
         *src_qfi = external_qfi;
         *dst_qfi = VK_QUEUE_FAMILY_IGNORED;
      } else if (*dst_qfi == *src_qfi || *dst_qfi == pool_qfi) {
         *src_qfi = external_qfi;
         *dst_qfi = pool_qfi;
      } else {
         *src_qfi = VK_QUEUE_FAMILY_IGNORED;
         *dst_qfi = VK_QUEUE_FAMILY_IGNORED;
         *new_layout = *old_layout;
      }
   } else {
      *new_layout = VK_IMAGE_LAYOUT_GENERAL;
      result.visibility_op_needed = false;
      if (sharing_mode == VK_SHARING_MODE_CONCURRENT) {
         *src_qfi = VK_QUEUE_FAMILY_IGNORED;
         *dst_qfi = external_qfi;
      } else if (*src_qfi == *dst_qfi || *src_qfi == pool_qfi) {
         *src_qfi = pool_qfi;
         *dst_qfi = external_qfi;
      } else {
         *src_qfi = VK_QUEUE_FAMILY_IGNORED;
         *dst_qfi = VK_QUEUE_FAMILY_IGNORED;
         *old_layout = *new_layout;
      }
   }
   return result;
}
#endif
