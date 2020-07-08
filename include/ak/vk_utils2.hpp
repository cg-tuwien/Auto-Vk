#pragma once

namespace ak 
{
	extern VkAccelerationStructureInstanceKHR convert_for_gpu_usage(const geometry_instance& aGeomInst);
	extern std::vector<VkAccelerationStructureInstanceKHR> convert_for_gpu_usage(const std::vector<geometry_instance>& aGeomInstances);

	extern std::optional<command_buffer> copy_image_to_another(image_t& aSrcImage, image_t& aDstImage, sync aSyncHandler = sync::wait_idle(), bool aRestoreSrcLayout = true, bool aRestoreDstLayout = true);
	extern std::optional<command_buffer> blit_image(image_t& aSrcImage, image_t& aDstImage, sync aSyncHandler = sync::wait_idle(), bool aRestoreSrcLayout = true, bool aRestoreDstLayout = true);

	extern std::optional<command_buffer> copy_buffer_to_image(const buffer_t& aSrcBuffer, image_t& aDstImage, sync aSyncHandler = sync::wait_idle());
}
