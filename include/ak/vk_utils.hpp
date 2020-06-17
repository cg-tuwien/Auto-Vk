#pragma once

namespace ak
{
	namespace cfg {
		enum struct blending_logic_operation;
	}

	/** Returns true if the given image format is a sRGB format
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_srgb_format(const vk::Format& pImageFormat);
	
	/** Returns true if the given image format stores the color channels in uint8-type storage
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_uint8_format(const vk::Format& pImageFormat);
	/** Returns true if the given image format stores the color channels in int8-type storage
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_int8_format(const vk::Format& pImageFormat);
	/** Returns true if the given image format stores the color channels in uint16-type storage
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_uint16_format(const vk::Format& pImageFormat);
	/** Returns true if the given image format stores the color channels in int16-type storage
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_int16_format(const vk::Format& pImageFormat);
	/** Returns true if the given image format stores the color channels in uint32-type storage
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_uint32_format(const vk::Format& pImageFormat);
	/** Returns true if the given image format stores the color channels in int32-type storage
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_int32_format(const vk::Format& pImageFormat);
	/** Returns true if the given image format stores the color channels in float-type storage
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_float_format(const vk::Format& pImageFormat);
	/** Returns true if the given image format stores the color channels in float16-type storage
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_float16_format(const vk::Format& pImageFormat);
	/** Returns true if the given image format stores the color channels in float32-type storage
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_float32_format(const vk::Format& pImageFormat);
	/** Returns true if the given image format stores the color channels in float64-type storage
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_float64_format(const vk::Format& pImageFormat);
	/** Returns true if the given image format is a format which has only one color component.
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_1component_format(const vk::Format& pImageFormat);

	/** Returns true if the given image format is a format which has two color components.
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_2component_format(const vk::Format& pImageFormat);
	
	/** Returns true if the given image's color channels are ordered like follows: RGB
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_rgb_format(const vk::Format& pImageFormat);
	/** Returns true if the given image's color channels are ordered like follows: BGR
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_bgr_format(const vk::Format& pImageFormat);
	/** Returns true if the given image format is a format which has three color component.
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_3component_format(const vk::Format& pImageFormat);
	
	/** Returns true if the given image's color channels are ordered like follows: RGBA
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_rgba_format(const vk::Format& pImageFormat);
	/** Returns true if the given image's color channels are ordered like follows: ARGB
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_argb_format(const vk::Format& pImageFormat);
	/** Returns true if the given image's color channels are ordered like follows: BGRA
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_bgra_format(const vk::Format& pImageFormat);
	/** Returns true if the given image's color channels are ordered like follows: ABGR
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_abgr_format(const vk::Format& pImageFormat);
	/** Returns true if the given image format is a format which has four color components (or three color components + one alpha component, more precisely).
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_4component_format(const vk::Format& pImageFormat);

	extern bool is_unorm_format(const vk::Format& pImageFormat);
	extern bool is_snorm_format(const vk::Format& pImageFormat);
	extern bool is_norm_format(const vk::Format& pImageFormat);
	
	/** Returns true if the given image format is a depth/depth-stencil format and has a stencil component.
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool has_stencil_component(const vk::Format& pImageFormat);
	/** Returns true if the given image format is a depth/depth-stencil format and has a stencil component.
	*	Please note: This function does not guarantee completeness for all formats, i.e. false negatives must be expected. */
	extern bool is_depth_format(const vk::Format& pImageFormat);


	/** Analyze the given `ak::image_usage` flags, and assemble some (hopefully valid) `vk::ImageUsageFlags`, and determine `vk::ImageLayout` and `vk::ImageTiling`. */
	extern std::tuple<vk::ImageUsageFlags, vk::ImageLayout, vk::ImageTiling, vk::ImageCreateFlags> determine_usage_layout_tiling_flags_based_on_image_usage(ak::image_usage aImageUsageFlags);

	
}
