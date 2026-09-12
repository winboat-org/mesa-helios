/* Copyright 2026 WinBoat
 * SPDX-License-Identifier: MIT
 */
#ifndef ZINK_VENDOR_H
#define ZINK_VENDOR_H

#include <stdint.h>

/* Vulkan vendorID describes the backing GPU, not the Venus/Zink distributor.
 * Keep an explicit unknown-ID fallback at the call site; never relabel an
 * unrecognized GPU as one of these manufacturers.
 */
static inline const char *
zink_device_vendor_name(uint32_t vendor_id)
{
   switch (vendor_id) {
   case 0x1002: return "AMD";
   case 0x10de: return "NVIDIA";
   case 0x8086: return "Intel";
   default: return 0;
   }
}

#endif
