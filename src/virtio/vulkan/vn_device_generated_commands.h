/*
 * Copyright 2026 Helios contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef VN_DEVICE_GENERATED_COMMANDS_H
#define VN_DEVICE_GENERATED_COMMANDS_H

#include "vn_common.h"

struct vn_indirect_commands_layout {
   struct vn_object_base base;
};
VK_DEFINE_NONDISP_HANDLE_CASTS(vn_indirect_commands_layout, base.vk,
                              VkIndirectCommandsLayoutEXT, VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_EXT)

struct vn_indirect_execution_set {
   struct vn_object_base base;
};
VK_DEFINE_NONDISP_HANDLE_CASTS(vn_indirect_execution_set, base.vk,
                              VkIndirectExecutionSetEXT, VK_OBJECT_TYPE_INDIRECT_EXECUTION_SET_EXT)

#endif /* VN_DEVICE_GENERATED_COMMANDS_H */
