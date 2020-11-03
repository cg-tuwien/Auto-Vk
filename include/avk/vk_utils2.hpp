#pragma once

namespace avk 
{
#if VK_HEADER_VERSION >= 135
	extern VkAccelerationStructureInstanceKHR convert_for_gpu_usage(const geometry_instance& aGeomInst);
	extern std::vector<VkAccelerationStructureInstanceKHR> convert_for_gpu_usage(const std::vector<geometry_instance>& aGeomInstances);
#endif
	
	extern std::optional<command_buffer> copy_image_to_another(avk::resource_reference<image_t> aSrcImage, avk::resource_reference<image_t> aDstImage, sync aSyncHandler = sync::wait_idle(), bool aRestoreSrcLayout = true, bool aRestoreDstLayout = true);
	extern std::optional<command_buffer> blit_image(avk::resource_reference<image_t> aSrcImage, avk::resource_reference<image_t> aDstImage, sync aSyncHandler = sync::wait_idle(), bool aRestoreSrcLayout = true, bool aRestoreDstLayout = true);

	extern std::optional<command_buffer> copy_buffer_to_image_mip_level(avk::resource_reference<const buffer_t> aSrcBuffer, avk::resource_reference<image_t> aDstImage, uint32_t aDstLevel, sync aSyncHandler = sync::wait_idle());
	extern std::optional<command_buffer> copy_buffer_to_image(avk::resource_reference<const buffer_t> aSrcBuffer, avk::resource_reference<image_t> aDstImage, sync aSyncHandler = sync::wait_idle());

	extern std::optional<command_buffer> copy_buffer_to_another(avk::resource_reference<buffer_t> aSrcBuffer, avk::resource_reference<buffer_t> aDstBuffer, std::optional<vk::DeviceSize> aSrcOffset = {}, std::optional<vk::DeviceSize> aDstOffset = {}, std::optional<vk::DeviceSize> aDataSize = {}, sync aSyncHandler = sync::wait_idle());
}
