/* SPDX-License-Identifier: MIT */
#include "vn_wsi_ownership.h"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); exit(1); } } while (0)

static void check(bool prime, bool external, bool acquire, bool concurrent)
{
   VkImageLayout old = acquire ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
   VkImageLayout next = acquire ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
   uint32_t src = VK_QUEUE_FAMILY_IGNORED, dst = VK_QUEUE_FAMILY_IGNORED;
   const struct vn_cmd_fix_image_memory_barrier_result result =
      vn_wsi_fix_image_memory_barrier(prime, external,
         concurrent ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
         7, &old, &next, &src, &dst);
   CHECK(old == (acquire ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
   CHECK(next == (acquire ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL));
   if (prime && (!external || !acquire)) {
      CHECK(src == VK_QUEUE_FAMILY_IGNORED && dst == VK_QUEUE_FAMILY_IGNORED);
      CHECK(result.availability_op_needed && result.visibility_op_needed);
      CHECK(!result.external_acquire_unmodified);
   } else {
      const uint32_t ext = external ? VK_QUEUE_FAMILY_EXTERNAL : VK_QUEUE_FAMILY_FOREIGN_EXT;
      CHECK(src == (acquire ? ext : concurrent ? VK_QUEUE_FAMILY_IGNORED : 7));
      CHECK(dst == (acquire ? concurrent ? VK_QUEUE_FAMILY_IGNORED : 7 : ext));
      CHECK(result.external_acquire_unmodified == acquire);
      CHECK(result.availability_op_needed == !acquire);
      CHECK(result.visibility_op_needed == acquire);
   }
}

int main(void)
{
   for (unsigned acquire = 0; acquire < 2; acquire++)
      for (unsigned concurrent = 0; concurrent < 2; concurrent++) {
         check(true, false, acquire, concurrent); /* ordinary prime unchanged */
         check(false, false, acquire, concurrent); /* ordinary scanout unchanged */
         check(true, true, acquire, concurrent); /* deferred release / app acquire */
      }
   /* WSI blit keeps ownership on entry; its explicit post-copy release survives
    * the mapper without being reinterpreted as another PRESENT transition. */
   VkImageLayout old = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, next = VK_IMAGE_LAYOUT_GENERAL;
   uint32_t src = 7, dst = VK_QUEUE_FAMILY_EXTERNAL;
   vn_wsi_fix_image_memory_barrier(true, true, VK_SHARING_MODE_EXCLUSIVE, 7, &old, &next, &src, &dst);
   CHECK(old == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && next == VK_IMAGE_LAYOUT_GENERAL);
   CHECK(src == 7 && dst == VK_QUEUE_FAMILY_EXTERNAL);
   old = next = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
   src = dst = VK_QUEUE_FAMILY_IGNORED;
   vn_wsi_fix_image_memory_barrier(true, true, VK_SHARING_MODE_EXCLUSIVE, 7, &old, &next, &src, &dst);
   CHECK(old == VK_IMAGE_LAYOUT_GENERAL && next == old && src == dst);
   /* The source side of an app's inter-family acquire must not double-acquire. */
   old = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; next = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
   src = 7; dst = 8;
   vn_wsi_fix_image_memory_barrier(true, true, VK_SHARING_MODE_EXCLUSIVE, 7, &old, &next, &src, &dst);
   CHECK(old == VK_IMAGE_LAYOUT_GENERAL && next == old);
   CHECK(src == VK_QUEUE_FAMILY_IGNORED && dst == src);
   puts("PASS: local prime, FOREIGN scanout and Helios post-blit EXTERNAL ownership");
}
