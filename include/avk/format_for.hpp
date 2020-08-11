#pragma once
#include <avk/avk.hpp>

namespace avk
{
	/** Convenience overloads for some known data types:
	 *	Feel free to provide custom overloads for your own/further types!
	 */
	template <typename T>
	vk::Format format_for();

	// f32
	template <>	inline vk::Format format_for<std::array<float, 4>>()		{ return vk::Format::eR32G32B32A32Sfloat; }
	template <>	inline vk::Format format_for<std::array<float, 3>>()		{ return vk::Format::eR32G32B32Sfloat; }
	template <>	inline vk::Format format_for<std::array<float, 2>>()		{ return vk::Format::eR32G32Sfloat; }
	template <>	inline vk::Format format_for<std::array<float, 1>>()		{ return vk::Format::eR32Sfloat; }
	template <>	inline vk::Format format_for<float>()						{ return vk::Format::eR32Sfloat; }
	// f64
	template <>	inline vk::Format format_for<std::array<double, 4>>()		{ return vk::Format::eR64G64B64A64Sfloat; }
	template <>	inline vk::Format format_for<std::array<double, 3>>()		{ return vk::Format::eR64G64B64Sfloat; }
	template <>	inline vk::Format format_for<std::array<double, 2>>()		{ return vk::Format::eR64G64Sfloat; }
	template <>	inline vk::Format format_for<std::array<double, 1>>()		{ return vk::Format::eR64Sfloat; }
	template <>	inline vk::Format format_for<double>()						{ return vk::Format::eR64Sfloat; }
	// i8
	template <>	inline vk::Format format_for<std::array<int8_t, 4>>()		{ return vk::Format::eR8G8B8A8Sint; }
	template <>	inline vk::Format format_for<std::array<int8_t, 3>>()		{ return vk::Format::eR8G8B8Sint; }
	template <>	inline vk::Format format_for<std::array<int8_t, 2>>()		{ return vk::Format::eR8G8Sint; }
	template <>	inline vk::Format format_for<std::array<int8_t, 1>>()		{ return vk::Format::eR8Sint; }
	template <>	inline vk::Format format_for<int8_t>()						{ return vk::Format::eR8Sint; }
	// i16
	template <>	inline vk::Format format_for<std::array<int16_t, 4>>()		{ return vk::Format::eR16G16B16A16Sint; }
	template <>	inline vk::Format format_for<std::array<int16_t, 3>>()		{ return vk::Format::eR16G16B16Sint; }
	template <>	inline vk::Format format_for<std::array<int16_t, 2>>()		{ return vk::Format::eR16G16Sint; }
	template <>	inline vk::Format format_for<std::array<int16_t, 1>>()		{ return vk::Format::eR16Sint; }
	template <>	inline vk::Format format_for<int16_t>()						{ return vk::Format::eR16Sint; }
	// i32
	template <>	inline vk::Format format_for<std::array<int32_t, 4>>()		{ return vk::Format::eR32G32B32A32Sint; }
	template <>	inline vk::Format format_for<std::array<int32_t, 3>>()		{ return vk::Format::eR32G32B32Sint; }
	template <>	inline vk::Format format_for<std::array<int32_t, 2>>()		{ return vk::Format::eR32G32Sint; }
	template <>	inline vk::Format format_for<std::array<int32_t, 1>>()		{ return vk::Format::eR32Sint; }
	template <>	inline vk::Format format_for<int32_t>()						{ return vk::Format::eR32Sint; }
	// i64
	template <>	inline vk::Format format_for<std::array<int64_t, 4>>()		{ return vk::Format::eR64G64B64A64Sint; }
	template <>	inline vk::Format format_for<std::array<int64_t, 3>>()		{ return vk::Format::eR64G64B64Sint; }
	template <>	inline vk::Format format_for<std::array<int64_t, 2>>()		{ return vk::Format::eR64G64Sint; }
	template <>	inline vk::Format format_for<std::array<int64_t, 1>>()		{ return vk::Format::eR64Sint; }
	template <>	inline vk::Format format_for<int64_t>()						{ return vk::Format::eR64Sint; }
	// u8
	template <> inline vk::Format format_for<std::array<uint8_t, 4>>()		{ return vk::Format::eR8G8B8A8Uint; }
	template <> inline vk::Format format_for<std::array<uint8_t, 3>>()		{ return vk::Format::eR8G8B8Uint; }
	template <> inline vk::Format format_for<std::array<uint8_t, 2>>()		{ return vk::Format::eR8G8Uint; }
	template <> inline vk::Format format_for<std::array<uint8_t, 1>>()		{ return vk::Format::eR8Uint; }
	template <> inline vk::Format format_for<uint8_t>()						{ return vk::Format::eR8Uint; }
	// u16
	template <>	inline vk::Format format_for<std::array<uint16_t, 4>>()		{ return vk::Format::eR16G16B16A16Uint; }
	template <>	inline vk::Format format_for<std::array<uint16_t, 3>>()		{ return vk::Format::eR16G16B16Uint; }
	template <>	inline vk::Format format_for<std::array<uint16_t, 2>>()		{ return vk::Format::eR16G16Uint; }
	template <>	inline vk::Format format_for<std::array<uint16_t, 1>>()		{ return vk::Format::eR16Uint; }
	template <>	inline vk::Format format_for<uint16_t>()					{ return vk::Format::eR16Uint; }
	// u32
	template <>	inline vk::Format format_for<std::array<uint32_t, 4>>()		{ return vk::Format::eR32G32B32A32Uint; }
	template <>	inline vk::Format format_for<std::array<uint32_t, 3>>()		{ return vk::Format::eR32G32B32Uint; }
	template <>	inline vk::Format format_for<std::array<uint32_t, 2>>()		{ return vk::Format::eR32G32Uint; }
	template <>	inline vk::Format format_for<std::array<uint32_t, 1>>()		{ return vk::Format::eR32Uint; }
	template <>	inline vk::Format format_for<uint32_t>()					{ return vk::Format::eR32Uint; }
	// u64
	template <>	inline vk::Format format_for<std::array<uint64_t, 4>>()		{ return vk::Format::eR64G64B64A64Uint; }
	template <>	inline vk::Format format_for<std::array<uint64_t, 3>>()		{ return vk::Format::eR64G64B64Uint; }
	template <>	inline vk::Format format_for<std::array<uint64_t, 2>>()		{ return vk::Format::eR64G64Uint; }
	template <>	inline vk::Format format_for<std::array<uint64_t, 1>>()		{ return vk::Format::eR64Uint; }
	template <>	inline vk::Format format_for<uint64_t>()					{ return vk::Format::eR64Uint; }
	
#if VK_HEADER_VERSION >= 135
	// aabb
	template <>	inline vk::Format format_for<VkAabbPositionsKHR>()			{ return vk::Format::eUndefined; }
	template <>	inline vk::Format format_for<VkAccelerationStructureInstanceKHR>() { return vk::Format::eUndefined; }
#endif
}
