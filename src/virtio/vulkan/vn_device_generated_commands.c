/*
 * Copyright 2026 Helios contributors
 * SPDX-License-Identifier: MIT
 */

#include "vn_device_generated_commands.h"

#include "venus-protocol/vn_protocol_driver_device_generated_commands.h"

#include "vn_device.h"

VKAPI_ATTR VkResult VKAPI_CALL
vn_CreateIndirectCommandsLayoutEXT(
   VkDevice device, const VkIndirectCommandsLayoutCreateInfoEXT *pCreateInfo,
   const VkAllocationCallbacks *pAllocator, VkIndirectCommandsLayoutEXT *pIndirectCommandsLayout)
{
   struct vn_device *dev = vn_device_from_handle(device);
   const VkAllocationCallbacks *alloc = pAllocator ? pAllocator : &dev->base.vk.alloc;
   *pIndirectCommandsLayout = VK_NULL_HANDLE;
   struct vn_indirect_commands_layout *obj = vk_zalloc(
      alloc, sizeof(*obj), VN_DEFAULT_ALIGN, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   if (!obj)
      return vn_error(dev->instance, VK_ERROR_OUT_OF_HOST_MEMORY);

   vn_object_base_init(&obj->base, VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_EXT, &dev->base);
   VkIndirectCommandsLayoutEXT handle = vn_indirect_commands_layout_to_handle(obj);
   VkResult result = vn_call_vkCreateIndirectCommandsLayoutEXT(
      dev->primary_ring, device, pCreateInfo, NULL, &handle);
   if (result != VK_SUCCESS) {
      vn_object_base_fini(&obj->base);
      vk_free(alloc, obj);
      return vn_error(dev->instance, result);
   }
   *pIndirectCommandsLayout = handle;
   return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL
vn_DestroyIndirectCommandsLayoutEXT(VkDevice device, VkIndirectCommandsLayoutEXT handle,
                               const VkAllocationCallbacks *pAllocator)
{
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_indirect_commands_layout *obj = vn_indirect_commands_layout_from_handle(handle);
   const VkAllocationCallbacks *alloc = pAllocator ? pAllocator : &dev->base.vk.alloc;
   if (!obj)
      return;
   vn_async_vkDestroyIndirectCommandsLayoutEXT(dev->primary_ring, device, handle, NULL);
   vn_object_base_fini(&obj->base);
   vk_free(alloc, obj);
}

VKAPI_ATTR VkResult VKAPI_CALL
vn_CreateIndirectExecutionSetEXT(
   VkDevice device, const VkIndirectExecutionSetCreateInfoEXT *pCreateInfo,
   const VkAllocationCallbacks *pAllocator, VkIndirectExecutionSetEXT *pIndirectExecutionSet)
{
   struct vn_device *dev = vn_device_from_handle(device);
   const VkAllocationCallbacks *alloc = pAllocator ? pAllocator : &dev->base.vk.alloc;
   *pIndirectExecutionSet = VK_NULL_HANDLE;
   struct vn_indirect_execution_set *obj = vk_zalloc(
      alloc, sizeof(*obj), VN_DEFAULT_ALIGN, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   if (!obj)
      return vn_error(dev->instance, VK_ERROR_OUT_OF_HOST_MEMORY);

   vn_object_base_init(&obj->base, VK_OBJECT_TYPE_INDIRECT_EXECUTION_SET_EXT, &dev->base);
   VkIndirectExecutionSetEXT handle = vn_indirect_execution_set_to_handle(obj);
   VkResult result = vn_call_vkCreateIndirectExecutionSetEXT(
      dev->primary_ring, device, pCreateInfo, NULL, &handle);
   if (result != VK_SUCCESS) {
      vn_object_base_fini(&obj->base);
      vk_free(alloc, obj);
      return vn_error(dev->instance, result);
   }
   *pIndirectExecutionSet = handle;
   return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL
vn_DestroyIndirectExecutionSetEXT(VkDevice device, VkIndirectExecutionSetEXT handle,
                               const VkAllocationCallbacks *pAllocator)
{
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_indirect_execution_set *obj = vn_indirect_execution_set_from_handle(handle);
   const VkAllocationCallbacks *alloc = pAllocator ? pAllocator : &dev->base.vk.alloc;
   if (!obj)
      return;
   vn_async_vkDestroyIndirectExecutionSetEXT(dev->primary_ring, device, handle, NULL);
   vn_object_base_fini(&obj->base);
   vk_free(alloc, obj);
}

VKAPI_ATTR void VKAPI_CALL
vn_GetGeneratedCommandsMemoryRequirementsEXT(
   VkDevice device, const VkGeneratedCommandsMemoryRequirementsInfoEXT *pInfo,
   VkMemoryRequirements2 *pMemoryRequirements)
{
   struct vn_device *dev = vn_device_from_handle(device);
   vn_call_vkGetGeneratedCommandsMemoryRequirementsEXT(
      dev->primary_ring, device, pInfo, pMemoryRequirements);
}

VKAPI_ATTR void VKAPI_CALL
vn_UpdateIndirectExecutionSetPipelineEXT(
   VkDevice device, VkIndirectExecutionSetEXT indirectExecutionSet,
   uint32_t executionSetWriteCount,
   const VkWriteIndirectExecutionSetPipelineEXT *pExecutionSetWrites)
{
   struct vn_device *dev = vn_device_from_handle(device);
   vn_async_vkUpdateIndirectExecutionSetPipelineEXT(
      dev->primary_ring, device, indirectExecutionSet, executionSetWriteCount,
      pExecutionSetWrites);
}

VKAPI_ATTR void VKAPI_CALL
vn_UpdateIndirectExecutionSetShaderEXT(
   VkDevice device, VkIndirectExecutionSetEXT indirectExecutionSet,
   uint32_t executionSetWriteCount,
   const VkWriteIndirectExecutionSetShaderEXT *pExecutionSetWrites)
{
   struct vn_device *dev = vn_device_from_handle(device);
   vn_async_vkUpdateIndirectExecutionSetShaderEXT(
      dev->primary_ring, device, indirectExecutionSet, executionSetWriteCount,
      pExecutionSetWrites);
}
