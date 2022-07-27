#pragma once

namespace avk 
{
#if VK_HEADER_VERSION >= 135
	extern VkAccelerationStructureInstanceKHR convert_for_gpu_usage(const geometry_instance& aGeomInst);
	extern std::vector<VkAccelerationStructureInstanceKHR> convert_for_gpu_usage(const std::vector<geometry_instance>& aGeomInstances);
#endif
	
	extern avk::command::action_type_command copy_image_to_another(avk::resource_argument<image_t> aSrcImage, avk::layout::image_layout aSrcImageLayout, avk::resource_argument<image_t> aDstImage, avk::layout::image_layout aDstImageLayout, vk::ImageAspectFlags aImageAspectFlags = vk::ImageAspectFlagBits::eColor);
	extern avk::command::action_type_command blit_image(avk::resource_argument<image_t> aSrcImage, avk::layout::image_layout aSrcImageLayout, avk::resource_argument<image_t> aDstImage, avk::layout::image_layout aDstImageLayout, vk::ImageAspectFlags aImageAspectFlags = vk::ImageAspectFlagBits::eColor, vk::Filter aFilter = vk::Filter::eNearest);

	extern avk::command::action_type_command copy_buffer_to_image_layer_mip_level(avk::resource_argument<buffer_t> aSrcBuffer, avk::resource_argument<image_t> aDstImage, uint32_t aDstLayer, uint32_t aDstLevel, avk::layout::image_layout aDstImageLayout, vk::ImageAspectFlags aImageAspectFlags = vk::ImageAspectFlagBits::eColor);
	extern avk::command::action_type_command copy_buffer_to_image_mip_level(avk::resource_argument<buffer_t> aSrcBuffer, avk::resource_argument<image_t> aDstImage, uint32_t aDstLevel, avk::layout::image_layout aDstImageLayout, vk::ImageAspectFlags aImageAspectFlags = vk::ImageAspectFlagBits::eColor);
	extern avk::command::action_type_command copy_buffer_to_image(avk::resource_argument<buffer_t> aSrcBuffer, avk::resource_argument<image_t> aDstImage, avk::layout::image_layout aDstImageLayout, vk::ImageAspectFlags aImageAspectFlags = vk::ImageAspectFlagBits::eColor);

	extern avk::command::action_type_command copy_buffer_to_another(avk::resource_argument<buffer_t> aSrcBuffer, avk::resource_argument<buffer_t> aDstBuffer, std::optional<vk::DeviceSize> aSrcOffset = {}, std::optional<vk::DeviceSize> aDstOffset = {}, std::optional<vk::DeviceSize> aDataSize = {});

	extern avk::command::action_type_command copy_image_layer_mip_level_to_buffer(avk::resource_argument<image_t> aSrcImage, avk::layout::image_layout aSrcImageLayout, uint32_t aSrcLayer, uint32_t aSrcLevel, vk::ImageAspectFlags aImageAspectFlags, avk::resource_argument<buffer_t> aDstBuffer, std::optional<vk::DeviceSize> aDstOffset = {});
	extern avk::command::action_type_command copy_image_mip_level_to_buffer(avk::resource_argument<image_t> aSrcImage, avk::layout::image_layout aSrcImageLayout, uint32_t aSrcLevel, vk::ImageAspectFlags aImageAspectFlags, avk::resource_argument<buffer_t> aDstBuffer, std::optional<vk::DeviceSize> aDstOffset = {});
	extern avk::command::action_type_command copy_image_to_buffer(avk::resource_argument<image_t> aSrcImage, avk::layout::image_layout aSrcImageLayout, vk::ImageAspectFlags aImageAspectFlags, avk::resource_argument<buffer_t> aDstBuffer, std::optional<vk::DeviceSize> aDstOffset = {});
}
