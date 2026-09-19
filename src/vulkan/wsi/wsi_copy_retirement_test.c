/* SPDX-License-Identifier: MIT
 * Execute the production wait loop against completion/cancellation races.
 * No GPU, Vulkan device, window, thread or sleeping is involved.
 */
#include "wsi_copy_retirement.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); exit(1); } } while (0)

struct fixture {
   uint64_t now, complete_at, cancel_at, failure_at;
   VkResult cancellation;
   unsigned waits, pending_reports;
   bool wait_error, restore_surface, failure;
};

static VkResult
presentation_status(void *data)
{
   struct fixture *f = data;
   if (f->restore_surface && f->waits > 1)
      return VK_SUCCESS;
   return f->now >= f->cancel_at ? f->cancellation : VK_SUCCESS;
}

static bool
device_lost(void *data)
{
   struct fixture *f = data;
   return f->failure || f->now >= f->failure_at;
}

static int32_t
wait_copy(void *data, uint32_t timeout_us)
{
   struct fixture *f = data;
   CHECK(++f->waits < 100);
   f->now += (uint64_t)timeout_us * 1000;
   if (f->wait_error)
      return -1;
   return f->now >= f->complete_at ? 0 : 1;
}

static uint64_t now_ns(void *data) { return ((struct fixture *)data)->now; }
static void pending(void *data) { ((struct fixture *)data)->pending_reports++; }

static const struct wsi_copy_retirement_ops ops = {
   presentation_status, device_lost, wait_copy, now_ns, pending,
};

static struct fixture
fixture(void)
{
   return (struct fixture) {
      .complete_at = 300000, .cancel_at = UINT64_MAX,
      .failure_at = UINT64_MAX, .cancellation = VK_ERROR_SURFACE_LOST_KHR,
   };
}

int main(void)
{
   struct fixture f = fixture();
   struct wsi_copy_retirement_result r = wsi_copy_retire(&ops, &f, 100, 500000);
   CHECK(r.status == VK_SUCCESS && r.completed && r.pending && !r.cancelled);
   CHECK(f.waits == 3 && f.pending_reports == 1);

   /* The cancellation budget is not a deadline for ordinary pending work. */
   f = fixture(); f.complete_at = 900000;
   r = wsi_copy_retire(&ops, &f, 100, 200000);
   CHECK(r.status == VK_SUCCESS && r.completed && f.waits == 9);

   /* Both close and resize cancel presentation but allow the exact pending
    * read to finish. Returning SUCCESS for the presentation would be wrong. */
   for (unsigned resize = 0; resize < 2; resize++) {
      f = fixture();
      f.cancel_at = 100000;
      f.cancellation = resize ? VK_ERROR_OUT_OF_DATE_KHR : VK_ERROR_SURFACE_LOST_KHR;
      r = wsi_copy_retire(&ops, &f, 100, 500000);
      CHECK(r.status == f.cancellation && r.completed && r.cancelled);
      CHECK(!r.drain_expired && f.waits == 3 && f.pending_reports == 1);
   }

   /* Cancellation and completion in the same wait preserve BOTH facts. */
   f = fixture(); f.cancel_at = f.complete_at = 100000;
   r = wsi_copy_retire(&ops, &f, 100, 500000);
   CHECK(r.status == VK_ERROR_SURFACE_LOST_KHR && r.completed && r.cancelled);

   /* Restoring the window cannot resurrect a cancelled chain or restart its
    * deadline. Repeated cancellation notifications cannot restart it either. */
   for (unsigned restore = 0; restore < 2; restore++) {
      f = fixture(); f.cancel_at = 0; f.complete_at = UINT64_MAX;
      f.restore_surface = restore;
      r = wsi_copy_retire(&ops, &f, 100, 250000);
      CHECK(r.status == VK_ERROR_SURFACE_LOST_KHR && !r.completed);
      CHECK(r.cancelled && r.drain_expired && f.now == 250000 && f.waits == 3);
   }

   /* A zero budget still distinguishes already complete from pending. */
   for (unsigned complete = 0; complete < 2; complete++) {
      f = fixture(); f.cancel_at = 0; f.complete_at = complete ? 0 : UINT64_MAX;
      r = wsi_copy_retire(&ops, &f, 100, 0);
      CHECK(r.status == VK_ERROR_SURFACE_LOST_KHR && r.completed == !!complete);
      CHECK(r.drain_expired == !complete && f.waits == 1);
   }

   /* Sub-microsecond time remaining rounds to one sleeping microsecond,
    * rather than spinning on zero-length waits before the deadline. */
   f = fixture(); f.cancel_at = 0; f.complete_at = UINT64_MAX;
   r = wsi_copy_retire(&ops, &f, 100, 250001);
   CHECK(!r.completed && r.drain_expired && f.now == 251000);

   /* Device failure dominates a coincident CPU completion notification. */
   f = fixture(); f.complete_at = f.failure_at = 100000;
   r = wsi_copy_retire(&ops, &f, 100, 500000);
   CHECK(r.status == VK_ERROR_DEVICE_LOST && !r.completed && f.waits == 1);
   f = fixture(); f.failure = true;
   r = wsi_copy_retire(&ops, &f, 100, 500000);
   CHECK(r.status == VK_ERROR_DEVICE_LOST && !r.completed && !f.waits);
   f = fixture(); f.wait_error = true; f.cancel_at = 0;
   r = wsi_copy_retire(&ops, &f, 100, 500000);
   CHECK(r.status == VK_ERROR_DEVICE_LOST && !r.completed && f.waits == 1);

   f = fixture(); f.cancel_at = 0; f.cancellation = VK_ERROR_OUT_OF_HOST_MEMORY;
   r = wsi_copy_retire(&ops, &f, 100, 500000);
   CHECK(r.status == VK_ERROR_OUT_OF_HOST_MEMORY && !r.completed && !f.waits);
   puts("PASS: exact copy retirement survives cancellation; timeout/failure retain the source");
   return 0;
}
