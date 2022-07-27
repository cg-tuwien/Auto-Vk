#pragma once
#include <avk/avk.hpp>

namespace avk
{
	/** Prints all the different memory types that are available on the device along with its memory property flags. */
	extern void print_available_memory_types_for_device(const vk::PhysicalDevice& aPhysicalDevice);
	
	/** Find (index of) memory with parameters
	 *	@param	aMemoryTypeBits		Bit field of the memory types that are suitable for the buffer. [9]
	 *	@param	aMemoryProperties	Special features of the memory, like being able to map it so we can write to it from the CPU. [9]
	 *	@return	A tuple with the following elements:
	 *			[0]: The selected memory index which satisfies the requirements indicated by both, aMemoryTypeBits and aMemoryProperties.
	 *				 (If no suitable memory can be found, this function will throw.)
	 *			[1]: The actual memory property flags which are supported by the selected memory. The include at least aMemoryProperties,
	 *				 but can also have additional memory property flags set.
	 */
	extern std::tuple<uint32_t, vk::MemoryPropertyFlags> find_memory_type_index_for_device(const vk::PhysicalDevice& aPhysicalDevice, uint32_t aMemoryTypeBits, vk::MemoryPropertyFlags aMemoryProperties);
	
	
	/** Returns true if the given image format is a sRGB format
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_srgb_format(const vk::Format& aImageFormat);
	
	/** Returns true if the given image format stores the color channels in uint8-type storage
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_uint8_format(const vk::Format& aImageFormat);
	/** Returns true if the given image format stores the color channels in int8-type storage
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_int8_format(const vk::Format& aImageFormat);
	/** Returns true if the given image format stores the color channels in uint16-type storage
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_uint16_format(const vk::Format& aImageFormat);
	/** Returns true if the given image format stores the color channels in int16-type storage
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_int16_format(const vk::Format& aImageFormat);
	/** Returns true if the given image format stores the color channels in uint32-type storage
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_uint32_format(const vk::Format& aImageFormat);
	/** Returns true if the given image format stores the color channels in int32-type storage
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_int32_format(const vk::Format& aImageFormat);
	/** Returns true if the given image format stores the color channels in float-type storage
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_float_format(const vk::Format& aImageFormat);
	/** Returns true if the given image format stores the color channels in float16-type storage
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_float16_format(const vk::Format& aImageFormat);
	/** Returns true if the given image format stores the color channels in float32-type storage
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_float32_format(const vk::Format& aImageFormat);
	/** Returns true if the given image format stores the color channels in float64-type storage
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_float64_format(const vk::Format& aImageFormat);
	/** Returns true if the given image format is a format which has only one color component.
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_1component_format(const vk::Format& aImageFormat);

	/** Returns true if the given image format is a format which has two color components.
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_2component_format(const vk::Format& aImageFormat);

	/** Returns true if the given image format is a format which is one of the signed integer formats.
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_int_format(const vk::Format& aImageFormat);

	/** Returns true if the given image format is a format which is one of the unsigned integer formats.
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_uint_format(const vk::Format& aImageFormat);
	
	/** Returns true if the given image's color channels are ordered like follows: RGB
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_rgb_format(const vk::Format& aImageFormat);
	/** Returns true if the given image's color channels are ordered like follows: BGR
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_bgr_format(const vk::Format& aImageFormat);
	/** Returns true if the given image format is a format which has three color component.
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_3component_format(const vk::Format& aImageFormat);
	
	/** Returns true if the given image's color channels are ordered like follows: RGBA
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_rgba_format(const vk::Format& aImageFormat);
	/** Returns true if the given image's color channels are ordered like follows: ARGB
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_argb_format(const vk::Format& aImageFormat);
	/** Returns true if the given image's color channels are ordered like follows: BGRA
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_bgra_format(const vk::Format& aImageFormat);
	/** Returns true if the given image's color channels are ordered like follows: ABGR
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_abgr_format(const vk::Format& aImageFormat);
	/** Returns true if the given image format is a format which has four color components (or three color components + one alpha component, more precisely).
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_4component_format(const vk::Format& aImageFormat);

	/** Returns true if the given image format is a format which is one of the unorm formats.
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_unorm_format(const vk::Format& aImageFormat);

	/** Returns true if the given image format is a format which is one of the snorm formats.
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_snorm_format(const vk::Format& aImageFormat);

	/** Returns true if the given image format is an unorm or snorm format.
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_norm_format(const vk::Format& aImageFormat);

	/** Returns true if the given image format is one of the block compressed formats.
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_block_compressed_format(const vk::Format& aImageFormat);
	
	/** Returns true if the given image format is a depth/depth-stencil format and has a stencil component.
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool has_stencil_component(const vk::Format& aImageFormat);
	/** Returns true if the given image format is a depth/depth-stencil format and has a stencil component.
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_depth_format(const vk::Format& aImageFormat);


	/** Analyze the given `ak::image_usage` flags, and assemble some (hopefully valid) `vk::ImageUsageFlags`, and determine `vk::ImageLayout` and `vk::ImageTiling`. */
	extern std::tuple<vk::ImageUsageFlags, vk::ImageLayout, vk::ImageTiling, vk::ImageCreateFlags> determine_usage_layout_tiling_flags_based_on_image_usage(avk::image_usage aImageUsageFlags);
}
