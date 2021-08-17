#define NOMINMAX
#include <avk/avk_log.hpp>
#include <avk/avk.hpp>

namespace avk
{
#pragma region root definitions
	void root::print_available_memory_types()
	{
		print_available_memory_types_for_device(physical_device());
	}

	std::tuple<uint32_t, vk::MemoryPropertyFlags> root::find_memory_type_index(uint32_t aMemoryTypeBits, vk::MemoryPropertyFlags aMemoryProperties)
	{
		return find_memory_type_index_for_device(physical_device(), aMemoryTypeBits, aMemoryProperties);
	}

	bool root::is_format_supported(vk::Format pFormat, vk::ImageTiling pTiling, vk::FormatFeatureFlags aFormatFeatures)
	{
		auto formatProps = physical_device().getFormatProperties(pFormat);
		if (pTiling == vk::ImageTiling::eLinear
			&& (formatProps.linearTilingFeatures & aFormatFeatures) == aFormatFeatures) {
			return true;
		}
		else if (pTiling == vk::ImageTiling::eOptimal
				 && (formatProps.optimalTilingFeatures & aFormatFeatures) == aFormatFeatures) {
			return true;
		}
		return false;
	}

#if VK_HEADER_VERSION >= 135
#if VK_HEADER_VERSION >= 162
	vk::PhysicalDeviceRayTracingPipelinePropertiesKHR root::get_ray_tracing_properties()
	{
		vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rtProps;
		vk::PhysicalDeviceProperties2 props2;
		props2.pNext = &rtProps;
		physical_device().getProperties2(&props2);
		return rtProps;
	}
#else
	vk::PhysicalDeviceRayTracingPropertiesKHR root::get_ray_tracing_properties()
	{
		vk::PhysicalDeviceRayTracingPropertiesKHR rtProps;
		vk::PhysicalDeviceProperties2 props2;
		props2.pNext = &rtProps;
		physical_device().getProperties2(&props2);
		return rtProps;
	}
#endif

	vk::DeviceAddress root::get_buffer_address(const vk::Device& aDevice, vk::Buffer aBufferHandle)
	{
		PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddress = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(aDevice, "vkGetBufferDeviceAddressKHR"));

	    VkBufferDeviceAddressInfo bufferAddressInfo;
		bufferAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		bufferAddressInfo.pNext = nullptr;
		bufferAddressInfo.buffer = aBufferHandle;

	    return vkGetBufferDeviceAddress(aDevice, &bufferAddressInfo);
	}

	vk::DeviceAddress root::get_buffer_address(vk::Buffer aBufferHandle)
	{
		return get_buffer_address(device(), aBufferHandle);
	}
#endif

	void root::finish_configuration(buffer_view_t& aBufferViewToBeFinished, vk::Format aViewFormat, std::function<void(buffer_view_t&)> aAlterConfigBeforeCreation)
	{
		aBufferViewToBeFinished.mInfo = vk::BufferViewCreateInfo{}
			.setBuffer(aBufferViewToBeFinished.buffer_handle())
			.setFormat(aViewFormat)
			.setOffset(0) // TODO: Support offsets
			.setRange(VK_WHOLE_SIZE); // TODO: Support ranges

		// Maybe alter the config?!
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(aBufferViewToBeFinished);
		}

		aBufferViewToBeFinished.mBufferView = device().createBufferViewUnique(aBufferViewToBeFinished.mInfo);

		// TODO: Descriptors?!
	}
#pragma endregion

#pragma region ak_error definitions
	runtime_error::runtime_error (const std::string& what_arg) : std::runtime_error(what_arg)
	{
		AVK_LOG_ERROR("!RUNTIME ERROR! " + what_arg);
	}

	runtime_error::runtime_error (const char* what_arg) : std::runtime_error(what_arg)
	{
		AVK_LOG_ERROR("!RUNTIME ERROR! " + std::string(what_arg));
	}

	logic_error::logic_error (const std::string& what_arg) : std::logic_error(what_arg)
	{
		AVK_LOG_ERROR("!LOGIC ERROR! " + what_arg);
	}

	logic_error::logic_error (const char* what_arg) : std::logic_error(what_arg)
	{
		AVK_LOG_ERROR("!LOGIC ERROR! " + std::string(what_arg));
	}
#pragma endregion

#pragma region vk_utils
	void print_available_memory_types_for_device(const vk::PhysicalDevice& aPhysicalDevice)
	{
		auto memProperties = aPhysicalDevice.getMemoryProperties();

		const auto deviceName = std::string(static_cast<const char*>(aPhysicalDevice.getProperties().deviceName));
		AVK_LOG_INFO("========== MEMORY PROPERTIES OF DEVICE '" + deviceName + "'");
		AVK_LOG_INFO("-------------------------------------------------------------");
		AVK_LOG_INFO(" HEAP TYPES:");
		AVK_LOG_INFO(" heap-idx |               bytes | heap flags");
		AVK_LOG_INFO("-------------------------------------------------------------");
		for (auto i = 0u; i < memProperties.memoryHeapCount; ++i) {
			std::string heapIndex =  "        " + std::to_string(i);
			heapIndex = heapIndex.substr(heapIndex.length() - 9);
			std::string heapSize =  std::to_string(memProperties.memoryHeaps[i].size);
			int n = static_cast<int>(heapSize.size());
			for (int x = n - 3; x > 0; x -= 3) {
				heapSize.insert(std::begin(heapSize) + x, ',');
			}
			heapSize = "                   " + heapSize;
			if (memProperties.memoryHeaps[i].size <= 999999999999999) {
				heapSize =  heapSize.substr(heapSize.length() - 19);
			}
			std::string heapFlags = vk::to_string(memProperties.memoryHeaps[i].flags);
			AVK_LOG_INFO(heapIndex + " | " + heapSize + " | " + heapFlags);
		}
		AVK_LOG_INFO("=============================================================");
		AVK_LOG_INFO(" MEMORY TYPES:");
		AVK_LOG_INFO(" mem-idx | heap-idx | memory propety flags");
		AVK_LOG_INFO("-------------------------------------------------------------");
		for (auto i = 0u; i < memProperties.memoryTypeCount; ++i) {
			std::string memIndex =  "       " + std::to_string(i);
			memIndex = memIndex.substr(memIndex.length() - 8);
			std::string heapIndex = "        " + std::to_string(memProperties.memoryTypes[i].heapIndex);
			heapIndex = heapIndex.substr(heapIndex.length() - 9);
			std::string propFlags = vk::to_string(memProperties.memoryTypes[i].propertyFlags);
			AVK_LOG_INFO(memIndex + " | " + heapIndex + " | " + propFlags);
		}
		AVK_LOG_INFO("=============================================================");

	}

	std::tuple<uint32_t, vk::MemoryPropertyFlags> find_memory_type_index_for_device(const vk::PhysicalDevice& aPhysicalDevice, uint32_t aMemoryTypeBits, vk::MemoryPropertyFlags aMemoryProperties)
	{
		// The VkPhysicalDeviceMemoryProperties structure has two arrays memoryTypes and memoryHeaps.
		// Memory heaps are distinct memory resources like dedicated VRAM and swap space in RAM for
		// when VRAM runs out. The different types of memory exist within these heaps. Right now we'll
		// only concern ourselves with the type of memory and not the heap it comes from, but you can
		// imagine that this can affect performance. (Source: https://vulkan-tutorial.com/)
		auto memProperties = aPhysicalDevice.getMemoryProperties();

		for (auto i = 0u; i < memProperties.memoryTypeCount; ++i) {
			if ((aMemoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & aMemoryProperties) == aMemoryProperties) {
				return std::make_tuple(i, memProperties.memoryTypes[i].propertyFlags);
			}
		}
		throw avk::runtime_error("failed to find suitable memory type!");
	}

	bool is_srgb_format(const vk::Format& aImageFormat)
	{
		// Note: Currently, the compressed formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> srgbFormats = {
			vk::Format::eR8Srgb,
			vk::Format::eR8G8Srgb,
			vk::Format::eR8G8B8Srgb,
			vk::Format::eB8G8R8Srgb,
			vk::Format::eR8G8B8A8Srgb,
			vk::Format::eB8G8R8A8Srgb,
			vk::Format::eA8B8G8R8SrgbPack32,
			vk::Format::eBc1RgbSrgbBlock,
			vk::Format::eBc1RgbaSrgbBlock,
			vk::Format::eBc2SrgbBlock,
			vk::Format::eBc3SrgbBlock,
			vk::Format::eBc7SrgbBlock,
		};
		auto it = std::find(std::begin(srgbFormats), std::end(srgbFormats), aImageFormat);
		return it != srgbFormats.end();
	}

	bool is_uint8_format(const vk::Format& aImageFormat)
	{
		// Note: Currently, the compressed formats are ignored => could/should be added in the future, maybe
		// TODO: sRGB-formats are assumed to be uint8-formats (not signed int8-formats) => is that true?
		static std::set<vk::Format> uint8Formats = {
			vk::Format::eR8Unorm,
			vk::Format::eR8Uscaled,
			vk::Format::eR8Uint,
			vk::Format::eR8Srgb,
			vk::Format::eR8G8Unorm,
			vk::Format::eR8G8Uscaled,
			vk::Format::eR8G8Uint,
			vk::Format::eR8G8Srgb,
			vk::Format::eR8G8B8Unorm,
			vk::Format::eR8G8B8Uscaled,
			vk::Format::eR8G8B8Uint,
			vk::Format::eR8G8B8Srgb,
			vk::Format::eB8G8R8Unorm,
			vk::Format::eB8G8R8Uscaled,
			vk::Format::eB8G8R8Uint,
			vk::Format::eB8G8R8Srgb,
			vk::Format::eR8G8B8A8Unorm,
			vk::Format::eR8G8B8A8Uscaled,
			vk::Format::eR8G8B8A8Uint,
			vk::Format::eR8G8B8A8Srgb,
			vk::Format::eB8G8R8A8Unorm,
			vk::Format::eB8G8R8A8Uscaled,
			vk::Format::eB8G8R8A8Uint,
			vk::Format::eB8G8R8A8Srgb,
			vk::Format::eA8B8G8R8UnormPack32,
			vk::Format::eA8B8G8R8UscaledPack32,
			vk::Format::eA8B8G8R8UintPack32,
			vk::Format::eA8B8G8R8SrgbPack32
		};
		auto it = std::find(std::begin(uint8Formats), std::end(uint8Formats), aImageFormat);
		return it != uint8Formats.end();
	}

	bool is_int8_format(const vk::Format& aImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> int8Formats = {
			vk::Format::eR8Snorm,
			vk::Format::eR8Sscaled,
			vk::Format::eR8Sint,
			vk::Format::eR8G8Snorm,
			vk::Format::eR8G8Sscaled,
			vk::Format::eR8G8Sint,
			vk::Format::eR8G8B8Snorm,
			vk::Format::eR8G8B8Sscaled,
			vk::Format::eR8G8B8Sint,
			vk::Format::eB8G8R8Snorm,
			vk::Format::eB8G8R8Sscaled,
			vk::Format::eB8G8R8Sint,
			vk::Format::eR8G8B8A8Snorm,
			vk::Format::eR8G8B8A8Sscaled,
			vk::Format::eR8G8B8A8Sint,
			vk::Format::eB8G8R8A8Snorm,
			vk::Format::eB8G8R8A8Sscaled,
			vk::Format::eB8G8R8A8Sint,
			vk::Format::eA8B8G8R8SnormPack32,
			vk::Format::eA8B8G8R8SscaledPack32,
			vk::Format::eA8B8G8R8SintPack32,
		};
		auto it = std::find(std::begin(int8Formats), std::end(int8Formats), aImageFormat);
		return it != int8Formats.end();
	}

	bool is_uint16_format(const vk::Format& aImageFormat)
	{
		// Note: Currently, the compressed formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> uint16Formats = {
			vk::Format::eR16Unorm,
			vk::Format::eR16Uscaled,
			vk::Format::eR16Uint,
			vk::Format::eR16G16Unorm,
			vk::Format::eR16G16Uscaled,
			vk::Format::eR16G16Uint,
			vk::Format::eR16G16B16Unorm,
			vk::Format::eR16G16B16Uscaled,
			vk::Format::eR16G16B16Uint,
			vk::Format::eR16G16B16A16Unorm,
			vk::Format::eR16G16B16A16Uscaled,
			vk::Format::eR16G16B16A16Uint
		};
		auto it = std::find(std::begin(uint16Formats), std::end(uint16Formats), aImageFormat);
		return it != uint16Formats.end();
	}

	bool is_int16_format(const vk::Format& aImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> int16Formats = {
			vk::Format::eR16Snorm,
			vk::Format::eR16Sscaled,
			vk::Format::eR16Sint,
			vk::Format::eR16G16Snorm,
			vk::Format::eR16G16Sscaled,
			vk::Format::eR16G16Sint,
			vk::Format::eR16G16B16Snorm,
			vk::Format::eR16G16B16Sscaled,
			vk::Format::eR16G16B16Sint,
			vk::Format::eR16G16B16A16Snorm,
			vk::Format::eR16G16B16A16Sscaled,
			vk::Format::eR16G16B16A16Sint
		};
		auto it = std::find(std::begin(int16Formats), std::end(int16Formats), aImageFormat);
		return it != int16Formats.end();
	}

	bool is_uint32_format(const vk::Format& aImageFormat)
	{
		// Note: Currently, the compressed formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> uint32Format = {
			vk::Format::eR32Uint,
			vk::Format::eR32G32Uint,
			vk::Format::eR32G32B32Uint,
			vk::Format::eR32G32B32A32Uint
		};
		auto it = std::find(std::begin(uint32Format), std::end(uint32Format), aImageFormat);
		return it != uint32Format.end();
	}

	bool is_int32_format(const vk::Format& aImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> int32Format = {
			vk::Format::eR32Sint,
			vk::Format::eR32G32Sint,
			vk::Format::eR32G32B32Sint,
			vk::Format::eR32G32B32A32Sint
		};
		auto it = std::find(std::begin(int32Format), std::end(int32Format), aImageFormat);
		return it != int32Format.end();
	}

	bool is_float_format(const vk::Format& aImageFormat)
	{
		return is_float16_format(aImageFormat) || is_float32_format(aImageFormat) || is_float64_format(aImageFormat);
	}

	bool is_float16_format(const vk::Format& aImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> float16Formats = {
			vk::Format::eR16Sfloat,
			vk::Format::eR16G16Sfloat,
			vk::Format::eR16G16B16Sfloat,
			vk::Format::eR16G16B16A16Sfloat
		};
		auto it = std::find(std::begin(float16Formats), std::end(float16Formats), aImageFormat);
		return it != float16Formats.end();
	}

	bool is_float32_format(const vk::Format& aImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> float32Formats = {
			vk::Format::eR32Sfloat,
			vk::Format::eR32G32Sfloat,
			vk::Format::eR32G32B32Sfloat,
			vk::Format::eR32G32B32A32Sfloat
		};
		auto it = std::find(std::begin(float32Formats), std::end(float32Formats), aImageFormat);
		return it != float32Formats.end();
	}

	bool is_float64_format(const vk::Format& aImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> float64Formats = {
			vk::Format::eR64Sfloat,
			vk::Format::eR64G64Sfloat,
			vk::Format::eR64G64B64Sfloat,
			vk::Format::eR64G64B64A64Sfloat
		};
		auto it = std::find(std::begin(float64Formats), std::end(float64Formats), aImageFormat);
		return it != float64Formats.end();
	}

	bool is_rgb_format(const vk::Format& aImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> rgbFormats = {
			vk::Format::eR5G6B5UnormPack16,
			vk::Format::eR8G8B8Unorm,
			vk::Format::eR8G8B8Snorm,
			vk::Format::eR8G8B8Uscaled,
			vk::Format::eR8G8B8Sscaled,
			vk::Format::eR8G8B8Uint,
			vk::Format::eR8G8B8Sint,
			vk::Format::eR8G8B8Srgb,
			vk::Format::eR16G16B16Unorm,
			vk::Format::eR16G16B16Snorm,
			vk::Format::eR16G16B16Uscaled,
			vk::Format::eR16G16B16Sscaled,
			vk::Format::eR16G16B16Uint,
			vk::Format::eR16G16B16Sint,
			vk::Format::eR16G16B16Sfloat,
			vk::Format::eR32G32B32Uint,
			vk::Format::eR32G32B32Sint,
			vk::Format::eR32G32B32Sfloat,
			vk::Format::eR64G64B64Uint,
			vk::Format::eR64G64B64Sint,
			vk::Format::eR64G64B64Sfloat,
			vk::Format::eBc1RgbUnormBlock,
			vk::Format::eBc1RgbSrgbBlock,
		};
		auto it = std::find(std::begin(rgbFormats), std::end(rgbFormats), aImageFormat);
		return it != rgbFormats.end();
	}

	bool is_rgba_format(const vk::Format& aImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> rgbaFormats = {
			vk::Format::eR4G4B4A4UnormPack16,
			vk::Format::eR5G5B5A1UnormPack16,
			vk::Format::eR8G8B8A8Unorm,
			vk::Format::eR8G8B8A8Snorm,
			vk::Format::eR8G8B8A8Uscaled,
			vk::Format::eR8G8B8A8Sscaled,
			vk::Format::eR8G8B8A8Uint,
			vk::Format::eR8G8B8A8Sint,
			vk::Format::eR8G8B8A8Srgb,
			vk::Format::eR16G16B16A16Unorm,
			vk::Format::eR16G16B16A16Snorm,
			vk::Format::eR16G16B16A16Uscaled,
			vk::Format::eR16G16B16A16Sscaled,
			vk::Format::eR16G16B16A16Uint,
			vk::Format::eR16G16B16A16Sint,
			vk::Format::eR16G16B16A16Sfloat,
			vk::Format::eR32G32B32A32Uint,
			vk::Format::eR32G32B32A32Sint,
			vk::Format::eR32G32B32A32Sfloat,
			vk::Format::eR64G64B64A64Uint,
			vk::Format::eR64G64B64A64Sint,
			vk::Format::eR64G64B64A64Sfloat,
			vk::Format::eBc1RgbaUnormBlock,
			vk::Format::eBc1RgbaSrgbBlock,
		};
		auto it = std::find(std::begin(rgbaFormats), std::end(rgbaFormats), aImageFormat);
		return it != rgbaFormats.end();
	}

	bool is_argb_format(const vk::Format& aImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> argbFormats = {
			vk::Format::eA1R5G5B5UnormPack16,
			vk::Format::eA2R10G10B10UnormPack32,
			vk::Format::eA2R10G10B10SnormPack32,
			vk::Format::eA2R10G10B10UscaledPack32,
			vk::Format::eA2R10G10B10SscaledPack32,
			vk::Format::eA2R10G10B10UintPack32,
			vk::Format::eA2R10G10B10SintPack32,
		};
		auto it = std::find(std::begin(argbFormats), std::end(argbFormats), aImageFormat);
		return it != argbFormats.end();
	}

	bool is_bgr_format(const vk::Format& aImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> bgrFormats = {
			vk::Format::eB5G6R5UnormPack16,
			vk::Format::eB8G8R8Unorm,
			vk::Format::eB8G8R8Snorm,
			vk::Format::eB8G8R8Uscaled,
			vk::Format::eB8G8R8Sscaled,
			vk::Format::eB8G8R8Uint,
			vk::Format::eB8G8R8Sint,
			vk::Format::eB8G8R8Srgb,
			vk::Format::eB10G11R11UfloatPack32,
		};
		auto it = std::find(std::begin(bgrFormats), std::end(bgrFormats), aImageFormat);
		return it != bgrFormats.end();
	}

	bool is_bgra_format(const vk::Format& aImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> bgraFormats = {
			vk::Format::eB4G4R4A4UnormPack16,
			vk::Format::eB5G5R5A1UnormPack16,
			vk::Format::eR8G8B8A8Unorm,
			vk::Format::eR8G8B8A8Snorm,
			vk::Format::eR8G8B8A8Uscaled,
			vk::Format::eR8G8B8A8Sscaled,
			vk::Format::eR8G8B8A8Uint,
			vk::Format::eR8G8B8A8Sint,
			vk::Format::eR8G8B8A8Srgb,
			vk::Format::eB8G8R8A8Unorm,
			vk::Format::eB8G8R8A8Snorm,
			vk::Format::eB8G8R8A8Uscaled,
			vk::Format::eB8G8R8A8Sscaled,
			vk::Format::eB8G8R8A8Uint,
			vk::Format::eB8G8R8A8Sint,
			vk::Format::eB8G8R8A8Srgb,
		};
		auto it = std::find(std::begin(bgraFormats), std::end(bgraFormats), aImageFormat);
		return it != bgraFormats.end();
	}

	bool is_abgr_format(const vk::Format& aImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> abgrFormats = {
			vk::Format::eA8B8G8R8UnormPack32,
			vk::Format::eA8B8G8R8SnormPack32,
			vk::Format::eA8B8G8R8UscaledPack32,
			vk::Format::eA8B8G8R8SscaledPack32,
			vk::Format::eA8B8G8R8UintPack32,
			vk::Format::eA8B8G8R8SintPack32,
			vk::Format::eA8B8G8R8SrgbPack32,
			vk::Format::eA2B10G10R10UnormPack32,
			vk::Format::eA2B10G10R10SnormPack32,
			vk::Format::eA2B10G10R10UscaledPack32,
			vk::Format::eA2B10G10R10SscaledPack32,
			vk::Format::eA2B10G10R10UintPack32,
			vk::Format::eA2B10G10R10SintPack32,
		};
		auto it = std::find(std::begin(abgrFormats), std::end(abgrFormats), aImageFormat);
		return it != abgrFormats.end();
	}

	bool is_block_compressed_format(const vk::Format& aImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> bcFormats = {
			vk::Format::eBc1RgbUnormBlock,
			vk::Format::eBc1RgbSrgbBlock,
			vk::Format::eBc1RgbaUnormBlock,
			vk::Format::eBc1RgbaSrgbBlock,
			vk::Format::eBc2UnormBlock,
			vk::Format::eBc2SrgbBlock,
			vk::Format::eBc3UnormBlock,
			vk::Format::eBc3SrgbBlock,
			vk::Format::eBc4UnormBlock,
			vk::Format::eBc4SnormBlock,
			vk::Format::eBc5UnormBlock,
			vk::Format::eBc5SnormBlock,
			vk::Format::eBc6HUfloatBlock,
			vk::Format::eBc6HSfloatBlock,
			vk::Format::eBc7UnormBlock,
			vk::Format::eBc7SrgbBlock,
		};
		auto it = std::find(std::begin(bcFormats), std::end(bcFormats), aImageFormat);
		return it != bcFormats.end();
	}

	bool has_stencil_component(const vk::Format& aImageFormat)
	{
		static std::set<vk::Format> stencilFormats = {
			vk::Format::eD16UnormS8Uint,
			vk::Format::eD32SfloatS8Uint,
			vk::Format::eD24UnormS8Uint,
		};
		auto it = std::find(std::begin(stencilFormats), std::end(stencilFormats), aImageFormat);
		return it != stencilFormats.end();
	}

	bool is_depth_format(const vk::Format& aImageFormat)
	{
		static std::set<vk::Format> depthFormats = {
			vk::Format::eD16Unorm,
			vk::Format::eD16UnormS8Uint,
			vk::Format::eD24UnormS8Uint,
			vk::Format::eD32Sfloat,
			vk::Format::eD32SfloatS8Uint,
		};
		auto it = std::find(std::begin(depthFormats), std::end(depthFormats), aImageFormat);
		return it != depthFormats.end();
	}

	bool is_1component_format(const vk::Format& aImageFormat)
	{
		static std::set<vk::Format> singleCompFormats = {
			vk::Format::eR8Srgb,
			vk::Format::eR8Unorm,
			vk::Format::eR8Uscaled,
			vk::Format::eR8Uint,
			vk::Format::eR8Srgb,
			vk::Format::eR8Snorm,
			vk::Format::eR8Sscaled,
			vk::Format::eR8Sint,
			vk::Format::eR16Unorm,
			vk::Format::eR16Uscaled,
			vk::Format::eR16Uint,
			vk::Format::eR16Snorm,
			vk::Format::eR16Sscaled,
			vk::Format::eR16Sint,
			vk::Format::eR32Uint,
			vk::Format::eR32Sint,
			vk::Format::eR16Sfloat,
			vk::Format::eR32Sfloat,
			vk::Format::eR64Sfloat,
		};
		auto it = std::find(std::begin(singleCompFormats), std::end(singleCompFormats), aImageFormat);
		return it != singleCompFormats.end();
	}

	bool is_2component_format(const vk::Format& aImageFormat)
	{
		static std::set<vk::Format> twoComponentFormats = {
			vk::Format::eR8G8Srgb,
			vk::Format::eR8G8Unorm,
			vk::Format::eR8G8Uscaled,
			vk::Format::eR8G8Uint,
			vk::Format::eR8G8Srgb,
			vk::Format::eR8G8Snorm,
			vk::Format::eR8G8Sscaled,
			vk::Format::eR8G8Sint,
			vk::Format::eR16G16Unorm,
			vk::Format::eR16G16Uscaled,
			vk::Format::eR16G16Uint,
			vk::Format::eR16G16Snorm,
			vk::Format::eR16G16Sscaled,
			vk::Format::eR16G16Sint,
			vk::Format::eR32G32Uint,
			vk::Format::eR32G32Sint,
			vk::Format::eR16G16Sfloat,
			vk::Format::eR32G32Sfloat,
			vk::Format::eR64G64Sfloat,
		};
		auto it = std::find(std::begin(twoComponentFormats), std::end(twoComponentFormats), aImageFormat);
		return it != twoComponentFormats.end();
	}

	bool is_3component_format(const vk::Format& aImageFormat)
	{
		static std::set<vk::Format> threeCompFormat = {
			vk::Format::eR8G8B8Srgb,
			vk::Format::eB8G8R8Srgb,
			vk::Format::eR5G6B5UnormPack16,
			vk::Format::eR8G8B8Unorm,
			vk::Format::eR8G8B8Snorm,
			vk::Format::eR8G8B8Uscaled,
			vk::Format::eR8G8B8Sscaled,
			vk::Format::eR8G8B8Uint,
			vk::Format::eR8G8B8Sint,
			vk::Format::eR8G8B8Srgb,
			vk::Format::eR16G16B16Unorm,
			vk::Format::eR16G16B16Snorm,
			vk::Format::eR16G16B16Uscaled,
			vk::Format::eR16G16B16Sscaled,
			vk::Format::eR16G16B16Uint,
			vk::Format::eR16G16B16Sint,
			vk::Format::eR16G16B16Sfloat,
			vk::Format::eR32G32B32Uint,
			vk::Format::eR32G32B32Sint,
			vk::Format::eR32G32B32Sfloat,
			vk::Format::eR64G64B64Uint,
			vk::Format::eR64G64B64Sint,
			vk::Format::eR64G64B64Sfloat,
			vk::Format::eB5G6R5UnormPack16,
			vk::Format::eB8G8R8Unorm,
			vk::Format::eB8G8R8Snorm,
			vk::Format::eB8G8R8Uscaled,
			vk::Format::eB8G8R8Sscaled,
			vk::Format::eB8G8R8Uint,
			vk::Format::eB8G8R8Sint,
			vk::Format::eB8G8R8Srgb,
			vk::Format::eB10G11R11UfloatPack32,
			vk::Format::eBc1RgbUnormBlock,
			vk::Format::eBc1RgbSrgbBlock,
		};
		auto it = std::find(std::begin(threeCompFormat), std::end(threeCompFormat), aImageFormat);
		return it != threeCompFormat.end();
	}

	bool is_4component_format(const vk::Format& aImageFormat)
	{
		static std::set<vk::Format> fourCompFormats = {
			vk::Format::eR8G8B8A8Srgb,
			vk::Format::eB8G8R8A8Srgb,
			vk::Format::eA8B8G8R8SrgbPack32,
			vk::Format::eR4G4B4A4UnormPack16,
			vk::Format::eR5G5B5A1UnormPack16,
			vk::Format::eR8G8B8A8Unorm,
			vk::Format::eR8G8B8A8Snorm,
			vk::Format::eR8G8B8A8Uscaled,
			vk::Format::eR8G8B8A8Sscaled,
			vk::Format::eR8G8B8A8Uint,
			vk::Format::eR8G8B8A8Sint,
			vk::Format::eR8G8B8A8Srgb,
			vk::Format::eR16G16B16A16Unorm,
			vk::Format::eR16G16B16A16Snorm,
			vk::Format::eR16G16B16A16Uscaled,
			vk::Format::eR16G16B16A16Sscaled,
			vk::Format::eR16G16B16A16Uint,
			vk::Format::eR16G16B16A16Sint,
			vk::Format::eR16G16B16A16Sfloat,
			vk::Format::eR32G32B32A32Uint,
			vk::Format::eR32G32B32A32Sint,
			vk::Format::eR32G32B32A32Sfloat,
			vk::Format::eR64G64B64A64Uint,
			vk::Format::eR64G64B64A64Sint,
			vk::Format::eR64G64B64A64Sfloat,
			vk::Format::eA1R5G5B5UnormPack16,
			vk::Format::eA2R10G10B10UnormPack32,
			vk::Format::eA2R10G10B10SnormPack32,
			vk::Format::eA2R10G10B10UscaledPack32,
			vk::Format::eA2R10G10B10SscaledPack32,
			vk::Format::eA2R10G10B10UintPack32,
			vk::Format::eA2R10G10B10SintPack32,
			vk::Format::eB4G4R4A4UnormPack16,
			vk::Format::eB5G5R5A1UnormPack16,
			vk::Format::eR8G8B8A8Unorm,
			vk::Format::eR8G8B8A8Snorm,
			vk::Format::eR8G8B8A8Uscaled,
			vk::Format::eR8G8B8A8Sscaled,
			vk::Format::eR8G8B8A8Uint,
			vk::Format::eR8G8B8A8Sint,
			vk::Format::eR8G8B8A8Srgb,
			vk::Format::eB8G8R8A8Unorm,
			vk::Format::eB8G8R8A8Snorm,
			vk::Format::eB8G8R8A8Uscaled,
			vk::Format::eB8G8R8A8Sscaled,
			vk::Format::eB8G8R8A8Uint,
			vk::Format::eB8G8R8A8Sint,
			vk::Format::eB8G8R8A8Srgb,
			vk::Format::eA8B8G8R8UnormPack32,
			vk::Format::eA8B8G8R8SnormPack32,
			vk::Format::eA8B8G8R8UscaledPack32,
			vk::Format::eA8B8G8R8SscaledPack32,
			vk::Format::eA8B8G8R8UintPack32,
			vk::Format::eA8B8G8R8SintPack32,
			vk::Format::eA8B8G8R8SrgbPack32,
			vk::Format::eA2B10G10R10UnormPack32,
			vk::Format::eA2B10G10R10SnormPack32,
			vk::Format::eA2B10G10R10UscaledPack32,
			vk::Format::eA2B10G10R10SscaledPack32,
			vk::Format::eA2B10G10R10UintPack32,
			vk::Format::eA2B10G10R10SintPack32,
			vk::Format::eBc1RgbaUnormBlock,
			vk::Format::eBc1RgbaSrgbBlock,
		};
		auto it = std::find(std::begin(fourCompFormats), std::end(fourCompFormats), aImageFormat);
		return it != fourCompFormats.end();
	}

	bool is_unorm_format(const vk::Format& aImageFormat)
	{
		static std::set<vk::Format> unormFormats = {
			vk::Format::eR8Unorm,
			vk::Format::eR8G8Unorm,
			vk::Format::eR8G8B8Unorm,
			vk::Format::eB8G8R8Unorm,
			vk::Format::eR8G8B8A8Unorm,
			vk::Format::eB8G8R8A8Unorm,
			vk::Format::eA8B8G8R8UnormPack32,
			vk::Format::eR16Unorm,
			vk::Format::eR16G16Unorm,
			vk::Format::eR16G16B16Unorm,
			vk::Format::eR16G16B16A16Unorm,
			vk::Format::eBc1RgbUnormBlock,
			vk::Format::eBc1RgbaUnormBlock,
			vk::Format::eBc2UnormBlock,
			vk::Format::eBc3UnormBlock,
			vk::Format::eBc4UnormBlock,
			vk::Format::eBc4SnormBlock,
			vk::Format::eBc5UnormBlock,
			vk::Format::eBc5SnormBlock,
			vk::Format::eBc7UnormBlock,
		};
		auto it = std::find(std::begin(unormFormats), std::end(unormFormats), aImageFormat);
		return it != unormFormats.end();
	}

	bool is_snorm_format(const vk::Format& aImageFormat)
	{
		static std::set<vk::Format> snormFormats = {
			vk::Format::eR8Snorm,
			vk::Format::eR8G8Snorm,
			vk::Format::eR8G8B8Snorm,
			vk::Format::eB8G8R8Snorm,
			vk::Format::eR8G8B8A8Snorm,
			vk::Format::eB8G8R8A8Snorm,
			vk::Format::eA8B8G8R8SnormPack32,
			vk::Format::eR16Snorm,
			vk::Format::eR16G16Snorm,
			vk::Format::eR16G16B16Snorm,
			vk::Format::eR16G16B16A16Snorm
		};
		auto it = std::find(std::begin(snormFormats), std::end(snormFormats), aImageFormat);
		return it != snormFormats.end();
	}

	bool is_norm_format(const vk::Format& aImageFormat)
	{
		return is_unorm_format(aImageFormat) || is_snorm_format(aImageFormat) || is_srgb_format(aImageFormat);
	}

	std::tuple<vk::ImageUsageFlags, vk::ImageLayout, vk::ImageTiling, vk::ImageCreateFlags> determine_usage_layout_tiling_flags_based_on_image_usage(avk::image_usage aImageUsageFlags)
	{
		vk::ImageUsageFlags imageUsage{};

		bool isReadOnly = avk::has_flag(aImageUsageFlags, avk::image_usage::read_only);
		avk::image_usage cleanedUpUsageFlagsForReadOnly = exclude(aImageUsageFlags, avk::image_usage::transfer_source | avk::image_usage::transfer_destination | avk::image_usage::sampled | avk::image_usage::read_only | avk::image_usage::presentable | avk::image_usage::shared_presentable | avk::image_usage::tiling_optimal | avk::image_usage::tiling_linear | avk::image_usage::sparse_memory_binding | avk::image_usage::cube_compatible | avk::image_usage::is_protected); // TODO: To be verified, it's just a guess.

		auto targetLayout = isReadOnly ? vk::ImageLayout::eShaderReadOnlyOptimal : vk::ImageLayout::eGeneral; // General Layout or Shader Read Only Layout is the default
		auto imageTiling = vk::ImageTiling::eOptimal; // Optimal is the default
		vk::ImageCreateFlags imageCreateFlags{};

		if (avk::has_flag(aImageUsageFlags, avk::image_usage::transfer_source)) {
			imageUsage |= vk::ImageUsageFlagBits::eTransferSrc;
			avk::image_usage cleanedUpUsageFlags = exclude(aImageUsageFlags, avk::image_usage::read_only | avk::image_usage::presentable | avk::image_usage::shared_presentable | avk::image_usage::tiling_optimal | avk::image_usage::tiling_linear | avk::image_usage::sparse_memory_binding | avk::image_usage::cube_compatible | avk::image_usage::is_protected | avk::image_usage::mip_mapped); // TODO: To be verified, it's just a guess.
			if (avk::image_usage::transfer_source == cleanedUpUsageFlags) {
				targetLayout = vk::ImageLayout::eTransferSrcOptimal;
			}
			else {
				targetLayout = vk::ImageLayout::eGeneral;
			}
		}
		if (avk::has_flag(aImageUsageFlags, avk::image_usage::transfer_destination)) {
			imageUsage |= vk::ImageUsageFlagBits::eTransferDst;
			avk::image_usage cleanedUpUsageFlags = exclude(aImageUsageFlags, avk::image_usage::read_only | avk::image_usage::presentable | avk::image_usage::shared_presentable | avk::image_usage::tiling_optimal | avk::image_usage::tiling_linear | avk::image_usage::sparse_memory_binding | avk::image_usage::cube_compatible | avk::image_usage::is_protected | avk::image_usage::mip_mapped); // TODO: To be verified, it's just a guess.
			if (avk::image_usage::transfer_destination == cleanedUpUsageFlags) {
				targetLayout = vk::ImageLayout::eTransferDstOptimal;
			}
			else {
				targetLayout = vk::ImageLayout::eGeneral;
			}
		}
		if (avk::has_flag(aImageUsageFlags, avk::image_usage::sampled)) {
			imageUsage |= vk::ImageUsageFlagBits::eSampled;
		}
		if (avk::has_flag(aImageUsageFlags, avk::image_usage::color_attachment)) {
			imageUsage |= vk::ImageUsageFlagBits::eColorAttachment;
			targetLayout = vk::ImageLayout::eColorAttachmentOptimal;
		}
		if (avk::has_flag(aImageUsageFlags, avk::image_usage::depth_stencil_attachment)) {
			imageUsage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
			if (isReadOnly && avk::image_usage::depth_stencil_attachment == cleanedUpUsageFlagsForReadOnly) {
				targetLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal;
			}
			else {
				targetLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
			}
		}
		if (avk::has_flag(aImageUsageFlags, avk::image_usage::input_attachment)) {
			imageUsage |= vk::ImageUsageFlagBits::eInputAttachment;
		}
		if (avk::has_flag(aImageUsageFlags, avk::image_usage::shading_rate_image)) {
			imageUsage |= vk::ImageUsageFlagBits::eShadingRateImageNV;
		}
		if (avk::has_flag(aImageUsageFlags, avk::image_usage::presentable)) {
			targetLayout = vk::ImageLayout::ePresentSrcKHR; // TODO: This probably needs some further action(s) => implement that further action(s)
		}
		if (avk::has_flag(aImageUsageFlags, avk::image_usage::shared_presentable)) {
			targetLayout = vk::ImageLayout::eSharedPresentKHR; // TODO: This probably needs some further action(s) => implement that further action(s)
		}
		if (avk::has_flag(aImageUsageFlags, avk::image_usage::tiling_optimal)) {
			imageTiling = vk::ImageTiling::eOptimal;
		}
		if (avk::has_flag(aImageUsageFlags, avk::image_usage::tiling_linear)) {
			imageTiling = vk::ImageTiling::eLinear;
		}
		if (avk::has_flag(aImageUsageFlags, avk::image_usage::sparse_memory_binding)) {
			imageCreateFlags |= vk::ImageCreateFlagBits::eSparseBinding;
		}
		if (avk::has_flag(aImageUsageFlags, avk::image_usage::cube_compatible)) {
			imageCreateFlags |= vk::ImageCreateFlagBits::eCubeCompatible;
		}
		if (avk::has_flag(aImageUsageFlags, avk::image_usage::is_protected)) {
			imageCreateFlags |= vk::ImageCreateFlagBits::eProtected;
		}
		if (avk::has_flag(aImageUsageFlags, avk::image_usage::mutable_format)) {
			imageCreateFlags |= vk::ImageCreateFlagBits::eMutableFormat;
		}
		if (avk::has_flag(aImageUsageFlags, avk::image_usage::shader_storage)) {
			imageUsage |= vk::ImageUsageFlagBits::eStorage;
			// Can not be Shader Read Only Layout
			targetLayout = vk::ImageLayout::eGeneral; // TODO: Verify that this should always be in general layout!
		}

		return std::make_tuple(imageUsage, targetLayout, imageTiling, imageCreateFlags);
	}
#pragma endregion

#pragma region vulkan helper functions
	vk::IndexType to_vk_index_type(size_t aSize)
	{
		if (aSize == sizeof(uint16_t)) {
			return vk::IndexType::eUint16;
		}
		if (aSize == sizeof(uint32_t)) {
			return vk::IndexType::eUint32;
		}
		AVK_LOG_ERROR("The given size[" + std::to_string(aSize) + "] does not correspond to a valid vk::IndexType");
		return vk::IndexType::eUint16;
	}

	vk::Bool32 to_vk_bool(bool value)
	{
		return value ? VK_TRUE : VK_FALSE;
	}

	vk::ShaderStageFlagBits to_vk_shader_stage(shader_type aType)
	{
		switch (aType) {
		case avk::shader_type::vertex:
			return vk::ShaderStageFlagBits::eVertex;
		case avk::shader_type::tessellation_control:
			return vk::ShaderStageFlagBits::eTessellationControl;
		case avk::shader_type::tessellation_evaluation:
			return vk::ShaderStageFlagBits::eTessellationEvaluation;
		case avk::shader_type::geometry:
			return vk::ShaderStageFlagBits::eGeometry;
		case avk::shader_type::fragment:
			return vk::ShaderStageFlagBits::eFragment;
		case avk::shader_type::compute:
			return vk::ShaderStageFlagBits::eCompute;
#if VK_HEADER_VERSION >= 135
		case avk::shader_type::ray_generation:
			return vk::ShaderStageFlagBits::eRaygenKHR;
		case avk::shader_type::any_hit:
			return vk::ShaderStageFlagBits::eAnyHitKHR;
		case avk::shader_type::closest_hit:
			return vk::ShaderStageFlagBits::eClosestHitKHR;
		case avk::shader_type::miss:
			return vk::ShaderStageFlagBits::eMissKHR;
		case avk::shader_type::intersection:
			return vk::ShaderStageFlagBits::eIntersectionKHR;
		case avk::shader_type::callable:
			return vk::ShaderStageFlagBits::eCallableKHR;
#endif
		case avk::shader_type::task:
			return vk::ShaderStageFlagBits::eTaskNV;
		case avk::shader_type::mesh:
			return vk::ShaderStageFlagBits::eMeshNV;
		default:
			throw avk::runtime_error("Invalid shader_type");
		}
	}

	vk::ShaderStageFlags to_vk_shader_stages(shader_type aType)
	{
		vk::ShaderStageFlags result;
		if ((aType & avk::shader_type::vertex) == avk::shader_type::vertex) {
			result |= vk::ShaderStageFlagBits::eVertex;
		}
		if ((aType & avk::shader_type::tessellation_control) == avk::shader_type::tessellation_control) {
			result |= vk::ShaderStageFlagBits::eTessellationControl;
		}
		if ((aType & avk::shader_type::tessellation_evaluation) == avk::shader_type::tessellation_evaluation) {
			result |= vk::ShaderStageFlagBits::eTessellationEvaluation;
		}
		if ((aType & avk::shader_type::geometry) == avk::shader_type::geometry) {
			result |= vk::ShaderStageFlagBits::eGeometry;
		}
		if ((aType & avk::shader_type::fragment) == avk::shader_type::fragment) {
			result |= vk::ShaderStageFlagBits::eFragment;
		}
		if ((aType & avk::shader_type::compute) == avk::shader_type::compute) {
			result |= vk::ShaderStageFlagBits::eCompute;
		}
#if VK_HEADER_VERSION >= 135
		if ((aType & avk::shader_type::ray_generation) == avk::shader_type::ray_generation) {
			result |= vk::ShaderStageFlagBits::eRaygenKHR;
		}
		if ((aType & avk::shader_type::any_hit) == avk::shader_type::any_hit) {
			result |= vk::ShaderStageFlagBits::eAnyHitKHR;
		}
		if ((aType & avk::shader_type::closest_hit) == avk::shader_type::closest_hit) {
			result |= vk::ShaderStageFlagBits::eClosestHitKHR;
		}
		if ((aType & avk::shader_type::miss) == avk::shader_type::miss) {
			result |= vk::ShaderStageFlagBits::eMissKHR;
		}
		if ((aType & avk::shader_type::intersection) == avk::shader_type::intersection) {
			result |= vk::ShaderStageFlagBits::eIntersectionKHR;
		}
		if ((aType & avk::shader_type::callable) == avk::shader_type::callable) {
			result |= vk::ShaderStageFlagBits::eCallableKHR;
		}
#endif
		if ((aType & avk::shader_type::task) == avk::shader_type::task) {
			result |= vk::ShaderStageFlagBits::eTaskNV;
		}
		if ((aType & avk::shader_type::mesh) == avk::shader_type::mesh) {
			result |= vk::ShaderStageFlagBits::eMeshNV;
		}
		return result;
	}

	vk::VertexInputRate to_vk_vertex_input_rate(vertex_input_buffer_binding::kind aValue)
	{
		switch (aValue) {
		case vertex_input_buffer_binding::kind::instance:
			return vk::VertexInputRate::eInstance;
		case vertex_input_buffer_binding::kind::vertex:
			return vk::VertexInputRate::eVertex;
		default:
			throw std::invalid_argument("Invalid vertex input rate");
		}
	}

	vk::PrimitiveTopology to_vk_primitive_topology(cfg::primitive_topology aValue)
	{
		using namespace cfg;

		switch (aValue) {
		case primitive_topology::points:
			return vk::PrimitiveTopology::ePointList;
		case primitive_topology::lines:
			return vk::PrimitiveTopology::eLineList;
		case primitive_topology::line_strip:
			return vk::PrimitiveTopology::eLineStrip;
		case primitive_topology::triangles:
			return vk::PrimitiveTopology::eTriangleList;
		case primitive_topology::triangle_strip:
			return vk::PrimitiveTopology::eTriangleStrip;
		case primitive_topology::triangle_fan:
			return vk::PrimitiveTopology::eTriangleFan;
		case primitive_topology::lines_with_adjacency:
			return vk::PrimitiveTopology::eLineListWithAdjacency;
		case primitive_topology::line_strip_with_adjacency:
			return vk::PrimitiveTopology::eLineStripWithAdjacency;
		case primitive_topology::triangles_with_adjacency:
			return vk::PrimitiveTopology::eTriangleListWithAdjacency;
		case primitive_topology::triangle_strip_with_adjacency:
			return vk::PrimitiveTopology::eTriangleStripWithAdjacency;
		case primitive_topology::patches:
			return vk::PrimitiveTopology::ePatchList;
		default:
			throw std::invalid_argument("Invalid primitive topology");
		}
	}

	vk::PolygonMode to_vk_polygon_mode(cfg::polygon_drawing_mode aValue)
	{
		using namespace cfg;

		switch (aValue) {
		case polygon_drawing_mode::fill:
			return vk::PolygonMode::eFill;
		case polygon_drawing_mode::line:
			return vk::PolygonMode::eLine;
		case polygon_drawing_mode::point:
			return vk::PolygonMode::ePoint;
		default:
			throw std::invalid_argument("Invalid polygon drawing mode.");
		}
	}

	vk::CullModeFlags to_vk_cull_mode(cfg::culling_mode aValue)
	{
		using namespace cfg;

		switch (aValue) {
		case culling_mode::disabled:
			return vk::CullModeFlagBits::eNone;
		case culling_mode::cull_front_faces:
			return vk::CullModeFlagBits::eFront;
		case culling_mode::cull_back_faces:
			return vk::CullModeFlagBits::eBack;
		case culling_mode::cull_front_and_back_faces:
			return vk::CullModeFlagBits::eFrontAndBack;
		default:
			throw std::invalid_argument("Invalid culling mode.");
		}
	}

	vk::FrontFace to_vk_front_face(cfg::winding_order aValue)
	{
		using namespace cfg;

		switch (aValue) {
		case winding_order::counter_clockwise:
			return vk::FrontFace::eCounterClockwise;
		case winding_order::clockwise:
			return vk::FrontFace::eClockwise;
		default:
			throw std::invalid_argument("Invalid front face winding order.");
		}
	}

	vk::CompareOp to_vk_compare_op(cfg::compare_operation aValue)
	{
		using namespace cfg;

		switch(aValue) {
		case compare_operation::never:
			return vk::CompareOp::eNever;
		case compare_operation::less:
			return vk::CompareOp::eLess;
		case compare_operation::equal:
			return vk::CompareOp::eEqual;
		case compare_operation::less_or_equal:
			return vk::CompareOp::eLessOrEqual;
		case compare_operation::greater:
			return vk::CompareOp::eGreater;
		case compare_operation::not_equal:
			return vk::CompareOp::eNotEqual;
		case compare_operation::greater_or_equal:
			return vk::CompareOp::eGreaterOrEqual;
		case compare_operation::always:
			return vk::CompareOp::eAlways;
		default:
			throw std::invalid_argument("Invalid compare operation.");
		}
	}

	vk::ColorComponentFlags to_vk_color_components(cfg::color_channel aValue)
	{
		using namespace cfg;

		switch (aValue)	{
		case color_channel::none:
			return vk::ColorComponentFlags{};
		case color_channel::red:
			return vk::ColorComponentFlagBits::eR;
		case color_channel::green:
			return vk::ColorComponentFlagBits::eG;
		case color_channel::blue:
			return vk::ColorComponentFlagBits::eB;
		case color_channel::alpha:
			return vk::ColorComponentFlagBits::eA;
		case color_channel::rg:
			return vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG;
		case color_channel::rgb:
			return vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB;
		case color_channel::rgba:
			return vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
		default:
			throw std::invalid_argument("Invalid color channel value.");
		}
	}

	vk::BlendFactor to_vk_blend_factor(cfg::blending_factor aValue)
	{
		using namespace cfg;

		switch (aValue) {
		case blending_factor::zero:
			return vk::BlendFactor::eZero;
		case blending_factor::one:
			return vk::BlendFactor::eOne;
		case blending_factor::source_color:
			return vk::BlendFactor::eSrcColor;
		case blending_factor::one_minus_source_color:
			return vk::BlendFactor::eOneMinusSrcColor;
		case blending_factor::destination_color:
			return vk::BlendFactor::eDstColor;
		case blending_factor::one_minus_destination_color:
			return vk::BlendFactor::eOneMinusDstColor;
		case blending_factor::source_alpha:
			return vk::BlendFactor::eSrcAlpha;
		case blending_factor::one_minus_source_alpha:
			return vk::BlendFactor::eOneMinusSrcAlpha;
		case blending_factor::destination_alpha:
			return vk::BlendFactor::eDstAlpha;
		case blending_factor::one_minus_destination_alpha:
			return vk::BlendFactor::eOneMinusDstAlpha;
		case blending_factor::constant_color:
			return vk::BlendFactor::eConstantColor;
		case blending_factor::one_minus_constant_color:
			return vk::BlendFactor::eOneMinusConstantColor;
		case blending_factor::constant_alpha:
			return vk::BlendFactor::eConstantAlpha;
		case blending_factor::one_minus_constant_alpha:
			return vk::BlendFactor::eOneMinusConstantAlpha;
		case blending_factor::source_alpha_saturate:
			return vk::BlendFactor::eSrcAlphaSaturate;
		default:
			throw std::invalid_argument("Invalid blend factor value.");
		}
	}

	vk::BlendOp to_vk_blend_operation(cfg::color_blending_operation aValue)
	{
		using namespace cfg;

		switch (aValue)
		{
		case color_blending_operation::add:
			return vk::BlendOp::eAdd;
		case color_blending_operation::subtract:
			return vk::BlendOp::eSubtract;
		case color_blending_operation::reverse_subtract:
			return vk::BlendOp::eReverseSubtract;
		case color_blending_operation::min:
			return vk::BlendOp::eMin;
		case color_blending_operation::max:
			return vk::BlendOp::eMax;
		default:
			throw std::invalid_argument("Invalid color blending operation.");
		}
	}

	vk::LogicOp to_vk_logic_operation(cfg::blending_logic_operation aValue)
	{
		using namespace cfg;

		switch (aValue)
		{
		case blending_logic_operation::op_clear:
			return vk::LogicOp::eClear;
		case blending_logic_operation::op_and:
			return vk::LogicOp::eAnd;
		case blending_logic_operation::op_and_reverse:
			return vk::LogicOp::eAndReverse;
		case blending_logic_operation::op_copy:
			return vk::LogicOp::eCopy;
		case blending_logic_operation::op_and_inverted:
			return vk::LogicOp::eAndInverted;
		case blending_logic_operation::no_op:
			return vk::LogicOp::eNoOp;
		case blending_logic_operation::op_xor:
			return vk::LogicOp::eXor;
		case blending_logic_operation::op_or:
			return vk::LogicOp::eOr;
		case blending_logic_operation::op_nor:
			return vk::LogicOp::eNor;
		case blending_logic_operation::op_equivalent:
			return vk::LogicOp::eEquivalent;
		case blending_logic_operation::op_invert:
			return vk::LogicOp::eInvert;
		case blending_logic_operation::op_or_reverse:
			return vk::LogicOp::eOrReverse;
		case blending_logic_operation::op_copy_inverted:
			return vk::LogicOp::eCopyInverted;
		case blending_logic_operation::op_or_inverted:
			return vk::LogicOp::eOrInverted;
		case blending_logic_operation::op_nand:
			return vk::LogicOp::eNand;
		case blending_logic_operation::op_set:
			return vk::LogicOp::eSet;
		default:
			throw std::invalid_argument("Invalid blending logic operation.");
		}
	}

	vk::AttachmentLoadOp to_vk_load_op(on_load aValue)
	{
		switch (aValue) {
		case on_load::dont_care:
			return vk::AttachmentLoadOp::eDontCare;
		case on_load::clear:
			return vk::AttachmentLoadOp::eClear;
		case on_load::load:
			return vk::AttachmentLoadOp::eLoad;
		default:
			throw std::invalid_argument("Invalid attachment load operation.");
		}
	}

	vk::AttachmentStoreOp to_vk_store_op(on_store aValue)
	{
		switch (aValue) {
		case on_store::dont_care:
			return vk::AttachmentStoreOp::eDontCare;
		case on_store::store:
		case on_store::store_in_presentable_format:
			return vk::AttachmentStoreOp::eStore;
		default:
			throw std::invalid_argument("Invalid attachment store operation.");
		}
	}

	vk::PipelineStageFlags to_vk_pipeline_stage_flags(avk::pipeline_stage aValue)
	{
		vk::PipelineStageFlags result;
		// TODO: This might be a bit expensive. Is there a different possible solution to this?
		if (avk::is_included(aValue, avk::pipeline_stage::top_of_pipe					)) { result |= vk::PipelineStageFlagBits::eTopOfPipe					; }
		if (avk::is_included(aValue, avk::pipeline_stage::draw_indirect					)) { result |= vk::PipelineStageFlagBits::eDrawIndirect					; }
		if (avk::is_included(aValue, avk::pipeline_stage::vertex_input					)) { result |= vk::PipelineStageFlagBits::eVertexInput					; }
		if (avk::is_included(aValue, avk::pipeline_stage::vertex_shader					)) { result |= vk::PipelineStageFlagBits::eVertexShader					; }
		if (avk::is_included(aValue, avk::pipeline_stage::tessellation_control_shader	)) { result |= vk::PipelineStageFlagBits::eTessellationControlShader	; }
		if (avk::is_included(aValue, avk::pipeline_stage::tessellation_evaluation_shader)) { result |= vk::PipelineStageFlagBits::eTessellationEvaluationShader	; }
		if (avk::is_included(aValue, avk::pipeline_stage::geometry_shader				)) { result |= vk::PipelineStageFlagBits::eGeometryShader				; }
		if (avk::is_included(aValue, avk::pipeline_stage::fragment_shader				)) { result |= vk::PipelineStageFlagBits::eFragmentShader				; }
		if (avk::is_included(aValue, avk::pipeline_stage::early_fragment_tests			)) { result |= vk::PipelineStageFlagBits::eEarlyFragmentTests			; }
		if (avk::is_included(aValue, avk::pipeline_stage::late_fragment_tests			)) { result |= vk::PipelineStageFlagBits::eLateFragmentTests			; }
		if (avk::is_included(aValue, avk::pipeline_stage::color_attachment_output		)) { result |= vk::PipelineStageFlagBits::eColorAttachmentOutput		; }
		if (avk::is_included(aValue, avk::pipeline_stage::compute_shader				)) { result |= vk::PipelineStageFlagBits::eComputeShader				; }
		if (avk::is_included(aValue, avk::pipeline_stage::transfer						)) { result |= vk::PipelineStageFlagBits::eTransfer						; }
		if (avk::is_included(aValue, avk::pipeline_stage::bottom_of_pipe				)) { result |= vk::PipelineStageFlagBits::eBottomOfPipe					; }
		if (avk::is_included(aValue, avk::pipeline_stage::host							)) { result |= vk::PipelineStageFlagBits::eHost							; }
		if (avk::is_included(aValue, avk::pipeline_stage::all_graphics			)) { result |= vk::PipelineStageFlagBits::eAllGraphics					; }
		if (avk::is_included(aValue, avk::pipeline_stage::all_commands					)) { result |= vk::PipelineStageFlagBits::eAllCommands					; }
		if (avk::is_included(aValue, avk::pipeline_stage::transform_feedback			)) { result |= vk::PipelineStageFlagBits::eTransformFeedbackEXT			; }
		if (avk::is_included(aValue, avk::pipeline_stage::conditional_rendering			)) { result |= vk::PipelineStageFlagBits::eConditionalRenderingEXT		; }
#if VK_HEADER_VERSION >= 135
		if (avk::is_included(aValue, avk::pipeline_stage::command_preprocess			)) { result |= vk::PipelineStageFlagBits::eCommandPreprocessNV			; }
#else
		if (avk::is_included(aValue, avk::pipeline_stage::command_preprocess			)) { result |= vk::PipelineStageFlagBits::eCommandProcessNVX			; }
#endif
		if (avk::is_included(aValue, avk::pipeline_stage::shading_rate_image			)) { result |= vk::PipelineStageFlagBits::eShadingRateImageNV			; }
#if VK_HEADER_VERSION >= 135
		if (avk::is_included(aValue, avk::pipeline_stage::ray_tracing_shaders			)) { result |= vk::PipelineStageFlagBits::eRayTracingShaderKHR			; }
		if (avk::is_included(aValue, avk::pipeline_stage::acceleration_structure_build	)) { result |= vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR; }
#endif
		if (avk::is_included(aValue, avk::pipeline_stage::task_shader					)) { result |= vk::PipelineStageFlagBits::eTaskShaderNV					; }
		if (avk::is_included(aValue, avk::pipeline_stage::mesh_shader					)) { result |= vk::PipelineStageFlagBits::eMeshShaderNV					; }
		if (avk::is_included(aValue, avk::pipeline_stage::fragment_density_process		)) { result |= vk::PipelineStageFlagBits::eFragmentDensityProcessEXT	; }
		return result;
	}

	vk::PipelineStageFlagBits to_vk_pipeline_stage_flag_bits(avk::pipeline_stage aValue)
	{
		if (avk::is_included(aValue, avk::pipeline_stage::top_of_pipe					)) { return vk::PipelineStageFlagBits::eTopOfPipe					; }
		if (avk::is_included(aValue, avk::pipeline_stage::draw_indirect					)) { return vk::PipelineStageFlagBits::eDrawIndirect					; }
		if (avk::is_included(aValue, avk::pipeline_stage::vertex_input					)) { return vk::PipelineStageFlagBits::eVertexInput					; }
		if (avk::is_included(aValue, avk::pipeline_stage::vertex_shader					)) { return vk::PipelineStageFlagBits::eVertexShader					; }
		if (avk::is_included(aValue, avk::pipeline_stage::tessellation_control_shader	)) { return vk::PipelineStageFlagBits::eTessellationControlShader	; }
		if (avk::is_included(aValue, avk::pipeline_stage::tessellation_evaluation_shader)) { return vk::PipelineStageFlagBits::eTessellationEvaluationShader	; }
		if (avk::is_included(aValue, avk::pipeline_stage::geometry_shader				)) { return vk::PipelineStageFlagBits::eGeometryShader				; }
		if (avk::is_included(aValue, avk::pipeline_stage::fragment_shader				)) { return vk::PipelineStageFlagBits::eFragmentShader				; }
		if (avk::is_included(aValue, avk::pipeline_stage::early_fragment_tests			)) { return vk::PipelineStageFlagBits::eEarlyFragmentTests			; }
		if (avk::is_included(aValue, avk::pipeline_stage::late_fragment_tests			)) { return vk::PipelineStageFlagBits::eLateFragmentTests			; }
		if (avk::is_included(aValue, avk::pipeline_stage::color_attachment_output		)) { return vk::PipelineStageFlagBits::eColorAttachmentOutput		; }
		if (avk::is_included(aValue, avk::pipeline_stage::compute_shader				)) { return vk::PipelineStageFlagBits::eComputeShader				; }
		if (avk::is_included(aValue, avk::pipeline_stage::transfer						)) { return vk::PipelineStageFlagBits::eTransfer						; }
		if (avk::is_included(aValue, avk::pipeline_stage::bottom_of_pipe				)) { return vk::PipelineStageFlagBits::eBottomOfPipe					; }
		if (avk::is_included(aValue, avk::pipeline_stage::host							)) { return vk::PipelineStageFlagBits::eHost							; }
		if (avk::is_included(aValue, avk::pipeline_stage::all_graphics					)) { return vk::PipelineStageFlagBits::eAllGraphics					; }
		if (avk::is_included(aValue, avk::pipeline_stage::all_commands					)) { return vk::PipelineStageFlagBits::eAllCommands					; }
		if (avk::is_included(aValue, avk::pipeline_stage::transform_feedback			)) { return vk::PipelineStageFlagBits::eTransformFeedbackEXT			; }
		if (avk::is_included(aValue, avk::pipeline_stage::conditional_rendering			)) { return vk::PipelineStageFlagBits::eConditionalRenderingEXT		; }
#if VK_HEADER_VERSION >= 135
		if (avk::is_included(aValue, avk::pipeline_stage::command_preprocess			)) { return vk::PipelineStageFlagBits::eCommandPreprocessNV			; }
#else
		if (avk::is_included(aValue, avk::pipeline_stage::command_preprocess			)) { return vk::PipelineStageFlagBits::eCommandProcessNVX			; }
#endif
		if (avk::is_included(aValue, avk::pipeline_stage::shading_rate_image			)) { return vk::PipelineStageFlagBits::eShadingRateImageNV			; }
#if VK_HEADER_VERSION >= 135
		if (avk::is_included(aValue, avk::pipeline_stage::ray_tracing_shaders			)) { return vk::PipelineStageFlagBits::eRayTracingShaderKHR			; }
		if (avk::is_included(aValue, avk::pipeline_stage::acceleration_structure_build	)) { return vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR; }
#endif
		if (avk::is_included(aValue, avk::pipeline_stage::task_shader					)) { return vk::PipelineStageFlagBits::eTaskShaderNV					; }
		if (avk::is_included(aValue, avk::pipeline_stage::mesh_shader					)) { return vk::PipelineStageFlagBits::eMeshShaderNV					; }
		if (avk::is_included(aValue, avk::pipeline_stage::fragment_density_process		)) { return vk::PipelineStageFlagBits::eFragmentDensityProcessEXT	; }

		throw avk::runtime_error("unknown pipeline_stage");
	}

	vk::PipelineStageFlags to_vk_pipeline_stage_flags(std::optional<avk::pipeline_stage> aValue)
	{
		if (aValue.has_value()) {
			return to_vk_pipeline_stage_flags(aValue.value());
		}
		return vk::PipelineStageFlags{};
	}

	vk::AccessFlags to_vk_access_flags(avk::memory_access aValue)
	{
		vk::AccessFlags result;
		// TODO: This might be a bit expensive. Is there a different possible solution to this?
		if (avk::is_included(aValue, avk::memory_access::indirect_command_data_read_access			)) { result |= vk::AccessFlagBits::eIndirectCommandRead; }
		if (avk::is_included(aValue, avk::memory_access::index_buffer_read_access					)) { result |= vk::AccessFlagBits::eIndexRead; }
		if (avk::is_included(aValue, avk::memory_access::vertex_buffer_read_access					)) { result |= vk::AccessFlagBits::eVertexAttributeRead; }
		if (avk::is_included(aValue, avk::memory_access::uniform_buffer_read_access					)) { result |= vk::AccessFlagBits::eUniformRead; }
		if (avk::is_included(aValue, avk::memory_access::input_attachment_read_access				)) { result |= vk::AccessFlagBits::eInputAttachmentRead; }
		if (avk::is_included(aValue, avk::memory_access::shader_buffers_and_images_read_access		)) { result |= vk::AccessFlagBits::eShaderRead; }
		if (avk::is_included(aValue, avk::memory_access::shader_buffers_and_images_write_access		)) { result |= vk::AccessFlagBits::eShaderWrite; }
		if (avk::is_included(aValue, avk::memory_access::color_attachment_read_access				)) { result |= vk::AccessFlagBits::eColorAttachmentRead; }
		if (avk::is_included(aValue, avk::memory_access::color_attachment_write_access				)) { result |= vk::AccessFlagBits::eColorAttachmentWrite; }
		if (avk::is_included(aValue, avk::memory_access::depth_stencil_attachment_read_access		)) { result |= vk::AccessFlagBits::eDepthStencilAttachmentRead; }
		if (avk::is_included(aValue, avk::memory_access::depth_stencil_attachment_write_access		)) { result |= vk::AccessFlagBits::eDepthStencilAttachmentWrite; }
		if (avk::is_included(aValue, avk::memory_access::transfer_read_access						)) { result |= vk::AccessFlagBits::eTransferRead; }
		if (avk::is_included(aValue, avk::memory_access::transfer_write_access						)) { result |= vk::AccessFlagBits::eTransferWrite; }
		if (avk::is_included(aValue, avk::memory_access::host_read_access							)) { result |= vk::AccessFlagBits::eHostRead; }
		if (avk::is_included(aValue, avk::memory_access::host_write_access							)) { result |= vk::AccessFlagBits::eHostWrite; }
		if (avk::is_included(aValue, avk::memory_access::any_read_access							)) { result |= vk::AccessFlagBits::eMemoryRead; }
		if (avk::is_included(aValue, avk::memory_access::any_write_access					 		)) { result |= vk::AccessFlagBits::eMemoryWrite; }
		if (avk::is_included(aValue, avk::memory_access::transform_feedback_write_access			)) { result |= vk::AccessFlagBits::eTransformFeedbackWriteEXT; }
		if (avk::is_included(aValue, avk::memory_access::transform_feedback_counter_read_access		)) { result |= vk::AccessFlagBits::eTransformFeedbackCounterReadEXT; }
		if (avk::is_included(aValue, avk::memory_access::transform_feedback_counter_write_access	)) { result |= vk::AccessFlagBits::eTransformFeedbackCounterWriteEXT; }
		if (avk::is_included(aValue, avk::memory_access::conditional_rendering_predicate_read_access)) { result |= vk::AccessFlagBits::eConditionalRenderingReadEXT; }
#if VK_HEADER_VERSION >= 135
		if (avk::is_included(aValue, avk::memory_access::command_preprocess_read_access				)) { result |= vk::AccessFlagBits::eCommandPreprocessReadNV; }
		if (avk::is_included(aValue, avk::memory_access::command_preprocess_write_access			)) { result |= vk::AccessFlagBits::eCommandPreprocessWriteNV; }
#else
		if (avk::is_included(aValue, avk::memory_access::command_preprocess_read_access				)) { result |= vk::AccessFlagBits::eCommandProcessReadNVX; }
		if (avk::is_included(aValue, avk::memory_access::command_preprocess_write_access			)) { result |= vk::AccessFlagBits::eCommandProcessWriteNVX; }
#endif
		if (avk::is_included(aValue, avk::memory_access::color_attachment_noncoherent_read_access	)) { result |= vk::AccessFlagBits::eColorAttachmentReadNoncoherentEXT; }
		if (avk::is_included(aValue, avk::memory_access::shading_rate_image_read_access				)) { result |= vk::AccessFlagBits::eShadingRateImageReadNV; }
#if VK_HEADER_VERSION >= 135
		if (avk::is_included(aValue, avk::memory_access::acceleration_structure_read_access			)) { result |= vk::AccessFlagBits::eAccelerationStructureReadKHR; }
		if (avk::is_included(aValue, avk::memory_access::acceleration_structure_write_access		)) { result |= vk::AccessFlagBits::eAccelerationStructureWriteKHR; }
#endif
		if (avk::is_included(aValue, avk::memory_access::fragment_density_map_attachment_read_access)) { result |= vk::AccessFlagBits::eFragmentDensityMapReadEXT; }

		return result;
	}

	vk::AccessFlags to_vk_access_flags(std::optional<avk::memory_access> aValue)
	{
		if (aValue.has_value()) {
			return to_vk_access_flags(aValue.value());
		}
		return vk::AccessFlags{};
	}

	avk::memory_access to_memory_access(avk::read_memory_access aValue)
	{
		return static_cast<avk::memory_access>(aValue);
	}

	std::optional<avk::memory_access> to_memory_access(std::optional<avk::read_memory_access> aValue)
	{
		if (aValue.has_value()) {
			return to_memory_access(aValue.value());
		}
		return {};
	}

	avk::memory_access to_memory_access(avk::write_memory_access aValue)
	{
		return static_cast<avk::memory_access>(aValue);
	}

	std::optional<avk::memory_access> to_memory_access(std::optional<avk::write_memory_access> aValue)
	{
		if (aValue.has_value()) {
			return to_memory_access(aValue.value());
		}
		return {};
	}

	filter_mode to_filter_mode(float aVulkanAnisotropy, bool aMipMappingAvailable)
	{
		if (aMipMappingAvailable) {
			if (aVulkanAnisotropy > 1.0f) {
				if (std::abs(aVulkanAnisotropy - 16.0f) <= 1.2e-7 /* ~machine epsilon */) {
					return avk::filter_mode::anisotropic_16x;
				}
				if (std::abs(aVulkanAnisotropy - 8.0f) <= 1.2e-7 /* ~machine epsilon */) {
					return avk::filter_mode::anisotropic_8x;
				}
				if (std::abs(aVulkanAnisotropy - 4.0f) <= 1.2e-7 /* ~machine epsilon */) {
					return avk::filter_mode::anisotropic_4x;
				}
				if (std::abs(aVulkanAnisotropy - 2.0f) <= 1.2e-7 /* ~machine epsilon */) {
					return avk::filter_mode::anisotropic_2x;
				}
				if (std::abs(aVulkanAnisotropy - 32.0f) <= 1.2e-7 /* ~machine epsilon */) {
					return avk::filter_mode::anisotropic_32x;
				}
				if (std::abs(aVulkanAnisotropy - 64.0f) <= 1.2e-7 /* ~machine epsilon */) {
					return avk::filter_mode::anisotropic_64x;
				}
				AVK_LOG_WARNING("Encountered a strange anisotropy value of " + std::to_string(aVulkanAnisotropy));
			}
			return avk::filter_mode::trilinear;
		}
		return avk::filter_mode::bilinear;
	}

	vk::ImageViewType to_image_view_type(const vk::ImageCreateInfo& info)
	{
		switch (info.imageType)
		{
		case vk::ImageType::e1D:
			if (info.arrayLayers > 1) {
				return vk::ImageViewType::e1DArray;
			}
			else {
				return vk::ImageViewType::e1D;
			}
		case vk::ImageType::e2D:
			if (info.flags & vk::ImageCreateFlagBits::eCubeCompatible) {
				// TODO: should a cubemap array with only 1 cubemap be supported?
				if (info.arrayLayers == 6) {
					return vk::ImageViewType::eCube;
				}
				else if (info.arrayLayers > 6) {
					return vk::ImageViewType::eCubeArray;
				}
			}
			else if (info.arrayLayers > 1) {
				return vk::ImageViewType::e2DArray;
			}
			else {
				return vk::ImageViewType::e2D;
			}
		case vk::ImageType::e3D:
			return vk::ImageViewType::e3D;
		}
		throw avk::runtime_error("It might be that the implementation of to_image_view_type(const vk::ImageCreateInfo& info) is incomplete. Please complete it!");
	}
#pragma endregion

#pragma region attachment definitions
	attachment attachment::declare(std::tuple<vk::Format, vk::SampleCountFlagBits> aFormatAndSamples, on_load aLoadOp, usage_desc aUsageInSubpasses, on_store aStoreOp)
	{
		return attachment{
			std::get<vk::Format>(aFormatAndSamples),
			std::get<vk::SampleCountFlagBits>(aFormatAndSamples),
			aLoadOp, aStoreOp,
			{},      {},
			std::move(aUsageInSubpasses),
			{ 0.0, 0.0, 0.0, 0.0 },
			1.0f, 0u
		};
	}

	attachment attachment::declare(vk::Format aFormat, on_load aLoadOp, usage_desc aUsageInSubpasses, on_store aStoreOp)
	{
		return declare({aFormat, vk::SampleCountFlagBits::e1}, aLoadOp, std::move(aUsageInSubpasses), aStoreOp);
	}

	attachment attachment::declare_for(resource_reference<const image_view_t> aImageView, avk::on_load aLoadOp, avk::usage_desc aUsageInSubpasses, avk::on_store aStoreOp)
	{
		const auto& imageConfig = aImageView->get_image().config();
		const auto format = imageConfig.format;
		const std::optional<image_usage> imageUsage = aImageView->get_image().usage_config();
		auto result = declare({format, imageConfig.samples}, aLoadOp, std::move(aUsageInSubpasses), aStoreOp);
		if (imageUsage.has_value()) {
			result.set_image_usage_hint(imageUsage.value());
		}
		return result;
	}
#pragma endregion

#pragma region acceleration structure definitions
#if VK_HEADER_VERSION >= 135
	acceleration_structure_size_requirements acceleration_structure_size_requirements::from_buffers(vertex_index_buffer_pair aPair)
	{
		const auto& vertexBufferMeta = aPair.vertex_buffer()->meta<vertex_buffer_meta>();
		const auto& indexBufferMeta = aPair.index_buffer()->meta<index_buffer_meta>();

		// Perform two sanity checks, because we really need the member descriptions to know where to find the positions.
		// 1st check:
		if (vertexBufferMeta.member_descriptions().size() == 0) {
			throw avk::runtime_error("ak::vertex_buffers passed to acceleration_structure_size_requirements::from_buffers must have a member_description for their positions element in their meta data.");
		}
		// Find member representing the positions
		auto posMember = vertexBufferMeta.member_description(content_description::position);

		return acceleration_structure_size_requirements{
			vk::GeometryTypeKHR::eTriangles,
			static_cast<uint32_t>(indexBufferMeta.num_elements()) / 3,
			indexBufferMeta.sizeof_one_element(),
			static_cast<uint32_t>(vertexBufferMeta.num_elements()),
			posMember.mFormat
		};
	}

	bottom_level_acceleration_structure root::create_bottom_level_acceleration_structure(std::vector<avk::acceleration_structure_size_requirements> aGeometryDescriptions, bool aAllowUpdates, std::function<void(bottom_level_acceleration_structure_t&)> aAlterConfigBeforeCreation, std::function<void(bottom_level_acceleration_structure_t&)> aAlterConfigBeforeMemoryAlloc)
	{
		bottom_level_acceleration_structure_t result;

		result.mFlags = aAllowUpdates
			? vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate | vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastBuild
			: vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;

		// 1. Gather all geometry descriptions and create vk::AccelerationStructureCreateGeometryTypeInfoKHR entries:
#if VK_HEADER_VERSION >= 162
		// This is the same structure that will later be used for the actual build, but the acceleration structure parameters and
		// geometry data pointers do not need to be fully populated at this point (although they can be), just the:
		//  - acceleration structure type,
		//  - and the geometry
		//     - types,
		//     - counts, and
		//     - maximum sizes.
		result.mAccStructureGeometries.reserve(aGeometryDescriptions.size());
		result.mBuildPrimitiveCounts.reserve(aGeometryDescriptions.size());

		for (auto& gd : aGeometryDescriptions) {
			auto& asg = result.mAccStructureGeometries.emplace_back();
			asg.setGeometryType(gd.mGeometryType)
			   .setFlags(vk::GeometryFlagsKHR{}); // TODO: Support flags
			switch(gd.mGeometryType) {
			case vk::GeometryTypeKHR::eTriangles:
				asg.setGeometry(vk::AccelerationStructureGeometryTrianglesDataKHR{}
					.setIndexType(avk::to_vk_index_type(gd.mIndexTypeSize))
					.setVertexFormat(gd.mVertexFormat)
					.setMaxVertex(gd.mNumVertices)
				);
				break;
			case vk::GeometryTypeKHR::eAabbs:
				asg.setGeometry(vk::AccelerationStructureGeometryAabbsDataKHR{});
				break;
			default:
				throw avk::runtime_error("Invalid vk::GeometryTypeKHR passed to create_bottom_level_acceleration_structure via avk::acceleration_structure_size_requirements");
			}

			result.mBuildPrimitiveCounts.push_back(gd.mNumPrimitives);
		}

		const auto* pointerToAnArray = result.mAccStructureGeometries.data();

		assert(result.mAccStructureGeometries.size() == result.mBuildPrimitiveCounts.size());
		result.mBuildGeometryInfo = vk::AccelerationStructureBuildGeometryInfoKHR{}
			.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
			.setFlags(result.mFlags)
			.setGeometryCount(result.mAccStructureGeometries.size())
			.setPpGeometries(&pointerToAnArray);
#else
		result.mGeometryInfos.reserve(aGeometryDescriptions.size());
		for (auto& gd : aGeometryDescriptions) {
			auto& back = result.mGeometryInfos.emplace_back()
				.setGeometryType(gd.mGeometryType)
				.setMaxPrimitiveCount(gd.mNumPrimitives)
				.setMaxVertexCount(gd.mNumVertices)
				.setVertexFormat(gd.mVertexFormat)
				.setAllowsTransforms(VK_FALSE); // TODO: Add support for transforms (allowsTransforms indicates whether transform data can be used by this acceleration structure or not, when geometryType is VK_GEOMETRY_TYPE_TRIANGLES_KHR.)
			if (vk::GeometryTypeKHR::eTriangles == gd.mGeometryType) {
				back.setIndexType(avk::to_vk_index_type(gd.mIndexTypeSize));
				// TODO: Support non-indexed geometry
			}
		} // for each geometry description
#endif

		// 2. Assemble info about the BOTTOM LEVEL acceleration structure and the set its geometry
		result.mCreateInfo = vk::AccelerationStructureCreateInfoKHR{}
			.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
#if VK_HEADER_VERSION >= 162
			// TODO: Something to do about compacted size?
			.setCreateFlags({}); // TODO: Support CreateFlags!
#else
			.setCompactedSize(0) // If compactedSize is 0 then maxGeometryCount must not be 0
			.setFlags(result.mFlags)
			.setMaxGeometryCount(static_cast<uint32_t>(result.mGeometryInfos.size()))
			.setPGeometryInfos(result.mGeometryInfos.data());
#endif

		// 3. Maybe alter the config?
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(result);
		}

		// Steps 5. to 10. in here:
		finish_acceleration_structure_creation(result, std::move(aAlterConfigBeforeMemoryAlloc));

		return result;
	}

	bottom_level_acceleration_structure_t::~bottom_level_acceleration_structure_t()
	{
		if (acceleration_structure_handle()) {
			mDevice.destroyAccelerationStructureKHR(acceleration_structure_handle(), nullptr, mDynamicDispatch);
			mAccStructure.reset();
		}
	}

	buffer_t& bottom_level_acceleration_structure_t::get_and_possibly_create_scratch_buffer()
	{
		if (!mScratchBuffer.has_value()) {
			mScratchBuffer = root::create_buffer(
				mPhysicalDevice, mDevice, mAllocator,
				avk::memory_usage::device,
#if VK_HEADER_VERSION >= 162
				// TODO: Looks like no special usage flags are required anymore?
				vk::BufferUsageFlagBits::eShaderDeviceAddressKHR,
#else
				vk::BufferUsageFlagBits::eRayTracingKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR,
#endif
				avk::generic_buffer_meta::create_from_size(std::max(required_scratch_buffer_build_size(), required_scratch_buffer_update_size()))
			);
		}
		assert(mScratchBuffer.has_value());
		return mScratchBuffer.value().get();
	}

	std::optional<command_buffer> bottom_level_acceleration_structure_t::build_or_update(const std::vector<vertex_index_buffer_pair>& aGeometries, std::optional<std::reference_wrapper<buffer_t>> aScratchBuffer, sync aSyncHandler, blas_action aBuildAction)
	{
		// TODO: into avk::commands

		// Set the aScratchBuffer parameter to an internal scratch buffer, if none has been passed:
		buffer_t& scratchBuffer = aScratchBuffer.value_or(get_and_possibly_create_scratch_buffer());

		std::vector<vk::AccelerationStructureGeometryKHR> accStructureGeometries;
		accStructureGeometries.reserve(aGeometries.size());

		std::vector<vk::AccelerationStructureBuildGeometryInfoKHR> buildGeometryInfos;
		buildGeometryInfos.reserve(aGeometries.size());

#if VK_HEADER_VERSION >= 162
		std::vector<vk::AccelerationStructureBuildRangeInfoKHR> buildRangeInfos;
		buildRangeInfos.reserve(aGeometries.size());
		std::vector<vk::AccelerationStructureBuildRangeInfoKHR*> buildRangeInfoPtrs; // Points to elements inside buildOffsetInfos... just... because!
		buildRangeInfoPtrs.reserve(aGeometries.size());
#else
		std::vector<vk::AccelerationStructureBuildOffsetInfoKHR> buildOffsetInfos;
		buildOffsetInfos.reserve(aGeometries.size());
		std::vector<vk::AccelerationStructureBuildOffsetInfoKHR*> buildOffsetInfoPtrs; // Points to elements inside buildOffsetInfos... just... because!
		buildOffsetInfoPtrs.reserve(aGeometries.size());
#endif

		for (auto& pair : aGeometries) {
			auto vertexBuffer = pair.vertex_buffer();
			const auto& vertexBufferMeta = vertexBuffer->meta<vertex_buffer_meta>();
			auto indexBuffer = pair.index_buffer();
			const auto& indexBufferMeta = indexBuffer->meta<index_buffer_meta>();

			if (vertexBufferMeta.member_descriptions().size() == 0) {
				throw avk::runtime_error("ak::vertex_buffers passed to acceleration_structure_size_requirements::from_buffers must have a member_description for their positions element in their meta data.");
			}
			// Find member representing the positions
			const auto& posMember = vertexBufferMeta.member_description(content_description::position);

			assert(vertexBuffer->has_device_address());
			assert(indexBuffer->has_device_address());

			accStructureGeometries.emplace_back()
				.setGeometryType(vk::GeometryTypeKHR::eTriangles)
				.setGeometry(vk::AccelerationStructureGeometryTrianglesDataKHR{}
					.setVertexFormat(posMember.mFormat)
					.setVertexData(vk::DeviceOrHostAddressConstKHR{ vertexBuffer->device_address() }) // TODO: Support host addresses
					.setVertexStride(static_cast<vk::DeviceSize>(vertexBufferMeta.sizeof_one_element()))
#if VK_HEADER_VERSION >= 162
					.setMaxVertex(static_cast<uint32_t>(vertexBufferMeta.num_elements()))
#endif
					.setIndexType(avk::to_vk_index_type(indexBufferMeta.sizeof_one_element()))
					.setIndexData(vk::DeviceOrHostAddressConstKHR{ indexBuffer->device_address() }) // TODO: Support host addresses
					.setTransformData(nullptr)
				)
				.setFlags(vk::GeometryFlagsKHR{}); // TODO: Support flags

#if VK_HEADER_VERSION >= 162
			auto& bri = buildRangeInfos.emplace_back()
				.setPrimitiveCount(static_cast<uint32_t>(indexBufferMeta.num_elements()) / 3u)
				.setPrimitiveOffset(0u)
				.setFirstVertex(0u)
				.setTransformOffset(0u); // TODO: Support different values for all these parameters?!
			buildRangeInfoPtrs.emplace_back(&bri);
#else
			auto& boi = buildOffsetInfos.emplace_back()
				.setPrimitiveCount(static_cast<uint32_t>(indexBufferMeta.num_elements()) / 3u)
				.setPrimitiveOffset(0u)
				.setFirstVertex(0u)
				.setTransformOffset(0u); // TODO: Support different values for all these parameters?!
			buildOffsetInfoPtrs.emplace_back(&boi);
#endif
		}

		const auto* pointerToAnArray = accStructureGeometries.data();

		buildGeometryInfos.emplace_back()
			.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
			.setFlags(mFlags) // TODO: support individual flags per geometry?
#if VK_HEADER_VERSION >= 162
			.setMode(aBuildAction == blas_action::build ? vk::BuildAccelerationStructureModeKHR::eBuild : vk::BuildAccelerationStructureModeKHR::eUpdate)
#else
			.setUpdate(aBuildAction == blas_action::build ? VK_FALSE : VK_TRUE)
			.setGeometryArrayOfPointers(VK_FALSE)
#endif
			.setSrcAccelerationStructure(aBuildAction == blas_action::build ? nullptr : acceleration_structure_handle())
			.setDstAccelerationStructure(acceleration_structure_handle())
			.setGeometryCount(static_cast<uint32_t>(accStructureGeometries.size()))
			.setPpGeometries(&pointerToAnArray)
			.setScratchData(vk::DeviceOrHostAddressKHR{ scratchBuffer.device_address() });

		auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
		// Sync before:
		aSyncHandler.establish_barrier_before_the_operation(pipeline_stage::acceleration_structure_build, read_memory_access{memory_access::acceleration_structure_read_access});

#if VK_HEADER_VERSION >= 162
		commandBuffer.handle().buildAccelerationStructuresKHR(
			static_cast<uint32_t>(buildGeometryInfos.size()),
			buildGeometryInfos.data(),
			buildRangeInfoPtrs.data(),
			mDynamicDispatch
		);
#else
		commandBuffer.handle().buildAccelerationStructureKHR(
			static_cast<uint32_t>(buildGeometryInfos.size()),
			buildGeometryInfos.data(),
			buildOffsetInfoPtrs.data(),
			mDynamicDispatch
		);
#endif

		// Sync after:
		aSyncHandler.establish_barrier_after_the_operation(pipeline_stage::acceleration_structure_build, write_memory_access{memory_access::acceleration_structure_write_access});

		// Finish him:
		return aSyncHandler.submit_and_sync();
	}

	std::optional<command_buffer> bottom_level_acceleration_structure_t::build(const std::vector<vertex_index_buffer_pair>& aGeometries, std::optional<std::reference_wrapper<buffer_t>> aScratchBuffer, sync aSyncHandler)
	{
		return build_or_update(aGeometries, aScratchBuffer, std::move(aSyncHandler), blas_action::build);
	}

	std::optional<command_buffer> bottom_level_acceleration_structure_t::update(const std::vector<vertex_index_buffer_pair>& aGeometries, std::optional<std::reference_wrapper<buffer_t>> aScratchBuffer, sync aSyncHandler)
	{
		return build_or_update(aGeometries, aScratchBuffer, std::move(aSyncHandler), blas_action::update);
	}

	std::optional<command_buffer> bottom_level_acceleration_structure_t::build_or_update(const std::vector<VkAabbPositionsKHR>& aGeometries, std::optional<std::reference_wrapper<buffer_t>> aScratchBuffer, sync aSyncHandler, blas_action aBuildAction)
	{
		// Create buffer for the AABBs:
		auto aabbDataBuffer = root::create_buffer(
			mPhysicalDevice, mDevice, mAllocator,
			memory_usage::device, {},
			aabb_buffer_meta::create_from_data(aGeometries)
		);
		aabbDataBuffer->fill(aGeometries.data(), 0, sync::wait_idle()); // TODO: Do not use wait_idle!
		// TODO: Probably better to NOT create an entirely new buffer at every invocation ^^

		auto result = build_or_update(aabbDataBuffer, aScratchBuffer, std::move(aSyncHandler), aBuildAction);
		if (result.has_value()) {
			// Handle lifetime:
			result.value()->set_custom_deleter([lOwnedAabbBuffer = std::move(aabbDataBuffer)](){});
		}
		else {
			AVK_LOG_INFO("Sorry for this mDevice::waitIdle call :( It will be gone after command/commands-refactoring");
			mDevice.waitIdle();
		}
		return result;
	}

	std::optional<command_buffer> bottom_level_acceleration_structure_t::build_or_update(const buffer& aGeometriesBuffer, std::optional<std::reference_wrapper<buffer_t>> aScratchBuffer, sync aSyncHandler, blas_action aBuildAction)
	{
		// Set the aScratchBuffer parameter to an internal scratch buffer, if none has been passed:
		buffer_t& scratchBuffer = aScratchBuffer.value_or(get_and_possibly_create_scratch_buffer());

		const auto& aabbMeta = aGeometriesBuffer->meta<aabb_buffer_meta>();
		auto startAddress = aGeometriesBuffer->device_address();
		const auto* aabbMemberDesc = aabbMeta.find_member_description(content_description::aabb);
		if (nullptr != aabbMemberDesc) {
			// Offset the device address:
			startAddress += aabbMemberDesc->mOffset;
		}

		auto accStructureGeometry = vk::AccelerationStructureGeometryKHR{}
			.setGeometryType(vk::GeometryTypeKHR::eAabbs)
			.setGeometry(vk::AccelerationStructureGeometryAabbsDataKHR{}
				.setData(vk::DeviceOrHostAddressConstKHR{ startAddress })
				.setStride(aabbMeta.sizeof_one_element())
			)
			.setFlags(vk::GeometryFlagsKHR{}); // TODO: Support flags

#if VK_HEADER_VERSION >= 162
		auto buildRangeInfo = vk::AccelerationStructureBuildRangeInfoKHR{}
			.setPrimitiveCount(static_cast<uint32_t>(aabbMeta.num_elements()))
			.setPrimitiveOffset(0u)
			.setFirstVertex(0u)
			.setTransformOffset(0u); // TODO: Support different values for all these parameters?!
		vk::AccelerationStructureBuildRangeInfoKHR* buildRangeInfoPtr = &buildRangeInfo;
#else
		auto buildOffsetInfo = vk::AccelerationStructureBuildOffsetInfoKHR{}
			.setPrimitiveCount(static_cast<uint32_t>(aabbMeta.num_elements()))
			.setPrimitiveOffset(0u)
			.setFirstVertex(0u)
			.setTransformOffset(0u); // TODO: Support different values for all these parameters?!
		vk::AccelerationStructureBuildOffsetInfoKHR* buildOffsetInfoPtr = &buildOffsetInfo;
#endif

		const auto* pointerToAnArray = &accStructureGeometry;

		auto buildGeometryInfos = vk::AccelerationStructureBuildGeometryInfoKHR{}
			.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
			.setFlags(mFlags) // TODO: support individual flags per geometry?
#if VK_HEADER_VERSION >= 162
			.setMode(aBuildAction == blas_action::build ? vk::BuildAccelerationStructureModeKHR::eBuild : vk::BuildAccelerationStructureModeKHR::eUpdate)
#else
			.setUpdate(aBuildAction == blas_action::build ? VK_FALSE : VK_TRUE)
			.setGeometryArrayOfPointers(VK_FALSE)
#endif
			.setSrcAccelerationStructure(aBuildAction == blas_action::build ? nullptr : acceleration_structure_handle()) // TODO: support different src acceleration structure?!
			.setDstAccelerationStructure(acceleration_structure_handle())
			.setGeometryCount(1u)
			.setPpGeometries(&pointerToAnArray)
			.setScratchData(vk::DeviceOrHostAddressKHR{ scratchBuffer.device_address() });

		auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
		// Sync before:
		aSyncHandler.establish_barrier_before_the_operation(pipeline_stage::acceleration_structure_build, read_memory_access{memory_access::acceleration_structure_read_access});

#if VK_HEADER_VERSION >= 162
		commandBuffer.handle().buildAccelerationStructuresKHR(
			1u,
			&buildGeometryInfos,
			&buildRangeInfoPtr,
			mDynamicDispatch
		);
#else
		commandBuffer.handle().buildAccelerationStructureKHR(
			1u,
			&buildGeometryInfos,
			&buildOffsetInfoPtr,
			mDynamicDispatch
		);
#endif

		// Sync after:
		aSyncHandler.establish_barrier_after_the_operation(pipeline_stage::acceleration_structure_build, write_memory_access{memory_access::acceleration_structure_write_access});

		// Finish him:
		return aSyncHandler.submit_and_sync();
	}

	std::optional<command_buffer> bottom_level_acceleration_structure_t::build(const std::vector<VkAabbPositionsKHR>& aGeometries, std::optional<std::reference_wrapper<buffer_t>> aScratchBuffer, sync aSyncHandler)
	{
		return build_or_update(aGeometries, aScratchBuffer, std::move(aSyncHandler), blas_action::build);
	}

	std::optional<command_buffer> bottom_level_acceleration_structure_t::update(const std::vector<VkAabbPositionsKHR>& aGeometries, std::optional<std::reference_wrapper<buffer_t>> aScratchBuffer, sync aSyncHandler)
	{
		return build_or_update(aGeometries, aScratchBuffer, std::move(aSyncHandler), blas_action::update);
	}

	std::optional<command_buffer> bottom_level_acceleration_structure_t::build(const buffer& aGeometriesBuffer, std::optional<std::reference_wrapper<buffer_t>> aScratchBuffer, sync aSyncHandler)
	{
		return build_or_update(aGeometriesBuffer, aScratchBuffer, std::move(aSyncHandler), blas_action::build);
	}

	std::optional<command_buffer> bottom_level_acceleration_structure_t::update(const buffer& aGeometriesBuffer, std::optional<std::reference_wrapper<buffer_t>> aScratchBuffer, sync aSyncHandler)
	{
		return build_or_update(aGeometriesBuffer, aScratchBuffer, std::move(aSyncHandler), blas_action::update);
	}


	top_level_acceleration_structure root::create_top_level_acceleration_structure(uint32_t aInstanceCount, bool aAllowUpdates, std::function<void(top_level_acceleration_structure_t&)> aAlterConfigBeforeCreation, std::function<void(top_level_acceleration_structure_t&)> aAlterConfigBeforeMemoryAlloc)
	{
		top_level_acceleration_structure_t result;

		result.mFlags = aAllowUpdates
			? vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate | vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastBuild
			: vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;

#if VK_HEADER_VERSION >= 162
		// This is the same structure that will later be used for the actual build, but the acceleration structure parameters and
		// geometry data pointers do not need to be fully populated at this point (although they can be), just the:
		//  - acceleration structure type,
		//  - and the geometry
		//     - types,
		//     - counts, and
		//     - maximum sizes.
		result.mAccStructureGeometries.reserve(1); // TODO: Support multiple?
		result.mBuildPrimitiveCounts.reserve(1);

		{
			auto& asg = result.mAccStructureGeometries.emplace_back();
			asg.setGeometryType(vk::GeometryTypeKHR::eInstances)
				.setFlags(vk::GeometryFlagsKHR{}) // TODO: Support flags
				.setGeometry(vk::AccelerationStructureGeometryInstancesDataKHR{});
			result.mBuildPrimitiveCounts.push_back(aInstanceCount);
		}

		const auto* pointerToAnArray = result.mAccStructureGeometries.data();

		assert(result.mAccStructureGeometries.size() == result.mBuildPrimitiveCounts.size());
		result.mBuildGeometryInfo = vk::AccelerationStructureBuildGeometryInfoKHR{}
			.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
			.setFlags(result.mFlags)
			.setGeometryCount(result.mAccStructureGeometries.size())
			.setPpGeometries(&pointerToAnArray);
#else
		// 2. Assemble info about the TOP LEVEL acceleration structure and the set its geometry
		auto geometryTypeInfo = vk::AccelerationStructureCreateGeometryTypeInfoKHR{}
			.setGeometryType(vk::GeometryTypeKHR::eInstances)
			.setMaxPrimitiveCount(aInstanceCount)
			.setMaxVertexCount(0u)
			.setVertexFormat(vk::Format::eUndefined)
			.setAllowsTransforms(VK_FALSE);
#endif

		result.mCreateInfo = vk::AccelerationStructureCreateInfoKHR{}
			.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
#if VK_HEADER_VERSION >= 162
			;
		// TODO: What about compacted size?
		// TODO: What about max geometry count?
#else
			.setCompactedSize(0) // If compactedSize is 0 then maxGeometryCount must not be 0
			.setFlags(result.mFlags)
			.setMaxGeometryCount(1u) // If type is VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR and compactedSize is 0, maxGeometryCount must be 1
			.setPGeometryInfos(&geometryTypeInfo);
#endif

		// 3. Maybe alter the config?
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(result);
		}

		// Steps 5. to 10. in here:
		finish_acceleration_structure_creation(result, std::move(aAlterConfigBeforeMemoryAlloc));

		return result;
	}

	top_level_acceleration_structure_t::~top_level_acceleration_structure_t()
	{
		if (acceleration_structure_handle()) {
			mDevice.destroyAccelerationStructureKHR(acceleration_structure_handle(), nullptr, mDynamicDispatch);
			mAccStructure.reset();
		}
	}

	buffer_t& top_level_acceleration_structure_t::get_and_possibly_create_scratch_buffer()
	{
		if (!mScratchBuffer.has_value()) {
			mScratchBuffer = root::create_buffer(
				mPhysicalDevice, mDevice, mAllocator,
				avk::memory_usage::device,
#if VK_HEADER_VERSION >= 162
#if VK_HEADER_VERSION >= 182
				vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
#endif
				vk::BufferUsageFlagBits::eShaderDeviceAddressKHR,
#else
				vk::BufferUsageFlagBits::eRayTracingKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR,
#endif
				avk::generic_buffer_meta::create_from_size(std::max(required_scratch_buffer_build_size(), required_scratch_buffer_update_size()))
			);
		}
		assert(mScratchBuffer.has_value());
		return mScratchBuffer.value().get();
	}

	std::optional<command_buffer> top_level_acceleration_structure_t::build_or_update(const std::vector<geometry_instance>& aGeometryInstances, std::optional<std::reference_wrapper<buffer_t>> aScratchBuffer, sync aSyncHandler, tlas_action aBuildAction)
	{
		auto geomInstances = convert_for_gpu_usage(aGeometryInstances);

		auto geomInstBuffer = root::create_buffer(
			mPhysicalDevice, mDevice, mAllocator,
			AVK_STAGING_BUFFER_MEMORY_USAGE,
#if VK_HEADER_VERSION >= 182
			vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
#else 
			{},
#endif
			geometry_instance_buffer_meta::create_from_data(geomInstances)
		);
		geomInstBuffer->fill(geomInstances.data(), 0, sync::not_required());

		auto result = build_or_update(geomInstBuffer, aScratchBuffer, std::move(aSyncHandler), aBuildAction);

		if (result.has_value()) {
			// Handle lifetime:
			result.value()->set_custom_deleter([lOwnedAabbBuffer = std::move(geomInstBuffer)](){});
		}
		else {
			AVK_LOG_INFO("Sorry for this mDevice::waitIdle call :( It will be gone after command/commands-refactoring");
			mDevice.waitIdle();
		}
		return result;
	}

	std::optional<command_buffer> top_level_acceleration_structure_t::build_or_update(const buffer& aGeometryInstancesBuffer, std::optional<std::reference_wrapper<buffer_t>> aScratchBuffer, sync aSyncHandler, tlas_action aBuildAction)
	{
		// Set the aScratchBuffer parameter to an internal scratch buffer, if none has been passed:
		buffer_t& scratchBuffer = aScratchBuffer.value_or(get_and_possibly_create_scratch_buffer());

		const auto& metaData = aGeometryInstancesBuffer->meta<geometry_instance_buffer_meta>();
		auto startAddress = aGeometryInstancesBuffer->device_address();
		const auto* memberDesc = metaData.find_member_description(content_description::geometry_instance);
		if (nullptr != memberDesc) {
			// Offset the device address:
			startAddress += memberDesc->mOffset;
		}
		const auto numInstances = static_cast<uint32_t>(metaData.num_elements());

		auto accStructureGeometries = vk::AccelerationStructureGeometryKHR{}
			.setGeometryType(vk::GeometryTypeKHR::eInstances)
			.setGeometry(vk::AccelerationStructureGeometryInstancesDataKHR{}
				.setArrayOfPointers(VK_FALSE) // arrayOfPointers specifies whether data is used as an array of addresses or just an array.
				// TODO: Is this ^ relevant? Probably only for host-builds if the data is structured in "array of pointers"-style?!
				.setData(vk::DeviceOrHostAddressConstKHR{ startAddress })
			)
			.setFlags(vk::GeometryFlagsKHR{}); // TODO: Support flags

#if VK_HEADER_VERSION >= 162
		auto bri = vk::AccelerationStructureBuildRangeInfoKHR{}
			// For geometries of type VK_GEOMETRY_TYPE_INSTANCES_KHR, primitiveCount is the number of acceleration
			// structures. primitiveCount VkAccelerationStructureInstanceKHR structures are consumed from
			// VkAccelerationStructureGeometryInstancesDataKHR::data, starting at an offset of primitiveOffset.
			.setPrimitiveCount(numInstances)
			.setPrimitiveOffset(0u)
			.setFirstVertex(0u)
			.setTransformOffset(0u); // TODO: Support different values for all these parameters?!
		vk::AccelerationStructureBuildRangeInfoKHR* buildRangeInfoPtr = &bri;
#else
		auto boi = vk::AccelerationStructureBuildOffsetInfoKHR{}
			// For geometries of type VK_GEOMETRY_TYPE_INSTANCES_KHR, primitiveCount is the number of acceleration
			// structures. primitiveCount VkAccelerationStructureInstanceKHR structures are consumed from
			// VkAccelerationStructureGeometryInstancesDataKHR::data, starting at an offset of primitiveOffset.
			.setPrimitiveCount(numInstances)
			.setPrimitiveOffset(0u)
			.setFirstVertex(0u)
			.setTransformOffset(0u); // TODO: Support different values for all these parameters?!
		vk::AccelerationStructureBuildOffsetInfoKHR* buildOffsetInfoPtr = &boi;
#endif

		const auto* pointerToAnArray = &accStructureGeometries;

		auto buildGeometryInfo = vk::AccelerationStructureBuildGeometryInfoKHR{}
			.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
			.setFlags(mFlags)
#if VK_HEADER_VERSION >= 162
			.setMode(aBuildAction == tlas_action::build ? vk::BuildAccelerationStructureModeKHR::eBuild : vk::BuildAccelerationStructureModeKHR::eUpdate)
#else
			.setUpdate(aBuildAction == tlas_action::build ? VK_FALSE : VK_TRUE)
			.setGeometryArrayOfPointers(VK_FALSE)
#endif
			.setSrcAccelerationStructure(aBuildAction == tlas_action::build ? nullptr : acceleration_structure_handle())
			.setDstAccelerationStructure(acceleration_structure_handle())
			.setGeometryCount(1u) // TODO: Correct?
			.setPpGeometries(&pointerToAnArray)
			.setScratchData(vk::DeviceOrHostAddressKHR{ scratchBuffer.device_address() });

		auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
		// Sync before:
		aSyncHandler.establish_barrier_before_the_operation(pipeline_stage::acceleration_structure_build, read_memory_access{memory_access::acceleration_structure_read_access});

#if VK_HEADER_VERSION >= 162
		commandBuffer.handle().buildAccelerationStructuresKHR(
			1u,
			&buildGeometryInfo,
			&buildRangeInfoPtr,
			mDynamicDispatch
		);
#else
		commandBuffer.handle().buildAccelerationStructureKHR(
			1u,
			&buildGeometryInfo,
			&buildOffsetInfoPtr,
			mDynamicDispatch
		);
#endif

		// Sync after:
		aSyncHandler.establish_barrier_after_the_operation(pipeline_stage::acceleration_structure_build, write_memory_access{memory_access::acceleration_structure_write_access});

		return aSyncHandler.submit_and_sync();
	}

	void top_level_acceleration_structure_t::build(const std::vector<geometry_instance>& aGeometryInstances, std::optional<std::reference_wrapper<buffer_t>> aScratchBuffer, sync aSyncHandler)
	{
		build_or_update(aGeometryInstances, aScratchBuffer, std::move(aSyncHandler), tlas_action::build);
	}

	void top_level_acceleration_structure_t::build(const buffer& aGeometryInstancesBuffer, std::optional<std::reference_wrapper<buffer_t>> aScratchBuffer, sync aSyncHandler)
	{
		build_or_update(aGeometryInstancesBuffer, aScratchBuffer, std::move(aSyncHandler), tlas_action::build);
	}

	void top_level_acceleration_structure_t::update(const std::vector<geometry_instance>& aGeometryInstances, std::optional<std::reference_wrapper<buffer_t>> aScratchBuffer, sync aSyncHandler)
	{
		build_or_update(aGeometryInstances, aScratchBuffer, std::move(aSyncHandler), tlas_action::update);
	}

	void top_level_acceleration_structure_t::update(const buffer& aGeometryInstancesBuffer, std::optional<std::reference_wrapper<buffer_t>> aScratchBuffer, sync aSyncHandler)
	{
		build_or_update(aGeometryInstancesBuffer, aScratchBuffer, std::move(aSyncHandler), tlas_action::update);
	}
#endif
#pragma endregion

#pragma region binding_data definitions
	uint32_t binding_data::descriptor_count() const
	{
		if (std::holds_alternative<std::vector<const buffer_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<const buffer_t*>>(mResourcePtr).size()); }
		if (std::holds_alternative<std::vector<const buffer_descriptor*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<const buffer_descriptor*>>(mResourcePtr).size()); }
		if (std::holds_alternative<std::vector<const buffer_view_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<const buffer_view_t*>>(mResourcePtr).size()); }
		if (std::holds_alternative<std::vector<const buffer_view_descriptor*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<const buffer_view_descriptor*>>(mResourcePtr).size()); }

		//                                                                         vvv There can only be ONE pNext (at least I think so) vvv
		if (std::holds_alternative<std::vector<const top_level_acceleration_structure_t*>>(mResourcePtr)) { return 1u; }

		if (std::holds_alternative<std::vector<const image_view_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<const image_view_t*>>(mResourcePtr).size()); }
		if (std::holds_alternative<std::vector<const image_view_as_input_attachment*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<const image_view_as_input_attachment*>>(mResourcePtr).size()); }
		if (std::holds_alternative<std::vector<const image_view_as_storage_image*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<const image_view_as_storage_image*>>(mResourcePtr).size()); }
		if (std::holds_alternative<std::vector<const sampler_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<const sampler_t*>>(mResourcePtr).size()); }
		if (std::holds_alternative<std::vector<const image_sampler_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<const image_sampler_t*>>(mResourcePtr).size()); }

		return 1u;
	}

	const vk::DescriptorImageInfo* binding_data::descriptor_image_info(descriptor_set& aDescriptorSet) const
	{
		if (std::holds_alternative<const buffer_t*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const buffer_descriptor*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const buffer_view_t*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const buffer_view_descriptor*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const top_level_acceleration_structure_t*>(mResourcePtr)) { return nullptr; }

		if (std::holds_alternative<const image_view_t*>(mResourcePtr)) {
			return aDescriptorSet.store_image_info(mLayoutBinding.binding, std::get<const image_view_t*>(mResourcePtr)->descriptor_info());
		}
		if (std::holds_alternative<const image_view_as_input_attachment*>(mResourcePtr)) {
			return aDescriptorSet.store_image_info(mLayoutBinding.binding, std::get<const image_view_as_input_attachment*>(mResourcePtr)->descriptor_info());
		}
		if (std::holds_alternative<const image_view_as_storage_image*>(mResourcePtr)) {
			return aDescriptorSet.store_image_info(mLayoutBinding.binding, std::get<const image_view_as_storage_image*>(mResourcePtr)->descriptor_info());
		}
		if (std::holds_alternative<const sampler_t*>(mResourcePtr)) {
			return aDescriptorSet.store_image_info(mLayoutBinding.binding, std::get<const sampler_t*>(mResourcePtr)->descriptor_info());
		}
		if (std::holds_alternative<const image_sampler_t*>(mResourcePtr)) {
			return aDescriptorSet.store_image_info(mLayoutBinding.binding, std::get<const image_sampler_t*>(mResourcePtr)->descriptor_info());
		}


		if (std::holds_alternative<std::vector<const buffer_t*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const buffer_descriptor*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const buffer_view_t*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const buffer_view_descriptor*>>(mResourcePtr)) { return nullptr; }

		if (std::holds_alternative<std::vector<const top_level_acceleration_structure_t*>>(mResourcePtr)) { return nullptr; }

		if (std::holds_alternative<std::vector<const image_view_t*>>(mResourcePtr)) {
			return aDescriptorSet.store_image_infos(mLayoutBinding.binding, gather_image_infos(std::get<std::vector<const image_view_t*>>(mResourcePtr)));
		}
		if (std::holds_alternative<std::vector<const image_view_as_input_attachment*>>(mResourcePtr)) {
			return aDescriptorSet.store_image_infos(mLayoutBinding.binding, gather_image_infos(std::get<std::vector<const image_view_as_input_attachment*>>(mResourcePtr)));
		}
		if (std::holds_alternative<std::vector<const image_view_as_storage_image*>>(mResourcePtr)) {
			return aDescriptorSet.store_image_infos(mLayoutBinding.binding, gather_image_infos(std::get<std::vector<const image_view_as_storage_image*>>(mResourcePtr)));
		}
		if (std::holds_alternative<std::vector<const sampler_t*>>(mResourcePtr)) {
			return aDescriptorSet.store_image_infos(mLayoutBinding.binding, gather_image_infos(std::get<std::vector<const sampler_t*>>(mResourcePtr)));
		}
		if (std::holds_alternative<std::vector<const image_sampler_t*>>(mResourcePtr)) {
			return aDescriptorSet.store_image_infos(mLayoutBinding.binding, gather_image_infos(std::get<std::vector<const image_sampler_t*>>(mResourcePtr)));
		}

		throw runtime_error("Some holds_alternative calls are not implemented.");
	}

	const vk::DescriptorBufferInfo* binding_data::descriptor_buffer_info(descriptor_set& aDescriptorSet) const
	{
		if (std::holds_alternative<const buffer_t*>(mResourcePtr)) {
			return aDescriptorSet.store_buffer_info(mLayoutBinding.binding, std::get<const buffer_t*>(mResourcePtr)->descriptor_info());
		}
		if (std::holds_alternative<const buffer_descriptor*>(mResourcePtr)) {
			return aDescriptorSet.store_buffer_info(mLayoutBinding.binding, std::get<const buffer_descriptor*>(mResourcePtr)->descriptor_info());
		}
		if (std::holds_alternative<const buffer_view_t*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const buffer_view_descriptor*>(mResourcePtr)) { return nullptr; }

		if (std::holds_alternative<const top_level_acceleration_structure_t*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const image_view_t*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const image_view_as_input_attachment*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const image_view_as_storage_image*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const sampler_t*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const image_sampler_t*>(mResourcePtr)) { return nullptr; }


		if (std::holds_alternative<std::vector<const buffer_t*>>(mResourcePtr)) {
			return aDescriptorSet.store_buffer_infos(mLayoutBinding.binding, gather_buffer_infos(std::get<std::vector<const buffer_t*>>(mResourcePtr)));
		}
		if (std::holds_alternative<std::vector<const buffer_descriptor*>>(mResourcePtr)) {
			return aDescriptorSet.store_buffer_infos(mLayoutBinding.binding, gather_buffer_infos(std::get<std::vector<const buffer_descriptor*>>(mResourcePtr)));
		}
		if (std::holds_alternative<std::vector<const buffer_view_t*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const buffer_view_descriptor*>>(mResourcePtr)) { return nullptr; }

		if (std::holds_alternative<std::vector<const top_level_acceleration_structure_t*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const image_view_t*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const image_view_as_input_attachment*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const image_view_as_storage_image*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const sampler_t*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const image_sampler_t*>>(mResourcePtr)) { return nullptr; }

		throw runtime_error("Some holds_alternative calls are not implemented.");
	}

	const void* binding_data::next_pointer(descriptor_set& aDescriptorSet) const
	{
		if (std::holds_alternative<const buffer_t*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const buffer_descriptor*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const buffer_view_t*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const buffer_view_descriptor*>(mResourcePtr)) { return nullptr; }

#if VK_HEADER_VERSION >= 135
		if (std::holds_alternative<const top_level_acceleration_structure_t*>(mResourcePtr)) {
			return aDescriptorSet.store_acceleration_structure_info(mLayoutBinding.binding, std::get<const top_level_acceleration_structure_t*>(mResourcePtr)->descriptor_info());
		}
#endif

		if (std::holds_alternative<const image_view_t*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const image_view_as_input_attachment*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const image_view_as_storage_image*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const sampler_t*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const image_sampler_t*>(mResourcePtr)) { return nullptr; }


		if (std::holds_alternative<std::vector<const buffer_t*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const buffer_descriptor*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const buffer_view_t*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const buffer_view_descriptor*>>(mResourcePtr)) { return nullptr; }

#if VK_HEADER_VERSION >= 135
		if (std::holds_alternative<std::vector<const top_level_acceleration_structure_t*>>(mResourcePtr)) {
			return aDescriptorSet.store_acceleration_structure_infos(mLayoutBinding.binding, gather_acceleration_structure_infos(std::get<std::vector<const top_level_acceleration_structure_t*>>(mResourcePtr)));
		}
#endif

		if (std::holds_alternative<std::vector<const image_view_t*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const image_view_as_input_attachment*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const image_view_as_storage_image*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const sampler_t*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const image_sampler_t*>>(mResourcePtr)) { return nullptr; }

		throw runtime_error("Some holds_alternative calls are not implemented.");
	}

	const vk::BufferView* binding_data::texel_buffer_view_info(descriptor_set& aDescriptorSet) const
	{
		if (std::holds_alternative<const buffer_t*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const buffer_descriptor*>(mResourcePtr)) { return nullptr; }

		if (std::holds_alternative<const buffer_view_t*>(mResourcePtr)) {
			return aDescriptorSet.store_buffer_view(mLayoutBinding.binding, std::get<const buffer_view_t*>(mResourcePtr)->view_handle());
		}
		if (std::holds_alternative<const buffer_view_descriptor*>(mResourcePtr)) {
			return aDescriptorSet.store_buffer_view(mLayoutBinding.binding, std::get<const buffer_view_descriptor*>(mResourcePtr)->view_handle());
		}

		if (std::holds_alternative<const top_level_acceleration_structure_t*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const image_view_t*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const image_view_as_input_attachment*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const image_view_as_storage_image*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const sampler_t*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const image_sampler_t*>(mResourcePtr)) { return nullptr; }


		if (std::holds_alternative<std::vector<const buffer_t*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const buffer_descriptor*>>(mResourcePtr)) { return nullptr; }

		if (std::holds_alternative<std::vector<const buffer_view_t*>>(mResourcePtr)) {
			return aDescriptorSet.store_buffer_views(mLayoutBinding.binding, gather_buffer_views(std::get<std::vector<const buffer_view_t*>>(mResourcePtr)));
		}
		if (std::holds_alternative<std::vector<const buffer_view_descriptor*>>(mResourcePtr)) {
			return aDescriptorSet.store_buffer_views(mLayoutBinding.binding, gather_buffer_views(std::get<std::vector<const buffer_view_descriptor*>>(mResourcePtr)));
		}

		if (std::holds_alternative<std::vector<const top_level_acceleration_structure_t*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const image_view_t*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const image_view_as_input_attachment*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const image_view_as_storage_image*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const sampler_t*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const image_sampler_t*>>(mResourcePtr)) { return nullptr; }

		throw runtime_error("Some holds_alternative calls are not implemented.");
	}
#pragma endregion

#pragma region buffer definitions
	std::string to_string(content_description aValue)
	{
		switch (aValue) {
		case content_description::unspecified			: return "unspecified";
		case content_description::user_defined_01		: return "user_defined_01";
		case content_description::user_defined_02		: return "user_defined_02";
		case content_description::user_defined_03		: return "user_defined_03";
		case content_description::user_defined_04		: return "user_defined_04";
		case content_description::user_defined_05		: return "user_defined_05";
		case content_description::user_defined_06		: return "user_defined_06";
		case content_description::user_defined_07		: return "user_defined_07";
		case content_description::user_defined_08		: return "user_defined_08";
		case content_description::user_defined_09		: return "user_defined_09";
		case content_description::user_defined_10		: return "user_defined_10";
		case content_description::user_defined_11		: return "user_defined_11";
		case content_description::user_defined_12		: return "user_defined_12";
		case content_description::user_defined_13		: return "user_defined_13";
		case content_description::user_defined_14		: return "user_defined_14";
		case content_description::user_defined_15		: return "user_defined_15";
		case content_description::user_defined_16		: return "user_defined_16";
		case content_description::user_defined_17		: return "user_defined_17";
		case content_description::user_defined_18		: return "user_defined_18";
		case content_description::user_defined_19		: return "user_defined_19";
		case content_description::user_defined_20		: return "user_defined_20";
		case content_description::user_defined_21		: return "user_defined_21";
		case content_description::user_defined_22		: return "user_defined_22";
		case content_description::user_defined_23		: return "user_defined_23";
		case content_description::user_defined_24		: return "user_defined_24";
		case content_description::user_defined_25		: return "user_defined_25";
		case content_description::user_defined_26		: return "user_defined_26";
		case content_description::user_defined_27		: return "user_defined_27";
		case content_description::user_defined_28		: return "user_defined_28";
		case content_description::user_defined_29		: return "user_defined_29";
		case content_description::user_defined_30		: return "user_defined_30";
		case content_description::user_defined_31		: return "user_defined_31";
		case content_description::user_defined_32		: return "user_defined_32";
		case content_description::user_defined_33		: return "user_defined_33";
		case content_description::user_defined_34		: return "user_defined_34";
		case content_description::user_defined_35		: return "user_defined_35";
		case content_description::user_defined_36		: return "user_defined_36";
		case content_description::user_defined_37		: return "user_defined_37";
		case content_description::user_defined_38		: return "user_defined_38";
		case content_description::user_defined_39		: return "user_defined_39";
		case content_description::user_defined_40		: return "user_defined_40";
		case content_description::user_defined_41		: return "user_defined_41";
		case content_description::user_defined_42		: return "user_defined_42";
		case content_description::user_defined_43		: return "user_defined_43";
		case content_description::user_defined_44		: return "user_defined_44";
		case content_description::user_defined_45		: return "user_defined_45";
		case content_description::user_defined_46		: return "user_defined_46";
		case content_description::user_defined_47		: return "user_defined_47";
		case content_description::user_defined_48		: return "user_defined_48";
		case content_description::user_defined_49		: return "user_defined_49";
		case content_description::user_defined_50		: return "user_defined_50";
		case content_description::user_defined_51		: return "user_defined_51";
		case content_description::user_defined_52		: return "user_defined_52";
		case content_description::user_defined_53		: return "user_defined_53";
		case content_description::user_defined_54		: return "user_defined_54";
		case content_description::user_defined_55		: return "user_defined_55";
		case content_description::user_defined_56		: return "user_defined_56";
		case content_description::user_defined_57		: return "user_defined_57";
		case content_description::user_defined_58		: return "user_defined_58";
		case content_description::user_defined_59		: return "user_defined_59";
		case content_description::user_defined_60		: return "user_defined_60";
		case content_description::user_defined_61		: return "user_defined_61";
		case content_description::user_defined_62		: return "user_defined_62";
		case content_description::user_defined_63		: return "user_defined_63";
		case content_description::user_defined_64		: return "user_defined_64";
		case content_description::user_defined_65		: return "user_defined_65";
		case content_description::user_defined_66		: return "user_defined_66";
		case content_description::user_defined_67		: return "user_defined_67";
		case content_description::user_defined_68		: return "user_defined_68";
		case content_description::user_defined_69		: return "user_defined_69";
		case content_description::user_defined_70		: return "user_defined_70";
		case content_description::user_defined_71		: return "user_defined_71";
		case content_description::user_defined_72		: return "user_defined_72";
		case content_description::user_defined_73		: return "user_defined_73";
		case content_description::user_defined_74		: return "user_defined_74";
		case content_description::user_defined_75		: return "user_defined_75";
		case content_description::user_defined_76		: return "user_defined_76";
		case content_description::user_defined_77		: return "user_defined_77";
		case content_description::user_defined_78		: return "user_defined_78";
		case content_description::user_defined_79		: return "user_defined_79";
		case content_description::user_defined_80		: return "user_defined_80";
		case content_description::user_defined_81		: return "user_defined_81";
		case content_description::user_defined_82		: return "user_defined_82";
		case content_description::user_defined_83		: return "user_defined_83";
		case content_description::user_defined_84		: return "user_defined_84";
		case content_description::user_defined_85		: return "user_defined_85";
		case content_description::user_defined_86		: return "user_defined_86";
		case content_description::user_defined_87		: return "user_defined_87";
		case content_description::user_defined_88		: return "user_defined_88";
		case content_description::user_defined_89		: return "user_defined_89";
		case content_description::user_defined_90		: return "user_defined_90";
		case content_description::user_defined_91		: return "user_defined_91";
		case content_description::user_defined_92		: return "user_defined_92";
		case content_description::user_defined_93		: return "user_defined_93";
		case content_description::user_defined_94		: return "user_defined_94";
		case content_description::user_defined_95		: return "user_defined_95";
		case content_description::user_defined_96		: return "user_defined_96";
		case content_description::user_defined_97		: return "user_defined_97";
		case content_description::user_defined_98		: return "user_defined_98";
		case content_description::user_defined_99		: return "user_defined_99";
		case content_description::index					: return "index";
		case content_description::position				: return "position";
		case content_description::normal				: return "normal";
		case content_description::tangent				: return "tangent";
		case content_description::bitangent				: return "bitangent";
		case content_description::color					: return "color";
		case content_description::texture_coordinate	: return "texture_coordinate";
		case content_description::bone_weight			: return "bone_weight";
		case content_description::bone_index			: return "bone_index";
		case content_description::aabb					: return "aabb";
		case content_description::geometry_instance		: return "geometry_instance" ;
		case content_description::query_result			: return "query_result" ;
		default:										return "<<ERROR: not all cases implemented>>";
		}
	}

	buffer root::create_buffer(
		const vk::PhysicalDevice& aPhysicalDevice,
		const vk::Device& aDevice,
		const AVK_MEM_ALLOCATOR_TYPE& aAllocator,
#if VK_HEADER_VERSION >= 135
		std::vector<std::variant<buffer_meta, generic_buffer_meta, uniform_buffer_meta, uniform_texel_buffer_meta, storage_buffer_meta, storage_texel_buffer_meta, vertex_buffer_meta, index_buffer_meta, instance_buffer_meta, aabb_buffer_meta, geometry_instance_buffer_meta, query_results_buffer_meta, indirect_buffer_meta>> aMetaData,
#else
		std::vector<std::variant<buffer_meta, generic_buffer_meta, uniform_buffer_meta, uniform_texel_buffer_meta, storage_buffer_meta, storage_texel_buffer_meta, vertex_buffer_meta, index_buffer_meta, instance_buffer_meta, query_results_buffer_meta, indirect_buffer_meta>> aMetaData,
#endif
		vk::BufferUsageFlags aBufferUsage,
		vk::MemoryPropertyFlags aMemoryProperties,
		std::initializer_list<queue*> aConcurrentQueueOwnership
	)
	{
		assert (aMetaData.size() > 0);
		buffer_t result;
		result.mMetaData = std::move(aMetaData);
		auto bufferSize = result.meta_at_index<buffer_meta>(0).total_size();

		std::vector<uint32_t> queueFamilyIndices;
		auto endQfi = std::end(queueFamilyIndices);
		if (aConcurrentQueueOwnership.size() > 0) {
			// Select unique queue family indices:
			std::transform(std::begin(aConcurrentQueueOwnership), std::end(aConcurrentQueueOwnership), std::back_inserter(queueFamilyIndices), [](queue* q) { return q->family_index(); });
			std::sort(std::begin(queueFamilyIndices), std::end(queueFamilyIndices));
			endQfi = std::unique(std::begin(queueFamilyIndices), std::end(queueFamilyIndices));
		}

		// Create (possibly multiple) buffer(s):
		auto bufferCreateInfo = vk::BufferCreateInfo()
			.setSize(static_cast<vk::DeviceSize>(bufferSize))
			.setUsage(aBufferUsage)
			// Always grant exclusive ownership to the queue.
			.setSharingMode(vk::SharingMode::eExclusive)
			// The flags parameter is used to configure sparse buffer memory, which is not relevant right now. We'll leave it at the default value of 0. [2]
			.setFlags(vk::BufferCreateFlags());

		if (queueFamilyIndices.size() > 0) {
			bufferCreateInfo
				.setSharingMode(vk::SharingMode::eConcurrent)
				.setQueueFamilyIndexCount(static_cast<uint32_t>( std::distance(std::begin(queueFamilyIndices), endQfi) ))
				.setPQueueFamilyIndices(queueFamilyIndices.data());
			// TODO: Untested ^ test vk::SharingMode::eConcurrent
		}

		result.mCreateInfo = bufferCreateInfo;
		result.mBufferUsageFlags = aBufferUsage;
		result.mBuffer = AVK_MEM_BUFFER_HANDLE{ aAllocator, aMemoryProperties, result.mCreateInfo };
		result.mPhysicalDevice = aPhysicalDevice;
		result.mDevice = aDevice;

#if VK_HEADER_VERSION >= 135
		if (   avk::has_flag(result.usage_flags(), vk::BufferUsageFlagBits::eShaderDeviceAddress)
			|| avk::has_flag(result.usage_flags(), vk::BufferUsageFlagBits::eShaderDeviceAddressKHR)
			|| avk::has_flag(result.usage_flags(), vk::BufferUsageFlagBits::eShaderDeviceAddressEXT)
			//|| avk::has_flag(result.usage_flags(), vk::BufferUsageFlagBits::eShaderBindingTableKHR)
			//|| avk::has_flag(result.usage_flags(), vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR)
			//|| avk::has_flag(result.usage_flags(), vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR)
			) {
			result.mDeviceAddress = get_buffer_address(aDevice, result.handle());
		}
#endif

		return result;
	}

	std::optional<command_buffer> buffer_t::fill(const void* aDataPtr, size_t aMetaDataIndex, sync aSyncHandler)
	{
		auto metaData = meta_at_index<buffer_meta>(aMetaDataIndex);
		auto bufferSize = static_cast<vk::DeviceSize>(metaData.total_size());
		return fill(aDataPtr, aMetaDataIndex, 0u, bufferSize, std::move(aSyncHandler));
	}

	std::optional<command_buffer> buffer_t::fill(const void* aDataPtr, size_t aMetaDataIndex, size_t aOffsetInBytes, size_t aDataSizeInBytes, sync aSyncHandler)
	{
		auto dataSize = static_cast<vk::DeviceSize>(aDataSizeInBytes);
		auto memProps = memory_properties();

#ifdef _DEBUG
		const auto& metaData = meta_at_index<buffer_meta>(aMetaDataIndex);
		assert(aOffsetInBytes + aDataSizeInBytes <= metaData.total_size()); // The fill operation would write beyond the buffer's size.
#endif

		// #1: Is our memory accessible from the CPU-SIDE?
		if (avk::has_flag(memProps, vk::MemoryPropertyFlagBits::eHostVisible)) {
			auto mapped = scoped_mapping{mBuffer, mapping_access::write};
			memcpy(static_cast<uint8_t *>(mapped.get()) + aOffsetInBytes, aDataPtr, dataSize);
			return {};
		}

		// #2: Otherwise, it must be on the GPU-SIDE!
		else {
			assert(avk::has_flag(memProps, vk::MemoryPropertyFlagBits::eDeviceLocal));

			// We have to create a (somewhat temporary) staging buffer and transfer it to the GPU
			// "somewhat temporary" means that it can not be deleted in this function, but only
			//						after the transfer operation has completed => handle via sync
			// We need to take care though, to not try to allocate buffer of size zero here.
			// If dataSize is zero, skip staging buffer creation and the copy command, but still
			// process the synchronization calls, as user code may rely on those.

			auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();

			// Sync before:
			aSyncHandler.establish_barrier_before_the_operation(pipeline_stage::transfer, read_memory_access{memory_access::transfer_read_access});

			if (dataSize != 0) {
				auto stagingBuffer = root::create_buffer(
					mPhysicalDevice, mDevice, mBuffer.allocator(),
					AVK_STAGING_BUFFER_MEMORY_USAGE,
					vk::BufferUsageFlagBits::eTransferSrc,
					generic_buffer_meta::create_from_size(dataSize)
				);
				stagingBuffer->fill(aDataPtr, 0, sync::wait_idle()); // Recurse into the other if-branch

				// Operation:
				copy_buffer_to_another(avk::referenced(stagingBuffer), avk::referenced(*this), 0, static_cast<vk::DeviceSize>(aOffsetInBytes), dataSize, sync::with_barriers_into_existing_command_buffer(commandBuffer, {}, {}));

				// Take care of the lifetime handling of the stagingBuffer, it might still be in use when this method returns:
				commandBuffer.set_custom_deleter([
					lOwnedStagingBuffer{ std::move(stagingBuffer) }
				]() { /* Nothing to do here, the buffers' destructors will do the cleanup, the lambda is just storing it. */ });
			}

			// Sync after:
			aSyncHandler.establish_barrier_after_the_operation(pipeline_stage::transfer, write_memory_access{memory_access::transfer_write_access});

			// Finish him:
			return aSyncHandler.submit_and_sync();
		}
	}

	//std::optional<commands> buffer_t::fill(const void* aDataPtr, size_t aMetaDataIndex)
	//{
	//	return commands{ pipeline_stage::transfer, memory_access::transfer_read_access, [this](command_buffer_t& aCmdBfr) {
	//		auto metaData = meta_at_index<buffer_meta>(aMetaDataIndex);
	//		auto bufferSize = static_cast<vk::DeviceSize>(metaData.total_size());
	//		auto memProps = memory_properties();

	//		// #1: Is our memory accessible from the CPU-SIDE?
	//		if (avk::has_flag(memProps, vk::MemoryPropertyFlagBits::eHostVisible)) {
	//			auto mapped = scoped_mapping{mBuffer, mapping_access::write};
	//			memcpy(mapped.get(), aDataPtr, bufferSize);
	//			return {};
	//		}

	//		// #2: Otherwise, it must be on the GPU-SIDE!
	//		else {
	//			assert(avk::has_flag(memProps, vk::MemoryPropertyFlagBits::eDeviceLocal));

	//			// We have to create a (somewhat temporary) staging buffer and transfer it to the GPU
	//			// "somewhat temporary" means that it can not be deleted in this function, but only
	//			//						after the transfer operation has completed => handle via sync
	//			auto stagingBuffer = root::create_buffer(
	//				mPhysicalDevice, mDevice, mBuffer.allocator(),
	//				AVK_STAGING_BUFFER_MEMORY_USAGE,
	//				vk::BufferUsageFlagBits::eTransferSrc,
	//				generic_buffer_meta::create_from_size(bufferSize)
	//			);
	//			stagingBuffer->fill(aDataPtr, 0, sync::wait_idle()); // Recurse into the other if-branch

	//			auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
	//			// Sync before:
	//			aSyncHandler.establish_barrier_before_the_operation(pipeline_stage::transfer, read_memory_access{memory_access::transfer_read_access});

	//			// Operation:
	//			auto copyRegion = vk::BufferCopy{}
	//				.setSrcOffset(0u) // TODO: Support different offsets or whatever?!
	//				.setDstOffset(0u)
	//				.setSize(bufferSize);
	//			commandBuffer.handle().copyBuffer(stagingBuffer->handle(), handle(), { copyRegion });

	//			// Sync after:
	//			aSyncHandler.establish_barrier_after_the_operation(pipeline_stage::transfer, write_memory_access{memory_access::transfer_write_access});

	//			// Take care of the lifetime handling of the stagingBuffer, it might still be in use:
	//			commandBuffer.set_custom_deleter([
	//				lOwnedStagingBuffer{ std::move(stagingBuffer) }
	//			]() { /* Nothing to do here, the buffers' destructors will do the cleanup, the lambda is just storing it. */ });
	//
	//			// Finish him:
	//			return aSyncHandler.submit_and_sync();
	//		}
	//	}, pipeline_stage::transfer, memory_access::transfer_write_access);
	//}

	std::optional<command_buffer> buffer_t::read(void* aDataPtr, size_t aMetaDataIndex, sync aSyncHandler) const
	{
		auto metaData = meta_at_index<buffer_meta>(aMetaDataIndex);
		auto bufferSize = static_cast<vk::DeviceSize>(metaData.total_size());
		auto memProps = memory_properties();

		// #1: Is our memory accessible on the CPU-SIDE?
		if (avk::has_flag(memProps, vk::MemoryPropertyFlagBits::eHostVisible)) {
			auto mapped = scoped_mapping{mBuffer, mapping_access::read};
			memcpy(aDataPtr, mapped.get(), bufferSize);
			return {};
		}

		// #2: Otherwise, it must be on the GPU-SIDE!
		else {
			assert(avk::has_flag(memProps, vk::MemoryPropertyFlagBits::eDeviceLocal));

			// We have to create a (somewhat temporary) staging buffer and transfer it to the GPU
			// "somewhat temporary" means that it can not be deleted in this function, but only
			//						after the transfer operation has completed => handle via avk::sync!
			auto stagingBuffer = root::create_buffer(
				mPhysicalDevice, mDevice, mBuffer.allocator(),
				AVK_STAGING_BUFFER_MEMORY_USAGE,
				vk::BufferUsageFlagBits::eTransferDst,
				generic_buffer_meta::create_from_size(bufferSize));
			// TODO: Creating a staging buffer in every read()-call is probably not optimal. => Think about alternative ways!

			// TODO: What about queue ownership?! If not the queue_selection_strategy::prefer_everything_on_single_queue strategy is being applied, it could very well be that this fails.
			auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
			// Sync before:
			aSyncHandler.establish_barrier_before_the_operation(pipeline_stage::transfer, read_memory_access{memory_access::transfer_read_access});

			// Operation:
			auto copyRegion = vk::BufferCopy{}
				.setSrcOffset(0u)
				.setDstOffset(0u)
				.setSize(bufferSize);
			commandBuffer.handle().copyBuffer(handle(), stagingBuffer->handle(), { copyRegion });

			// Sync after:
			aSyncHandler.establish_barrier_after_the_operation(pipeline_stage::transfer, write_memory_access{memory_access::transfer_write_access});

			// Take care of the stagingBuffer's lifetime handling and also of reading the data for this branch:
			commandBuffer.set_custom_deleter([
				lOwnedStagingBuffer{ std::move(stagingBuffer) },
				aMetaDataIndex,
				aDataPtr
			]() {
				lOwnedStagingBuffer->read(aDataPtr, aMetaDataIndex, sync::not_required()); // TODO: not sure about that sync
			});

			// Finish him:
			return aSyncHandler.submit_and_sync();
		}
	}
#pragma endregion

#pragma region buffer view definitions
	vk::Buffer buffer_view_t::buffer_handle() const
	{
		if (std::holds_alternative<buffer>(mBuffer)) {
			return std::get<buffer>(mBuffer)->handle();
		}
		return std::get<vk::Buffer>(std::get<std::tuple<vk::Buffer, vk::BufferCreateInfo>>(mBuffer));
	}

	const vk::BufferCreateInfo& buffer_view_t::buffer_config() const
	{
		if (std::holds_alternative<buffer>(mBuffer)) {
			return std::get<buffer>(mBuffer)->config();
		}
		return std::get<vk::BufferCreateInfo>(std::get<std::tuple<vk::Buffer, vk::BufferCreateInfo>>(mBuffer));
	}

	vk::DescriptorType buffer_view_t::descriptor_type(size_t aMetaDataIndex) const
	{
		if (std::holds_alternative<buffer>(mBuffer)) {
			return std::get<buffer>(mBuffer)->meta_at_index<buffer_meta>(aMetaDataIndex).descriptor_type().value();
		}
		throw avk::runtime_error("Which descriptor type?");
	}

	buffer_view root::create_buffer_view(resource_ownership<buffer_t> aBufferToOwn, vk::Format aViewFormat, std::function<void(buffer_view_t&)> aAlterConfigBeforeCreation)
	{
		buffer_view_t result;
		result.mBuffer = aBufferToOwn.own(); // moved
		finish_configuration(result, aViewFormat, std::move(aAlterConfigBeforeCreation));
		return result;
	}

	buffer_view root::create_buffer_view(vk::Buffer aBufferToReference, vk::BufferCreateInfo aBufferInfo, vk::Format aViewFormat, std::function<void(buffer_view_t&)> aAlterConfigBeforeCreation)
	{
		buffer_view_t result;
		// Store handles:
		result.mBuffer = std::make_tuple(aBufferToReference, aBufferInfo);
		finish_configuration(result, aViewFormat, std::move(aAlterConfigBeforeCreation));
		return result;
	}
#pragma endregion

#pragma region command pool and command buffer definitions
	command_pool root::create_command_pool(uint32_t aQueueFamilyIndex, vk::CommandPoolCreateFlags aCreateFlags)
	{
		auto createInfo = vk::CommandPoolCreateInfo()
			.setQueueFamilyIndex(aQueueFamilyIndex)
			.setFlags(aCreateFlags);
		command_pool_t result;
		result.mQueueFamilyIndex = aQueueFamilyIndex;
		result.mCreateInfo = createInfo;
		result.mCommandPool = std::make_shared<vk::UniqueCommandPool>(device().createCommandPoolUnique(createInfo));
		return result;
	}

	std::vector<command_buffer> command_pool_t::alloc_command_buffers(uint32_t aCount, vk::CommandBufferUsageFlags aUsageFlags, vk::CommandBufferLevel aLevel)
	{
		auto bufferAllocInfo = vk::CommandBufferAllocateInfo()
			.setCommandPool(handle())
			.setLevel(aLevel)
			.setCommandBufferCount(aCount);

		auto tmp = mCommandPool->getOwner().allocateCommandBuffersUnique(bufferAllocInfo);

		// Iterate over all the "raw"-Vk objects in `tmp` and...
		std::vector<command_buffer> buffers;
		buffers.reserve(aCount);
		std::transform(std::begin(tmp), std::end(tmp),
			std::back_inserter(buffers),
			// ...transform them into `ak::command_buffer_t` objects:
			[lUsageFlags = aUsageFlags, poolPtr = mCommandPool](auto& vkCb) -> command_buffer {
				command_buffer_t result;
				result.mBeginInfo = vk::CommandBufferBeginInfo()
					.setFlags(lUsageFlags)
					.setPInheritanceInfo(nullptr);
				result.mCommandBuffer = std::move(vkCb);
				result.mCommandPool = std::move(poolPtr);
				return result;
			});
		return buffers;
	}

	command_buffer command_pool_t::alloc_command_buffer(vk::CommandBufferUsageFlags aUsageFlags, vk::CommandBufferLevel aLevel)
	{
		auto result = std::move(alloc_command_buffers(1, aUsageFlags, aLevel)[0]);
		return result;
	}

	// prepare command buffer for re-recording
	void command_buffer_t::prepare_for_reuse()
	{
		if (mPostExecutionHandler.has_value()) {
			// Clear post-execution handler
			mPostExecutionHandler.reset();
		}
		if (mCustomDeleter.has_value() && *mCustomDeleter) {
			// If there is a custom deleter => call it now
			(*mCustomDeleter)();
			mCustomDeleter.reset();
		}
	}

	command_buffer_t::~command_buffer_t()
	{
		// prepare_for_reuse() invokes and cleans up the custom deleter. This is
		// exactly what we need here. ATTENTION: If prepare_for_reuse() gets some
		// additional functionality in the future which would not be appropriate
		// to being executed from the destructor, don't call it anymore from here!
		prepare_for_reuse();
		// Destroy the dependant instance before destroying myself
		// ^ This is ensured by the order of the members
		//   See: https://isocpp.org/wiki/faq/dtors#calling-member-dtors
	}

	void command_buffer_t::invoke_post_execution_handler() const
	{
		if (mPostExecutionHandler.has_value() && *mPostExecutionHandler) {
			(*mPostExecutionHandler)();
		}
	}

	void command_buffer_t::begin_recording()
	{
		mCommandBuffer->begin(mBeginInfo);
		mState = command_buffer_state::recording;
	}

	void command_buffer_t::end_recording()
	{
		mCommandBuffer->end();
		mState = command_buffer_state::finished_recording;
	}

	void command_buffer_t::begin_render_pass_for_framebuffer(resource_reference<const renderpass_t> aRenderpass, resource_reference<framebuffer_t> aFramebuffer, vk::Offset2D aRenderAreaOffset, std::optional<vk::Extent2D> aRenderAreaExtent, bool aSubpassesInline)
	{
		const auto firstAttachmentsSize = aFramebuffer->image_view_at(0)->get_image().config().extent;
		const auto& clearValues = aRenderpass->clear_values();
		auto renderPassBeginInfo = vk::RenderPassBeginInfo()
			.setRenderPass(aRenderpass->handle())
			.setFramebuffer(aFramebuffer->handle())
			.setRenderArea(vk::Rect2D()
				.setOffset(vk::Offset2D{ aRenderAreaOffset.x, aRenderAreaOffset.y })
				.setExtent(aRenderAreaExtent.has_value()
							? vk::Extent2D{ aRenderAreaExtent.value() }
							: vk::Extent2D{ firstAttachmentsSize.width,  firstAttachmentsSize.height }
					)
				)
			.setClearValueCount(static_cast<uint32_t>(clearValues.size()))
			.setPClearValues(clearValues.data());

		mSubpassContentsState = aSubpassesInline ? vk::SubpassContents::eInline : vk::SubpassContents::eSecondaryCommandBuffers;
		mCommandBuffer->beginRenderPass(renderPassBeginInfo, mSubpassContentsState);
		// 2nd parameter: how the drawing commands within the render pass will be provided. It can have one of two values [7]:
		//  - VK_SUBPASS_CONTENTS_INLINE: The render pass commands will be embedded in the primary command buffer itself and no secondary command buffers will be executed.
		//  - VK_SUBPASS_CONTENTS_SECONDARY_command_buffer_tS : The render pass commands will be executed from secondary command buffers.

		// Sorry, but have to do this:
#ifdef _DEBUG
		bool hadToEnable = false;
#endif
		std::vector<avk::image_view> imageViews;
		for (auto& view : aFramebuffer->image_views()) {
			if (!view.is_shared_ownership_enabled()) { // TODO: Make layout transitions explicit and get rid of this
				view.enable_shared_ownership();
#ifdef _DEBUG
				hadToEnable = true;
#endif
			}
			imageViews.push_back(view);
		}
#ifdef _DEBUG
		if (hadToEnable) {
			AVK_LOG_DEBUG("Had to enable shared ownership on all the framebuffers' views in command_buffer_t::begin_render_pass_for_framebuffer, fyi.");
		}
#endif
		set_post_execution_handler([lAttachmentDescs = aRenderpass->attachment_descriptions(), lImageViews = std::move(imageViews)] () {
			const auto n = lImageViews.size();
			for (size_t i = 0; i < n; ++i) {
				// I think, the const_cast is justified here:
				const_cast<image_t&>(lImageViews[i]->get_image()).set_current_layout(lAttachmentDescs[i].finalLayout);
			}
		});
	}

	void command_buffer_t::next_subpass()
	{
		mCommandBuffer->nextSubpass(mSubpassContentsState);
	}

	void command_buffer_t::establish_execution_barrier(pipeline_stage aSrcStage, pipeline_stage aDstStage)
	{
		mCommandBuffer->pipelineBarrier(
			to_vk_pipeline_stage_flags(aSrcStage), // Up to which stage to execute before making memory available
			to_vk_pipeline_stage_flags(aDstStage), // Which stage has to wait until memory has been made visible
			vk::DependencyFlags{}, // TODO: support dependency flags
			{},	{}, {} // no memory barriers
		);
	}

	void command_buffer_t::establish_global_memory_barrier(pipeline_stage aSrcStage, pipeline_stage aDstStage, std::optional<memory_access> aSrcAccessToBeMadeAvailable, std::optional<memory_access> aDstAccessToBeMadeVisible)
	{
		mCommandBuffer->pipelineBarrier(
			to_vk_pipeline_stage_flags(aSrcStage),				// Up to which stage to execute before making memory available
			to_vk_pipeline_stage_flags(aDstStage),				// Which stage has to wait until memory has been made visible
			vk::DependencyFlags{},								// TODO: support dependency flags
			{ vk::MemoryBarrier{								// Establish a global memory barrier, ...
				to_vk_access_flags(aSrcAccessToBeMadeAvailable),//  ... making memory from these access types available (after aSrcStage),
				to_vk_access_flags(aDstAccessToBeMadeVisible)	//  ... and for these access types visible (before aDstStage)
			}},
			{}, {} // no buffer/image memory barriers
		);
	}

	void command_buffer_t::establish_global_memory_barrier_rw(pipeline_stage aSrcStage, pipeline_stage aDstStage, std::optional<write_memory_access> aSrcAccessToBeMadeAvailable, std::optional<read_memory_access> aDstAccessToBeMadeVisible)
	{
		establish_global_memory_barrier(aSrcStage, aDstStage, to_memory_access(aSrcAccessToBeMadeAvailable), to_memory_access(aDstAccessToBeMadeVisible));
	}

	void command_buffer_t::establish_image_memory_barrier(image_t& aImage, pipeline_stage aSrcStage, pipeline_stage aDstStage, std::optional<memory_access> aSrcAccessToBeMadeAvailable, std::optional<memory_access> aDstAccessToBeMadeVisible)
	{
		mCommandBuffer->pipelineBarrier(
			to_vk_pipeline_stage_flags(aSrcStage),						// Up to which stage to execute before making memory available
			to_vk_pipeline_stage_flags(aDstStage),						// Which stage has to wait until memory has been made visible
			vk::DependencyFlags{},										// TODO: support dependency flags
			{}, {},														// no global memory barriers, no buffer memory barriers
			{
				vk::ImageMemoryBarrier{
					to_vk_access_flags(aSrcAccessToBeMadeAvailable),	// After the aSrcStage, make this memory available
					to_vk_access_flags(aDstAccessToBeMadeVisible),		// Before the aDstStage, make this memory visible
					aImage.current_layout(), aImage.target_layout(),	// Transition for the former to the latter
					VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,	// TODO: Support queue family ownership transfer
					aImage.handle(),
					aImage.entire_subresource_range()					// TODO: Support different subresource ranges
				}
			}
		);
		aImage.set_current_layout(aImage.target_layout()); // Just optimistically set it
	}

	void command_buffer_t::establish_image_memory_barrier_rw(image_t& aImage, pipeline_stage aSrcStage, pipeline_stage aDstStage, std::optional<write_memory_access> aSrcAccessToBeMadeAvailable, std::optional<read_memory_access> aDstAccessToBeMadeVisible)
	{
		establish_image_memory_barrier(aImage, aSrcStage, aDstStage, to_memory_access(aSrcAccessToBeMadeAvailable), to_memory_access(aDstAccessToBeMadeVisible));
	}

	void command_buffer_t::establish_buffer_memory_barrier(buffer_t& aBuffer, pipeline_stage aSrcStage, pipeline_stage aDstStage, std::optional<memory_access> aSrcAccessToBeMadeAvailable, std::optional<memory_access> aDstAccessToBeMadeVisible)
	{
		mCommandBuffer->pipelineBarrier(
			to_vk_pipeline_stage_flags(aSrcStage),						// Up to which stage to execute before making memory available
			to_vk_pipeline_stage_flags(aDstStage),						// Which stage has to wait until memory has been made visible
			vk::DependencyFlags{},										// TODO: support dependency flags
			{},
			{
				vk::BufferMemoryBarrier{
					to_vk_access_flags(aSrcAccessToBeMadeAvailable),	// After the aSrcStage, make this memory available
					to_vk_access_flags(aDstAccessToBeMadeVisible),		// Before the aDstStage, make this memory visible
					VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
					aBuffer.handle(),
					0, aBuffer.meta_at_index<buffer_meta>().total_size()
				}
			},
			{}
		);
	}

	void command_buffer_t::establish_buffer_memory_barrier_rw(buffer_t& aBuffer, pipeline_stage aSrcStage, pipeline_stage aDstStage, std::optional<write_memory_access> aSrcAccessToBeMadeAvailable, std::optional<read_memory_access> aDstAccessToBeMadeVisible)
	{
		establish_buffer_memory_barrier(aBuffer, aSrcStage, aDstStage, to_memory_access(aSrcAccessToBeMadeAvailable), to_memory_access(aDstAccessToBeMadeVisible));
	}

	void command_buffer_t::establish(const pipeline_barrier_data& aBarrierData)
	{
		aBarrierData.make_barrier(*this);
	}

	void command_buffer_t::copy_image(const image_t& aSource, const vk::Image& aDestination)
	{ // TODO: fix this hack after the RTX-VO!
		auto fullImageOffset = vk::Offset3D(0, 0, 0);
		auto fullImageExtent = aSource.config().extent;
		auto halfImageOffset = vk::Offset3D(0, 0, 0); //vk::Offset3D(pSource.mInfo.extent.width / 2, 0, 0);
		auto halfImageExtent = vk::Extent3D(aSource.config().extent.width, aSource.config().extent.height, aSource.config().extent.depth);
		auto offset = halfImageOffset;
		auto extent = halfImageExtent;

		auto copyInfo = vk::ImageCopy()
			.setSrcSubresource(vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0u, 0u, 1u))
			.setSrcOffset(offset)
			.setDstSubresource(vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0u, 0u, 1u))
			.setDstOffset(offset)
			.setExtent(extent);
		mCommandBuffer->copyImage(aSource.handle(), vk::ImageLayout::eTransferSrcOptimal, aDestination, vk::ImageLayout::eTransferDstOptimal, { copyInfo });
	}

	void command_buffer_t::end_render_pass()
	{
		mCommandBuffer->endRenderPass();
	}

	void command_buffer_t::bind_descriptors(vk::PipelineBindPoint aBindingPoint, vk::PipelineLayout aLayoutHandle, std::vector<descriptor_set> aDescriptorSets)
	{
		if (aDescriptorSets.size() == 0) {
			AVK_LOG_WARNING("command_buffer_t::bind_descriptors has been called, but there are no descriptor sets to be bound.");
			return;
		}

		std::vector<vk::DescriptorSet> handles;
		handles.reserve(aDescriptorSets.size());
		for (const auto& dset : aDescriptorSets)
		{
			handles.push_back(dset.handle());
		}

		if (aDescriptorSets.empty()) {
			return;
		}

		// Issue one or multiple bindDescriptorSets commands. We can only bind CONSECUTIVELY NUMBERED sets.
		size_t descIdx = 0;
		while (descIdx < aDescriptorSets.size()) {
			const uint32_t setId = aDescriptorSets[descIdx].set_id();
			uint32_t count = 1u;
			while ((descIdx + count) < aDescriptorSets.size() && aDescriptorSets[descIdx + count].set_id() == (setId + count)) {
				++count;
			}

			handle().bindDescriptorSets(
				aBindingPoint,
				aLayoutHandle,
				setId, count,
				&handles[descIdx],
				0, // TODO: Dynamic offset count
				nullptr); // TODO: Dynamic offset

			descIdx += count;
		}
	}

#if VK_HEADER_VERSION >= 135
	void command_buffer_t::trace_rays(
		vk::Extent3D aRaygenDimensions,
		const shader_binding_table_ref& aShaderBindingTableRef,
		vk::DispatchLoaderDynamic aDynamicDispatch,
#if VK_HEADER_VERSION >= 162
		const vk::StridedDeviceAddressRegionKHR& aRaygenSbtRef,
		const vk::StridedDeviceAddressRegionKHR& aRaymissSbtRef,
		const vk::StridedDeviceAddressRegionKHR& aRayhitSbtRef,
		const vk::StridedDeviceAddressRegionKHR& aCallableSbtRef

#else
		const vk::StridedBufferRegionKHR& aRaygenSbtRef,
		const vk::StridedBufferRegionKHR& aRaymissSbtRef,
		const vk::StridedBufferRegionKHR& aRayhitSbtRef,
		const vk::StridedBufferRegionKHR& aCallableSbtRef
#endif
	)
	{
		const auto sbtHandle = aShaderBindingTableRef.mSbtBufferHandle;
		const auto entrySize = aShaderBindingTableRef.mSbtEntrySize;
		handle().traceRaysKHR(
			&aRaygenSbtRef, &aRaymissSbtRef, &aRayhitSbtRef, &aCallableSbtRef,
			aRaygenDimensions.width, aRaygenDimensions.height, aRaygenDimensions.depth,
			aDynamicDispatch
		);
	}
#endif
#pragma endregion

#pragma region compute pipeline definitions
	void root::rewire_config_and_create_compute_pipeline(compute_pipeline_t& aPreparedPipeline)
	{
		aPreparedPipeline.mShaderStageCreateInfo
			.setModule(aPreparedPipeline.mShader.handle())
			.setPName(aPreparedPipeline.mShader.info().mEntryPoint.c_str());

		if (aPreparedPipeline.mSpecializationInfo.has_value()) {
			assert(aPreparedPipeline.mShader.info().mSpecializationConstants.has_value());
			aPreparedPipeline.mSpecializationInfo.value().pMapEntries = aPreparedPipeline.mShader.info().mSpecializationConstants.value().mMapEntries.data();
			aPreparedPipeline.mSpecializationInfo.value().pData = aPreparedPipeline.mShader.info().mSpecializationConstants.value().mData.data();
			aPreparedPipeline.mShaderStageCreateInfo.setPSpecializationInfo(&aPreparedPipeline.mSpecializationInfo.value());
		}

		// Layout must already be configured and created properly!

		// Create the PIPELINE LAYOUT
		aPreparedPipeline.mPipelineLayout = device().createPipelineLayoutUnique(aPreparedPipeline.mPipelineLayoutCreateInfo);
		assert(static_cast<bool>(aPreparedPipeline.layout_handle()));

		// Create the PIPELINE, a.k.a. putting it all together:
		auto pipelineInfo = vk::ComputePipelineCreateInfo{}
			.setFlags(aPreparedPipeline.mPipelineCreateFlags)
			.setStage(aPreparedPipeline.mShaderStageCreateInfo)
			.setLayout(aPreparedPipeline.layout_handle())
			.setBasePipelineHandle(nullptr) // Optional
			.setBasePipelineIndex(-1); // Optional
#if VK_HEADER_VERSION >= 141
		auto result = device().createComputePipelineUnique(nullptr, pipelineInfo);
		aPreparedPipeline.mPipeline = std::move(result.value);
#else
		aPreparedPipeline.mPipeline = device().createComputePipelineUnique(nullptr, pipelineInfo);
#endif
	}

	compute_pipeline root::create_compute_pipeline(compute_pipeline_config aConfig, std::function<void(compute_pipeline_t&)> aAlterConfigBeforeCreation)
	{
		compute_pipeline_t result;

		// 1. Compile and store the one and only shader:
		if (!aConfig.mShaderInfo.has_value()) {
			throw avk::logic_error("Shader missing in compute_pipeline_config! A compute pipeline can not be constructed without a shader.");
		}
		//    Compile the shader
		result.mShader = create_shader(aConfig.mShaderInfo.value());
		assert(result.mShader.has_been_built());
		//    Just fill in the create struct
		result.mShaderStageCreateInfo = vk::PipelineShaderStageCreateInfo{}
			.setStage(to_vk_shader_stage(result.mShader.info().mShaderType))
			.setModule(result.mShader.handle())
			.setPName(result.mShader.info().mEntryPoint.c_str());
		if (result.mShader.info().mSpecializationConstants.has_value()) {
			result.mSpecializationInfo = vk::SpecializationInfo{
				result.mShader.info().mSpecializationConstants.value().num_entries(),
				result.mShader.info().mSpecializationConstants.value().mMapEntries.data(),
				result.mShader.info().mSpecializationConstants.value().data_size(),
				result.mShader.info().mSpecializationConstants.value().mData.data()
			};
			// Add it to the stageCreateInfo:
			result.mShaderStageCreateInfo.setPSpecializationInfo(&result.mSpecializationInfo.value());
		}

		// 2. Flags
		// TODO: Support all flags (only one of the flags is handled at the moment)
		result.mPipelineCreateFlags = {};
		if ((aConfig.mPipelineSettings & cfg::pipeline_settings::disable_optimization) == cfg::pipeline_settings::disable_optimization) {
			result.mPipelineCreateFlags |= vk::PipelineCreateFlagBits::eDisableOptimization;
		}

		// 3. Compile the PIPELINE LAYOUT data and create-info
		// Get the descriptor set layouts
		result.mAllDescriptorSetLayouts = set_of_descriptor_set_layouts::prepare(std::move(aConfig.mResourceBindings));
		allocate_set_of_descriptor_set_layouts(result.mAllDescriptorSetLayouts);

		// Gather the push constant data
		result.mPushConstantRanges.reserve(aConfig.mPushConstantsBindings.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!
		for (const auto& pcBinding : aConfig.mPushConstantsBindings) {
			result.mPushConstantRanges.push_back(vk::PushConstantRange{}
				.setStageFlags(to_vk_shader_stages(pcBinding.mShaderStages))
				.setOffset(static_cast<uint32_t>(pcBinding.mOffset))
				.setSize(static_cast<uint32_t>(pcBinding.mSize))
			);
			// TODO: Push Constants need a prettier interface
		}

		auto descriptorSetLayoutHandles = result.mAllDescriptorSetLayouts.layout_handles();
		result.mPipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo{}
			.setSetLayoutCount(static_cast<uint32_t>(descriptorSetLayoutHandles.size()))
			.setPSetLayouts(descriptorSetLayoutHandles.data())
			.setPushConstantRangeCount(static_cast<uint32_t>(result.mPushConstantRanges.size()))
			.setPPushConstantRanges(result.mPushConstantRanges.data());

		// 4. Maybe alter the config?!
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(result);
		}

		rewire_config_and_create_compute_pipeline(result);
		return result;
	}

	compute_pipeline root::create_compute_pipeline_from_template(resource_reference<const compute_pipeline_t> aTemplate, std::function<void(compute_pipeline_t&)> aAlterConfigBeforeCreation)
	{
		compute_pipeline_t result;
		result.mPipelineCreateFlags			= aTemplate->mPipelineCreateFlags;
		result.mShader						= create_shader_from_template(aTemplate->mShader);
		result.mShaderStageCreateInfo		= aTemplate->mShaderStageCreateInfo;
		result.mSpecializationInfo			= aTemplate->mSpecializationInfo;
		result.mBasePipelineIndex			= aTemplate->mBasePipelineIndex;
		result.mAllDescriptorSetLayouts		= create_set_of_descriptor_set_layouts_from_template(aTemplate->mAllDescriptorSetLayouts);
		result.mPushConstantRanges			= aTemplate->mPushConstantRanges;
		result.mPipelineLayoutCreateInfo	= aTemplate->mPipelineLayoutCreateInfo;

		auto descriptorSetLayoutHandles = result.mAllDescriptorSetLayouts.layout_handles();
		result.mPipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo{}
			.setSetLayoutCount(static_cast<uint32_t>(descriptorSetLayoutHandles.size()))
			.setPSetLayouts(descriptorSetLayoutHandles.data())
			.setPushConstantRangeCount(static_cast<uint32_t>(result.mPushConstantRanges.size()))
			.setPPushConstantRanges(result.mPushConstantRanges.data());

		// 4. Maybe alter the config?!
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(result);
		}

		rewire_config_and_create_compute_pipeline(result);
		return result;
	}
#pragma endregion

#pragma region descriptor alloc request
	descriptor_alloc_request::descriptor_alloc_request()
		: mNumSets{ 0u }
	{}

	descriptor_alloc_request::descriptor_alloc_request(const std::vector<std::reference_wrapper<const descriptor_set_layout>>& aLayouts)
	{
		mNumSets = static_cast<uint32_t>(aLayouts.size());

		for (const auto& layout : aLayouts) {
			// Accumulate all the memory requirements of all the sets
			for (const auto& entry : layout.get().required_pool_sizes()) {
				auto it = std::lower_bound(std::begin(mAccumulatedSizes), std::end(mAccumulatedSizes),
					entry,
					[](const vk::DescriptorPoolSize& first, const vk::DescriptorPoolSize& second) -> bool {
						using EnumType = std::underlying_type<vk::DescriptorType>::type;
						return static_cast<EnumType>(first.type) < static_cast<EnumType>(second.type);
					});
				if (it != std::end(mAccumulatedSizes) && it->type == entry.type) {
					it->descriptorCount += entry.descriptorCount;
				}
				else {
					mAccumulatedSizes.insert(it, entry);
				}
			}
		}
	}

	void descriptor_alloc_request::add_size_requirements(vk::DescriptorPoolSize aToAdd)
	{
		auto it = std::lower_bound(std::begin(mAccumulatedSizes), std::end(mAccumulatedSizes),
			aToAdd,
			[](const vk::DescriptorPoolSize& first, const vk::DescriptorPoolSize& second) -> bool {
				using EnumType = std::underlying_type<vk::DescriptorType>::type;
				return static_cast<EnumType>(first.type) < static_cast<EnumType>(second.type);
			});
		if (it != std::end(mAccumulatedSizes) && it->type == aToAdd.type) {
			it->descriptorCount += aToAdd.descriptorCount;
		}
		else {
			mAccumulatedSizes.insert(it, aToAdd);
		}
	}

	descriptor_alloc_request descriptor_alloc_request::multiply_size_requirements(uint32_t mFactor) const
	{
		auto copy = descriptor_alloc_request{*this};
		for (auto& sr : copy.mAccumulatedSizes) {
			sr.descriptorCount *= mFactor;
		}
		return copy;
	}
#pragma endregion

#pragma region descriptor pool definitions
	descriptor_pool root::create_descriptor_pool(vk::Device aDevice, const std::vector<vk::DescriptorPoolSize>& aSizeRequirements, int aNumSets)
	{
		descriptor_pool result;
		result.mInitialCapacities = aSizeRequirements;
		result.mRemainingCapacities = aSizeRequirements;
		result.mNumInitialSets = aNumSets;
		result.mNumRemainingSets = aNumSets;

		// Create it:
		auto createInfo = vk::DescriptorPoolCreateInfo()
			.setPoolSizeCount(static_cast<uint32_t>(result.mInitialCapacities.size()))
			.setPPoolSizes(result.mInitialCapacities.data())
			.setMaxSets(aNumSets)
			.setFlags(vk::DescriptorPoolCreateFlags()); // The structure has an optional flag similar to command pools that determines if individual descriptor sets can be freed or not: VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT. We're not going to touch the descriptor set after creating it, so we don't need this flag. [10]
		result.mDescriptorPool = aDevice.createDescriptorPoolUnique(createInfo);

		AVK_LOG_DEBUG("Allocated pool with flags[" + vk::to_string(createInfo.flags) + "], maxSets[" + std::to_string(createInfo.maxSets) + "], remaining-sets[" + std::to_string(result.mNumRemainingSets) + "], size-entries[" + std::to_string(createInfo.poolSizeCount) + "]");
#if defined(_DEBUG)
		for (size_t i=0; i < aSizeRequirements.size(); ++i) {
			AVK_LOG_DEBUG_VERBOSE("          [" + std::to_string(i) + "]: descriptorCount[" + std::to_string(aSizeRequirements[i].descriptorCount) + "], descriptorType[" + vk::to_string(aSizeRequirements[i].type) + "]");
		}
#endif

		return result;
	}

	descriptor_pool root::create_descriptor_pool(const std::vector<vk::DescriptorPoolSize>& aSizeRequirements, int aNumSets)
	{
		return create_descriptor_pool(device(), aSizeRequirements, aNumSets);
	}

	bool descriptor_pool::has_capacity_for(const descriptor_alloc_request& pRequest) const
	{
		if (mNumRemainingSets < static_cast<int>(pRequest.num_sets())) {
			return false;
		}

		const auto& weNeed = pRequest.accumulated_pool_sizes();
		const auto& weHave = mRemainingCapacities;

#ifdef _DEBUG
		for (size_t i = 0; i < weNeed.size() - 1; ++i) {
			assert(weNeed[i].type < weNeed[i + 1].type);
		}
		for (size_t i = 0; i < weHave.size() - 1; ++i) {
			assert(weHave[i].type < weHave[i + 1].type);
		}
#endif

		// Accumulate all the requirements of all the sets
		using EnumType = std::underlying_type<vk::DescriptorType>::type;
		size_t n = 0, h = 0, N = weNeed.size(), H = weHave.size();
		while (n < N && h < H) {
			auto needType = static_cast<EnumType>(weNeed[n].type);
			auto haveType = static_cast<EnumType>(weHave[h].type);
			if (haveType < needType) {
				h++;
				continue;
			}
			if (needType == haveType && weNeed[n].descriptorCount <= weHave[n].descriptorCount) {
				n++;
				h++;
				continue;
			}
			return false;
		}
		return n == N; // if true => all checks have passed, if false => checks failed
	}

	std::vector<vk::DescriptorSet> descriptor_pool::allocate(const std::vector<std::reference_wrapper<const descriptor_set_layout>>& aLayouts)
	{
		std::vector<vk::DescriptorSetLayout> setLayouts;
		setLayouts.reserve(aLayouts.size());
		for (auto& in : aLayouts) {
			setLayouts.emplace_back(in.get().handle());
		}

		auto allocInfo = vk::DescriptorSetAllocateInfo()
			.setDescriptorPool(mDescriptorPool.get())
			.setDescriptorSetCount(static_cast<uint32_t>(setLayouts.size()))
			.setPSetLayouts(setLayouts.data());

		AVK_LOG_DEBUG_VERBOSE("Allocating from pool with remaining-sets[" + std::to_string(mNumRemainingSets) + "] and remaining-capacities:");
#if defined(_DEBUG)
		for (size_t i=0; i < mRemainingCapacities.size(); ++i) {
			AVK_LOG_DEBUG_VERBOSE("          [" + std::to_string(i) + "]: descriptorCount[" + std::to_string(mRemainingCapacities[i].descriptorCount) + "], descriptorType[" + vk::to_string(mRemainingCapacities[i].type) + "]");
		}
#endif
		AVK_LOG_DEBUG_VERBOSE("...going to allocate " + std::to_string(aLayouts.size()) + " set(s) of the following:");
#if defined(_DEBUG)
		for (size_t i=0; i < aLayouts.size(); ++i) {
			AVK_LOG_DEBUG_VERBOSE("          [" + std::to_string(i) + "]: number_of_bindings[" + std::to_string(aLayouts[i].get().number_of_bindings()) + "]");
			for (size_t j=0; j < aLayouts[i].get().number_of_bindings(); j++) {
				AVK_LOG_DEBUG_VERBOSE("               [" + std::to_string(j) + "]: descriptorCount[" + std::to_string(aLayouts[i].get().binding_at(j).descriptorCount) + "], descriptorType[" + vk::to_string(aLayouts[i].get().binding_at(j).descriptorType) + "]");
			}
			AVK_LOG_DEBUG_VERBOSE("          [" + std::to_string(i) + "]: required pool sizes (whatever the difference to 'bindings' is)");
			auto& rps = aLayouts[i].get().required_pool_sizes();
			for (size_t j=0; j < rps.size(); j++) {
				AVK_LOG_DEBUG_VERBOSE("               [" + std::to_string(j) + "]: descriptorCount[" + std::to_string(rps[j].descriptorCount) + "], descriptorType[" + vk::to_string(rps[j].type) + "]");
			}
		}
#endif

		assert(mDescriptorPool);
		auto result = mDescriptorPool.getOwner().allocateDescriptorSets(allocInfo);

		// Update the pool's stats:
		for (const descriptor_set_layout& dsl : aLayouts) {
			for (const auto& dps : dsl.required_pool_sizes()) {
				auto it = std::find_if(std::begin(mRemainingCapacities), std::end(mRemainingCapacities), [&dps](vk::DescriptorPoolSize& el){
					return el.type == dps.type;
				});
				if (std::end(mRemainingCapacities) == it) {
					AVK_LOG_WARNING("Couldn't find the descriptor type that we have just allocated in mRemainingCapacities. How could this have happened?");
				}
				else {
					it->descriptorCount -= std::min(dps.descriptorCount, it->descriptorCount);
				}
			}
		}

		mNumRemainingSets -= static_cast<int>(aLayouts.size());

		return result;
	}

	descriptor_cache root::create_descriptor_cache(std::string aName)
	{
		if (aName.empty()) {
			static int sDescCacheId = 1;
			aName = "Descriptor Cache #" + std::to_string(sDescCacheId++);
		}

		descriptor_cache result;
		result.mName = std::move(aName);
		result.mPhysicalDevice = physical_device();
		result.mDevice = device();
		return result;
	}
#pragma endregion

#pragma region descriptor set layout definitions

	bool operator ==(const descriptor_set_layout& left, const descriptor_set_layout& right) {
		const auto n = left.mOrderedBindings.size();
		if (n != right.mOrderedBindings.size()) {
			return false;
		}
		for (size_t i = 0; i < n; ++i) {
			if (left.mOrderedBindings[i] != right.mOrderedBindings[i]) {
				return false;
			}
		}
		return true;
	}

	bool operator !=(const descriptor_set_layout& left, const descriptor_set_layout& right) {
		return !(left == right);
	}

	void root::allocate_descriptor_set_layout(vk::Device aDevice, descriptor_set_layout& aLayoutToBeAllocated)
	{
		if (!aLayoutToBeAllocated.mLayout) {
			// Allocate the layout and return the result:
			auto createInfo = vk::DescriptorSetLayoutCreateInfo()
				.setBindingCount(static_cast<uint32_t>(aLayoutToBeAllocated.mOrderedBindings.size()))
				.setPBindings(aLayoutToBeAllocated.mOrderedBindings.data());
			aLayoutToBeAllocated.mLayout = aDevice.createDescriptorSetLayoutUnique(createInfo);
		}
		else {
			AVK_LOG_ERROR("descriptor_set_layout's handle already has a value => it most likely has already been allocated. Won't do it again.");
		}
	}

	void root::allocate_descriptor_set_layout(descriptor_set_layout& aLayoutToBeAllocated)
	{
		return allocate_descriptor_set_layout(device(), aLayoutToBeAllocated);
	}

	descriptor_set_layout root::create_descriptor_set_layout_from_template(const descriptor_set_layout& aTemplate)
	{
		descriptor_set_layout result;
		result.mBindingRequirements = aTemplate.mBindingRequirements;
		result.mOrderedBindings = aTemplate.mOrderedBindings;
		allocate_descriptor_set_layout(result);
		return result;
	}

	set_of_descriptor_set_layouts set_of_descriptor_set_layouts::prepare(std::vector<binding_data> pBindings)
	{
		set_of_descriptor_set_layouts result;
		std::vector<binding_data> orderedBindings;
		uint32_t minSetId = std::numeric_limits<uint32_t>::max();
		uint32_t maxSetId = std::numeric_limits<uint32_t>::min();

		// Step 1: order the bindings
		for (auto& b : pBindings) {
			minSetId = std::min(minSetId, b.mSetId);
			maxSetId = std::max(maxSetId, b.mSetId);
			auto it = std::lower_bound(std::begin(orderedBindings), std::end(orderedBindings), b); // use operator<
			orderedBindings.insert(it, b);
		}

		// Step 2: assemble the separate sets
		result.mFirstSetId = minSetId;
		result.mLayouts.reserve(maxSetId); // Also create layouts for sets that have no bindings, i.e. ALWAYS prepare ALL sets for EACH set-id from 0 to maxSetId
		for (uint32_t setId = 0u; setId <= maxSetId; ++setId) {
			auto lb = std::lower_bound(std::begin(orderedBindings), std::end(orderedBindings), binding_data{ setId },
				[](const binding_data& first, const binding_data& second) -> bool {
					return first.mSetId < second.mSetId;
				});
			auto ub = std::upper_bound(std::begin(orderedBindings), std::end(orderedBindings), binding_data{ setId },
				[](const binding_data& first, const binding_data& second) -> bool {
					return first.mSetId < second.mSetId;
				});
			// For empty sets, lb==ub, which means no descriptors will be regarded. This should be fine.
			result.mLayouts.push_back(descriptor_set_layout::prepare(lb, ub));
		}

		// Step 3: Accumulate the binding requirements a.k.a. vk::DescriptorPoolSize entries
		for (auto& dsl : result.mLayouts) {
			for (auto& dps : dsl.required_pool_sizes()) {
				// find position where to insert in vector
				auto it = std::lower_bound(std::begin(result.mBindingRequirements), std::end(result.mBindingRequirements),
					dps,
					[](const vk::DescriptorPoolSize& first, const vk::DescriptorPoolSize& second) -> bool {
						using EnumType = std::underlying_type<vk::DescriptorType>::type;
						return static_cast<EnumType>(first.type) < static_cast<EnumType>(second.type);
					});
				// Maybe accumulate
				if (it != std::end(result.mBindingRequirements) && it->type == dps.type) {
					it->descriptorCount += dps.descriptorCount;
				}
				else {
					result.mBindingRequirements.insert(it, dps);
				}
			}
		}

		// Done with the preparation. (None of the descriptor_set_layout members have been allocated at this point.)
		return result;
	}

	void root::allocate_set_of_descriptor_set_layouts(set_of_descriptor_set_layouts& aLayoutsToBeAllocated)
	{
		for (auto& dsl : aLayoutsToBeAllocated.mLayouts) {
			allocate_descriptor_set_layout(dsl);
		}
	}

	set_of_descriptor_set_layouts root::create_set_of_descriptor_set_layouts_from_template(const set_of_descriptor_set_layouts& aTemplate)
	{
		set_of_descriptor_set_layouts result;
		result.mBindingRequirements = aTemplate.mBindingRequirements;
		result.mFirstSetId = aTemplate.mFirstSetId;
		for (const auto& lay : aTemplate.mLayouts) {
			result.mLayouts.push_back(create_descriptor_set_layout_from_template(lay));
		}
		return result;
	}

	std::vector<vk::DescriptorSetLayout> set_of_descriptor_set_layouts::layout_handles() const
	{
		std::vector<vk::DescriptorSetLayout> allHandles;
		allHandles.reserve(mLayouts.size());
		for (const auto& dsl : mLayouts) {
			allHandles.push_back(dsl.handle());
		}
		return allHandles;
	}
#pragma endregion

#pragma region standard descriptor set

	const descriptor_set_layout& descriptor_cache::get_or_alloc_layout(descriptor_set_layout aPreparedLayout)
	{
		const auto it = mLayouts.find(aPreparedLayout);
		if (mLayouts.end() != it) {
			assert(it->handle());
			return *it;
		}

		root::allocate_descriptor_set_layout(mDevice, aPreparedLayout);

		const auto result = mLayouts.insert(std::move(aPreparedLayout));
		assert(result.second);
		return *result.first;
	}

	std::optional<descriptor_set> descriptor_cache::get_descriptor_set_from_cache(const descriptor_set& aPreparedSet)
	{
		const auto it = mSets.find(aPreparedSet);
		if (mSets.end() != it) {
			auto found = *it;
			// This might not be the veeeery best place to alter the set-id, but let's go for it:
			found.set_set_id(aPreparedSet.set_id());
			return found;
		}
		return {};
	}

	std::vector<descriptor_set> descriptor_cache::alloc_new_descriptor_sets(const std::vector<std::reference_wrapper<const descriptor_set_layout>>& aLayouts, std::vector<descriptor_set> aPreparedSets)
	{
		assert(aLayouts.size() == aPreparedSets.size());

		std::vector<descriptor_set> result;
		if (aLayouts.empty()) {
			return result;
		}

		const int n = static_cast<int>(aLayouts.size());
#ifdef _DEBUG // Perform an extensive sanity check:
		for (int i = 0; i < n; ++i) {
			const auto dbgB = aLayouts[i].get().number_of_bindings();
			assert(dbgB == aPreparedSets[i].number_of_writes());
			for (size_t j = 0; j < dbgB; ++j) {
				assert(aLayouts[i].get().binding_at(j).binding			== aPreparedSets[i].write_at(j).dstBinding);
				assert(aLayouts[i].get().binding_at(j).descriptorCount	== aPreparedSets[i].write_at(j).descriptorCount);
				assert(aLayouts[i].get().binding_at(j).descriptorType	== aPreparedSets[i].write_at(j).descriptorType);
			}
		}
#endif

		// Find possible duplicates within the descriptor sets, and store the unique layouts so that we do not over-allocate:
		std::vector<std::reference_wrapper<const descriptor_set_layout>> layoutsOfUniqueSets;
		std::vector<int> duplicateSetIndices; // -1 ... no duplicate, [0..n) ... duplicate at the given index
		layoutsOfUniqueSets.push_back(aLayouts[0]);
		duplicateSetIndices.emplace_back(-1);
		for (int i = 1; i < n; ++i) {
			for (int j = 0; j < i; ++j) {
				if (aPreparedSets[i] == aPreparedSets[j]) {
					duplicateSetIndices.emplace_back(j);
				}
			}
			if (static_cast<int>(duplicateSetIndices.size()) == i) { // No duplicate found => nothing inserted
				layoutsOfUniqueSets.push_back(aLayouts[i]);
				duplicateSetIndices.emplace_back(-1);
			}
		}
		assert(layoutsOfUniqueSets.size() <= aLayouts.size());
		assert(duplicateSetIndices.size() == aPreparedSets.size());

		// Find a pool with enough space left for the layouts (only those required => layoutsOfUniqueSets),
		// or alloc a new pool:
		auto allocRequest = descriptor_alloc_request{ layoutsOfUniqueSets };

		std::shared_ptr<descriptor_pool> pool = nullptr;
		std::vector<vk::DescriptorSet> setHandles;

		auto poolToTry = get_descriptor_pool_for_layouts(allocRequest);

		int maxTries = 3;
		while (!pool && maxTries-- > 0) {
			try {
				assert(poolToTry->has_capacity_for(allocRequest));
				// Alloc the whole thing:
				setHandles = poolToTry->allocate(aLayouts);
				assert(setHandles.size() == aPreparedSets.size());
				// Success
				pool = poolToTry;
			}
			catch (vk::OutOfPoolMemoryError& fail) {
				AVK_LOG_ERROR(std::string("Failed to allocate descriptor sets from pool: ") + fail.what());
				switch (maxTries) {
				case 1:
					AVK_LOG_INFO("Trying again with doubled size requirements...");
					allocRequest = allocRequest.multiply_size_requirements(2u);
					poolToTry = get_descriptor_pool_for_layouts(allocRequest);
				default:
					AVK_LOG_INFO("Trying again with new pool..."); // and possibly doubled size requirements, depending on whether maxTries is 2 or 0
					poolToTry = get_descriptor_pool_for_layouts(allocRequest, true);
				}
			}
		}

		assert(pool);
		assert(setHandles.size() > 0);

		// Finish configuration (most importantly: write descriptors), and just make copies for the duplicates:
		for (int i = 0; i < n; ++i) {
			const bool isDuplicate = -1 != duplicateSetIndices[i];
			const int setIndex = isDuplicate ? duplicateSetIndices[i] : i;

			if (!isDuplicate) {
				assert(setIndex == i);
				auto& setToBeCompleted = aPreparedSets[setIndex];
				setToBeCompleted.link_to_handle_and_pool(std::move(setHandles[setIndex]), pool);
				setToBeCompleted.update_data_pointers();
				setToBeCompleted.write_descriptors();

				// Your soul... is mine:
				const auto cachedSet = mSets.insert(std::move(setToBeCompleted));
				assert(cachedSet.second); // This must not happen. Duplicate handling should have caught such cases.
				// Done. Store for result:
				result.push_back(*cachedSet.first); // Make a copy!
			}
			else {
				assert(setIndex < i);
				result.emplace_back(result[setIndex]).set_set_id(aPreparedSets[i].set_id()); // Copy the duplicate set
			}
		}

		return result;
	}

	void descriptor_cache::cleanup()
	{
		mSets.clear();
		mLayouts.clear();
	}

	std::shared_ptr<descriptor_pool> descriptor_cache::get_descriptor_pool_for_layouts(const descriptor_alloc_request& aAllocRequest, bool aRequestNewPool)
	{
		// We'll allocate the pools per (thread and name)
		auto tId = std::this_thread::get_id();
		auto& pools = mDescriptorPools[tId];

		// First of all, do some cleanup => remove all pools which no longer exist:
		pools.erase(std::remove_if(std::begin(pools), std::end(pools), [](const std::weak_ptr<descriptor_pool>& ptr) {
			return ptr.expired();
		}), std::end(pools));

		// Find a pool which is capable of allocating this:
		if (!aRequestNewPool) {
			for (auto& pool : pools) {
				if (auto sptr = pool.lock()) {
					if (sptr->has_capacity_for(aAllocRequest)) {
						return sptr;
					}
				}
			}
		}

		// We weren't lucky (or new pool has been requested) => create a new pool:
		AVK_LOG_INFO("Allocating new descriptor pool for thread[" + [tId]() { std::stringstream ss; ss << tId; return ss.str(); }() + "] and name['" + mName + "]");

		// TODO: On AMD, it seems that all the entries have to be multiplied as well, while on NVIDIA, only multiplying the number of sets seems to be sufficient
		//       => How to handle this? Overallocation is as bad as underallocation. Shall we make use of exceptions? Shall we 'if' on the vendor?

		const bool isNvidia = 0x12d2 == mPhysicalDevice.getProperties().vendorID;
		auto amplifiedAllocRequest = aAllocRequest.multiply_size_requirements(prealloc_factor());
		//if (!isNvidia) { // Let's 'if' on the vendor and see what happens...
		//}

		auto newPool = root::create_descriptor_pool(mDevice,
			isNvidia
			  ? aAllocRequest.accumulated_pool_sizes()
			  : amplifiedAllocRequest.accumulated_pool_sizes(),
			isNvidia
			  ? aAllocRequest.num_sets() * prealloc_factor()
			  : aAllocRequest.num_sets() * prealloc_factor() * 2 // the last factor is a "magic number"/"educated guess"/"preemtive strike"
		);

		auto newPoolPtr = std::make_shared<descriptor_pool>(std::move(newPool));

		//  However, set the stored capacities to the amplified version, to not mess up our internal "has_capacity_for-logic":
		newPoolPtr->set_remaining_capacities(amplifiedAllocRequest.accumulated_pool_sizes());
		//if (!isNvidia) {
		//}

		pools.emplace_back(newPoolPtr); // Store as a weak_ptr
		return newPoolPtr;
	}
#pragma endregion

#pragma region descriptor set definitions

	bool operator ==(const descriptor_set& left, const descriptor_set& right)
	{
		const auto n = left.mOrderedDescriptorDataWrites.size();
		if (n != right.mOrderedDescriptorDataWrites.size()) {
			return false;
		}
		for (size_t i = 0; i < n; ++i) {
			if (left.mOrderedDescriptorDataWrites[i].dstBinding			!= right.mOrderedDescriptorDataWrites[i].dstBinding			)			{ return false; }
			if (left.mOrderedDescriptorDataWrites[i].dstArrayElement	!= right.mOrderedDescriptorDataWrites[i].dstArrayElement	)			{ return false; }
			if (left.mOrderedDescriptorDataWrites[i].descriptorCount	!= right.mOrderedDescriptorDataWrites[i].descriptorCount	)			{ return false; }
			if (left.mOrderedDescriptorDataWrites[i].descriptorType		!= right.mOrderedDescriptorDataWrites[i].descriptorType		)			{ return false; }
			if (nullptr != left.mOrderedDescriptorDataWrites[i].pImageInfo) {
				if (nullptr == right.mOrderedDescriptorDataWrites[i].pImageInfo)																{ return false; }
				for (size_t j = 0; j < left.mOrderedDescriptorDataWrites[i].descriptorCount; ++j) {
					if (left.mOrderedDescriptorDataWrites[i].pImageInfo[j] != right.mOrderedDescriptorDataWrites[i].pImageInfo[j])				{ return false; }
				}
			}
			if (nullptr != left.mOrderedDescriptorDataWrites[i].pBufferInfo) {
				if (nullptr == right.mOrderedDescriptorDataWrites[i].pBufferInfo)																{ return false; }
				for (size_t j = 0; j < left.mOrderedDescriptorDataWrites[i].descriptorCount; ++j) {
					if (left.mOrderedDescriptorDataWrites[i].pBufferInfo[j] != right.mOrderedDescriptorDataWrites[i].pBufferInfo[j])			{ return false; }
				}
			}
			if (nullptr != left.mOrderedDescriptorDataWrites[i].pTexelBufferView) {
				if (nullptr == right.mOrderedDescriptorDataWrites[i].pTexelBufferView)															{ return false; }
				for (size_t j = 0; j < left.mOrderedDescriptorDataWrites[i].descriptorCount; ++j) {
					if (left.mOrderedDescriptorDataWrites[i].pTexelBufferView[j] != right.mOrderedDescriptorDataWrites[i].pTexelBufferView[j])	{ return false; }
				}
			}

#if VK_HEADER_VERSION >= 135
			if (nullptr != left.mOrderedDescriptorDataWrites[i].pNext) {
				if (nullptr == right.mOrderedDescriptorDataWrites[i].pNext)																		{ return false; }
				if (left.mOrderedDescriptorDataWrites[i].descriptorType == vk::DescriptorType::eAccelerationStructureKHR) {
					const auto* asInfoLeft = reinterpret_cast<const VkWriteDescriptorSetAccelerationStructureKHR*>(left.mOrderedDescriptorDataWrites[i].pNext);
					const auto* asInfoRight = reinterpret_cast<const VkWriteDescriptorSetAccelerationStructureKHR*>(right.mOrderedDescriptorDataWrites[i].pNext);
					if (asInfoLeft->accelerationStructureCount != asInfoRight->accelerationStructureCount)										{ return false; }
					for (size_t j = 0; j < asInfoLeft->accelerationStructureCount; ++j) {
						if (asInfoLeft->pAccelerationStructures[j] != asInfoRight->pAccelerationStructures[j])									{ return false; }
					}
				}
			}
#endif
		}
		return true;
	}

	bool operator !=(const descriptor_set& left, const descriptor_set& right)
	{
		return !(left == right);
	}

	void descriptor_set::update_data_pointers()
	{
		for (auto& w : mOrderedDescriptorDataWrites) {
			assert(w.dstSet == mOrderedDescriptorDataWrites[0].dstSet);
			{
				auto it = std::find_if(std::begin(mStoredImageInfos), std::end(mStoredImageInfos), [binding = w.dstBinding](const auto& element) { return std::get<uint32_t>(element) == binding; });
				if (it != std::end(mStoredImageInfos)) {
					w.pImageInfo = std::get<std::vector<vk::DescriptorImageInfo>>(*it).data();
				}
				else {
					w.pImageInfo = nullptr;
				}
			}
			{
				auto it = std::find_if(std::begin(mStoredBufferInfos), std::end(mStoredBufferInfos), [binding = w.dstBinding](const auto& element) { return std::get<uint32_t>(element) == binding; });
				if (it != std::end(mStoredBufferInfos)) {
					w.pBufferInfo = std::get<std::vector<vk::DescriptorBufferInfo>>(*it).data();
				}
				else {
					w.pBufferInfo = nullptr;
				}
			}
#if VK_HEADER_VERSION >= 135
			{
				auto it = std::find_if(std::begin(mStoredAccelerationStructureWrites), std::end(mStoredAccelerationStructureWrites), [binding = w.dstBinding](const auto& element) { return std::get<uint32_t>(element) == binding; });
				if (it != std::end(mStoredAccelerationStructureWrites)) {
					auto& tpl = std::get<1>(*it);
					w.pNext = &std::get<vk::WriteDescriptorSetAccelerationStructureKHR>(tpl);
					// Also update the pointer WITHIN the vk::WriteDescriptorSetAccelerationStructureKHR... OMG!
					std::get<vk::WriteDescriptorSetAccelerationStructureKHR>(tpl).pAccelerationStructures = std::get<1>(tpl).data();
				}
				else {
					w.pNext = nullptr;
				}
			}
#endif
			{
				auto it = std::find_if(std::begin(mStoredBufferViews), std::end(mStoredBufferViews), [binding = w.dstBinding](const auto& element) { return std::get<uint32_t>(element) == binding; });
				if (it != std::end(mStoredBufferViews)) {
					w.pTexelBufferView = std::get<std::vector<vk::BufferView>>(*it).data();
				}
				else {
					w.pTexelBufferView = nullptr;
				}
			}
		}
	}

	void descriptor_set::link_to_handle_and_pool(vk::DescriptorSet aHandle, std::shared_ptr<descriptor_pool> aPool)
	{
		mDescriptorSet = aHandle;
		for (auto& w : mOrderedDescriptorDataWrites) {
			w.setDstSet(handle());
		}
		mPool = std::move(aPool);
	}

	void descriptor_set::write_descriptors()
	{
		assert(mDescriptorSet);
		update_data_pointers();
		mPool.get()->mDescriptorPool.getOwner().updateDescriptorSets(static_cast<uint32_t>(mOrderedDescriptorDataWrites.size()), mOrderedDescriptorDataWrites.data(), 0u, nullptr);
	}

	std::vector<descriptor_set> descriptor_cache::get_or_create_descriptor_sets(std::initializer_list<binding_data> aBindings)
	{
		std::vector<binding_data> orderedBindings;
		uint32_t minSetId = std::numeric_limits<uint32_t>::max();
		uint32_t maxSetId = std::numeric_limits<uint32_t>::min();

		// Step 1: order the bindings
		for (auto& b : aBindings) {
			minSetId = std::min(minSetId, b.mSetId);
			maxSetId = std::max(maxSetId, b.mSetId);
			auto it = std::lower_bound(std::begin(orderedBindings), std::end(orderedBindings), b); // use operator<
			orderedBindings.insert(it, b);
		}

		std::vector<std::reference_wrapper<const descriptor_set_layout>> layouts;
		std::vector<descriptor_set> preparedSets;
		std::vector<descriptor_set> cachedSets;
		std::vector<bool>           validSets;

		// Step 2: go through all the sets, get or alloc layouts, and see if the descriptor sets are already in cache, by chance.
		for (uint32_t setId = minSetId; setId <= maxSetId; ++setId) {
			auto lb = std::lower_bound(std::begin(orderedBindings), std::end(orderedBindings), binding_data{ setId },
				[](const binding_data& first, const binding_data& second) -> bool {
					return first.mSetId < second.mSetId;
				});
			auto ub = std::upper_bound(std::begin(orderedBindings), std::end(orderedBindings), binding_data{ setId },
				[](const binding_data& first, const binding_data& second) -> bool {
					return first.mSetId < second.mSetId;
				});

			// Handle empty sets:
			if (lb == ub) {
				continue;
			}

			const auto& layout = get_or_alloc_layout(descriptor_set_layout::prepare(lb, ub));
			layouts.emplace_back(layout);
			auto preparedSet = descriptor_set::prepare(lb, ub);
			auto cachedSet = get_descriptor_set_from_cache(preparedSet);
			if (cachedSet.has_value()) {
				auto& back = cachedSets.emplace_back(std::move(cachedSet.value()));
				validSets.push_back(true);
			}
			else {
				cachedSets.emplace_back();
				validSets.push_back(false);
			}
			preparedSets.emplace_back(std::move(preparedSet));
		}

		if (static_cast<int>(cachedSets.size()) == std::count(std::begin(validSets), std::end(validSets), true)) {
			// Everything is cached; we're done.
			return cachedSets;
		}

		// HOWEVER, if not...
		std::vector<std::reference_wrapper<const descriptor_set_layout>> layoutsForAlloc;
		std::vector<descriptor_set> toBeAlloced;
		std::vector<size_t> indexMapping;
		for (size_t i = 0; i < cachedSets.size(); ++i) {
			if (!validSets[i]) {
				layoutsForAlloc.push_back(layouts[i]);
				toBeAlloced.push_back(std::move(preparedSets[i]));
				indexMapping.push_back(i);
			}
		}
		auto nowAlsoInCache = alloc_new_descriptor_sets(layoutsForAlloc, std::move(toBeAlloced));
		for (size_t i = 0; i < indexMapping.size(); ++i) {
			cachedSets[indexMapping[i]] = nowAlsoInCache[i];
		}
		return cachedSets;
	}

	int descriptor_cache::remove_sets_with_handle(vk::ImageView aHandle)
	{
		int numDeleted = 0;
		auto it = std::begin(mSets);
		do {
			it = std::find_if(std::begin(mSets), std::end(mSets), [aHandle](const descriptor_set& aSet) {
				auto n = aSet.number_of_writes();
				for (decltype(n) i = 0; i < n; ++i) {
					const auto& w = aSet.write_at(i);
					auto dn = w.descriptorCount;
					if (0u == dn || nullptr == w.pImageInfo) {
						continue;
					}
					for (decltype(dn) di = 0; di < dn; ++di) {
						if (w.pImageInfo[di].imageView == aHandle) {
							return true;
						}
					}
				}
				return false;
			});

			if (std::end(mSets) != it) {
				mSets.erase(it);
				++numDeleted;
				// Iterator could (will) have been invalidated => reinitialize:
				it = std::begin(mSets);
			}
		} while (std::end(mSets) != it);
		return numDeleted;
	}

	int descriptor_cache::remove_sets_with_handle(vk::Buffer aHandle)
	{
		int numDeleted = 0;
		auto it = std::begin(mSets);
		do {
			it = std::find_if(std::begin(mSets), std::end(mSets), [aHandle](const descriptor_set& aSet) {
				auto n = aSet.number_of_writes();
				for (decltype(n) i = 0; i < n; ++i) {
					const auto& w = aSet.write_at(i);
					auto dn = w.descriptorCount;
					if (0u == dn || nullptr == w.pBufferInfo) {
						continue;
					}
					for (decltype(dn) di = 0; di < dn; ++di) {
						if (w.pBufferInfo[di].buffer == aHandle) {
							return true;
						}
					}
				}
				return false;
				});

			if (std::end(mSets) != it) {
				mSets.erase(it);
				++numDeleted;
				// Iterator could (will) have been invalidated => reinitialize:
				it = std::begin(mSets);
			}
		} while (std::end(mSets) != it);
		return numDeleted;
	}

	int descriptor_cache::remove_sets_with_handle(vk::Sampler aHandle)
	{
		int numDeleted = 0;
		auto it = std::begin(mSets);
		do {
			it = std::find_if(std::begin(mSets), std::end(mSets), [aHandle](const descriptor_set& aSet) {
				auto n = aSet.number_of_writes();
				for (decltype(n) i = 0; i < n; ++i) {
					const auto& w = aSet.write_at(i);
					auto dn = w.descriptorCount;
					if (0u == dn || nullptr == w.pImageInfo) {
						continue;
					}
					for (decltype(dn) di = 0; di < dn; ++di) {
						if (w.pImageInfo[di].sampler == aHandle) {
							return true;
						}
					}
				}
				return false;
				});

			if (std::end(mSets) != it) {
				mSets.erase(it);
				++numDeleted;
				// Iterator could (will) have been invalidated => reinitialize:
				it = std::begin(mSets);
			}
		} while (std::end(mSets) != it);
		return numDeleted;
	}

	int descriptor_cache::remove_sets_with_handle(vk::BufferView aHandle)
	{
		int numDeleted = 0;
		auto it = std::begin(mSets);
		do {
			it = std::find_if(std::begin(mSets), std::end(mSets), [aHandle](const descriptor_set& aSet) {
				auto n = aSet.number_of_writes();
				for (decltype(n) i = 0; i < n; ++i) {
					const auto& w = aSet.write_at(i);
					auto dn = w.descriptorCount;
					if (0u == dn || nullptr == w.pTexelBufferView) {
						continue;
					}
					for (decltype(dn) di = 0; di < dn; ++di) {
						if (w.pTexelBufferView[di] == aHandle) {
							return true;
						}
					}
				}
				return false;
				});

			if (std::end(mSets) != it) {
				mSets.erase(it);
				++numDeleted;
				// Iterator could (will) have been invalidated => reinitialize:
				it = std::begin(mSets);
			}
		} while (std::end(mSets) != it);
		return numDeleted;
	}

#pragma endregion

#pragma region fence definitions
	fence_t::~fence_t()
	{
		if (mCustomDeleter.has_value() && *mCustomDeleter) {
			// If there is a custom deleter => call it now
			(*mCustomDeleter)();
			mCustomDeleter.reset();
		}
		// Destroy the dependant instance before destroying myself
		// ^ This is ensured by the order of the members
		//   See: https://isocpp.org/wiki/faq/dtors#calling-member-dtors
	}

	fence_t& fence_t::set_designated_queue(queue& _Queue)
	{
		mQueue = &_Queue;
		return *this;
	}

	void fence_t::wait_until_signalled(std::optional<uint64_t> aTimeout) const
	{
		// ReSharper disable once CppExpressionWithoutSideEffects
		auto result = mFence.getOwner().waitForFences(1u, handle_ptr(), VK_TRUE, aTimeout.value_or(UINT64_MAX));
		assert(static_cast<VkResult>(result) >= 0);
	}

	void fence_t::reset()
	{
		// ReSharper disable once CppExpressionWithoutSideEffects
		mFence.getOwner().resetFences(1u, handle_ptr());
		if (mCustomDeleter.has_value() && *mCustomDeleter) {
			// If there is a custom deleter => call it now
			(*mCustomDeleter)();
			mCustomDeleter.reset();
		}
	}

	fence root::create_fence(vk::Device aDevice, bool aCreateInSignalledState, std::function<void(fence_t&)> aAlterConfigBeforeCreation)
	{
		fence_t result;
		result.mCreateInfo = vk::FenceCreateInfo()
			.setFlags(aCreateInSignalledState
						? vk::FenceCreateFlagBits::eSignaled
						: vk::FenceCreateFlags()
			);

		// Maybe alter the config?
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(result);
		}

		result.mFence = aDevice.createFenceUnique(result.mCreateInfo);
		return result;
	}

	fence root::create_fence(bool aCreateInSignalledState, std::function<void(fence_t&)> aAlterConfigBeforeCreation)
	{
		return create_fence(device(), aCreateInSignalledState, std::move(aAlterConfigBeforeCreation));
	}
#pragma endregion

#pragma region framebuffer definitions
	void root::check_and_config_attachments_based_on_views(std::vector<attachment>& aAttachments, std::vector<resource_ownership<image_view_t>>& aImageViews)
	{
		if (aAttachments.size() != aImageViews.size()) {
			throw avk::runtime_error("Incomplete config for framebuffer creation: number of attachments (" + std::to_string(aAttachments.size()) + ") does not equal the number of image views (" + std::to_string(aImageViews.size()) + ")");
		}
		auto n = aAttachments.size();
		for (size_t i = 0; i < n; ++i) {
			auto& a = aAttachments[i];
			auto& v = aImageViews[i];
			if ((is_depth_format(v->get_image().format()) || has_stencil_component(v->get_image().format())) && !a.is_used_as_depth_stencil_attachment()) {
				AVK_LOG_WARNING("Possibly misconfigured framebuffer: image[" + std::to_string(i) + "] is a depth/stencil format, but it is never indicated to be used as such in the attachment-description[" + std::to_string(i) + "].");
			}
			// TODO: Maybe further checks?
			if (!a.mImageUsageHintBefore.has_value() && !a.mImageUsageHintAfter.has_value()) {
				a.mImageUsageHintAfter = a.mImageUsageHintBefore = v->get_image().usage_config();
			}
		}
	}

	framebuffer root::create_framebuffer(resource_ownership<renderpass_t> aRenderpass, std::vector<resource_ownership<image_view_t>> aImageViews, uint32_t aWidth, uint32_t aHeight, std::function<void(framebuffer_t&)> aAlterConfigBeforeCreation)
	{
		framebuffer_t result;
		result.mRenderpass = aRenderpass.own(); // move
		result.mImageViews.reserve(aImageViews.size());
		for (auto& iv : aImageViews) {
			result.mImageViews.emplace_back(iv.own()); // move
		}

		std::vector<vk::ImageView> imageViewHandles;
		for (const auto& iv : result.mImageViews) {
			imageViewHandles.push_back(iv->handle());
		}

		result.mCreateInfo = vk::FramebufferCreateInfo{}
			.setRenderPass(result.mRenderpass->handle())
			.setAttachmentCount(static_cast<uint32_t>(imageViewHandles.size()))
			.setPAttachments(imageViewHandles.data())
			.setWidth(aWidth)
			.setHeight(aHeight)
			// TODO: Support multiple layers of image arrays!
			.setLayers(1u); // number of layers in image arrays [6]

		// Maybe alter the config?!
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(result);
		}

		result.mFramebuffer = device().createFramebufferUnique(result.mCreateInfo);

		// Set the right layouts for the images:
		const auto n = result.mImageViews.size();
		const auto& attDescs = result.mRenderpass->attachment_descriptions();
		for (size_t i = 0; i < n; ++i) {
			result.mImageViews[i]->get_image().transition_to_layout(attDescs[i].initialLayout);
		}

		return result;
	}

	framebuffer root::create_framebuffer(std::vector<attachment> aAttachments, std::vector<resource_ownership<image_view_t>> aImageViews, uint32_t aWidth, uint32_t aHeight, std::function<void(framebuffer_t&)> aAlterConfigBeforeCreation)
	{
		check_and_config_attachments_based_on_views(aAttachments, aImageViews);
		return create_framebuffer(
			owned(create_renderpass(std::move(aAttachments))),
			std::move(aImageViews),
			aWidth, aHeight,
			std::move(aAlterConfigBeforeCreation)
		);
	}

	framebuffer root::create_framebuffer(resource_ownership<renderpass_t> aRenderpass, std::vector<resource_ownership<image_view_t>> aImageViews, std::function<void(framebuffer_t&)> aAlterConfigBeforeCreation)
	{
		assert(!aImageViews.empty());
		auto extent = aImageViews.front()->get_image().config().extent;
		return create_framebuffer(std::move(aRenderpass), std::move(aImageViews), extent.width, extent.height, std::move(aAlterConfigBeforeCreation));
	}

	framebuffer root::create_framebuffer(std::vector<avk::attachment> aAttachments, std::vector<resource_ownership<image_view_t>> aImageViews, std::function<void(framebuffer_t&)> aAlterConfigBeforeCreation)
	{
		check_and_config_attachments_based_on_views(aAttachments, aImageViews);
		return create_framebuffer(
			owned(create_renderpass(std::move(aAttachments))),
			std::move(aImageViews),
			std::move(aAlterConfigBeforeCreation)
		);
	}

	framebuffer root::create_framebuffer_from_template(resource_reference<const framebuffer_t> aTemplate, std::function<void(image_t&)> aAlterImageConfigBeforeCreation,
		std::function<void(image_view_t&)> aAlterImageViewConfigBeforeCreation, std::function<void(framebuffer_t&)> aAlterFramebufferConfigBeforeCreation)
	{
		const auto& templateImageViews = aTemplate->image_views();
		std::vector<resource_ownership<avk::image_view_t>> imageViews;

		for (auto& imView : templateImageViews)	{
			auto imageView = create_image_view_from_template(imView, aAlterImageConfigBeforeCreation, aAlterImageViewConfigBeforeCreation);
			imageViews.emplace_back(std::move(imageView));
		}

		return create_framebuffer(
			owned(create_renderpass_from_template(aTemplate->get_renderpass())),
			std::move(imageViews),
			aTemplate.get().mCreateInfo.width,
			aTemplate.get().mCreateInfo.height,
			aAlterFramebufferConfigBeforeCreation
		);
	}

	std::optional<command_buffer> framebuffer_t::initialize_attachments(sync aSync)
	{
		aSync.establish_barrier_before_the_operation(pipeline_stage::transfer, {}); // TODO: Don't use transfer after barrier-stage-refactoring

		const int n = mImageViews.size();
		assert (n == mRenderpass->attachment_descriptions().size());
		for (size_t i = 0; i < n; ++i) {
			mImageViews[i]->get_image().transition_to_layout(mRenderpass->attachment_descriptions()[i].finalLayout, sync::auxiliary_with_barriers(aSync, {}, {}));
		}

		aSync.establish_barrier_after_the_operation(pipeline_stage::transfer, {}); // TODO: Don't use transfer after barrier-stage-refactoring
		return aSync.submit_and_sync();
	}
#pragma endregion

#pragma region geometry instance definitions
#if VK_HEADER_VERSION >= 135
	geometry_instance root::create_geometry_instance(resource_reference<const bottom_level_acceleration_structure_t> aBlas)
	{
		// glm::mat4 mTransform;
		// uint32_t mInstanceCustomIndex;
		// uint32_t mMask;
		// size_t mInstanceOffset;
		// vk::GeometryInstanceFlagsKHR mFlags;
		// uint64_t mAccelerationStructureDeviceHandle;
		return geometry_instance
		{
			{ 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f },
			0,
			0xff,
			0,
			vk::GeometryInstanceFlagsKHR(),
			aBlas->device_address()
		};
	}

	geometry_instance& geometry_instance::set_transform(VkTransformMatrixKHR aTransformationMatrix)
	{
		mTransform.matrix[0][0] = aTransformationMatrix.matrix[0][0];
		mTransform.matrix[0][1] = aTransformationMatrix.matrix[0][1];
		mTransform.matrix[0][2] = aTransformationMatrix.matrix[0][2];
		mTransform.matrix[0][3] = aTransformationMatrix.matrix[0][3];
		mTransform.matrix[1][0] = aTransformationMatrix.matrix[1][0];
		mTransform.matrix[1][1] = aTransformationMatrix.matrix[1][1];
		mTransform.matrix[1][2] = aTransformationMatrix.matrix[1][2];
		mTransform.matrix[1][3] = aTransformationMatrix.matrix[1][3];
		mTransform.matrix[2][0] = aTransformationMatrix.matrix[2][0];
		mTransform.matrix[2][1] = aTransformationMatrix.matrix[2][1];
		mTransform.matrix[2][2] = aTransformationMatrix.matrix[2][2];
		mTransform.matrix[2][3] = aTransformationMatrix.matrix[2][3];
		return *this;
	}

	geometry_instance& geometry_instance::set_transform_row_major(float aTransformationMatrix[3][4])
	{
		mTransform.matrix[0][0] = aTransformationMatrix[0][0];
		mTransform.matrix[0][1] = aTransformationMatrix[0][1];
		mTransform.matrix[0][2] = aTransformationMatrix[0][2];
		mTransform.matrix[0][3] = aTransformationMatrix[0][3];
		mTransform.matrix[1][0] = aTransformationMatrix[1][0];
		mTransform.matrix[1][1] = aTransformationMatrix[1][1];
		mTransform.matrix[1][2] = aTransformationMatrix[1][2];
		mTransform.matrix[1][3] = aTransformationMatrix[1][3];
		mTransform.matrix[2][0] = aTransformationMatrix[2][0];
		mTransform.matrix[2][1] = aTransformationMatrix[2][1];
		mTransform.matrix[2][2] = aTransformationMatrix[2][2];
		mTransform.matrix[2][3] = aTransformationMatrix[2][3];
		return *this;
	}

	geometry_instance& geometry_instance::set_transform_row_major(std::array<float, 16> aTransformationMatrix)
	{
		mTransform.matrix[0][0] = aTransformationMatrix[0];
		mTransform.matrix[0][1] = aTransformationMatrix[1];
		mTransform.matrix[0][2] = aTransformationMatrix[2];
		mTransform.matrix[0][3] = aTransformationMatrix[3];
		mTransform.matrix[1][0] = aTransformationMatrix[4];
		mTransform.matrix[1][1] = aTransformationMatrix[5];
		mTransform.matrix[1][2] = aTransformationMatrix[6];
		mTransform.matrix[1][3] = aTransformationMatrix[7];
		mTransform.matrix[2][0] = aTransformationMatrix[8];
		mTransform.matrix[2][1] = aTransformationMatrix[9];
		mTransform.matrix[2][2] = aTransformationMatrix[10];
		mTransform.matrix[2][3] = aTransformationMatrix[11];
		return *this;
	}

	geometry_instance& geometry_instance::set_transform_column_major(std::array<float, 16> aTransformationMatrix)
	{
		mTransform.matrix[0][0] = aTransformationMatrix[0];
		mTransform.matrix[0][1] = aTransformationMatrix[4];
		mTransform.matrix[0][2] = aTransformationMatrix[8];
		mTransform.matrix[0][3] = aTransformationMatrix[12];
		mTransform.matrix[1][0] = aTransformationMatrix[1];
		mTransform.matrix[1][1] = aTransformationMatrix[5];
		mTransform.matrix[1][2] = aTransformationMatrix[9];
		mTransform.matrix[1][3] = aTransformationMatrix[13];
		mTransform.matrix[2][0] = aTransformationMatrix[2];
		mTransform.matrix[2][1] = aTransformationMatrix[6];
		mTransform.matrix[2][2] = aTransformationMatrix[10];
		mTransform.matrix[2][3] = aTransformationMatrix[14];
		return *this;
	}

	geometry_instance& geometry_instance::set_custom_index(uint32_t aCustomIndex)
	{
		mInstanceCustomIndex = aCustomIndex;
		return *this;
	}

	geometry_instance& geometry_instance::set_mask(uint32_t aMask)
	{
		mMask = aMask;
		return *this;
	}

	geometry_instance& geometry_instance::set_instance_offset(size_t aOffset)
	{
		mInstanceOffset = aOffset;
		return *this;
	}

	geometry_instance& geometry_instance::set_flags(vk::GeometryInstanceFlagsKHR aFlags)
	{
		mFlags = aFlags;
		return *this;
	}

	geometry_instance& geometry_instance::add_flags(vk::GeometryInstanceFlagsKHR aFlags)
	{
		mFlags |= aFlags;
		return *this;
	}

	geometry_instance& geometry_instance::disable_culling()
	{
		mFlags |= vk::GeometryInstanceFlagBitsKHR::eTriangleCullDisable;
		return *this;
	}

	geometry_instance& geometry_instance::define_front_faces_to_be_counter_clockwise()
	{
		mFlags |= vk::GeometryInstanceFlagBitsKHR::eTriangleFrontCounterclockwise;
		return *this;
	}

	geometry_instance& geometry_instance::force_opaque()
	{
		mFlags |= vk::GeometryInstanceFlagBitsKHR::eForceOpaque;
		return *this;
	}

	geometry_instance& geometry_instance::force_non_opaque()
	{
		mFlags |= vk::GeometryInstanceFlagBitsKHR::eForceNoOpaque;
		return *this;
	}

	geometry_instance& geometry_instance::reset_flags()
	{
		mFlags = vk::GeometryInstanceFlagsKHR();
		return *this;
	}

	VkAccelerationStructureInstanceKHR convert_for_gpu_usage(const geometry_instance& aGeomInst)
	{
		VkAccelerationStructureInstanceKHR element;
		element.transform = aGeomInst.mTransform;
		element.instanceCustomIndex = aGeomInst.mInstanceCustomIndex;
		element.mask = aGeomInst.mMask;
		element.instanceShaderBindingTableRecordOffset = aGeomInst.mInstanceOffset;
		element.flags = static_cast<uint32_t>(aGeomInst.mFlags);
		element.accelerationStructureReference = aGeomInst.mAccelerationStructureDeviceHandle;
		return element;
	}

	std::vector<VkAccelerationStructureInstanceKHR> convert_for_gpu_usage(const std::vector<geometry_instance>& aGeomInstances)
	{
		if (aGeomInstances.empty()) {
			AVK_LOG_WARNING("Empty vector of geometry instances passed to convert_for_gpu_usage");
		}

		std::vector<VkAccelerationStructureInstanceKHR> instancesGpu;
		instancesGpu.reserve(aGeomInstances.size());
		for (auto& data : aGeomInstances) {
			instancesGpu.emplace_back(convert_for_gpu_usage(data));
		}
		return instancesGpu;
	}
#endif
#pragma endregion

#pragma region graphics pipeline config definitions

	// Set sensible defaults:
	graphics_pipeline_config::graphics_pipeline_config()
		: mPipelineSettings{ cfg::pipeline_settings::nothing }
		, mRenderPassSubpass {} // not set by default
		, mPrimitiveTopology{ cfg::primitive_topology::triangles } // triangles after one another
		, mRasterizerGeometryMode{ cfg::rasterizer_geometry_mode::rasterize_geometry } // don't discard, but rasterize!
		, mPolygonDrawingModeAndConfig{ cfg::polygon_drawing::config_for_filling() } // Fill triangles
		, mCullingMode{ cfg::culling_mode::cull_back_faces } // Cull back faces
		, mFrontFaceWindingOrder{ cfg::front_face::define_front_faces_to_be_counter_clockwise() } // CCW == front face
		, mDepthClampBiasConfig{ cfg::depth_clamp_bias::config_nothing_special() } // no clamp, no bias, no factors
		, mDepthTestConfig{ cfg::depth_test::enabled() } // enable depth testing
		, mDepthWriteConfig{ cfg::depth_write::enabled() } // enable depth writing
		, mDepthBoundsConfig{ cfg::depth_bounds::disable() }
		, mColorBlendingSettings{ cfg::color_blending_settings::disable_logic_operation() }
		, mTessellationPatchControlPoints {}
	{
	}

	namespace cfg
	{
		viewport_depth_scissors_config viewport_depth_scissors_config::from_framebuffer(resource_reference<const framebuffer_t> aFramebuffer)
		{
			const auto width = aFramebuffer->create_info().width;
			const auto height = aFramebuffer->create_info().height;
			return viewport_depth_scissors_config{
				std::array<float, 2>{{ 0.0f, 0.0f }},
				std::array<float, 2>{{ static_cast<float>(width), static_cast<float>(height)  }},
				0.0f, 1.0f,		// TODO: make min/max depth configurable?!
				vk::Offset2D{ 0, 0 },			// TODO: support different settings for scissor?!
				vk::Extent2D{{ width, height }},
				false,
				false
			};
		}
	}
#pragma endregion

#pragma region graphics pipeline definitions
	void root::rewire_config_and_create_graphics_pipeline(graphics_pipeline_t& aPreparedPipeline)
	{
		aPreparedPipeline.mPipelineVertexInputStateCreateInfo
			.setPVertexBindingDescriptions(aPreparedPipeline.mOrderedVertexInputBindingDescriptions.data())
			.setPVertexAttributeDescriptions(aPreparedPipeline.mVertexInputAttributeDescriptions.data());

		assert(aPreparedPipeline.mShaders.size() == aPreparedPipeline.mShaderStageCreateInfos.size());
		assert(aPreparedPipeline.mShaders.size() == aPreparedPipeline.mSpecializationInfos.size());
		for (size_t i = 0; i < aPreparedPipeline.mShaders.size(); ++i) {
			aPreparedPipeline.mShaderStageCreateInfos[i]
				.setModule(aPreparedPipeline.mShaders[i].handle())
				.setPName(aPreparedPipeline.mShaders[i].info().mEntryPoint.c_str());

			if (aPreparedPipeline.mSpecializationInfos[i] != vk::SpecializationInfo{}) {
				assert(aPreparedPipeline.mShaders[i].info().mSpecializationConstants.has_value());
				aPreparedPipeline.mSpecializationInfos[i].pMapEntries = aPreparedPipeline.mShaders[i].info().mSpecializationConstants.value().mMapEntries.data();
				aPreparedPipeline.mSpecializationInfos[i].pData = aPreparedPipeline.mShaders[i].info().mSpecializationConstants.value().mData.data();
				aPreparedPipeline.mShaderStageCreateInfos[i].setPSpecializationInfo(&aPreparedPipeline.mSpecializationInfos[i]);
			}
		}

		aPreparedPipeline.mViewportStateCreateInfo
			.setViewportCount(static_cast<uint32_t>(aPreparedPipeline.mViewports.size()))
			.setPViewports(aPreparedPipeline.mViewports.data())
			.setScissorCount(static_cast<uint32_t>(aPreparedPipeline.mScissors.size()))
			.setPScissors(aPreparedPipeline.mScissors.data());

		aPreparedPipeline.mColorBlendStateCreateInfo
			.setAttachmentCount(static_cast<uint32_t>(aPreparedPipeline.mBlendingConfigsForColorAttachments.size()))
			.setPAttachments(aPreparedPipeline.mBlendingConfigsForColorAttachments.data());

		aPreparedPipeline.mMultisampleStateCreateInfo
			.setPSampleMask(nullptr);

		aPreparedPipeline.mDynamicStateCreateInfo
			.setDynamicStateCount(static_cast<uint32_t>(aPreparedPipeline.mDynamicStateEntries.size()))
			.setPDynamicStates(aPreparedPipeline.mDynamicStateEntries.data());

		// Pipeline Layout must be rewired already before calling this function

		// Create the PIPELINE LAYOUT
		aPreparedPipeline.mPipelineLayout = device().createPipelineLayoutUnique(aPreparedPipeline.mPipelineLayoutCreateInfo);
		assert(static_cast<bool>(aPreparedPipeline.layout_handle()));

		// Create the PIPELINE, a.k.a. putting it all together:
		auto pipelineInfo = vk::GraphicsPipelineCreateInfo{}
			// 0. Render Pass
			.setRenderPass((*aPreparedPipeline.mRenderPass).handle())
			.setSubpass(aPreparedPipeline.mSubpassIndex)
			// 1., 2., and 3.
			.setPVertexInputState(&aPreparedPipeline.mPipelineVertexInputStateCreateInfo)
			// 4.
			.setPInputAssemblyState(&aPreparedPipeline.mInputAssemblyStateCreateInfo)
			// 5.
			.setStageCount(static_cast<uint32_t>(aPreparedPipeline.mShaderStageCreateInfos.size()))
			.setPStages(aPreparedPipeline.mShaderStageCreateInfos.data())
			// 6.
			.setPViewportState(&aPreparedPipeline.mViewportStateCreateInfo)
			// 7.
			.setPRasterizationState(&aPreparedPipeline.mRasterizationStateCreateInfo)
			// 8.
			.setPDepthStencilState(&aPreparedPipeline.mDepthStencilConfig)
			// 9.
			.setPColorBlendState(&aPreparedPipeline.mColorBlendStateCreateInfo)
			// 10.
			.setPMultisampleState(&aPreparedPipeline.mMultisampleStateCreateInfo)
			// 11.
			.setPDynamicState(aPreparedPipeline.mDynamicStateEntries.size() == 0 ? nullptr : &aPreparedPipeline.mDynamicStateCreateInfo) // Optional
			// 12.
			.setFlags(aPreparedPipeline.mPipelineCreateFlags)
			// LAYOUT:
			.setLayout(aPreparedPipeline.layout_handle())
			// Base pipeline:
			.setBasePipelineHandle(nullptr) // Optional
			.setBasePipelineIndex(-1); // Optional

		// 13.
		if (aPreparedPipeline.mPipelineTessellationStateCreateInfo.has_value()) {
			pipelineInfo.setPTessellationState(&aPreparedPipeline.mPipelineTessellationStateCreateInfo.value());
		}

		// TODO: Shouldn't the config be altered HERE, after the pipelineInfo has been compiled?!

#if VK_HEADER_VERSION >= 141
		auto result = device().createGraphicsPipelineUnique(nullptr, pipelineInfo);
		aPreparedPipeline.mPipeline = std::move(result.value);
#else
		aPreparedPipeline.mPipeline = device().createGraphicsPipelineUnique(nullptr, pipelineInfo);
#endif
	}

	graphics_pipeline root::create_graphics_pipeline(graphics_pipeline_config aConfig, std::function<void(graphics_pipeline_t&)> aAlterConfigBeforeCreation)
	{
		using namespace cpplinq;
		using namespace cfg;

		graphics_pipeline_t result;

		// 0. Own the renderpass
		{
			assert(aConfig.mRenderPassSubpass.has_value());
			auto [rp, sp] = std::move(aConfig.mRenderPassSubpass.value());
			result.mRenderPass = std::move(rp);
			result.mSubpassIndex = sp;
		}

		// 1. Compile the array of vertex input binding descriptions
		{
			// Select DISTINCT bindings (both vertex and instance inputs):
			auto bindings = from(aConfig.mInputBindingLocations)
				>> select([](const input_binding_to_location_mapping& bindingData) { return bindingData.mGeneralData; })
				>> distinct() // this will invoke operator ==(const vertex_input_buffer_binding& left, const vertex_input_buffer_binding& right), we have selected vertex_input_buffer_binding in the line above
				>> orderby([](const vertex_input_buffer_binding& generalData) { return generalData.mBinding; })
				>> to_vector();
			result.mOrderedVertexInputBindingDescriptions.reserve(bindings.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!

			for (auto& bindingData : bindings) {

				const auto numRecordsWithSameBinding = std::count_if(std::begin(bindings), std::end(bindings),
					[bindingId = bindingData.mBinding](const vertex_input_buffer_binding& generalData) {
						return generalData.mBinding == bindingId;
					});
				if (1 != numRecordsWithSameBinding) {
					throw avk::runtime_error("The input binding #" + std::to_string(bindingData.mBinding) + " is defined in multiple times in different ways. Make sure to define it uniformly across different bindings/attribute descriptions!");
				}

				result.mOrderedVertexInputBindingDescriptions.push_back(vk::VertexInputBindingDescription{}
					// The following parameters are guaranteed to be the same. We have checked this.
					.setBinding(bindingData.mBinding)
					.setStride(static_cast<uint32_t>(bindingData.mStride))
					.setInputRate(to_vk_vertex_input_rate(bindingData.mKind))
					// We don't need the location here
				);
			}
		}

		// 2. Compile the array of vertex input attribute descriptions
		//  They will reference the bindings created in step 1.
		result.mVertexInputAttributeDescriptions.reserve(aConfig.mInputBindingLocations.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!
		for (auto& attribData : aConfig.mInputBindingLocations) {
			result.mVertexInputAttributeDescriptions.push_back(vk::VertexInputAttributeDescription{}
				.setBinding(attribData.mGeneralData.mBinding)
				.setLocation(attribData.mLocation)
				.setFormat(attribData.mMemberMetaData.mFormat)
				.setOffset(static_cast<uint32_t>(attribData.mMemberMetaData.mOffset))
			);
		}

		// 3. With the data from 1. and 2., create the complete vertex input info struct, passed to the pipeline creation
		result.mPipelineVertexInputStateCreateInfo = vk::PipelineVertexInputStateCreateInfo()
			.setVertexBindingDescriptionCount(static_cast<uint32_t>(result.mOrderedVertexInputBindingDescriptions.size()))
			.setPVertexBindingDescriptions(result.mOrderedVertexInputBindingDescriptions.data())
			.setVertexAttributeDescriptionCount(static_cast<uint32_t>(result.mVertexInputAttributeDescriptions.size()))
			.setPVertexAttributeDescriptions(result.mVertexInputAttributeDescriptions.data());

		// 4. Set how the data (from steps 1.-3.) is to be interpreted (e.g. triangles, points, lists, patches, etc.)
		result.mInputAssemblyStateCreateInfo = vk::PipelineInputAssemblyStateCreateInfo()
			.setTopology(to_vk_primitive_topology(aConfig.mPrimitiveTopology))
			.setPrimitiveRestartEnable(VK_FALSE);

		// 5. Compile and store the shaders:
		result.mShaders.reserve(aConfig.mShaderInfos.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!
		result.mShaderStageCreateInfos.reserve(aConfig.mShaderInfos.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!
		result.mSpecializationInfos.reserve(aConfig.mShaderInfos.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!
		for (auto& shaderInfo : aConfig.mShaderInfos) {
			// 5.0 Sanity check
			if (result.mShaders.end() != std::find_if(std::begin(result.mShaders), std::end(result.mShaders), [&shaderInfo](const shader& existing) { return existing.info().mShaderType == shaderInfo.mShaderType; })) {
				throw avk::runtime_error("There's already a " + vk::to_string(to_vk_shader_stages(shaderInfo.mShaderType)) + "-type shader contained in this graphics pipeline. Can not add another one of the same type.");
			}
			// 5.1 Compile the shader
			result.mShaders.push_back(create_shader(shaderInfo));
			assert(result.mShaders.back().has_been_built());
			// 5.2 Combine
			auto& stageCreateInfo = result.mShaderStageCreateInfos.emplace_back()
				.setStage(to_vk_shader_stage(result.mShaders.back().info().mShaderType))
				.setModule(result.mShaders.back().handle())
				.setPName(result.mShaders.back().info().mEntryPoint.c_str());
			if (shaderInfo.mSpecializationConstants.has_value()) {
				auto& specInfo = result.mSpecializationInfos.emplace_back(
					shaderInfo.mSpecializationConstants.value().num_entries(),
					shaderInfo.mSpecializationConstants.value().mMapEntries.data(),
					shaderInfo.mSpecializationConstants.value().data_size(),
					shaderInfo.mSpecializationConstants.value().mData.data()
				);
				// Add it to the stageCreateInfo:
				stageCreateInfo.setPSpecializationInfo(&specInfo);
			}
			else {
				result.mSpecializationInfos.emplace_back(); // Just to keep the indices into the vectors in sync!
			}
		}

		// 6. Viewport configuration
		{
			// 6.1 Viewport and depth configuration(s):
			result.mViewports.reserve(aConfig.mViewportDepthConfig.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!
			result.mScissors.reserve(aConfig.mViewportDepthConfig.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!
			for (auto& vp : aConfig.mViewportDepthConfig) {
				result.mViewports.push_back(vk::Viewport{}
					.setX(vp.x())
					.setY(vp.y())
					.setWidth(vp.width())
					.setHeight(vp.height())
					.setMinDepth(vp.min_depth())
					.setMaxDepth(vp.max_depth())
				);
				// 6.2 Skip scissors for now
				// TODO: Implement scissors support properly
				result.mScissors.push_back(vk::Rect2D{}
					.setOffset({static_cast<int32_t>(vp.x()), static_cast<int32_t>(vp.y())})
					.setExtent({static_cast<uint32_t>(vp.width()), static_cast<uint32_t>(vp.height())})
				);
			}
			// 6.3 Add everything together
			result.mViewportStateCreateInfo = vk::PipelineViewportStateCreateInfo{}
				.setViewportCount(static_cast<uint32_t>(result.mViewports.size()))
				.setPViewports(result.mViewports.data())
				.setScissorCount(static_cast<uint32_t>(result.mScissors.size()))
				.setPScissors(result.mScissors.data());
		}

		// 7. Rasterization state
		result.mRasterizationStateCreateInfo =  vk::PipelineRasterizationStateCreateInfo{}
			// Various, but important settings:
			.setRasterizerDiscardEnable(to_vk_bool(aConfig.mRasterizerGeometryMode == rasterizer_geometry_mode::discard_geometry))
			.setPolygonMode(to_vk_polygon_mode(aConfig.mPolygonDrawingModeAndConfig.drawing_mode()))
			.setLineWidth(aConfig.mPolygonDrawingModeAndConfig.line_width())
			.setCullMode(to_vk_cull_mode(aConfig.mCullingMode))
			.setFrontFace(to_vk_front_face(aConfig.mFrontFaceWindingOrder.winding_order_of_front_faces()))
			// Depth-related settings:
			.setDepthClampEnable(to_vk_bool(aConfig.mDepthClampBiasConfig.is_clamp_to_frustum_enabled()))
			.setDepthBiasEnable(to_vk_bool(aConfig.mDepthClampBiasConfig.is_depth_bias_enabled()))
			.setDepthBiasConstantFactor(aConfig.mDepthClampBiasConfig.bias_constant_factor())
			.setDepthBiasClamp(aConfig.mDepthClampBiasConfig.bias_clamp_value())
			.setDepthBiasSlopeFactor(aConfig.mDepthClampBiasConfig.bias_slope_factor());

		// 8. Depth-stencil config
		result.mDepthStencilConfig = vk::PipelineDepthStencilStateCreateInfo{}
			.setDepthTestEnable(to_vk_bool(aConfig.mDepthTestConfig.is_enabled()))
			.setDepthCompareOp(to_vk_compare_op(aConfig.mDepthTestConfig.depth_compare_operation()))
			.setDepthWriteEnable(to_vk_bool(aConfig.mDepthWriteConfig.is_enabled()))
			.setDepthBoundsTestEnable(to_vk_bool(aConfig.mDepthBoundsConfig.is_enabled()))
			.setMinDepthBounds(aConfig.mDepthBoundsConfig.min_bounds())
			.setMaxDepthBounds(aConfig.mDepthBoundsConfig.max_bounds())
			.setStencilTestEnable(VK_FALSE);

		// TODO: Add better support for stencil testing (better abstraction!)
		if (aConfig.mStencilTest.has_value() && aConfig.mStencilTest.value().mEnabled) {
			result.mDepthStencilConfig
				.setStencilTestEnable(VK_TRUE)
				.setFront(aConfig.mStencilTest.value().mFrontStencilTestActions)
				.setBack(aConfig.mStencilTest.value().mBackStencilTestActions);
		}

		// 9. Color Blending
		{
			// Do we have an "universal" color blending config? That means, one that is not assigned to a specific color target attachment id.
			auto universalConfig = from(aConfig.mColorBlendingPerAttachment)
				>> where([](const color_blending_config& config) { return !config.mTargetAttachment.has_value(); })
				>> to_vector();

			if (universalConfig.size() > 1) {
				throw avk::runtime_error("Ambiguous 'universal' color blending configurations. Either provide only one 'universal' "
					"config (which is not attached to a specific color target) or assign them to specific color target attachment ids.");
			}

			// Iterate over all color target attachments and set a color blending config
			if (result.subpass_id() >= result.mRenderPass->attachment_descriptions().size()) {
				throw avk::runtime_error(
					"There are fewer subpasses in the renderpass ("
					+ std::to_string(result.mRenderPass->attachment_descriptions().size()) +
					") than the subpass index ("
					+ std::to_string(result.subpass_id()) +
					") indicates. I.e. the subpass index is out of bounds.");
			}
			const auto n = result.mRenderPass->color_attachments_for_subpass(result.subpass_id()).size(); /////////////////// TODO: (doublecheck or) FIX this section (after renderpass refactoring)
			result.mBlendingConfigsForColorAttachments.reserve(n); // Important! Otherwise the vector might realloc and .data() will become invalid!
			for (size_t i = 0; i < n; ++i) {
				// Do we have a specific blending config for color attachment i?
				auto configForI = from(aConfig.mColorBlendingPerAttachment)
					>> where([i](const color_blending_config& config) { return config.mTargetAttachment.has_value() && config.mTargetAttachment.value() == i; })
					>> to_vector();
				if (configForI.size() > 1) {
					throw avk::runtime_error("Ambiguous color blending configuration for color attachment at index #" + std::to_string(i) + ". Provide only one config per color attachment!");
				}
				// Determine which color blending to use for this attachment:
				color_blending_config toUse = configForI.size() == 1
												? configForI[0]
												: universalConfig.size() == 1
													? universalConfig[0]
													: color_blending_config::disable();
				result.mBlendingConfigsForColorAttachments.push_back(vk::PipelineColorBlendAttachmentState()
					.setColorWriteMask(to_vk_color_components(toUse.affected_color_channels()))
					.setBlendEnable(to_vk_bool(toUse.is_blending_enabled())) // If blendEnable is set to VK_FALSE, then the new color from the fragment shader is passed through unmodified. [4]
					.setSrcColorBlendFactor(to_vk_blend_factor(toUse.color_source_factor()))
					.setDstColorBlendFactor(to_vk_blend_factor(toUse.color_destination_factor()))
					.setColorBlendOp(to_vk_blend_operation(toUse.color_operation()))
					.setSrcAlphaBlendFactor(to_vk_blend_factor(toUse.alpha_source_factor()))
					.setDstAlphaBlendFactor(to_vk_blend_factor(toUse.alpha_destination_factor()))
					.setAlphaBlendOp(to_vk_blend_operation(toUse.alpha_operation()))
				);
			}

			// General blending settings and reference to the array of color attachment blending configs
			result.mColorBlendStateCreateInfo = vk::PipelineColorBlendStateCreateInfo()
				.setLogicOpEnable(to_vk_bool(aConfig.mColorBlendingSettings.is_logic_operation_enabled())) // If you want to use the second method of blending (bitwise combination), then you should set logicOpEnable to VK_TRUE. The bitwise operation can then be specified in the logicOp field. [4]
				.setLogicOp(to_vk_logic_operation(aConfig.mColorBlendingSettings.logic_operation()))
				.setAttachmentCount(static_cast<uint32_t>(result.mBlendingConfigsForColorAttachments.size()))
				.setPAttachments(result.mBlendingConfigsForColorAttachments.data())
				.setBlendConstants(aConfig.mColorBlendingSettings.blend_constants());
		}

		// 10. Multisample state
		// TODO: Can the settings be inferred from the renderpass' color attachments (as they are right now)? If they can't, how to handle this situation?
		{ /////////////////// TODO: FIX this section (after renderpass refactoring)
			vk::SampleCountFlagBits numSamples = vk::SampleCountFlagBits::e1;

			// See what is configured in the render pass
			auto colorAttConfigs = from ((*result.mRenderPass).color_attachments_for_subpass(result.subpass_id()))
				>> where ([](const vk::AttachmentReference& colorAttachment) { return colorAttachment.attachment != VK_ATTACHMENT_UNUSED; })
				// The color_attachments() contain indices of the actual attachment_descriptions() => select the latter!
				>> select ([&rp = (*result.mRenderPass)](const vk::AttachmentReference& colorAttachment) { return rp.attachment_descriptions()[colorAttachment.attachment]; })
				>> to_vector();

			for (const vk::AttachmentDescription& config: colorAttConfigs) {
				typedef std::underlying_type<vk::SampleCountFlagBits>::type EnumType;
				numSamples = static_cast<vk::SampleCountFlagBits>(std::max(static_cast<EnumType>(config.samples), static_cast<EnumType>(numSamples)));
			}

#if defined(_DEBUG)
			for (const vk::AttachmentDescription& config: colorAttConfigs) {
				if (config.samples != numSamples) {
					AVK_LOG_DEBUG("Not all of the color target attachments have the same number of samples configured, fyi. This might be fine, though.");
				}
			}
#endif

			if (vk::SampleCountFlagBits::e1 == numSamples) {
				auto depthAttConfigs = from ((*result.mRenderPass).depth_stencil_attachments_for_subpass(result.subpass_id()))
					>> where ([](const vk::AttachmentReference& depthStencilAttachment) { return depthStencilAttachment.attachment != VK_ATTACHMENT_UNUSED; })
					>> select ([&rp = (*result.mRenderPass)](const vk::AttachmentReference& depthStencilAttachment) { return rp.attachment_descriptions()[depthStencilAttachment.attachment]; })
					>> to_vector();

				for (const vk::AttachmentDescription& config: depthAttConfigs) {
					typedef std::underlying_type<vk::SampleCountFlagBits>::type EnumType;
					numSamples = static_cast<vk::SampleCountFlagBits>(std::max(static_cast<EnumType>(config.samples), static_cast<EnumType>(numSamples)));
				}

#if defined(_DEBUG)
					for (const vk::AttachmentDescription& config: depthAttConfigs) {
						if (config.samples != numSamples) {
							AVK_LOG_DEBUG("Not all of the depth/stencil target attachments have the same number of samples configured, fyi. This might be fine, though.");
						}
					}
#endif

#if defined(_DEBUG)
					for (const vk::AttachmentDescription& config: colorAttConfigs) {
						if (config.samples != numSamples) {
							AVK_LOG_DEBUG("Some of the color target attachments have different numbers of samples configured as the depth/stencil attachments, fyi. This might be fine, though.");
						}
					}
#endif
			}

			// Evaluate and set the PER SAMPLE shading configuration:
			auto perSample = aConfig.mPerSampleShading.value_or(per_sample_shading_config{ false, 1.0f });

			result.mMultisampleStateCreateInfo = vk::PipelineMultisampleStateCreateInfo()
				.setRasterizationSamples(numSamples)
				.setSampleShadingEnable(perSample.mPerSampleShadingEnabled ? VK_TRUE : VK_FALSE) // enable/disable Sample Shading
				.setMinSampleShading(perSample.mMinFractionOfSamplesShaded) // specifies a minimum fraction of sample shading if sampleShadingEnable is set to VK_TRUE.
				.setPSampleMask(nullptr) // If pSampleMask is NULL, it is treated as if the mask has all bits enabled, i.e. no coverage is removed from fragments. See https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#fragops-samplemask
				.setAlphaToCoverageEnable(VK_FALSE) // controls whether a temporary coverage value is generated based on the alpha component of the fragment's first color output as specified in the Multisample Coverage section.
				.setAlphaToOneEnable(VK_FALSE); // controls whether the alpha component of the fragment's first color output is replaced with one as described in Multisample Coverage.
			// TODO: That is probably not enough for every case. Further customization options should be added!
		}

		// 11. Dynamic state
		{
			// Don't need to pre-alloc the storage for this one

			// Check for viewport dynamic state
			for (const auto& vpdc : aConfig.mViewportDepthConfig) {
				if (vpdc.is_dynamic_viewport_enabled())	{
					result.mDynamicStateEntries.push_back(vk::DynamicState::eViewport);
				}
			}
			// Check for scissor dynamic state
			for (const auto& vpdc : aConfig.mViewportDepthConfig) {
				if (vpdc.is_dynamic_scissor_enabled())	{
					result.mDynamicStateEntries.push_back(vk::DynamicState::eScissor);
				}
			}
			// Check for dynamic line width
			if (aConfig.mPolygonDrawingModeAndConfig.dynamic_line_width()) {
				result.mDynamicStateEntries.push_back(vk::DynamicState::eLineWidth);
			}
			// Check for dynamic depth bias
			if (aConfig.mDepthClampBiasConfig.is_dynamic_depth_bias_enabled()) {
				result.mDynamicStateEntries.push_back(vk::DynamicState::eDepthBias);
			}
			// Check for dynamic depth bounds
			if (aConfig.mDepthBoundsConfig.is_dynamic_depth_bounds_enabled()) {
				result.mDynamicStateEntries.push_back(vk::DynamicState::eDepthBounds);
			}
			// Check for dynamic stencil values // TODO: make them configurable separately
			if (aConfig.mStencilTest.has_value() && aConfig.mStencilTest.value().is_dynamic_enabled()) {
				result.mDynamicStateEntries.push_back(vk::DynamicState::eStencilCompareMask);
				result.mDynamicStateEntries.push_back(vk::DynamicState::eStencilReference);
				result.mDynamicStateEntries.push_back(vk::DynamicState::eStencilWriteMask);
			}
			// TODO: Support further dynamic states

			result.mDynamicStateCreateInfo = vk::PipelineDynamicStateCreateInfo{}
				.setDynamicStateCount(static_cast<uint32_t>(result.mDynamicStateEntries.size()))
				.setPDynamicStates(result.mDynamicStateEntries.data());
		}

		// 12. Flags
		// TODO: Support all flags (only one of the flags is handled at the moment)
		result.mPipelineCreateFlags = {};
		if ((aConfig.mPipelineSettings & pipeline_settings::disable_optimization) == pipeline_settings::disable_optimization) {
			result.mPipelineCreateFlags |= vk::PipelineCreateFlagBits::eDisableOptimization;
		}

		// 13. Patch Control Points for Tessellation
		if (aConfig.mTessellationPatchControlPoints.has_value()) {
			result.mPipelineTessellationStateCreateInfo = vk::PipelineTessellationStateCreateInfo{}
				.setPatchControlPoints(aConfig.mTessellationPatchControlPoints.value().mPatchControlPoints);
		}

		// 14. Compile the PIPELINE LAYOUT data and create-info
		// Get the descriptor set layouts
		result.mAllDescriptorSetLayouts = set_of_descriptor_set_layouts::prepare(std::move(aConfig.mResourceBindings));
		allocate_set_of_descriptor_set_layouts(result.mAllDescriptorSetLayouts);

		// Gather the push constant data
		result.mPushConstantRanges.reserve(aConfig.mPushConstantsBindings.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!
		for (const auto& pcBinding : aConfig.mPushConstantsBindings) {
			result.mPushConstantRanges.push_back(vk::PushConstantRange{}
				.setStageFlags(to_vk_shader_stages(pcBinding.mShaderStages))
				.setOffset(static_cast<uint32_t>(pcBinding.mOffset))
				.setSize(static_cast<uint32_t>(pcBinding.mSize))
			);
			// TODO: Push Constants need a prettier interface
		}

		auto descriptorSetLayoutHandles = result.mAllDescriptorSetLayouts.layout_handles();
		// These uniform values (Anm.: passed to shaders) need to be specified during pipeline creation by creating a VkPipelineLayout object. [4]
		result.mPipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo{}
			.setSetLayoutCount(static_cast<uint32_t>(descriptorSetLayoutHandles.size()))
			.setPSetLayouts(descriptorSetLayoutHandles.data())
			.setPushConstantRangeCount(static_cast<uint32_t>(result.mPushConstantRanges.size()))
			.setPPushConstantRanges(result.mPushConstantRanges.data());

		// 15. Maybe alter the config?!
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(result);
		}

		assert (aConfig.mRenderPassSubpass.has_value());
		rewire_config_and_create_graphics_pipeline(result);
		return result;
	}

	graphics_pipeline root::create_graphics_pipeline_from_template(resource_reference<const graphics_pipeline_t> aTemplate, std::function<void(graphics_pipeline_t&)> aAlterConfigBeforeCreation)
	{
		graphics_pipeline_t result;

		if (aTemplate->mRenderPass.is_shared_ownership_enabled()) {
			result.mRenderPass							= aTemplate->mRenderPass								;
		}
		else {
			result.mRenderPass = create_renderpass_from_template(const_referenced(aTemplate->mRenderPass), {});
		}

		result.mSubpassIndex							= aTemplate->mSubpassIndex						   ;
		result.mOrderedVertexInputBindingDescriptions	= aTemplate->mOrderedVertexInputBindingDescriptions;
		result.mVertexInputAttributeDescriptions		= aTemplate->mVertexInputAttributeDescriptions	   ;
		result.mPipelineVertexInputStateCreateInfo		= aTemplate->mPipelineVertexInputStateCreateInfo   ;
		result.mInputAssemblyStateCreateInfo			= aTemplate->mInputAssemblyStateCreateInfo		   ;

		for (const auto& shdr : aTemplate->mShaders) {
			result.mShaders.push_back(create_shader_from_template(shdr));
		}

		result.mShaderStageCreateInfos					= aTemplate->mShaderStageCreateInfos					;
		result.mSpecializationInfos						= aTemplate->mSpecializationInfos				   ;
		result.mViewports								= aTemplate->mViewports							   ;
		result.mScissors								= aTemplate->mScissors							   ;
		result.mViewportStateCreateInfo					= aTemplate->mViewportStateCreateInfo			   ;
		result.mRasterizationStateCreateInfo			= aTemplate->mRasterizationStateCreateInfo		   ;
		result.mDepthStencilConfig						= aTemplate->mDepthStencilConfig						;
		result.mBlendingConfigsForColorAttachments		= aTemplate->mBlendingConfigsForColorAttachments   ;
		result.mColorBlendStateCreateInfo				= aTemplate->mColorBlendStateCreateInfo			   ;
		result.mMultisampleStateCreateInfo				= aTemplate->mMultisampleStateCreateInfo				;
		result.mDynamicStateEntries						= aTemplate->mDynamicStateEntries				   ;
		result.mDynamicStateCreateInfo					= aTemplate->mDynamicStateCreateInfo					;

		result.mAllDescriptorSetLayouts = create_set_of_descriptor_set_layouts_from_template(aTemplate->mAllDescriptorSetLayouts);

		result.mPushConstantRanges						= aTemplate->mPushConstantRanges						;
		result.mPipelineTessellationStateCreateInfo		= aTemplate->mPipelineTessellationStateCreateInfo  ;

		auto descriptorSetLayoutHandles = result.mAllDescriptorSetLayouts.layout_handles();
		// These uniform values (Anm.: passed to shaders) need to be specified during pipeline creation by creating a VkPipelineLayout object. [4]
		result.mPipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo{}
			.setSetLayoutCount(static_cast<uint32_t>(descriptorSetLayoutHandles.size()))
			.setPSetLayouts(descriptorSetLayoutHandles.data())
			.setPushConstantRangeCount(static_cast<uint32_t>(result.mPushConstantRanges.size()))
			.setPPushConstantRanges(result.mPushConstantRanges.data());

		// 15. Maybe alter the config?!
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(result);
		}

		rewire_config_and_create_graphics_pipeline(result);
		return result;
	}

	renderpass root::replace_render_pass_for_pipeline(graphics_pipeline& aPipeline, renderpass aNewRenderPass)
	{
		if (aPipeline->mRenderPass.is_shared_ownership_enabled()) {
			aNewRenderPass.enable_shared_ownership();
		}

		auto oldRenderPass = std::move(aPipeline->mRenderPass);
		aPipeline->mRenderPass = std::move(aNewRenderPass);

		return oldRenderPass;
	}
#pragma endregion

#pragma region image definitions
	image_t::image_t(const image_t& aOther)
	{
		if (std::holds_alternative<vk::Image>(aOther.mImage)) {
			mCreateInfo = aOther.mCreateInfo;
			mImage = std::get<vk::Image>(aOther.mImage);
			mTargetLayout = aOther.mTargetLayout;
			mCurrentLayout = aOther.mCurrentLayout;
			mImageUsage = aOther.mImageUsage;
			mAspectFlags = aOther.mAspectFlags;
		}
		else {
			throw avk::runtime_error("Can not copy this image instance!");
		}
	}

	image root::create_image_from_template(resource_reference<const image_t> aTemplate, std::function<void(image_t&)> aAlterConfigBeforeCreation)
	{
		image_t result;
		result.mCreateInfo = aTemplate->mCreateInfo;
		result.mTargetLayout = aTemplate->mTargetLayout;
		result.mCurrentLayout = vk::ImageLayout::eUndefined;
		result.mImageUsage = aTemplate->mImageUsage;
		result.mAspectFlags = aTemplate->mAspectFlags;

		if (std::holds_alternative<vk::Image>(aTemplate->mImage)) {
			// If the template is a wrapped image, there's not a lot we can do.
			//result.mImage = std::get<vk::Image>(aTemplate.mImage);
			return result;
		}

		assert(!std::holds_alternative<std::monostate>(aTemplate->mImage));

		// Maybe alter the config?!
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(result);
		}

		result.mImage = AVK_MEM_IMAGE_HANDLE{ memory_allocator(), aTemplate->memory_properties(), result.mCreateInfo };

		return result;
	}

	image root::create_image(uint32_t aWidth, uint32_t aHeight, std::tuple<vk::Format, vk::SampleCountFlagBits> aFormatAndSamples, int aNumLayers, memory_usage aMemoryUsage, image_usage aImageUsage, std::function<void(image_t&)> aAlterConfigBeforeCreation)
	{
		// Determine image usage flags, image layout, and memory usage flags:
		auto [imageUsage, targetLayout, imageTiling, imageCreateFlags] = determine_usage_layout_tiling_flags_based_on_image_usage(aImageUsage);

		vk::MemoryPropertyFlags memoryPropFlags{};
		switch (aMemoryUsage) {
		case avk::memory_usage::host_visible:
			memoryPropFlags = vk::MemoryPropertyFlagBits::eHostVisible;
			break;
		case avk::memory_usage::host_coherent:
			memoryPropFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
			break;
		case avk::memory_usage::host_cached:
			memoryPropFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCached;
			break;
		case avk::memory_usage::device:
			memoryPropFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;
			imageUsage |= vk::ImageUsageFlagBits::eTransferDst;
			break;
		case avk::memory_usage::device_readback:
			memoryPropFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;
			imageUsage |= vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc;
			break;
		case avk::memory_usage::device_protected:
			memoryPropFlags = vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eProtected;
			imageUsage |= vk::ImageUsageFlagBits::eTransferDst;
			break;
		}

		// How many MIP-map levels are we going to use?
		auto mipLevels = avk::has_flag(aImageUsage, avk::image_usage::mip_mapped)
			? static_cast<uint32_t>(1 + std::floor(std::log2(std::max(aWidth, aHeight))))
			: 1u;

		const auto format = std::get<vk::Format>(aFormatAndSamples);
		const auto samples = std::get<vk::SampleCountFlagBits>(aFormatAndSamples);

		if (avk::has_flag(imageUsage, vk::ImageUsageFlagBits::eDepthStencilAttachment) && vk::ImageTiling::eOptimal == imageTiling) { // only for AMD |-(
			auto formatProps = physical_device().getFormatProperties(format);
			if (!has_flag(formatProps.optimalTilingFeatures, vk::FormatFeatureFlagBits::eDepthStencilAttachment)) {
				imageTiling = vk::ImageTiling::eLinear;
			}
		}

		vk::ImageAspectFlags aspectFlags = {};
		if (is_depth_format(format)) {
			aspectFlags |= vk::ImageAspectFlagBits::eDepth;
		}
		if (has_stencil_component(format)) {
			aspectFlags |= vk::ImageAspectFlagBits::eStencil;
		}
		if (!aspectFlags) {
			aspectFlags = vk::ImageAspectFlagBits::eColor;
			// TODO: maybe support further aspect flags?!
		}

		image_t result;
		result.mCreateInfo = vk::ImageCreateInfo()
			.setImageType(vk::ImageType::e2D) // TODO: Support 3D textures
			.setExtent(vk::Extent3D(static_cast<uint32_t>(aWidth), static_cast<uint32_t>(aHeight), 1u))
			.setMipLevels(mipLevels)
			.setArrayLayers(aNumLayers)
			.setFormat(format)
			.setTiling(imageTiling)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setUsage(imageUsage)
			.setSharingMode(vk::SharingMode::eExclusive) // TODO: Not sure yet how to handle this one, Exclusive should be the default, though.
			.setSamples(samples)
			.setFlags(imageCreateFlags);
		result.mTargetLayout = targetLayout;
		result.mCurrentLayout = vk::ImageLayout::eUndefined;
		result.mImageUsage = aImageUsage;
		result.mAspectFlags = aspectFlags;

		// Maybe alter the config?!
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(result);
		}

		result.mImage = AVK_MEM_IMAGE_HANDLE{ memory_allocator(), memoryPropFlags, result.mCreateInfo };

		return result;
	}

	image root::create_image(uint32_t aWidth, uint32_t aHeight, vk::Format aFormat, int aNumLayers, memory_usage aMemoryUsage, avk::image_usage aImageUsage, std::function<void(image_t&)> aAlterConfigBeforeCreation)
	{
		return create_image(aWidth, aHeight, std::make_tuple(aFormat, vk::SampleCountFlagBits::e1), aNumLayers, aMemoryUsage, aImageUsage, std::move(aAlterConfigBeforeCreation));
	}


	image root::create_depth_image(uint32_t aWidth, uint32_t aHeight, std::optional<vk::Format> aFormat, int aNumLayers,  memory_usage aMemoryUsage, avk::image_usage aImageUsage, std::function<void(image_t&)> aAlterConfigBeforeCreation)
	{
		// Select a suitable depth format
		if (!aFormat) {
			std::array depthFormats = { vk::Format::eD32Sfloat, vk::Format::eD24UnormS8Uint, vk::Format::eD16Unorm };
			for (auto format : depthFormats) {
				if (is_format_supported(format, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment)) {
					aFormat = format;
					break;
				}
			}
		}
		if (!aFormat) {
			throw avk::runtime_error("No suitable depth format could be found.");
		}

		aImageUsage |= avk::image_usage::depth_stencil_attachment;

		// Create the image (by default only on the device which should be sufficient for a depth buffer => see pMemoryUsage's default value):
		auto result = create_image(aWidth, aHeight, *aFormat, aNumLayers, aMemoryUsage, aImageUsage, std::move(aAlterConfigBeforeCreation));
		result->mAspectFlags |= vk::ImageAspectFlagBits::eDepth;
		return result;
	}

	image root::create_depth_stencil_image(uint32_t aWidth, uint32_t aHeight, std::optional<vk::Format> aFormat, int aNumLayers,  memory_usage aMemoryUsage, avk::image_usage aImageUsage, std::function<void(image_t&)> aAlterConfigBeforeCreation)
	{
		// Select a suitable depth+stencil format
		if (!aFormat) {
			std::array depthFormats = { vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint, vk::Format::eD16UnormS8Uint };
			for (auto format : depthFormats) {
				if (is_format_supported(format, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment)) {
					aFormat = format;
					break;
				}
			}
		}
		if (!aFormat) {
			throw avk::runtime_error("No suitable depth+stencil format could be found.");
		}

		// Create the image (by default only on the device which should be sufficient for a depth+stencil buffer => see pMemoryUsage's default value):
		auto result = create_depth_image(aWidth, aHeight, *aFormat, aNumLayers, aMemoryUsage, aImageUsage, std::move(aAlterConfigBeforeCreation));
		result->mAspectFlags |= vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
		return result;
	}

	image_t root::wrap_image(vk::Image aImageToWrap, vk::ImageCreateInfo aImageCreateInfo, avk::image_usage aImageUsage, vk::ImageAspectFlags aImageAspectFlags)
	{
		auto [imageUsage, targetLayout, imageTiling, imageCreateFlags] = determine_usage_layout_tiling_flags_based_on_image_usage(aImageUsage);

		image_t result;
		result.mCreateInfo = aImageCreateInfo;
		result.mImage = aImageToWrap;
		result.mTargetLayout = targetLayout;
		result.mCurrentLayout = vk::ImageLayout::eUndefined;
		result.mImageUsage = aImageUsage;
		result.mAspectFlags = aImageAspectFlags;
		return result;
	}

	vk::ImageSubresourceRange image_t::entire_subresource_range() const
	{
		return vk::ImageSubresourceRange{
			mAspectFlags,
			0u, mCreateInfo.mipLevels,	// MIP info
			0u, mCreateInfo.arrayLayers	// Layers info
		};
	}

	std::optional<command_buffer> image_t::transition_to_layout(std::optional<vk::ImageLayout> aTargetLayout, sync aSyncHandler)
	{
		const auto curLayout = current_layout();
		const auto trgLayout = aTargetLayout.value_or(target_layout());
		mTargetLayout = trgLayout;

		if (curLayout == trgLayout) {
			return {}; // done (:
		}
		if (vk::ImageLayout::eUndefined == trgLayout || vk::ImageLayout::ePreinitialized == trgLayout) {
			AVK_LOG_VERBOSE("Won't transition into layout " + vk::to_string(trgLayout));
			return {}; // Won't do it!
		}

		// Not done => perform a transition via an image memory barrier inside a command buffer
		auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
		aSyncHandler.establish_barrier_before_the_operation(
			pipeline_stage::transfer,	// Just use the transfer stage to create an execution dependency chain
			read_memory_access{memory_access::transfer_read_access}
		);

		// An image's layout is tranformed by the means of an image memory barrier:
		commandBuffer.establish_image_memory_barrier(*this,
			pipeline_stage::transfer, pipeline_stage::transfer,				// Execution dependency chain
			std::optional<memory_access>{}, std::optional<memory_access>{}	// There should be no need to make any memory available or visible... the image should be available already (see above)
		); // establish_image_memory_barrier ^ will set the mCurrentLayout to mTargetLayout

		aSyncHandler.establish_barrier_after_the_operation(
			pipeline_stage::transfer,	// The end of the execution dependency chain
			write_memory_access{memory_access::transfer_write_access}
		);
		return aSyncHandler.submit_and_sync();
	}


	std::optional<command_buffer> image_t::generate_mip_maps(sync aSyncHandler)
	{
		if (config().mipLevels <= 1u) {
			return {};
		}

		auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
		aSyncHandler.establish_barrier_before_the_operation(pipeline_stage::transfer, read_memory_access{ memory_access::transfer_read_access }); // Make memory visible

		const auto originalLayout = current_layout();
		const auto targetLayout = target_layout();

		for (uint32_t l = 0u; l < config().arrayLayers; ++l) {

			auto w = static_cast<int32_t>(width());
			auto h = static_cast<int32_t>(height());

			std::array layoutTransitions = { // during the loop, we'll use 1 or 2 of these
				vk::ImageMemoryBarrier{
					{}, {}, // Memory is available AND already visible for transfer read because that has been established in establish_barrier_before_the_operation above.
					originalLayout, vk::ImageLayout::eTransferSrcOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, handle(), vk::ImageSubresourceRange{ mAspectFlags, 0u, 1u, l, 1u }},
				vk::ImageMemoryBarrier{
					{}, vk::AccessFlagBits::eTransferWrite, // This is the first mip-level we're going to write to
					originalLayout, vk::ImageLayout::eTransferDstOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, handle(), vk::ImageSubresourceRange{ mAspectFlags, 1u, 1u, l, 1u }},
				vk::ImageMemoryBarrier{} // To be used in loop
			};

			commandBuffer.handle().pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eTransfer,
				vk::DependencyFlags{},
				0u, nullptr,
				0u, nullptr,
				2u /* initially, only 2 required */, layoutTransitions.data()
			);

			for (uint32_t i = 1u; i < config().mipLevels; ++i) {

				commandBuffer.handle().blitImage(
					handle(), vk::ImageLayout::eTransferSrcOptimal,
					handle(), vk::ImageLayout::eTransferDstOptimal,
					{ vk::ImageBlit{
						vk::ImageSubresourceLayers{ mAspectFlags, i - 1, l, 1u }, { vk::Offset3D{ 0, 0, 0 }, vk::Offset3D{ w      , h      , 1 } },
						vk::ImageSubresourceLayers{ mAspectFlags, i  , l, 1u }, { vk::Offset3D{ 0, 0, 0 }, vk::Offset3D{ w > 1 ? w / 2 : 1, h > 1 ? h / 2 : 1, 1 } }
					  }
					},
					vk::Filter::eLinear
				);

				// mip-level  i-1  is done:
				layoutTransitions[0] = vk::ImageMemoryBarrier{
					{}, {}, // Blit Read -> Done
					vk::ImageLayout::eTransferSrcOptimal, targetLayout, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, handle(), vk::ImageSubresourceRange{ mAspectFlags, i - 1, 1u, l, 1u } };
				// mip-level   i   has been transfer destination, but is going to be transfer source:
				layoutTransitions[1] = vk::ImageMemoryBarrier{
					vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eTransferRead, // Blit Write -> Blit Read
					vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, handle(), vk::ImageSubresourceRange{ mAspectFlags, i, 1u, l, 1u } };
				// mip-level  i+1  is entering the game:
				layoutTransitions[2] = vk::ImageMemoryBarrier{
					{}, vk::AccessFlagBits::eTransferWrite, // make visible to Blit Write
					originalLayout, vk::ImageLayout::eTransferDstOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, handle(), vk::ImageSubresourceRange{ mAspectFlags, i + 1, 1u, l, 1u } };

				uint32_t numBarriersRequired = std::min(3u, config().mipLevels - i + 1);
				if (config().mipLevels - 1 == i) {
					layoutTransitions[1].newLayout = targetLayout; // Last one => done
				}

				commandBuffer.handle().pipelineBarrier(
					vk::PipelineStageFlagBits::eTransfer,
					vk::PipelineStageFlagBits::eTransfer, // Dependency from previous BLIT to subsequent BLIT
					vk::DependencyFlags{},
					0u, nullptr,
					0u, nullptr,
					numBarriersRequired, layoutTransitions.data()
				);

				w = w > 1 ? w / 2 : 1;
				h = h > 1 ? h / 2 : 1;
			}
		}

		aSyncHandler.establish_barrier_after_the_operation(pipeline_stage::transfer, write_memory_access{ memory_access::transfer_write_access });
		return aSyncHandler.submit_and_sync();
	}
#pragma endregion

#pragma region image view definitions
	image_view root::create_image_view_from_template(resource_reference<const image_view_t> aTemplate, std::function<void(image_t&)> aAlterImageConfigBeforeCreation, std::function<void(image_view_t&)> aAlterImageViewConfigBeforeCreation)
	{
		image_view_t result;

		// Transfer ownership:
		if (std::holds_alternative<image>(aTemplate->mImage)) {
			auto& im = std::get<image>(aTemplate->mImage);
			result.mImage = create_image_from_template(const_referenced(im), std::move(aAlterImageConfigBeforeCreation));
			if (im.is_shared_ownership_enabled()) {
				std::get<image>(result.mImage).enable_shared_ownership();
			}
		}
		else {
			AVK_LOG_ERROR("Can not create an image_view from a template which wraps an image_t.");
		}

		result.mInfo = aTemplate->mInfo;
		result.mInfo.setImage(result.get_image().handle());
		result.mInfo.subresourceRange
			.setLevelCount(result.get_image().config().mipLevels)
			.setLayerCount(result.get_image().config().arrayLayers);

		result.mUsageInfo = aTemplate->mUsageInfo;

		// Maybe alter the config?!
		if (aAlterImageViewConfigBeforeCreation) {
			aAlterImageViewConfigBeforeCreation(result);
		}

		result.mImageView = device().createImageViewUnique(result.mInfo);
		result.mDescriptorInfo = vk::DescriptorImageInfo{}
			.setImageView(result.handle())
			.setImageLayout(result.get_image().target_layout()); // TODO: Better use the image's current layout or its target layout?

		return result;
	}


	image_view root::create_image_view(resource_ownership<image_t> aImageToOwn, std::optional<vk::Format> aViewFormat, std::optional<avk::image_usage> aImageViewUsage, std::function<void(image_view_t&)> aAlterConfigBeforeCreation)
	{
		image_view_t result;

		// Transfer ownership:
		result.mImage = aImageToOwn.own();

		// What's the format of the image view?
		if (!aViewFormat.has_value()) {
			aViewFormat = result.get_image().format();
		}

		finish_configuration(result, aViewFormat.value(), {}, aImageViewUsage, std::move(aAlterConfigBeforeCreation));

		return result;
	}

	image_view root::create_depth_image_view(resource_ownership<image_t> aImageToOwn, std::optional<vk::Format> aViewFormat, std::optional<avk::image_usage> aImageViewUsage, std::function<void(image_view_t&)> aAlterConfigBeforeCreation)
	{
		image_view_t result;

		// Transfer ownership:
		result.mImage = aImageToOwn.own();

		// What's the format of the image view?
		if (!aViewFormat.has_value()) {
			aViewFormat = result.get_image().format();
		}

		finish_configuration(result, aViewFormat.value(), vk::ImageAspectFlagBits::eDepth, aImageViewUsage, std::move(aAlterConfigBeforeCreation));

		return result;
	}

	image_view root::create_stencil_image_view(resource_ownership<image_t> aImageToOwn, std::optional<vk::Format> aViewFormat, std::optional<avk::image_usage> aImageViewUsage, std::function<void(image_view_t&)> aAlterConfigBeforeCreation)
	{
		image_view_t result;

		// Transfer ownership:
		result.mImage = aImageToOwn.own();

		// What's the format of the image view?
		if (!aViewFormat.has_value()) {
			aViewFormat = result.get_image().format();
		}

		finish_configuration(result, aViewFormat.value(), vk::ImageAspectFlagBits::eStencil, aImageViewUsage, std::move(aAlterConfigBeforeCreation));

		return result;
	}

	image_view root::create_image_view(image_t aImageToWrap, std::optional<vk::Format> aViewFormat, std::optional<avk::image_usage> aImageViewUsage)
	{
		image_view_t result;

		// Transfer ownership:
		result.mImage = image_view_t::helper_t( std::move(aImageToWrap) );

		// What's the format of the image view?
		if (!aViewFormat.has_value()) {
			aViewFormat = result.get_image().format();
		}

		finish_configuration(result, aViewFormat.value(), {}, aImageViewUsage, nullptr);

		return result;
	}

	void root::finish_configuration(image_view_t& aImageView, vk::Format aViewFormat, std::optional<vk::ImageAspectFlags> aImageAspectFlags, std::optional<avk::image_usage> aImageViewUsage, std::function<void(image_view_t&)> aAlterConfigBeforeCreation)
	{
		if (!aImageAspectFlags.has_value()) {
			const auto imageFormat = aImageView.get_image().config().format;
			aImageAspectFlags = aImageView.get_image().aspect_flags();

			if (is_depth_format(imageFormat)) {
				if (has_stencil_component(imageFormat)) {
					AVK_LOG_ERROR("Can infer whether the image view shall refer to the depth component or to the stencil component => State it explicitly by using image_view_t::create_depth or image_view_t::create_stencil");
				}
				aImageAspectFlags = vk::ImageAspectFlagBits::eDepth;
				// TODO: use vk::ImageAspectFlagBits' underlying type and exclude eStencil rather than only setting eDepth!
			}
			else if(has_stencil_component(imageFormat)) {
				aImageAspectFlags = vk::ImageAspectFlagBits::eStencil;
				// TODO: use vk::ImageAspectFlagBits' underlying type and exclude eDepth rather than only setting eStencil!
			}
		}

		// Proceed with config creation (and use the imageAspectFlags there):
		aImageView.mInfo = vk::ImageViewCreateInfo{}
			.setImage(aImageView.get_image().handle())
			.setViewType(to_image_view_type(aImageView.get_image().config()))
			.setFormat(aViewFormat)
			.setComponents(vk::ComponentMapping() // The components field allows you to swizzle the color channels around. In our case we'll stick to the default mapping. [3]
							  .setR(vk::ComponentSwizzle::eIdentity)
							  .setG(vk::ComponentSwizzle::eIdentity)
							  .setB(vk::ComponentSwizzle::eIdentity)
							  .setA(vk::ComponentSwizzle::eIdentity))
			.setSubresourceRange(vk::ImageSubresourceRange() // The subresourceRange field describes what the image's purpose is and which part of the image should be accessed. Our images will be used as color targets without any mipmapping levels or multiple layers. [3]
				.setAspectMask(aImageAspectFlags.value())
				.setBaseMipLevel(0u)
				.setLevelCount(aImageView.get_image().config().mipLevels)
				.setBaseArrayLayer(0u)
				.setLayerCount(aImageView.get_image().config().arrayLayers));

		if (aImageViewUsage.has_value()) {
			auto [imageUsage, imageLayout, imageTiling, imageCreateFlags] = determine_usage_layout_tiling_flags_based_on_image_usage(aImageViewUsage.value());
			aImageView.mUsageInfo = vk::ImageViewUsageCreateInfo()
				.setUsage(imageUsage);
			aImageView.mInfo.setPNext(&aImageView.mUsageInfo);
		}

		// Maybe alter the config?!
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(aImageView);
		}

		aImageView.mImageView = device().createImageViewUnique(aImageView.mInfo);
		aImageView.mDescriptorInfo = vk::DescriptorImageInfo{}
			.setImageView(aImageView.handle())
			.setImageLayout(aImageView.get_image().target_layout()); // TODO: Better use the image's current layout or its target layout?
	}
#pragma endregion

#pragma region sampler and image sampler definitions
	sampler root::create_sampler(filter_mode aFilterMode, std::array<border_handling_mode, 3> aBorderHandlingModes, float aMipMapMaxLod, std::function<void(sampler_t&)> aAlterConfigBeforeCreation)
	{
		vk::Filter magFilter;
		vk::Filter minFilter;
		vk::SamplerMipmapMode mipmapMode;
		vk::Bool32 enableAnisotropy = VK_FALSE;
		float maxAnisotropy = 1.0f;
		switch (aFilterMode)
		{
		case filter_mode::nearest_neighbor:
			magFilter = vk::Filter::eNearest;
			minFilter = vk::Filter::eNearest;
			mipmapMode = vk::SamplerMipmapMode::eNearest;
			break;
		case filter_mode::bilinear:
			magFilter = vk::Filter::eLinear;
			minFilter = vk::Filter::eLinear;
			mipmapMode = vk::SamplerMipmapMode::eNearest;
			break;
		case filter_mode::trilinear:
			magFilter = vk::Filter::eLinear;
			minFilter = vk::Filter::eLinear;
			mipmapMode = vk::SamplerMipmapMode::eLinear;
			break;
		case filter_mode::cubic: // I have no idea what I'm doing.
			magFilter = vk::Filter::eCubicIMG;
			minFilter = vk::Filter::eCubicIMG;
			mipmapMode = vk::SamplerMipmapMode::eLinear;
			break;
		case filter_mode::anisotropic_2x:
			magFilter = vk::Filter::eLinear;
			minFilter = vk::Filter::eLinear;
			mipmapMode = vk::SamplerMipmapMode::eLinear;
			enableAnisotropy = VK_TRUE;
			maxAnisotropy = 2.0f;
			break;
		case filter_mode::anisotropic_4x:
			magFilter = vk::Filter::eLinear;
			minFilter = vk::Filter::eLinear;
			mipmapMode = vk::SamplerMipmapMode::eLinear;
			enableAnisotropy = VK_TRUE;
			maxAnisotropy = 4.0f;
			break;
		case filter_mode::anisotropic_8x:
			magFilter = vk::Filter::eLinear;
			minFilter = vk::Filter::eLinear;
			mipmapMode = vk::SamplerMipmapMode::eLinear;
			enableAnisotropy = VK_TRUE;
			maxAnisotropy = 8.0f;
			break;
		case filter_mode::anisotropic_16x:
			magFilter = vk::Filter::eLinear;
			minFilter = vk::Filter::eLinear;
			mipmapMode = vk::SamplerMipmapMode::eLinear;
			enableAnisotropy = VK_TRUE;
			maxAnisotropy = 16.0f;
			break;
		case filter_mode::anisotropic_32x:
			magFilter = vk::Filter::eLinear;
			minFilter = vk::Filter::eLinear;
			mipmapMode = vk::SamplerMipmapMode::eLinear;
			enableAnisotropy = VK_TRUE;
			maxAnisotropy = 32.0f;
			break;
		case filter_mode::anisotropic_64x:
			magFilter = vk::Filter::eLinear;
			minFilter = vk::Filter::eLinear;
			mipmapMode = vk::SamplerMipmapMode::eLinear;
			enableAnisotropy = VK_TRUE;
			maxAnisotropy = 64.0f;
			break;
		default:
			throw avk::runtime_error("invalid filter_mode");
		}

		// Determine how to handle the borders:
		std::array<vk::SamplerAddressMode, 3> addressModes{};
		for (int i = 0; i < 3; ++i) {
			switch (aBorderHandlingModes[i])
			{
			case border_handling_mode::clamp_to_edge:
				addressModes[i] = vk::SamplerAddressMode::eClampToEdge;
				break;
			case border_handling_mode::mirror_clamp_to_edge:
				addressModes[i] = vk::SamplerAddressMode::eMirrorClampToEdge;
				break;
			case border_handling_mode::clamp_to_border:
				addressModes[i] = vk::SamplerAddressMode::eClampToBorder;
				break;
			case border_handling_mode::repeat:
				addressModes[i] = vk::SamplerAddressMode::eRepeat;
				break;
			case border_handling_mode::mirrored_repeat:
				addressModes[i] = vk::SamplerAddressMode::eMirroredRepeat;
				break;
			default:
				throw avk::runtime_error("invalid border_handling_mode at index " + std::to_string(i) + (0 == i ? " (address mode u)" : 1 == i ? " (address mode v)" : " (address mode w)"));
			}
		}

		// Compile the config for this sampler:
		sampler_t result;
		result.mInfo = vk::SamplerCreateInfo()
			.setMagFilter(magFilter)
			.setMinFilter(minFilter)
			.setAddressModeU(addressModes[0])
			.setAddressModeV(addressModes[1])
			.setAddressModeW(addressModes[2])
			.setAnisotropyEnable(enableAnisotropy)
			.setMaxAnisotropy(maxAnisotropy)
			.setBorderColor(vk::BorderColor::eFloatOpaqueBlack)
			// The unnormalizedCoordinates field specifies which coordinate system you want to use to address texels in an image.
			// If this field is VK_TRUE, then you can simply use coordinates within the [0, texWidth) and [0, texHeight) range.
			// If it is VK_FALSE, then the texels are addressed using the [0, 1) range on all axes. Real-world applications almost
			// always use normalized coordinates, because then it's possible to use textures of varying resolutions with the exact
			// same coordinates. [4]
			.setUnnormalizedCoordinates(VK_FALSE)
			// If a comparison function is enabled, then texels will first be compared to a value, and the result of that comparison
			// is used in filtering operations. This is mainly used for percentage-closer filtering on shadow maps. [4]
			.setCompareEnable(VK_FALSE)
			.setCompareOp(vk::CompareOp::eAlways)
			.setMipmapMode(mipmapMode)
			.setMipLodBias(0.0f)
			.setMinLod(0.0f)
			.setMaxLod(aMipMapMaxLod);

		// Call custom config function
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(result);
		}

		result.mSampler = device().createSamplerUnique(result.config());
		result.mDescriptorInfo = vk::DescriptorImageInfo{}
			.setSampler(result.handle());
		result.mDescriptorType = vk::DescriptorType::eSampler;
		return result;
	}

	image_sampler root::create_image_sampler(resource_ownership<image_view_t> aImageView, resource_ownership<sampler_t> aSampler)
	{
		image_sampler_t result;
		result.mImageView = aImageView.own();
		result.mSampler = aSampler.own();

		result.mDescriptorInfo = vk::DescriptorImageInfo{}
			.setImageView(result.view_handle())
			.setSampler(result.sampler_handle());
		result.mDescriptorInfo.setImageLayout(result.mImageView->get_image().target_layout());

		result.mDescriptorType = vk::DescriptorType::eCombinedImageSampler;
		return result;
	}
#pragma endregion

#pragma region input description definitions
	input_description input_description::declare(std::initializer_list<input_binding_to_location_mapping> aBindings)
	{
		input_description result;

		for (const auto& bindingLoc : aBindings) {
			auto& bfr = result.mInputBuffers[bindingLoc.mGeneralData.mBinding];
			// Create if it doesn't exist
			if (std::holds_alternative<std::monostate>(bfr)) {
				switch (bindingLoc.mGeneralData.mKind)
				{
				case vertex_input_buffer_binding::kind::vertex:
					bfr = vertex_buffer_meta::create_from_element_size(bindingLoc.mGeneralData.mStride);
					break;
				case vertex_input_buffer_binding::kind::instance:
					bfr = instance_buffer_meta::create_from_element_size(bindingLoc.mGeneralData.mStride);
					break;
				default:
					throw avk::runtime_error("Invalid input_binding_location_data::kind value");
				}
			}

#if defined(_DEBUG)
			// It exists => perform some sanity checks:
			if (std::holds_alternative<std::monostate>(bfr)
				|| (vertex_input_buffer_binding::kind::vertex == bindingLoc.mGeneralData.mKind && std::holds_alternative<instance_buffer_meta>(bfr))
				|| (vertex_input_buffer_binding::kind::instance == bindingLoc.mGeneralData.mKind && std::holds_alternative<vertex_buffer_meta>(bfr))
				) {
				throw avk::logic_error("All locations of the same binding must come from the same buffer type (vertex buffer or instance buffer).");
			}
#endif

			if (std::holds_alternative<vertex_buffer_meta>(bfr)) {
				std::get<vertex_buffer_meta>(bfr).describe_member(
					bindingLoc.mMemberMetaData.mOffset,
					bindingLoc.mMemberMetaData.mFormat);
			}
			else { // must be instance_buffer_meta
				std::get<instance_buffer_meta>(bfr).describe_member(
					bindingLoc.mMemberMetaData.mOffset,
					bindingLoc.mMemberMetaData.mFormat);
			}
		}

		return result;
	}
#pragma endregion

#pragma region memory access definitions
	read_memory_access::operator memory_access() const
	{
		validate_or_throw();
		return mMemoryAccess;
	}

	memory_access read_memory_access::value() const
	{
		return operator memory_access();
	}

	void read_memory_access::validate_or_throw() const
	{
		if (!is_read_access(mMemoryAccess)) {
			throw avk::runtime_error("The access flag represented by this instance of read_memory_access is not a read-type access flag.");
		}
	}

	write_memory_access::operator memory_access() const
	{
		validate_or_throw();
		return mMemoryAccess;
	}

	memory_access write_memory_access::value() const
	{
		return operator memory_access();
	}

	void write_memory_access::validate_or_throw() const
	{
		if (is_read_access(mMemoryAccess)) {
			throw avk::runtime_error("The access flag represented by this instance of write_memory_access is not a write-type access flag.");
		}
	}
#pragma endregion

#pragma region queue definitions
	std::vector<std::tuple<uint32_t, vk::QueueFamilyProperties>> queue::find_queue_families_for_criteria(
		vk::PhysicalDevice aPhysicalDevice,
		vk::QueueFlags aRequiredFlags,
		vk::QueueFlags aForbiddenFlags,
		std::optional<vk::SurfaceKHR> aSurface)
	{
		assert(aPhysicalDevice);
		// All queue families:
		auto queueFamilies = aPhysicalDevice.getQueueFamilyProperties();
		std::vector<std::tuple<uint32_t, vk::QueueFamilyProperties>> indexedQueueFamilies;
		std::transform(std::begin(queueFamilies), std::end(queueFamilies),
					   std::back_inserter(indexedQueueFamilies),
					   [index = uint32_t(0)](const decltype(queueFamilies)::value_type& input) mutable {
						   auto tpl = std::make_tuple(index, input);
						   index += 1;
						   return tpl;
					   });
		// Subset to which the criteria applies:
		std::vector<std::tuple<uint32_t, vk::QueueFamilyProperties>> selection;
		// Select the subset
		std::copy_if(std::begin(indexedQueueFamilies), std::end(indexedQueueFamilies),
					 std::back_inserter(selection),
					 [aPhysicalDevice, aRequiredFlags, aForbiddenFlags, aSurface](const std::tuple<uint32_t, decltype(queueFamilies)::value_type>& tpl) {
						 bool requirementsMet = true;
						 if (aRequiredFlags) {
							 requirementsMet = requirementsMet && ((std::get<1>(tpl).queueFlags & aRequiredFlags) == aRequiredFlags);
						 }
						 if (aForbiddenFlags) {
							 requirementsMet = requirementsMet && ((std::get<1>(tpl).queueFlags & aForbiddenFlags) != aForbiddenFlags);
						 }
						 if (aSurface) {
							 requirementsMet = requirementsMet && (aPhysicalDevice.getSurfaceSupportKHR(std::get<0>(tpl), *aSurface));
						 }
						 return requirementsMet;
					 });
		return selection;
	}

	std::vector<std::tuple<uint32_t, vk::QueueFamilyProperties>> queue::find_best_queue_family_for(
		vk::PhysicalDevice aPhysicalDevice,
		vk::QueueFlags aRequiredFlags,
		queue_selection_preference aQueueSelectionPreference,
		std::optional<vk::SurfaceKHR> aSurface
	)
	{
		static std::array queueTypes = {
			vk::QueueFlagBits::eGraphics,
			vk::QueueFlagBits::eCompute,
			vk::QueueFlagBits::eTransfer,
		};
		// sparse binding and protected bits are ignored, for now

		std::vector<std::tuple<uint32_t, vk::QueueFamilyProperties>> selection;

		switch (aQueueSelectionPreference) {
		case queue_selection_preference::specialized_queue:
			{
				vk::QueueFlags forbiddenFlags = {};
				for (auto f : queueTypes) { // TODO: Maybe a differend loosening order would be better?
					forbiddenFlags |= f;
				}
				forbiddenFlags &= ~aRequiredFlags;

				int32_t loosenIndex = 0;

				while (loosenIndex <= queueTypes.size()) { // might result in returning an empty selection
					selection = queue::find_queue_families_for_criteria(aPhysicalDevice, aRequiredFlags, forbiddenFlags, aSurface);
					if (selection.size() > 0 || loosenIndex == queueTypes.size()) {
						break;
					}
					forbiddenFlags = forbiddenFlags & ~queueTypes[loosenIndex++]; // gradually loosen restrictions
				}
			}
			break;
		case queue_selection_preference::versatile_queue:
			{
				vk::QueueFlags additionalVersatileFlags = {};
				for (auto f : queueTypes) { // TODO: Maybe a different addition order would be better?
					additionalVersatileFlags |= f;
				}

				int32_t loosenIndex = 0;

				while (loosenIndex <= queueTypes.size()) {
					selection = queue::find_queue_families_for_criteria(aPhysicalDevice, aRequiredFlags | additionalVersatileFlags, vk::QueueFlags{}, aSurface);
					if (selection.size() > 0 || loosenIndex == queueTypes.size()) {
						break;
					}
					additionalVersatileFlags = additionalVersatileFlags & ~queueTypes[loosenIndex++]; // gradually loosen versatile-additions
				}
			}
			break;
		}

		return selection;
	}

	uint32_t queue::select_queue_family_index(
		vk::PhysicalDevice aPhysicalDevice,
		vk::QueueFlags aRequiredFlags,
		queue_selection_preference aQueueSelectionPreference,
		std::optional<vk::SurfaceKHR> aSupportForSurface
	)
	{
		auto families = find_best_queue_family_for(aPhysicalDevice, aRequiredFlags, aQueueSelectionPreference, aSupportForSurface);
		if (families.size() == 0) {
			throw avk::runtime_error("Couldn't find queue families satisfying the given criteria.");
		}

		uint32_t familyIndex = std::get<0>(families[0]);
		return familyIndex;
	}

	queue queue::prepare(
		vk::PhysicalDevice aPhysicalDevice,
		uint32_t aQueueFamilyIndex,
		uint32_t aQueueIndex,
		float aQueuePriority
	)
	{
		auto queueFamilies = aPhysicalDevice.getQueueFamilyProperties();
		if (queueFamilies.size() <= aQueueFamilyIndex) {
			throw avk::runtime_error("Invalid queue family index in queue::prepare");
		}
		if (queueFamilies[aQueueFamilyIndex].queueCount <= aQueueIndex) {
			throw avk::runtime_error("Queue family #" + std::to_string(aQueueFamilyIndex) + " does not provide enough queues (requested index: " + std::to_string(aQueueIndex) + ")");
		}

		queue result;
		result.mQueueFamilyIndex = aQueueFamilyIndex;
		result.mQueueIndex = aQueueIndex;
		result.mPriority = aQueuePriority;
		result.mPhysicalDevice = aPhysicalDevice;
		result.mDevice = nullptr;
		result.mQueue = nullptr;
		return result;
	}

	void queue::assign_handle(vk::Device aDevice)
	{
		mDevice = aDevice;
		mQueue = aDevice.getQueue(mQueueFamilyIndex, mQueueIndex);
	}




	void queue::submit_with_semaphore(resource_reference<semaphore_t> aSemaphoreToSignal, resource_reference<command_buffer_t> aCommandBuffer, std::optional<resource_reference<semaphore_t>> aWaitSemaphores)
	{
		assert(aCommandBuffer->state() >= command_buffer_state::finished_recording);

		auto submitInfo = vk::SubmitInfo{}
			.setCommandBufferCount(1u)
			.setPCommandBuffers(aCommandBuffer->handle_ptr())
			.setSignalSemaphoreCount(1u)
			.setPSignalSemaphores(aSemaphoreToSignal->handle_addr());
		if (aWaitSemaphores.has_value()) {
			submitInfo
				.setWaitSemaphoreCount(1u)
				.setPWaitSemaphores(aWaitSemaphores->get().handle_addr())
				.setPWaitDstStageMask(aWaitSemaphores->get().semaphore_wait_stage_addr());
		}

		handle().submit({ submitInfo },  nullptr);
		aCommandBuffer->invoke_post_execution_handler();

		aCommandBuffer->mState = command_buffer_state::submitted;
	}

	semaphore queue::submit_with_semaphore(resource_reference<command_buffer_t> aCommandBuffer, std::optional<resource_reference<semaphore_t>> aWaitSemaphores)
	{
		assert(aCommandBuffer->state() >= command_buffer_state::finished_recording);

		auto sem = root::create_semaphore(mDevice);

		auto submitInfo = vk::SubmitInfo{}
			.setCommandBufferCount(1u)
			.setPCommandBuffers(aCommandBuffer->handle_ptr())
			.setSignalSemaphoreCount(1u)
			.setPSignalSemaphores(sem->handle_addr());
		if (aWaitSemaphores.has_value()) {
			submitInfo
				.setWaitSemaphoreCount(1u)
				.setPWaitSemaphores(aWaitSemaphores->get().handle_addr())
				.setPWaitDstStageMask(aWaitSemaphores->get().semaphore_wait_stage_addr());
		}

		handle().submit({ submitInfo },  nullptr);
		aCommandBuffer->invoke_post_execution_handler();

		aCommandBuffer->mState = command_buffer_state::submitted;

		return sem;
	}

	void queue::submit(resource_reference<command_buffer_t> aCommandBuffer, std::optional<resource_reference<semaphore_t>> aWaitSemaphore)
	{
		assert(aCommandBuffer->state() >= command_buffer_state::finished_recording);

		auto submitInfo = vk::SubmitInfo{}
			.setCommandBufferCount(1u)
			.setPCommandBuffers(aCommandBuffer->handle_ptr());
		if (aWaitSemaphore.has_value()) {
			submitInfo
				.setWaitSemaphoreCount(1u)
				.setPWaitSemaphores(aWaitSemaphore->get().handle_addr())
				.setPWaitDstStageMask(aWaitSemaphore->get().semaphore_wait_stage_addr());
		}

		handle().submit({ submitInfo },  nullptr);
		aCommandBuffer->invoke_post_execution_handler();

		aCommandBuffer->mState = command_buffer_state::submitted;
	}

	void queue::submit(resource_reference<command_buffer_t> aCommandBuffer, std::vector<resource_reference<semaphore_t>> aWaitSemaphores)
	{
		assert(aCommandBuffer->state() >= command_buffer_state::finished_recording);

		auto submitInfo = vk::SubmitInfo{}
			.setCommandBufferCount(1u)
			.setPCommandBuffers(aCommandBuffer->handle_ptr());

		std::vector<vk::Semaphore> waitSemHandles;
		std::vector<vk::PipelineStageFlags> waitSemStages;
		if (!aWaitSemaphores.empty()) {
			for (auto& ws : aWaitSemaphores) {
				waitSemHandles.push_back(ws.get().handle());
				waitSemStages.push_back(ws.get().semaphore_wait_stage());
			}
			assert (waitSemHandles.size() == waitSemStages.size());
			submitInfo
				.setWaitSemaphoreCount(static_cast<uint32_t>(waitSemHandles.size()))
				.setPWaitSemaphores(waitSemHandles.data())
				.setPWaitDstStageMask(waitSemStages.data());
		}

		handle().submit({ submitInfo },  nullptr);
		aCommandBuffer->invoke_post_execution_handler();

		aCommandBuffer->mState = command_buffer_state::submitted;
	}

	// The code between these two ^ and v is mostly copied... I know. It avoids the usage of an unneccessary
	// temporary vector in single command buffer-case. Should, however, probably be refactored.
	void queue::submit(std::vector<resource_reference<command_buffer_t>> aCommandBuffers)
	{
		std::vector<vk::CommandBuffer> handles;
		handles.reserve(aCommandBuffers.size());
		for (auto& cb : aCommandBuffers) {
			assert(cb.get().state() >= command_buffer_state::finished_recording);
			handles.push_back(cb->handle());
		}

		const auto submitInfo = vk::SubmitInfo{}
			.setCommandBufferCount(static_cast<uint32_t>(handles.size()))
			.setPCommandBuffers(handles.data());

		handle().submit({ submitInfo }, nullptr);
		for (auto& cb : aCommandBuffers) {
			cb.get().invoke_post_execution_handler();

			cb.get().mState = command_buffer_state::submitted;
		}
	}

	fence queue::submit_with_fence(resource_reference<command_buffer_t> aCommandBuffer, std::vector<resource_ownership<semaphore_t>> aWaitSemaphores)
	{
		assert(aCommandBuffer->state() >= command_buffer_state::finished_recording);

		auto fen = root::create_fence(mDevice);

		if (0 == aWaitSemaphores.size()) {
			// Optimized route for 0 _WaitSemaphores
			const auto submitInfo = vk::SubmitInfo{}
				.setCommandBufferCount(1u)
				.setPCommandBuffers(aCommandBuffer->handle_ptr())
				.setWaitSemaphoreCount(0u)
				.setPWaitSemaphores(nullptr)
				.setPWaitDstStageMask(nullptr)
				.setSignalSemaphoreCount(0u)
				.setPSignalSemaphores(nullptr);

			handle().submit({ submitInfo }, fen->handle());
			aCommandBuffer->invoke_post_execution_handler();

			aCommandBuffer->mState = command_buffer_state::submitted;
		}
		else {
			// Also set the wait semaphores and take care of their lifetimes
			std::vector<vk::Semaphore> waitSemaphoreHandles;
			waitSemaphoreHandles.reserve(aWaitSemaphores.size());
			std::vector<vk::PipelineStageFlags> waitDstStageMasks;
			waitDstStageMasks.reserve(aWaitSemaphores.size());

			for (const auto& semaphoreDependency : aWaitSemaphores) {
				waitSemaphoreHandles.push_back(semaphoreDependency->handle());
				waitDstStageMasks.push_back(semaphoreDependency->semaphore_wait_stage());
			}

			const auto submitInfo = vk::SubmitInfo{}
				.setCommandBufferCount(1u)
				.setPCommandBuffers(aCommandBuffer->handle_ptr())
				.setWaitSemaphoreCount(static_cast<uint32_t>(waitSemaphoreHandles.size()))
				.setPWaitSemaphores(waitSemaphoreHandles.data())
				.setPWaitDstStageMask(waitDstStageMasks.data())
				.setSignalSemaphoreCount(0u)
				.setPSignalSemaphores(nullptr);

			handle().submit({ submitInfo }, fen->handle());
			aCommandBuffer->invoke_post_execution_handler();

			aCommandBuffer->mState = command_buffer_state::submitted;

			// TODO: Use ranges
			std::vector<semaphore> ownedSemaphores;
			for (auto& e : aWaitSemaphores) {
				ownedSemaphores.push_back(e.own());
			}

			fen->set_custom_deleter([
				lOwnedWaitSemaphores = std::move(ownedSemaphores)
			](){});
		}

		return fen;
	}

	fence queue::submit_with_fence(std::vector<resource_reference<command_buffer_t>> aCommandBuffers, std::vector<resource_ownership<semaphore_t>> aWaitSemaphores)
	{
		std::vector<vk::CommandBuffer> handles;
		handles.reserve(aCommandBuffers.size());
		for (auto& cb : aCommandBuffers) {
			assert(cb->state() >= command_buffer_state::finished_recording);
			handles.push_back(cb->handle());
		}

		auto fen = root::create_fence(mDevice);

		if (aWaitSemaphores.empty()) {
			// Optimized route for 0 _WaitSemaphores
			const auto submitInfo = vk::SubmitInfo{}
				.setCommandBufferCount(static_cast<uint32_t>(handles.size()))
				.setPCommandBuffers(handles.data())
				.setWaitSemaphoreCount(0u)
				.setPWaitSemaphores(nullptr)
				.setPWaitDstStageMask(nullptr)
				.setSignalSemaphoreCount(0u)
				.setPSignalSemaphores(nullptr);

			handle().submit({ submitInfo }, fen->handle());
			for (auto& cb : aCommandBuffers) {
				cb.get().invoke_post_execution_handler();

				cb.get().mState = command_buffer_state::submitted;
			}
		}
		else {
			// Also set the wait semaphores and take care of their lifetimes
			std::vector<vk::Semaphore> waitSemaphoreHandles;
			waitSemaphoreHandles.reserve(aWaitSemaphores.size());
			std::vector<vk::PipelineStageFlags> waitDstStageMasks;
			waitDstStageMasks.reserve(aWaitSemaphores.size());

			for (const auto& semaphoreDependency : aWaitSemaphores) {
				waitSemaphoreHandles.push_back(semaphoreDependency->handle());
				waitDstStageMasks.push_back(semaphoreDependency->semaphore_wait_stage());
			}

			const auto submitInfo = vk::SubmitInfo{}
				.setCommandBufferCount(static_cast<uint32_t>(handles.size()))
				.setPCommandBuffers(handles.data())
				.setWaitSemaphoreCount(static_cast<uint32_t>(waitSemaphoreHandles.size()))
				.setPWaitSemaphores(waitSemaphoreHandles.data())
				.setPWaitDstStageMask(waitDstStageMasks.data())
				.setSignalSemaphoreCount(0u)
				.setPSignalSemaphores(nullptr);

			handle().submit({ submitInfo }, fen->handle());
			for (auto& cb : aCommandBuffers) {
				cb.get().invoke_post_execution_handler();

				cb.get().mState = command_buffer_state::submitted;
			}

			// TODO: Use ranges
			std::vector<semaphore> ownedSemaphores;
			for (auto& e : aWaitSemaphores) {
				ownedSemaphores.push_back(e.own());
			}

			fen->set_custom_deleter([
				lOwnedWaitSemaphores{ std::move(ownedSemaphores) }
			](){});
		}

		return fen;
	}

	semaphore queue::submit_and_handle_with_semaphore(resource_ownership<command_buffer_t> aCommandBuffer, std::vector<resource_ownership<semaphore_t>> aWaitSemaphores)
	{
		assert(aCommandBuffer->state() >= command_buffer_state::finished_recording);

		// Create a semaphore which can, or rather, MUST be used to wait for the results
		auto signalWhenCompleteSemaphore = root::create_semaphore(mDevice);

		if (aWaitSemaphores.empty()) {
			// Optimized route for 0 _WaitSemaphores
			const auto submitInfo = vk::SubmitInfo{}
				.setCommandBufferCount(1u)
				.setPCommandBuffers(aCommandBuffer->handle_ptr())
				.setWaitSemaphoreCount(0u)
				.setPWaitSemaphores(nullptr)
				.setPWaitDstStageMask(nullptr)
				.setSignalSemaphoreCount(1u)
				.setPSignalSemaphores(signalWhenCompleteSemaphore->handle_addr());

			handle().submit({ submitInfo }, nullptr);
			aCommandBuffer->invoke_post_execution_handler();

			aCommandBuffer->mState = command_buffer_state::submitted;

			signalWhenCompleteSemaphore->set_custom_deleter([
				lOwnedCommandBuffer{ std::move(aCommandBuffer) } // Take care of the command_buffer's lifetime.. OMG!
			](){});
		}
		else {
			// Also set the wait semaphores and take care of their lifetimes
			std::vector<vk::Semaphore> waitSemaphoreHandles;
			waitSemaphoreHandles.reserve(aWaitSemaphores.size());
			std::vector<vk::PipelineStageFlags> waitDstStageMasks;
			waitDstStageMasks.reserve(aWaitSemaphores.size());

			for (const auto& semaphoreDependency : aWaitSemaphores) {
				waitSemaphoreHandles.push_back(semaphoreDependency->handle());
				waitDstStageMasks.push_back(semaphoreDependency->semaphore_wait_stage());
			}

			const auto submitInfo = vk::SubmitInfo{}
				.setCommandBufferCount(1u)
				.setPCommandBuffers(aCommandBuffer->handle_ptr())
				.setWaitSemaphoreCount(static_cast<uint32_t>(waitSemaphoreHandles.size()))
				.setPWaitSemaphores(waitSemaphoreHandles.data())
				.setPWaitDstStageMask(waitDstStageMasks.data())
				.setSignalSemaphoreCount(1u)
				.setPSignalSemaphores(signalWhenCompleteSemaphore->handle_addr());

			handle().submit({ submitInfo }, nullptr);
			aCommandBuffer->invoke_post_execution_handler();

			aCommandBuffer->mState = command_buffer_state::submitted;

			// TODO: Use ranges
			std::vector<semaphore> ownedSemaphores;
			for (auto& e : aWaitSemaphores) {
				ownedSemaphores.push_back(e.own());
			}

			signalWhenCompleteSemaphore->set_custom_deleter([
				lOwnedWaitSemaphores{ std::move(ownedSemaphores) },
				lOwnedCommandBuffer{ aCommandBuffer.own() } // Take care of the command_buffer's lifetime.. OMG!
			](){});
		}

		return signalWhenCompleteSemaphore;
	}

	semaphore queue::submit_and_handle_with_semaphore(std::optional<resource_ownership<command_buffer_t>> aCommandBuffer, std::vector<resource_ownership<semaphore_t>> aWaitSemaphores)
	{
		if (!aCommandBuffer.has_value()) {
			throw avk::runtime_error("std::optional<command_buffer> submitted and it has no value.");
		}
		return submit_and_handle_with_semaphore(std::move(aCommandBuffer.value()), std::move(aWaitSemaphores));
	}

	// The code between these two ^ and v is mostly copied... I know. It avoids the usage of an unneccessary
	// temporary vector in single command buffer-case. Should, however, probably be refactored.
	semaphore queue::submit_and_handle_with_semaphore(std::vector<resource_ownership<command_buffer_t>> aCommandBuffers, std::vector<resource_ownership<semaphore_t>> aWaitSemaphores)
	{
		std::vector<vk::CommandBuffer> handles;
		handles.reserve(aCommandBuffers.size());
		for (auto& cb : aCommandBuffers) {
			assert(cb->state() >= command_buffer_state::finished_recording);
			handles.push_back(cb->handle());
		}

		// Create a semaphore which can, or rather, MUST be used to wait for the results
		auto signalWhenCompleteSemaphore = root::create_semaphore(mDevice);

		if (aWaitSemaphores.empty()) {
			// Optimized route for 0 _WaitSemaphores
			const auto submitInfo = vk::SubmitInfo{}
				.setCommandBufferCount(static_cast<uint32_t>(handles.size()))
				.setPCommandBuffers(handles.data())
				.setWaitSemaphoreCount(0u)
				.setPWaitSemaphores(nullptr)
				.setPWaitDstStageMask(nullptr)
				.setSignalSemaphoreCount(1u)
				.setPSignalSemaphores(signalWhenCompleteSemaphore->handle_addr());

			handle().submit({ submitInfo }, nullptr);
			for (auto& cb : aCommandBuffers) {
				cb->invoke_post_execution_handler();

				cb->mState = command_buffer_state::submitted;
			}

			signalWhenCompleteSemaphore->set_custom_deleter([
				lOwnedCommandBuffer{ std::move(aCommandBuffers) } // Take care of the command_buffer's lifetime.. OMG!
			](){});
		}
		else {
			// Also set the wait semaphores and take care of their lifetimes
			std::vector<vk::Semaphore> waitSemaphoreHandles;
			waitSemaphoreHandles.reserve(aWaitSemaphores.size());
			std::vector<vk::PipelineStageFlags> waitDstStageMasks;
			waitDstStageMasks.reserve(aWaitSemaphores.size());

			for (const auto& semaphoreDependency : aWaitSemaphores) {
				waitSemaphoreHandles.push_back(semaphoreDependency->handle());
				waitDstStageMasks.push_back(semaphoreDependency->semaphore_wait_stage());
			}

			const auto submitInfo = vk::SubmitInfo{}
				.setCommandBufferCount(static_cast<uint32_t>(handles.size()))
				.setPCommandBuffers(handles.data())
				.setWaitSemaphoreCount(static_cast<uint32_t>(waitSemaphoreHandles.size()))
				.setPWaitSemaphores(waitSemaphoreHandles.data())
				.setPWaitDstStageMask(waitDstStageMasks.data())
				.setSignalSemaphoreCount(1u)
				.setPSignalSemaphores(signalWhenCompleteSemaphore->handle_addr());

			handle().submit({ submitInfo }, nullptr);
			for (auto& cb : aCommandBuffers) {
				cb->invoke_post_execution_handler();

				cb->mState = command_buffer_state::submitted;
			}

			// TODO: Use ranges
			std::vector<command_buffer> ownedCommandBuffers;
			for (auto& e : aCommandBuffers) {
				ownedCommandBuffers.push_back(e.own());
			}
			std::vector<semaphore> ownedSemaphores;
			for (auto& e : aWaitSemaphores) {
				ownedSemaphores.push_back(e.own());
			}

			signalWhenCompleteSemaphore->set_custom_deleter([
				lOwnedWaitSemaphores{ std::move(ownedSemaphores) },
				lOwnedCommandBuffer{ std::move(aCommandBuffers) } // Take care of the command_buffer's lifetime.. OMG!
			](){});
		}

		return signalWhenCompleteSemaphore;
	}

#pragma endregion

#pragma region ray tracing pipeline definitions
#if VK_HEADER_VERSION >= 135
	triangles_hit_group triangles_hit_group::create_with_rahit_only(shader_info aAnyHitShader)
	{
		if (aAnyHitShader.mShaderType != shader_type::any_hit) {
			throw avk::runtime_error("Shader is not of type shader_type::any_hit");
		}
		return triangles_hit_group { std::move(aAnyHitShader), std::nullopt };
	}

	triangles_hit_group triangles_hit_group::create_with_rchit_only(shader_info aClosestHitShader)
	{
		if (aClosestHitShader.mShaderType != shader_type::closest_hit) {
			throw avk::runtime_error("Shader is not of type shader_type::closest_hit");
		}
		return triangles_hit_group { std::nullopt, std::move(aClosestHitShader) };
	}

	triangles_hit_group triangles_hit_group::create_with_rahit_and_rchit(shader_info aAnyHitShader, shader_info aClosestHitShader)
	{
		if (aAnyHitShader.mShaderType != shader_type::any_hit) {
			throw avk::runtime_error("Shader is not of type shader_type::any_hit");
		}
		if (aClosestHitShader.mShaderType != shader_type::closest_hit) {
			throw avk::runtime_error("Shader is not of type shader_type::closest_hit");
		}
		return triangles_hit_group { std::move(aAnyHitShader), std::move(aClosestHitShader) };
	}

	triangles_hit_group triangles_hit_group::create_with_rahit_only(std::string aAnyHitShaderPath)
	{
		return create_with_rahit_only(
			shader_info::describe(std::move(aAnyHitShaderPath), "main", false, shader_type::any_hit)
		);
	}

	triangles_hit_group triangles_hit_group::create_with_rchit_only(std::string aClosestHitShaderPath)
	{
		return create_with_rchit_only(
			shader_info::describe(std::move(aClosestHitShaderPath), "main", false, shader_type::closest_hit)
		);
	}

	triangles_hit_group triangles_hit_group::create_with_rahit_and_rchit(std::string aAnyHitShaderPath, std::string aClosestHitShaderPath)
	{
		return create_with_rahit_and_rchit(
			shader_info::describe(std::move(aAnyHitShaderPath), "main", false, shader_type::any_hit),
			shader_info::describe(std::move(aClosestHitShaderPath), "main", false, shader_type::closest_hit)
		);
	}


	procedural_hit_group procedural_hit_group::create_with_rint_only(shader_info aIntersectionShader)
	{
		if (aIntersectionShader.mShaderType != shader_type::intersection) {
			throw avk::runtime_error("Shader is not of type shader_type::intersection");
		}
		return procedural_hit_group { std::move(aIntersectionShader), std::nullopt, std::nullopt };
	}

	procedural_hit_group procedural_hit_group::create_with_rint_and_rahit(shader_info aIntersectionShader, shader_info aAnyHitShader)
	{
		if (aIntersectionShader.mShaderType != shader_type::intersection) {
			throw avk::runtime_error("Shader is not of type shader_type::intersection");
		}
		if (aAnyHitShader.mShaderType != shader_type::any_hit) {
			throw avk::runtime_error("Shader is not of type shader_type::any_hit");
		}
		return procedural_hit_group { std::move(aIntersectionShader), std::move(aAnyHitShader), std::nullopt };
	}

	procedural_hit_group procedural_hit_group::create_with_rint_and_rchit(shader_info aIntersectionShader, shader_info aClosestHitShader)
	{
		if (aIntersectionShader.mShaderType != shader_type::intersection) {
			throw avk::runtime_error("Shader is not of type shader_type::intersection");
		}
		if (aClosestHitShader.mShaderType != shader_type::closest_hit) {
			throw avk::runtime_error("Shader is not of type shader_type::closest_hit");
		}
		return procedural_hit_group { std::move(aIntersectionShader), std::nullopt, std::move(aClosestHitShader) };
	}

	procedural_hit_group procedural_hit_group::create_with_rint_and_rahit_and_rchit(shader_info aIntersectionShader, shader_info aAnyHitShader, shader_info aClosestHitShader)
	{
		if (aIntersectionShader.mShaderType != shader_type::intersection) {
			throw avk::runtime_error("Shader is not of type shader_type::intersection");
		}
		if (aAnyHitShader.mShaderType != shader_type::any_hit) {
			throw avk::runtime_error("Shader is not of type shader_type::any_hit");
		}
		if (aClosestHitShader.mShaderType != shader_type::closest_hit) {
			throw avk::runtime_error("Shader is not of type shader_type::closest_hit");
		}
		return procedural_hit_group { std::move(aIntersectionShader), std::move(aAnyHitShader), std::move(aClosestHitShader) };
	}

	procedural_hit_group procedural_hit_group::create_with_rint_only(std::string aIntersectionShader)
	{
		return create_with_rint_only(
			shader_info::describe(std::move(aIntersectionShader), "main", false, shader_type::intersection)
		);
	}

	procedural_hit_group procedural_hit_group::create_with_rint_and_rahit(std::string aIntersectionShader, std::string aAnyHitShader)
	{
		return create_with_rint_and_rahit(
			shader_info::describe(std::move(aIntersectionShader), "main", false, shader_type::intersection),
			shader_info::describe(std::move(aAnyHitShader), "main", false, shader_type::any_hit)
		);
	}

	procedural_hit_group procedural_hit_group::create_with_rint_and_rchit(std::string aIntersectionShader, std::string aClosestHitShader)
	{
		return create_with_rint_and_rchit(
			shader_info::describe(std::move(aIntersectionShader), "main", false, shader_type::intersection),
			shader_info::describe(std::move(aClosestHitShader), "main", false, shader_type::closest_hit)
		);
	}

	procedural_hit_group procedural_hit_group::create_with_rint_and_rahit_and_rchit(std::string aIntersectionShader, std::string aAnyHitShader, std::string aClosestHitShader)
	{
		return create_with_rint_and_rahit_and_rchit(
			shader_info::describe(std::move(aIntersectionShader), "main", false, shader_type::intersection),
			shader_info::describe(std::move(aAnyHitShader), "main", false, shader_type::any_hit),
			shader_info::describe(std::move(aClosestHitShader), "main", false, shader_type::closest_hit)
		);
	}


	max_recursion_depth max_recursion_depth::disable_recursion()
	{
		return max_recursion_depth { 0u };
	}

	max_recursion_depth max_recursion_depth::set_to(uint32_t aValue)
	{
		return max_recursion_depth { aValue };
	}


	ray_tracing_pipeline_config::ray_tracing_pipeline_config()
		: mPipelineSettings{ cfg::pipeline_settings::nothing }
		, mShaderTableConfig{ }
		, mMaxRecursionDepth{ 16u } // 16 ... why not?!
	{
	}

	max_recursion_depth root::get_max_ray_tracing_recursion_depth()
	{
#if VK_HEADER_VERSION >= 162
		vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rtProps;
#else
		vk::PhysicalDeviceRayTracingPropertiesKHR rtProps;
#endif
		vk::PhysicalDeviceProperties2 props2;
		props2.pNext = &rtProps;
		physical_device().getProperties2(&props2);
		return max_recursion_depth{
#if VK_HEADER_VERSION >= 162
			rtProps.maxRayRecursionDepth
#else
			rtProps.maxRecursionDepth
#endif
		};
	}

	void root::rewire_config_and_create_ray_tracing_pipeline(ray_tracing_pipeline_t& aPreparedPipeline)
	{
		assert(aPreparedPipeline.mShaders.size() == aPreparedPipeline.mShaderStageCreateInfos.size());
		assert(aPreparedPipeline.mShaders.size() == aPreparedPipeline.mSpecializationInfos.size());
		for (size_t i = 0; i < aPreparedPipeline.mShaders.size(); ++i) {
			aPreparedPipeline.mShaderStageCreateInfos[i]
				.setModule(aPreparedPipeline.mShaders[i].handle())
				.setPName(aPreparedPipeline.mShaders[i].info().mEntryPoint.c_str());

			if (aPreparedPipeline.mSpecializationInfos[i] != vk::SpecializationInfo{}) {
				assert(aPreparedPipeline.mShaders[i].info().mSpecializationConstants.has_value());
				aPreparedPipeline.mSpecializationInfos[i].pMapEntries = aPreparedPipeline.mShaders[i].info().mSpecializationConstants.value().mMapEntries.data();
				aPreparedPipeline.mSpecializationInfos[i].pData = aPreparedPipeline.mShaders[i].info().mSpecializationConstants.value().mData.data();
				aPreparedPipeline.mShaderStageCreateInfos[i].setPSpecializationInfo(&aPreparedPipeline.mSpecializationInfos[i]);
			}
		}

		// Pipeline Layout must be rewired already before calling this function

		aPreparedPipeline.mPipelineLayout = device().createPipelineLayoutUnique(aPreparedPipeline.mPipelineLayoutCreateInfo);
		assert(aPreparedPipeline.layout_handle());


		// Create the PIPELINE LAYOUT
		aPreparedPipeline.mPipelineLayout = device().createPipelineLayoutUnique(aPreparedPipeline.mPipelineLayoutCreateInfo);
		assert(static_cast<bool>(aPreparedPipeline.layout_handle()));

		auto pipelineCreateInfo = vk::RayTracingPipelineCreateInfoKHR{}
			.setFlags(vk::PipelineCreateFlagBits{}) // TODO: Support flags
			.setStageCount(static_cast<uint32_t>(aPreparedPipeline.mShaderStageCreateInfos.size()))
			.setPStages(aPreparedPipeline.mShaderStageCreateInfos.data())
			.setGroupCount(static_cast<uint32_t>(aPreparedPipeline.mShaderGroupCreateInfos.size()))
			.setPGroups(aPreparedPipeline.mShaderGroupCreateInfos.data())
			.setPLibraryInterface(nullptr)
#if VK_HEADER_VERSION >= 162
			.setMaxPipelineRayRecursionDepth(aPreparedPipeline.mMaxRecursionDepth)
#else
			.setMaxRecursionDepth(aPreparedPipeline.mMaxRecursionDepth)
#endif
			.setLayout(aPreparedPipeline.layout_handle());

#if VK_HEADER_VERSION >= 162
		auto pipeCreationResult = device().createRayTracingPipelineKHR(
			{}, {},
			pipelineCreateInfo,
			nullptr,
			dynamic_dispatch());
#else
		auto pipeCreationResult = device().createRayTracingPipelineKHR(
			nullptr,
			pipelineCreateInfo,
			nullptr,
			dynamic_dispatch());
#endif

		aPreparedPipeline.mPipeline = pipeCreationResult.value;

		//result.mPipeline = std::move(pipeCreationResult.value);
		// TODO: This ^ will be fixed with vulkan headers v 136 ... or maybe not?!
	}

	void root::build_shader_binding_table(ray_tracing_pipeline_t& aPipeline)
	{
		// According to https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/#shaderbindingtable this is the way:
		const size_t groupCount = aPipeline.mShaderGroupCreateInfos.size();
		const size_t shaderBindingTableSize = aPipeline.mShaderBindingTableGroupsInfo.mTotalSize;

		// TODO: All of this SBT-stuff probably needs some refactoring
		aPipeline.mShaderBindingTable = create_buffer(
			AVK_STAGING_BUFFER_MEMORY_USAGE, // TODO: This should be a device buffer, right?!
#if VK_HEADER_VERSION >= 162
			vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, // TODO: eShaderDeviceAddressKHR or eShaderDeviceAddress?
																												// TODO: Make meta data for it!
#else
			vk::BufferUsageFlagBits::eRayTracingKHR,
#endif
			generic_buffer_meta::create_from_size(shaderBindingTableSize)
		);

		assert(aPipeline.mShaderBindingTable->meta_at_index<buffer_meta>(0).total_size() == shaderBindingTableSize);

		// Copy to temporary buffer:
		std::vector<uint8_t> shaderHandleStorage(shaderBindingTableSize); // The order MUST be the same as during step 3, we just need to ensure to align the TARGET offsets properly (see below)
		auto result = device().getRayTracingShaderGroupHandlesKHR(aPipeline.handle(), 0, groupCount, shaderBindingTableSize, shaderHandleStorage.data(), dynamic_dispatch());
		assert(static_cast<VkResult>(result) >= 0);

		auto mapped = scoped_mapping{aPipeline.mShaderBindingTable->mBuffer, mapping_access::write};
		// Transfer all the groups into the buffer's memory, taking all the offsets determined in step 3 into account:
		vk::DeviceSize off = 0;
		size_t iRaygen = 0;
		size_t iMiss = 0;
		size_t iHit = 0;
		size_t iCallable = 0;
		auto* pData  = reinterpret_cast<uint8_t*>(mapped.get());
		size_t srcByteOffset = 0;
		while (off < aPipeline.mShaderBindingTableGroupsInfo.mEndOffset) {
			size_t dstOffset = 0;
			size_t copySize = 0;

			if (iRaygen   < aPipeline.mShaderBindingTableGroupsInfo.mRaygenGroupsInfo.size() && aPipeline.mShaderBindingTableGroupsInfo.mRaygenGroupsInfo[iRaygen].mOffset == off) {
				dstOffset = aPipeline.mShaderBindingTableGroupsInfo.mRaygenGroupsInfo[iRaygen].mByteOffset;
				off      += aPipeline.mShaderBindingTableGroupsInfo.mRaygenGroupsInfo[iRaygen].mNumEntries;
				copySize  = aPipeline.mShaderBindingTableGroupsInfo.mRaygenGroupsInfo[iRaygen].mNumEntries * aPipeline.mShaderGroupHandleSize;
				++iRaygen;
			}
			else if (iMiss < aPipeline.mShaderBindingTableGroupsInfo.mMissGroupsInfo.size() && aPipeline.mShaderBindingTableGroupsInfo.mMissGroupsInfo[iMiss].mOffset == off) {
				dstOffset  = aPipeline.mShaderBindingTableGroupsInfo.mMissGroupsInfo[iMiss].mByteOffset;
				off       += aPipeline.mShaderBindingTableGroupsInfo.mMissGroupsInfo[iMiss].mNumEntries;
				copySize   = aPipeline.mShaderBindingTableGroupsInfo.mMissGroupsInfo[iMiss].mNumEntries * aPipeline.mShaderGroupHandleSize;
				++iMiss;
			}
			else if (iHit < aPipeline.mShaderBindingTableGroupsInfo.mHitGroupsInfo.size() && aPipeline.mShaderBindingTableGroupsInfo.mHitGroupsInfo[iHit].mOffset == off) {
				dstOffset = aPipeline.mShaderBindingTableGroupsInfo.mHitGroupsInfo[iHit].mByteOffset;
				off      += aPipeline.mShaderBindingTableGroupsInfo.mHitGroupsInfo[iHit].mNumEntries;
				copySize  = aPipeline.mShaderBindingTableGroupsInfo.mHitGroupsInfo[iHit].mNumEntries * aPipeline.mShaderGroupHandleSize;
				++iHit;
			}
			else if (iCallable < aPipeline.mShaderBindingTableGroupsInfo.mCallableGroupsInfo.size() && aPipeline.mShaderBindingTableGroupsInfo.mCallableGroupsInfo[iCallable].mOffset == off) {
				dstOffset = aPipeline.mShaderBindingTableGroupsInfo.mCallableGroupsInfo[iCallable].mByteOffset;
				off      += aPipeline.mShaderBindingTableGroupsInfo.mCallableGroupsInfo[iCallable].mNumEntries;
				copySize  = aPipeline.mShaderBindingTableGroupsInfo.mCallableGroupsInfo[iCallable].mNumEntries * aPipeline.mShaderGroupHandleSize;
				++iCallable;
			}
			else {
				throw avk::runtime_error("Can't be");
			}

			memcpy(pData + dstOffset, shaderHandleStorage.data() + srcByteOffset, copySize);
			srcByteOffset += copySize;
		}
		//for(uint32_t g = 0; g < groupCount; g++)
		//{
		//}
	}

	ray_tracing_pipeline root::create_ray_tracing_pipeline(ray_tracing_pipeline_config aConfig, std::function<void(ray_tracing_pipeline_t&)> aAlterConfigBeforeCreation)
	{
		using namespace cfg;

		ray_tracing_pipeline_t result;
		result.mDevice = device();
		result.mDynamicDispatch = dynamic_dispatch();

		// 1. Set pipeline flags
		result.mPipelineCreateFlags = {};
		// TODO: Support all flags (only one of the flags is handled at the moment)
		if ((aConfig.mPipelineSettings & pipeline_settings::disable_optimization) == pipeline_settings::disable_optimization) {
			result.mPipelineCreateFlags |= vk::PipelineCreateFlagBits::eDisableOptimization;
		}

		// Get the offsets. We'll really need them in step 10. but already in step 3., we are gathering the correct byte offsets:
		{
#if VK_HEADER_VERSION >= 162
			vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rtProps;
#else
			vk::PhysicalDeviceRayTracingPropertiesKHR rtProps;
#endif
			vk::PhysicalDeviceProperties2 props2;
			props2.pNext = &rtProps;
			physical_device().getProperties2(&props2);

			result.mShaderGroupBaseAlignment = static_cast<uint32_t>(rtProps.shaderGroupBaseAlignment);
			result.mShaderGroupHandleSize = static_cast<uint32_t>(rtProps.shaderGroupHandleSize);
		}

		// 2. Gather and build shaders
		// First of all, gather unique shaders and build them
		std::vector<shader_info> orderedUniqueShaderInfos;
		for (auto& tableEntry : aConfig.mShaderTableConfig.mShaderTableEntries) {
			if (std::holds_alternative<shader_info>(tableEntry)) {
				add_to_vector_if_not_already_contained(orderedUniqueShaderInfos, std::get<shader_info>(tableEntry));
			}
			else if (std::holds_alternative<triangles_hit_group>(tableEntry)) {
				const auto& hitGroup = std::get<triangles_hit_group>(tableEntry);
				if (hitGroup.mAnyHitShader.has_value()) {
					add_to_vector_if_not_already_contained(orderedUniqueShaderInfos, hitGroup.mAnyHitShader.value());
				}
				if (hitGroup.mClosestHitShader.has_value()) {
					add_to_vector_if_not_already_contained(orderedUniqueShaderInfos, hitGroup.mClosestHitShader.value());
				}
			}
			else if (std::holds_alternative<procedural_hit_group>(tableEntry)) {
				const auto& hitGroup = std::get<procedural_hit_group>(tableEntry);
				add_to_vector_if_not_already_contained(orderedUniqueShaderInfos, hitGroup.mIntersectionShader);
				if (hitGroup.mAnyHitShader.has_value()) {
					add_to_vector_if_not_already_contained(orderedUniqueShaderInfos, hitGroup.mAnyHitShader.value());
				}
				if (hitGroup.mClosestHitShader.has_value()) {
					add_to_vector_if_not_already_contained(orderedUniqueShaderInfos, hitGroup.mClosestHitShader.value());
				}
			}
			else {
				throw avk::runtime_error("tableEntry holds an unknown alternative. That's mysterious.");
			}
		}
		result.mShaders.reserve(orderedUniqueShaderInfos.size());
		result.mShaderStageCreateInfos.reserve(orderedUniqueShaderInfos.size());
		result.mSpecializationInfos.reserve(orderedUniqueShaderInfos.size());
		for (auto& shaderInfo : orderedUniqueShaderInfos) {
			// 2.2 Compile the shader
			result.mShaders.push_back(create_shader(shaderInfo));
			assert(result.mShaders.back().has_been_built());
			// 2.3 Create shader info
			auto& stageCreateInfo = result.mShaderStageCreateInfos.emplace_back()
				.setStage(to_vk_shader_stage(result.mShaders.back().info().mShaderType))
				.setModule(result.mShaders.back().handle())
				.setPName(result.mShaders.back().info().mEntryPoint.c_str());
			if (shaderInfo.mSpecializationConstants.has_value()) {
				auto& specInfo = result.mSpecializationInfos.emplace_back(
					shaderInfo.mSpecializationConstants.value().num_entries(),
					shaderInfo.mSpecializationConstants.value().mMapEntries.data(),
					shaderInfo.mSpecializationConstants.value().data_size(),
					shaderInfo.mSpecializationConstants.value().mData.data()
				);
				// Add it to the stageCreateInfo:
				stageCreateInfo.setPSpecializationInfo(&specInfo);
			}
			else {
				result.mSpecializationInfos.emplace_back(); // Just to keep the indices into the vectors in sync
			}
		}
		assert(orderedUniqueShaderInfos.size() == result.mShaders.size());
		assert(result.mShaders.size() == result.mShaderStageCreateInfos.size());
#if defined(_DEBUG)
		// Perform a sanity check:
		for (size_t i = 0; i < orderedUniqueShaderInfos.size(); ++i) {
			assert(orderedUniqueShaderInfos[i] == result.mShaders[i].info());
		}
#endif

		// 3. Create the shader table (with references to the shaders from step 2.)
		// Iterate over the shader table... AGAIN! ...But this time, build the shader groups for Vulkan's Ray Tracing Pipeline:
		result.mShaderGroupCreateInfos.reserve(aConfig.mShaderTableConfig.mShaderTableEntries.size());

		// During iterating, also compile the data for the shader binding table groups information:
		result.mShaderBindingTableGroupsInfo = {};
		enum struct group_type { none, raygen, miss, hit, callable };
		group_type prevType = group_type::none;
		vk::DeviceSize groupOffset = 0;
		vk::DeviceSize byteOffset = 0;
		shader_group_info* curEdited = nullptr;

		for (auto& tableEntry : aConfig.mShaderTableConfig.mShaderTableEntries) {
			group_type curType = group_type::none;

			if (std::holds_alternative<shader_info>(tableEntry)) {
				const auto& shaderInfo = std::get<shader_info>(tableEntry);
				switch (shaderInfo.mShaderType) {
				case shader_type::ray_generation: curType = group_type::raygen;   break;
				case shader_type::miss:           curType = group_type::miss;     break;
				case shader_type::callable:       curType = group_type::callable; break;
				default: throw avk::runtime_error("Invalid shader type passed to create_ray_tracing_pipeline, recognized during gathering of SBT infos");
				}

				// The shader indices are actually indices into `result.mShaders` not into
				// `orderedUniqueShaderInfos`. However, both vectors are aligned perfectly,
				// so we are just using `orderedUniqueShaderInfos` for convenience.
				const uint32_t generalShaderIndex = static_cast<uint32_t>(index_of(orderedUniqueShaderInfos, shaderInfo));
				result.mShaderGroupCreateInfos.emplace_back()
					.setType(vk::RayTracingShaderGroupTypeNV::eGeneral)
					.setGeneralShader(generalShaderIndex)
					.setIntersectionShader(VK_SHADER_UNUSED_KHR)
					.setAnyHitShader(VK_SHADER_UNUSED_KHR)
					.setClosestHitShader(VK_SHADER_UNUSED_KHR);
			}
			else if (std::holds_alternative<triangles_hit_group>(tableEntry)) {
				curType = group_type::hit;

				const auto& hitGroup = std::get<triangles_hit_group>(tableEntry);
				uint32_t rahitShaderIndex = VK_SHADER_UNUSED_KHR;
				if (hitGroup.mAnyHitShader.has_value()) {
					rahitShaderIndex = static_cast<uint32_t>(index_of(orderedUniqueShaderInfos, hitGroup.mAnyHitShader.value()));
				}
				uint32_t rchitShaderIndex = VK_SHADER_UNUSED_KHR;
				if (hitGroup.mClosestHitShader.has_value()) {
					rchitShaderIndex = static_cast<uint32_t>(index_of(orderedUniqueShaderInfos, hitGroup.mClosestHitShader.value()));
				}
				result.mShaderGroupCreateInfos.emplace_back()
					.setType(vk::RayTracingShaderGroupTypeNV::eTrianglesHitGroup)
					.setGeneralShader(VK_SHADER_UNUSED_KHR)
					.setIntersectionShader(VK_SHADER_UNUSED_KHR)
					.setAnyHitShader(rahitShaderIndex)
					.setClosestHitShader(rchitShaderIndex);
			}
			else if (std::holds_alternative<procedural_hit_group>(tableEntry)) {
				curType = group_type::hit;

				const auto& hitGroup = std::get<procedural_hit_group>(tableEntry);
				uint32_t rintShaderIndex = static_cast<uint32_t>(index_of(orderedUniqueShaderInfos, hitGroup.mIntersectionShader));
				uint32_t rahitShaderIndex = VK_SHADER_UNUSED_KHR;
				if (hitGroup.mAnyHitShader.has_value()) {
					rahitShaderIndex = static_cast<uint32_t>(index_of(orderedUniqueShaderInfos, hitGroup.mAnyHitShader.value()));
				}
				uint32_t rchitShaderIndex = VK_SHADER_UNUSED_KHR;
				if (hitGroup.mClosestHitShader.has_value()) {
					rchitShaderIndex = static_cast<uint32_t>(index_of(orderedUniqueShaderInfos, hitGroup.mClosestHitShader.value()));
				}
				result.mShaderGroupCreateInfos.emplace_back()
					.setType(vk::RayTracingShaderGroupTypeNV::eProceduralHitGroup)
					.setGeneralShader(VK_SHADER_UNUSED_KHR)
					.setIntersectionShader(rintShaderIndex)
					.setAnyHitShader(rahitShaderIndex)
					.setClosestHitShader(rchitShaderIndex);
			}
			else {
				throw avk::runtime_error("tableEntry holds an unknown alternative. That's mysterious.");
			}

			// Set that shader binding table groups information:
			byteOffset += result.mShaderGroupHandleSize;
			assert (group_type::none != curType);
			if (curType == prevType) {
				// same same is easy
				assert (nullptr != curEdited);
				curEdited->mNumEntries += 1;
			}
			else {
				// different => create new entry
				switch (curType) {
				case group_type::raygen:
					curEdited = &result.mShaderBindingTableGroupsInfo.mRaygenGroupsInfo.emplace_back();
					break;
				case group_type::miss:
					curEdited = &result.mShaderBindingTableGroupsInfo.mMissGroupsInfo.emplace_back();
					break;
				case group_type::hit:
					curEdited = &result.mShaderBindingTableGroupsInfo.mHitGroupsInfo.emplace_back();
					break;
				case group_type::callable:
					curEdited = &result.mShaderBindingTableGroupsInfo.mCallableGroupsInfo.emplace_back();
					break;
				default: throw avk::runtime_error("Can't be!");
				}
				assert (nullptr != curEdited);
				curEdited->mOffset = groupOffset;
				curEdited->mNumEntries = 1;

				// A new shader group must start at a multiple of result.mShaderGroupBaseAlignment
				if (byteOffset % static_cast<vk::DeviceSize>(result.mShaderGroupBaseAlignment) != 0) {
					byteOffset = (byteOffset / static_cast<vk::DeviceSize>(result.mShaderGroupBaseAlignment) + 1) * static_cast<vk::DeviceSize>(result.mShaderGroupBaseAlignment);
				}
				curEdited->mByteOffset = byteOffset;
			}
			prevType = curType;
			++groupOffset;
		}
		result.mShaderBindingTableGroupsInfo.mEndOffset = groupOffset;
		result.mShaderBindingTableGroupsInfo.mTotalSize = byteOffset;

		// 4. Maximum recursion depth:
		result.mMaxRecursionDepth = aConfig.mMaxRecursionDepth.mMaxRecursionDepth;

		// 5. Pipeline layout
		result.mAllDescriptorSetLayouts = set_of_descriptor_set_layouts::prepare(std::move(aConfig.mResourceBindings));
		allocate_set_of_descriptor_set_layouts(result.mAllDescriptorSetLayouts);

		// Gather the push constant data
		result.mPushConstantRanges.reserve(aConfig.mPushConstantsBindings.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!
		for (const auto& pcBinding : aConfig.mPushConstantsBindings) {
			result.mPushConstantRanges.push_back(vk::PushConstantRange{}
				.setStageFlags(to_vk_shader_stages(pcBinding.mShaderStages))
				.setOffset(static_cast<uint32_t>(pcBinding.mOffset))
				.setSize(static_cast<uint32_t>(pcBinding.mSize))
			);
			// TODO: Push Constants need a prettier interface
		}

		auto descriptorSetLayoutHandles = result.mAllDescriptorSetLayouts.layout_handles();
		result.mPipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo{}
			.setSetLayoutCount(static_cast<uint32_t>(descriptorSetLayoutHandles.size()))
			.setPSetLayouts(descriptorSetLayoutHandles.data())
			.setPushConstantRangeCount(static_cast<uint32_t>(result.mPushConstantRanges.size()))
			.setPPushConstantRanges(result.mPushConstantRanges.data());

		// 6. Maybe alter the config?
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(result);
		}

		// 8. Create the pipeline's layout
		// 9. Build the Ray Tracing Pipeline
		rewire_config_and_create_ray_tracing_pipeline(result);

		// 10. Build the shader binding table
		build_shader_binding_table(result);
		return result;
	}

	ray_tracing_pipeline root::create_ray_tracing_pipeline_from_template(resource_reference<const ray_tracing_pipeline_t> aTemplate, std::function<void(ray_tracing_pipeline_t&)> aAlterConfigBeforeCreation)
	{
		ray_tracing_pipeline_t result;

		result.mPipelineCreateFlags						= aTemplate->mPipelineCreateFlags;
		for (const auto& shdr : aTemplate->mShaders) {
			result.mShaders.push_back(create_shader_from_template(shdr));
		}
		result.mShaderStageCreateInfos					= aTemplate->mShaderStageCreateInfos;
		result.mSpecializationInfos						= aTemplate->mSpecializationInfos;
		result.mShaderGroupCreateInfos					= aTemplate->mShaderGroupCreateInfos;
		result.mShaderBindingTableGroupsInfo			= aTemplate->mShaderBindingTableGroupsInfo;
		result.mMaxRecursionDepth						= aTemplate->mMaxRecursionDepth;
		result.mBasePipelineIndex						= aTemplate->mBasePipelineIndex;
		result.mAllDescriptorSetLayouts = create_set_of_descriptor_set_layouts_from_template(aTemplate->mAllDescriptorSetLayouts);
		result.mPushConstantRanges						= aTemplate->mPushConstantRanges;

		auto descriptorSetLayoutHandles = result.mAllDescriptorSetLayouts.layout_handles();
		result.mPipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo{}
			.setSetLayoutCount(static_cast<uint32_t>(descriptorSetLayoutHandles.size()))
			.setPSetLayouts(descriptorSetLayoutHandles.data())
			.setPushConstantRangeCount(static_cast<uint32_t>(result.mPushConstantRanges.size()))
			.setPPushConstantRanges(result.mPushConstantRanges.data());

		result.mShaderGroupBaseAlignment				= aTemplate->mShaderGroupBaseAlignment;
		result.mShaderGroupHandleSize					= aTemplate->mShaderGroupHandleSize;

		result.mDevice									= aTemplate->mDevice;
		result.mDynamicDispatch							= aTemplate->mDynamicDispatch;

		// 15. Maybe alter the config?!
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(result);
		}

		rewire_config_and_create_ray_tracing_pipeline(result);
		build_shader_binding_table(result);
		return result;
	}

	ray_tracing_pipeline_t::~ray_tracing_pipeline_t()
	{
		if (handle()) {
			mDevice.destroy(handle());
			mPipeline.reset();
		}
	}

	size_t ray_tracing_pipeline_t::num_raygen_groups_in_shader_binding_table() const
	{
		return shader_binding_table_groups().mRaygenGroupsInfo.size();
	}

	size_t ray_tracing_pipeline_t::num_miss_groups_in_shader_binding_table() const
	{
		return shader_binding_table_groups().mMissGroupsInfo.size();
	}

	size_t ray_tracing_pipeline_t::num_hit_groups_in_shader_binding_table() const
	{
		return shader_binding_table_groups().mHitGroupsInfo.size();
	}

	size_t ray_tracing_pipeline_t::num_callable_groups_in_shader_binding_table() const
	{
		return shader_binding_table_groups().mCallableGroupsInfo.size();
	}

	void ray_tracing_pipeline_t::print_shader_binding_table_groups() const
	{
		vk::DeviceSize off = 0;
		size_t iRaygen = 0;
		size_t iMiss = 0;
		size_t iHit = 0;
		size_t iCallable = 0;
		auto printRow = [](std::string aOffset, std::string aShaders, std::string aRaygen, std::string aMiss, std::string aHit, std::string aCallable){
			static const std::string offsetStr	 = "      ";
			static const std::string shadersStr	 = "                                               ";
			static const std::string raygenStr	 = "          ";
			static const std::string missStr	 = "          ";
			static const std::string hitStr		 = "         ";
			static const std::string callableStr = "          ";
			if (aOffset.empty())	{ aOffset = offsetStr; }
			if (aShaders.empty())	{ aShaders = shadersStr; }
			if (aRaygen.empty())	{ aRaygen = raygenStr; }
			if (aMiss.empty())		{ aMiss = missStr; }
			if (aHit.empty())		{ aHit = hitStr; }
			if (aCallable.empty())	{ aCallable = callableStr; }
			if (aOffset.length() < offsetStr.length())		{ aOffset = offsetStr		+ aOffset; }
			if (aShaders.length() < shadersStr.length())	{ aShaders = shadersStr		+ aShaders; }
			if (aRaygen.length() < raygenStr.length())		{ aRaygen = raygenStr		+ aRaygen; }
			if (aMiss.length() < missStr.length())			{ aMiss = missStr			+ aMiss; }
			if (aHit.length() < hitStr.length())			{ aHit = hitStr				+ aHit; }
			if (aCallable.length() < callableStr.length())	{ aCallable = callableStr	+ aCallable; }
			if (aOffset.length() > offsetStr.length())		{ aOffset = aOffset.substr(aOffset.length() - offsetStr.length()); }
			if (aShaders.length() > shadersStr.length())	{ aShaders = aShaders.substr(aShaders.length() - shadersStr.length()); }
			if (aRaygen.length() > raygenStr.length())		{ aRaygen = aRaygen.substr(aRaygen.length() - raygenStr.length()); }
			if (aMiss.length() > missStr.length())			{ aMiss =   aMiss.substr(aMiss.length()   - missStr.length()); }
			if (aHit.length() > hitStr.length())			{ aHit =    aHit.substr(aHit.length()    - hitStr.length()); }
			if (aCallable.length() > callableStr.length())	{ aCallable = aCallable.substr(aCallable.length() - callableStr.length()); }
			AVK_LOG_INFO("| " + aOffset + " | " + aShaders + " | " + aRaygen + " | " + aMiss + " | " + aHit + " | " + aCallable + " |");
		};
		auto getShaderName = [this](uint32_t aIndex, bool aPrintFileExt = true){
			auto filename = avk::extract_file_name(mShaders[aIndex].info().mPath);
			const auto spvPos = filename.find(".spv");
			if (spvPos != std::string::npos) {
				filename = filename.substr(0, spvPos);
			}
			if (!aPrintFileExt) {
				const auto dotPos = filename.find_first_of('.');
				if (dotPos != std::string::npos) {
					return filename.substr(0, dotPos);
				}
			}
			return filename;
		};
		AVK_LOG_INFO("+=============================================================================================================+");
		AVK_LOG_INFO("|                          +++++++++++++ SHADER BINDING TABLE +++++++++++++                                   |");
		AVK_LOG_INFO("|                          BYTE-OFFSETS, SHADERS, and GROUP-INDICES (G.IDX)                                   |");
		AVK_LOG_INFO("+=============================================================================================================+");
		AVK_LOG_INFO("| OFFSET | SHADERS: GENERAL or INTERS.|ANY-HIT|CLOSEST-HIT | RGEN G.IDX | MISS G.IDX | HIT G.IDX | CALL G.IDX |");
		while (off < mShaderBindingTableGroupsInfo.mEndOffset) {
			AVK_LOG_INFO("+-------------------------------------------------------------------------------------------------------------+");
			if (iRaygen   < mShaderBindingTableGroupsInfo.mRaygenGroupsInfo.size() && mShaderBindingTableGroupsInfo.mRaygenGroupsInfo[iRaygen].mOffset == off) {
				std::string byteOff = std::to_string(mShaderBindingTableGroupsInfo.mRaygenGroupsInfo[iRaygen].mByteOffset);
				std::string grpIdx = "[" + std::to_string(iRaygen) + "]";
				for (size_t i = 0; i < mShaderBindingTableGroupsInfo.mRaygenGroupsInfo[iRaygen].mNumEntries; ++i) {
					printRow(byteOff, getShaderName(mShaderGroupCreateInfos[off + i].generalShader) + ": " + std::to_string(i), grpIdx, "", "", "");
					byteOff = ""; grpIdx = "";
				}
				off      += mShaderBindingTableGroupsInfo.mRaygenGroupsInfo[iRaygen].mNumEntries;
				++iRaygen;
			}
			else if (iMiss < mShaderBindingTableGroupsInfo.mMissGroupsInfo.size() && mShaderBindingTableGroupsInfo.mMissGroupsInfo[iMiss].mOffset == off) {
				std::string byteOff = std::to_string(mShaderBindingTableGroupsInfo.mMissGroupsInfo[iMiss].mByteOffset);
				std::string grpIdx = "[" + std::to_string(iMiss) + "]";
				for (size_t i = 0; i < mShaderBindingTableGroupsInfo.mMissGroupsInfo[iMiss].mNumEntries; ++i) {
					printRow(byteOff, getShaderName(mShaderGroupCreateInfos[off + i].generalShader) + ": " + std::to_string(i), "", grpIdx, "", "");
					byteOff = ""; grpIdx = "";
				}
				off       += mShaderBindingTableGroupsInfo.mMissGroupsInfo[iMiss].mNumEntries;
				++iMiss;
			}
			else if (iHit < mShaderBindingTableGroupsInfo.mHitGroupsInfo.size() && mShaderBindingTableGroupsInfo.mHitGroupsInfo[iHit].mOffset == off) {
				std::string byteOff = std::to_string(mShaderBindingTableGroupsInfo.mHitGroupsInfo[iHit].mByteOffset);
				std::string grpIdx = "[" + std::to_string(iHit) + "]";
				for (size_t i = 0; i < mShaderBindingTableGroupsInfo.mHitGroupsInfo[iHit].mNumEntries; ++i) {
					assert(vk::RayTracingShaderGroupTypeKHR::eGeneral != mShaderGroupCreateInfos[off + iHit].type);
					std::string hitInfo;
					hitInfo += mShaderGroupCreateInfos[off + i].intersectionShader != VK_SHADER_UNUSED_KHR ? getShaderName(mShaderGroupCreateInfos[off + i].intersectionShader, false) : "--";
					hitInfo += "|";
					hitInfo += mShaderGroupCreateInfos[off + i].anyHitShader != VK_SHADER_UNUSED_KHR ? getShaderName(mShaderGroupCreateInfos[off + i].anyHitShader, false) : "--";
					hitInfo += "|";
					hitInfo += mShaderGroupCreateInfos[off + i].closestHitShader != VK_SHADER_UNUSED_KHR ? getShaderName(mShaderGroupCreateInfos[off + i].closestHitShader, false) : "--";
					printRow(byteOff, hitInfo + ": " + std::to_string(i), "", "", grpIdx, "");
					byteOff = ""; grpIdx = "";
				}
				off      += mShaderBindingTableGroupsInfo.mHitGroupsInfo[iHit].mNumEntries;
				++iHit;
			}
			else if (iCallable < mShaderBindingTableGroupsInfo.mCallableGroupsInfo.size() && mShaderBindingTableGroupsInfo.mCallableGroupsInfo[iCallable].mOffset == off) {
				std::string byteOff = std::to_string(mShaderBindingTableGroupsInfo.mCallableGroupsInfo[iCallable].mByteOffset);
				std::string grpIdx = "[" + std::to_string(iCallable) + "]";
				for (size_t i = 0; i < mShaderBindingTableGroupsInfo.mCallableGroupsInfo[iCallable].mNumEntries; ++i) {
					printRow(byteOff, getShaderName(mShaderGroupCreateInfos[off + i].generalShader) + ": " + std::to_string(i), "", "", "", grpIdx);
					byteOff = ""; grpIdx = "";
				}
				off      += mShaderBindingTableGroupsInfo.mCallableGroupsInfo[iCallable].mNumEntries;
				++iCallable;
			}
			else {
				throw avk::runtime_error("Can't be");
			}
		}
		AVK_LOG_INFO("+-------------------------------------------------------------------------------------------------------------+");
	}
#endif
#pragma endregion

#pragma region renderpass definitions
using namespace cpplinq;

	struct subpass_desc_helper
	{
		size_t mSubpassId;
		std::map<uint32_t, vk::AttachmentReference> mSpecificInputLocations;
		std::queue<vk::AttachmentReference> mUnspecifiedInputLocations;
		int mInputMaxLoc;
		std::map<uint32_t, vk::AttachmentReference> mSpecificColorLocations;
		std::queue<vk::AttachmentReference> mUnspecifiedColorLocations;
		int mColorMaxLoc;
		std::map<uint32_t, vk::AttachmentReference> mSpecificDepthStencilLocations;
		std::queue<vk::AttachmentReference> mUnspecifiedDepthStencilLocations;
		int mDepthStencilMaxLoc;
		std::map<uint32_t, vk::AttachmentReference> mSpecificResolveLocations;
		std::queue<vk::AttachmentReference> mUnspecifiedResolveLocations;
		std::vector<uint32_t> mPreserveAttachments;
	};

	void root::rewire_subpass_descriptions(renderpass_t& aRenderpass)
	{
		const auto nSubpasses = aRenderpass.mSubpassData.size();

		aRenderpass.mSubpasses.clear(); // Start with a clean state
		aRenderpass.mSubpasses.reserve(nSubpasses);

		for (size_t i = 0; i < nSubpasses; ++i) {
			auto& b = aRenderpass.mSubpassData[i];

			aRenderpass.mSubpasses.push_back(vk::SubpassDescription()
				// pipelineBindPoint must be VK_PIPELINE_BIND_POINT_GRAPHICS [1] because subpasses are only relevant for graphics at the moment
				.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
				.setColorAttachmentCount(static_cast<uint32_t>(b.mOrderedColorAttachmentRefs.size()))
				.setPColorAttachments(b.mOrderedColorAttachmentRefs.data())
				// If pResolveAttachments is not NULL, each of its elements corresponds to a color attachment
				//  (the element in pColorAttachments at the same index), and a multisample resolve operation
				//  is defined for each attachment. [1]
				.setPResolveAttachments(b.mOrderedResolveAttachmentRefs.size() == 0 ? nullptr : b.mOrderedResolveAttachmentRefs.data())
				// If pDepthStencilAttachment is NULL, or if its attachment index is VK_ATTACHMENT_UNUSED, it
				//  indicates that no depth/stencil attachment will be used in the subpass. [1]
				.setPDepthStencilAttachment(b.mOrderedDepthStencilAttachmentRefs.size() == 0 ? nullptr : &b.mOrderedDepthStencilAttachmentRefs[0])
				// The following two attachment types are probably totally irrelevant if we only have one subpass
				.setInputAttachmentCount(static_cast<uint32_t>(b.mOrderedInputAttachmentRefs.size()))
				.setPInputAttachments(b.mOrderedInputAttachmentRefs.data())
				.setPreserveAttachmentCount(static_cast<uint32_t>(b.mPreserveAttachments.size()))
				.setPPreserveAttachments(b.mPreserveAttachments.data()));
		}
	}

	renderpass root::create_renderpass(std::vector<avk::attachment> aAttachments, std::function<void(renderpass_sync&)> aSync, std::function<void(renderpass_t&)> aAlterConfigBeforeCreation)
	{
		renderpass_t result;

		std::vector<subpass_desc_helper> subpasses;

		if (aAttachments.empty()) {
			throw avk::runtime_error("No attachments have been passed to the creation of a renderpass.");
		}
		const auto numSubpassesFirst = aAttachments.front().mSubpassUsages.num_subpasses();
		// All further attachments must have the same number of subpasses! It will be checked.
		subpasses.reserve(numSubpassesFirst);
		for (size_t i = 0; i < numSubpassesFirst; ++i) {
			auto& a = subpasses.emplace_back();
			a.mSubpassId = i;
			a.mInputMaxLoc = -1;
			a.mColorMaxLoc = -1;
			a.mDepthStencilMaxLoc = -1;
		}

		result.mAttachmentDescriptions.reserve(aAttachments.size());
		for (const auto& a : aAttachments) {
			// Try to infer initial and final image layouts (If this isn't cool => user must use aAlterConfigBeforeCreation)
			vk::ImageLayout initialLayout = vk::ImageLayout::eUndefined;
			vk::ImageLayout finalLayout = vk::ImageLayout::eUndefined;

			const auto isLoad = avk::on_load::load == a.mLoadOperation;
			const auto isClear = avk::on_load::clear == a.mLoadOperation;
			const auto isStore  = avk::on_store::store == a.mStoreOperation || avk::on_store::store_in_presentable_format == a.mStoreOperation;
			const auto makePresentable = avk::on_store::store_in_presentable_format == a.mStoreOperation;

			const auto hasSeparateStencilLoad = a.mStencilLoadOperation.has_value();
			const auto hasSeparateStencilStore = a.mStencilStoreOperation.has_value();
			const auto isStencilLoad = avk::on_load::load == a.get_stencil_load_op();
			const auto isStencilClear = avk::on_load::clear == a.get_stencil_load_op();
			const auto isStencilStore  = avk::on_store::store == a.get_stencil_store_op() || avk::on_store::store_in_presentable_format == a.get_stencil_store_op();
			const auto makeStencilPresentable = avk::on_store::store_in_presentable_format == a.get_stencil_store_op();
			const auto hasStencilComponent = has_stencil_component(a.format());

			bool initialLayoutFixed = false;
			auto firstUsage = a.get_first_color_depth_input();
			if (firstUsage.as_input()) {
				if (isLoad) {
					initialLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
					initialLayoutFixed = true;
				}
				if (isClear) {
					initialLayoutFixed = true;
				}
			}
			if (firstUsage.as_color()) { // this potentially overwrites the above
				if (isLoad) {
					initialLayout = vk::ImageLayout::eColorAttachmentOptimal;
					initialLayoutFixed = true;
				}
				if (isClear) {
					initialLayoutFixed = true;
				}
			}
			if (firstUsage.as_depth_stencil()) {
				if (isLoad) {
					initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
					{
						// TODO: Set other depth/stencil-specific formats
						//       - vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal
						//       - vk::ImageLayout::eDepthStencilReadOnlyOptimal
						//       - vk::ImageLayout::eDepthReadOnlyOptimal
						//       - vk::ImageLayout::eStencilAttachmentOptimal
						//       - vk::ImageLayout::eStencilReadOnlyOptimal
					}
					initialLayoutFixed = true;
				}
				if (isClear) {
					initialLayoutFixed = true;
				}
			}
			if (!initialLayoutFixed) {
				if (a.mImageUsageHintBefore.has_value()) {
					// If we detect the image usage to be more generic, we should change the layout to something more generic
					if (avk::has_flag(a.mImageUsageHintBefore.value(), avk::image_usage::sampled)) {
						initialLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
					}
					if (avk::has_flag(a.mImageUsageHintBefore.value(), avk::image_usage::shader_storage)) {
						initialLayout = vk::ImageLayout::eGeneral;
					}
				}
			}

			auto lastUsage = a.get_last_color_depth_input();
			if (lastUsage.as_input()) {
				finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			}
			if (lastUsage.as_color()) { // This potentially overwrites the above
				finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
			}
			if (lastUsage.as_depth_stencil()) {
				finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
				{
					// TODO: Set other depth/stencil-specific formats
					//       - vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal
					//       - vk::ImageLayout::eDepthStencilReadOnlyOptimal
					//       - vk::ImageLayout::eDepthReadOnlyOptimal
					//       - vk::ImageLayout::eStencilAttachmentOptimal
					//       - vk::ImageLayout::eStencilReadOnlyOptimal
				}
			}
			if (isStore && vk::ImageLayout::eUndefined == finalLayout) {
				if (a.is_used_as_color_attachment()) {
					finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
				}
				else if (a.is_used_as_depth_stencil_attachment()) {
					finalLayout = vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal;
				}
				else if (a.is_used_as_input_attachment()) {
					finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
				}
			}
			if (a.mImageUsageHintAfter.has_value()) {
				// If we detect the image usage to be more generic, we should change the layout to something more generic
				if (avk::has_flag(a.mImageUsageHintAfter.value(), avk::image_usage::sampled)) {
					finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
				}
				if (avk::has_flag(a.mImageUsageHintAfter.value(), avk::image_usage::shader_storage)) {
					finalLayout = vk::ImageLayout::eGeneral;
				}
			}
			if (vk::ImageLayout::eUndefined == finalLayout) {
				// We can just guess:
				finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			}

			if (a.shall_be_presentable()) {
				finalLayout = vk::ImageLayout::ePresentSrcKHR;
			}

			if (!initialLayoutFixed && isLoad) {
				initialLayout = finalLayout;
			}
			// ^^^ I have no idea what I'm assuming ^^^

			// 1. Create the attachment descriptions
			result.mAttachmentDescriptions.push_back(vk::AttachmentDescription{}
				.setFormat(a.format())
				.setSamples(a.sample_count())
				.setLoadOp(to_vk_load_op(a.mLoadOperation))
				.setStoreOp(to_vk_store_op(a.mStoreOperation))
				.setStencilLoadOp(to_vk_load_op(a.get_stencil_load_op()))
				.setStencilStoreOp(to_vk_store_op(a.get_stencil_store_op()))
				.setInitialLayout(initialLayout)
				.setFinalLayout(finalLayout)
			);

			const auto attachmentIndex = static_cast<uint32_t>(result.mAttachmentDescriptions.size() - 1); // Index of this attachment as used in the further subpasses

			// 2. Go throught the subpasses and gather data for subpass config
			const auto nSubpasses = a.mSubpassUsages.num_subpasses();
			if (nSubpasses != numSubpassesFirst) {
				throw avk::runtime_error("All attachments must have the exact same number of subpasses!");
			}

			// Determine and fill clear values:
			assert(result.mAttachmentDescriptions.size() == result.mClearValues.size() + 1);
			size_t spId = 0;
			while (result.mAttachmentDescriptions.size() != result.mClearValues.size() && spId < nSubpasses) {
				auto subpassUsage = a.mSubpassUsages.get_subpass_usage(spId);
				if (subpassUsage.as_color()) {
					result.mClearValues.emplace_back(vk::ClearColorValue{ a.clear_color() });
				}
				if (subpassUsage.as_depth_stencil()) {
					result.mClearValues.emplace_back(vk::ClearDepthStencilValue{ a.depth_clear_value(), a.stencil_clear_value() });
				}
				++spId;
			}
			if (result.mAttachmentDescriptions.size() != result.mClearValues.size() ) {
				result.mClearValues.emplace_back(); // just an empty clear value
			}
			assert(result.mAttachmentDescriptions.size() == result.mClearValues.size());

			for (size_t i = 0; i < nSubpasses; ++i) {
				auto& sp = subpasses[i];
				auto subpassUsage = a.mSubpassUsages.get_subpass_usage(i);
				if (subpassUsage.as_input()) {
					assert(!subpassUsage.has_resolve() || subpassUsage.as_color()); // Can not resolve input attachments, it's fine if it's also used as color attachment
					if (subpassUsage.has_input_location()) {
						auto loc = subpassUsage.input_location();
						if (sp.mSpecificInputLocations.count(loc) != 0) {
							throw avk::runtime_error("Layout location " + std::to_string(loc) + " is used multiple times for an input attachments in subpass " + std::to_string(i) + ". This is not allowed.");
						}
						sp.mSpecificInputLocations[loc] = vk::AttachmentReference{attachmentIndex, vk::ImageLayout::eShaderReadOnlyOptimal};
						sp.mInputMaxLoc = std::max(sp.mInputMaxLoc, loc);
					}
					else {
						AVK_LOG_WARNING("No layout location is specified for an input attachment in subpass " + std::to_string(i) + ". This might be problematic. Consider declaring it 'unused'.");
						sp.mUnspecifiedInputLocations.push(vk::AttachmentReference{attachmentIndex, vk::ImageLayout::eShaderReadOnlyOptimal});
					}
				}
				if (subpassUsage.as_color()) {
					auto resolve = subpassUsage.has_resolve();
					if (subpassUsage.has_color_location()) {
						auto loc = subpassUsage.color_location();
						if (sp.mSpecificColorLocations.count(loc) != 0) {
							throw avk::runtime_error("Layout location " + std::to_string(loc) + " is used multiple times for a color attachments in subpass " + std::to_string(i) + ". This is not allowed.");
						}
						sp.mSpecificColorLocations[loc] =	 vk::AttachmentReference{attachmentIndex,									vk::ImageLayout::eColorAttachmentOptimal};
						sp.mSpecificResolveLocations[loc] =	 vk::AttachmentReference{resolve ? subpassUsage.resolve_target_index() : VK_ATTACHMENT_UNUSED,	vk::ImageLayout::eColorAttachmentOptimal};
						sp.mColorMaxLoc = std::max(sp.mColorMaxLoc, loc);
					}
					else {
						AVK_LOG_WARNING("No layout location is specified for a color attachment in subpass " + std::to_string(i) + ". This might be problematic. Consider declaring it 'unused'.");
						sp.mUnspecifiedColorLocations.push(	 vk::AttachmentReference{attachmentIndex,									vk::ImageLayout::eColorAttachmentOptimal});
						sp.mUnspecifiedResolveLocations.push(vk::AttachmentReference{resolve ? subpassUsage.resolve_target_index() : VK_ATTACHMENT_UNUSED,	vk::ImageLayout::eColorAttachmentOptimal});
					}
				}
				if (subpassUsage.as_depth_stencil()) {
					assert(!subpassUsage.has_resolve() || subpassUsage.as_color()); // Can not resolve input attachments, it's fine if it's also used as color attachment // TODO: Support depth/stencil resolve by using VkSubpassDescription2
					//if (hasLoc) { // Depth/stencil attachments have no location... have they?
					//	if (sp.mSpecificDepthStencilLocations.count(loc) != 0) {
					//		throw avk::runtime_error(fmt::format("Layout location {} is used multiple times for a depth/stencil attachments in subpass {}. This is not allowed.", loc, i));
					//	}
					//	sp.mSpecificDepthStencilLocations[loc] = vk::AttachmentReference{attachmentIndex, vk::ImageLayout::eDepthStencilAttachmentOptimal};
					//	sp.mDepthStencilMaxLoc = std::max(sp.mDepthStencilMaxLoc, loc);
					//}
					sp.mUnspecifiedDepthStencilLocations.push(vk::AttachmentReference{attachmentIndex, vk::ImageLayout::eDepthStencilAttachmentOptimal});
				}
				if (subpassUsage.as_preserve()) {
					assert(!subpassUsage.has_resolve() || subpassUsage.as_color()); // Can not resolve input attachments, it's fine if it's also used as color attachment
					assert(!subpassUsage.as_input() && !subpassUsage.as_color() && !subpassUsage.as_depth_stencil()); // Makes no sense to preserve and use as something else
					sp.mPreserveAttachments.push_back(attachmentIndex);
				}
			}
		}

		// 3. Fill all the vectors in the right order:
		const auto unusedAttachmentRef = vk::AttachmentReference().setAttachment(VK_ATTACHMENT_UNUSED);
		result.mSubpassData.reserve(numSubpassesFirst);
		for (size_t i = 0; i < numSubpassesFirst; ++i) {
			auto& a = subpasses[i];
			auto& b = result.mSubpassData.emplace_back();
			assert(result.mSubpassData.size() == i + 1);
			// INPUT ATTACHMENTS
			for (int loc = 0; loc <= a.mInputMaxLoc || !a.mUnspecifiedInputLocations.empty(); ++loc) {
				if (a.mSpecificInputLocations.count(loc) > 0) {
					assert (a.mSpecificInputLocations.count(loc) == 1);
					b.mOrderedInputAttachmentRefs.push_back(a.mSpecificInputLocations[loc]);
				}
				else {
					if (!a.mUnspecifiedInputLocations.empty()) {
						b.mOrderedInputAttachmentRefs.push_back(a.mUnspecifiedInputLocations.front());
						a.mUnspecifiedInputLocations.pop();
					}
					else {
						b.mOrderedInputAttachmentRefs.push_back(unusedAttachmentRef);
					}
				}
			}
			// COLOR ATTACHMENTS
			for (int loc = 0; loc <= a.mColorMaxLoc || !a.mUnspecifiedColorLocations.empty(); ++loc) {
				if (a.mSpecificColorLocations.count(loc) > 0) {
					assert (a.mSpecificColorLocations.count(loc) == 1);
					assert (a.mSpecificResolveLocations.count(loc) == 1);
					b.mOrderedColorAttachmentRefs.push_back(a.mSpecificColorLocations[loc]);
					b.mOrderedResolveAttachmentRefs.push_back(a.mSpecificResolveLocations[loc]);
				}
				else {
					if (!a.mUnspecifiedColorLocations.empty()) {
						assert(a.mUnspecifiedColorLocations.size() == a.mUnspecifiedResolveLocations.size());
						b.mOrderedColorAttachmentRefs.push_back(a.mUnspecifiedColorLocations.front());
						a.mUnspecifiedColorLocations.pop();
						b.mOrderedResolveAttachmentRefs.push_back(a.mUnspecifiedResolveLocations.front());
						a.mUnspecifiedResolveLocations.pop();
					}
					else {
						b.mOrderedColorAttachmentRefs.push_back(unusedAttachmentRef);
						b.mOrderedResolveAttachmentRefs.push_back(unusedAttachmentRef);
					}
				}
			}
			// DEPTH/STENCIL ATTACHMENTS
			for (int loc = 0; loc <= a.mDepthStencilMaxLoc || !a.mUnspecifiedDepthStencilLocations.empty(); ++loc) {
				if (a.mSpecificDepthStencilLocations.count(loc) > 0) {
					assert (a.mSpecificDepthStencilLocations.count(loc) == 1);
					b.mOrderedDepthStencilAttachmentRefs.push_back(a.mSpecificDepthStencilLocations[loc]);
				}
				else {
					if (!a.mUnspecifiedDepthStencilLocations.empty()) {
						b.mOrderedDepthStencilAttachmentRefs.push_back(a.mUnspecifiedDepthStencilLocations.front());
						a.mUnspecifiedDepthStencilLocations.pop();
					}
					else {
						b.mOrderedDepthStencilAttachmentRefs.push_back(unusedAttachmentRef);
					}
				}
			}
			b.mPreserveAttachments = std::move(a.mPreserveAttachments);

			// SOME SANITY CHECKS:
			// - The resolve attachments must either be empty or there must be a entry for each color attachment
			assert(b.mOrderedResolveAttachmentRefs.empty() || b.mOrderedResolveAttachmentRefs.size() == b.mOrderedColorAttachmentRefs.size());
			// - There must not be more than 1 depth/stencil attachements
			assert(b.mOrderedDepthStencilAttachmentRefs.size() <= 1);
		}

		// Done with the helper structure:
		subpasses.clear();

		// 4. Now we can fill the subpass description
		rewire_subpass_descriptions(result);

		// ======== Regarding Subpass Dependencies ==========
		// At this point, we can not know how a subpass shall
		// be synchronized exactly with whatever comes before
		// and whatever comes after.
		//  => Let's establish very (overly) cautious dependencies to ensure correctness, but user can set more tight sync via the callback

		const uint32_t firstSubpassId = 0u;
		const uint32_t lastSubpassId = numSubpassesFirst - 1;
		const auto addDependency = [&result](renderpass_sync& rps){
			result.mSubpassDependencies.push_back(vk::SubpassDependency()
				// Between which two subpasses is this dependency:
				.setSrcSubpass(rps.source_vk_subpass_id())
				.setDstSubpass(rps.destination_vk_subpass_id())
				// Which stage from whatever comes before are we waiting on, and which operations from whatever comes before are we waiting on:
				.setSrcStageMask(to_vk_pipeline_stage_flags(rps.mSourceStage))
				.setSrcAccessMask(to_vk_access_flags(to_memory_access(rps.mSourceMemoryDependency)))
				// Which stage and which operations of our subpass ZERO shall wait:
				.setDstStageMask(to_vk_pipeline_stage_flags(rps.mDestinationStage))
				.setDstAccessMask(to_vk_access_flags(rps.mDestinationMemoryDependency))
			);
		};

		{
			renderpass_sync syncBefore {renderpass_sync::sExternal, static_cast<int>(firstSubpassId),
				pipeline_stage::all_commands,			memory_access::any_write_access,
				pipeline_stage::all_graphics,	        memory_access::any_graphics_read_access | memory_access::any_graphics_basic_write_access
			};
			// Let the user modify this sync
			if (aSync) {
				aSync(syncBefore);
			}
			assert(syncBefore.source_vk_subpass_id() == VK_SUBPASS_EXTERNAL);
			assert(syncBefore.destination_vk_subpass_id() == 0u);
			addDependency(syncBefore);
		}

		// Iterate over all combinations of [id, id] and [id, id+1]
		for (auto i = firstSubpassId, j = firstSubpassId; j <= lastSubpassId; i += (j-i), j += static_cast<uint32_t>(i==j)) {

			// Default to ALL_GRAPHICS -> ALL_GRAPHICS sync between different sub passes,
			// but default to NO SYNC for subpass self-dependencies, and leave them out if not set by the user
			// (because subpass self-dependency rules have stricter rules to follow: https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#synchronization-pipeline-barriers-subpass-self-dependencies)
			renderpass_sync syncBetween {static_cast<int>(i), static_cast<int>(j),
				pipeline_stage::all_graphics,	memory_access::any_graphics_basic_write_access,
				pipeline_stage::all_graphics,	memory_access::any_graphics_read_access | memory_access::any_graphics_basic_write_access,
			};
			if (i == j) {
				syncBetween.mSourceStage = pipeline_stage::top_of_pipe;
				syncBetween.mSourceMemoryDependency = std::nullopt;
				syncBetween.mDestinationStage = pipeline_stage::bottom_of_pipe;
				syncBetween.mDestinationMemoryDependency = std::nullopt;
			}

			// Let the user modify this sync
			if (aSync) {
				aSync(syncBetween);
			}
			assert(syncBetween.source_vk_subpass_id() == i);
			assert(syncBetween.destination_vk_subpass_id() == j);

			if (i != j // or else -- if it is a self dependency -- if the user has set some values:
				|| syncBetween.mSourceStage != pipeline_stage::top_of_pipe
				|| syncBetween.mSourceMemoryDependency.has_value()
				|| syncBetween.mDestinationStage != pipeline_stage::bottom_of_pipe
				|| syncBetween.mDestinationMemoryDependency.has_value()) {
				addDependency(syncBetween);
			}
		}

		{
			renderpass_sync syncAfter {static_cast<int>(lastSubpassId), renderpass_sync::sExternal,
				pipeline_stage::all_graphics,	        memory_access::any_graphics_basic_write_access,
				pipeline_stage::all_commands,			memory_access::any_read_access
			};
			// Let the user modify this sync
			if (aSync) {
				aSync(syncAfter);
			}
			assert(syncAfter.source_vk_subpass_id() == lastSubpassId);
			assert(syncAfter.destination_vk_subpass_id() == VK_SUBPASS_EXTERNAL);
			addDependency(syncAfter);
		}

		assert(result.mSubpassDependencies.size() == numSubpassesFirst + 1);

		// Maybe alter the config?!
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(result);
		}

		// Finally, create the render pass
		auto createInfo = vk::RenderPassCreateInfo()
			.setAttachmentCount(static_cast<uint32_t>(result.mAttachmentDescriptions.size()))
			.setPAttachments(result.mAttachmentDescriptions.data())
			.setSubpassCount(static_cast<uint32_t>(result.mSubpasses.size()))
			.setPSubpasses(result.mSubpasses.data())
			.setDependencyCount(static_cast<uint32_t>(result.mSubpassDependencies.size()))
			.setPDependencies(result.mSubpassDependencies.data());
		result.mRenderPass = device().createRenderPassUnique(createInfo);
		return result;

		// TODO: Support VkSubpassDescriptionDepthStencilResolveKHR in order to enable resolve-settings for the depth attachment (see [1] and [2] for more details)

		// References:
		// [1] https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkSubpassDescription.html
		// [2] https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkSubpassDescriptionDepthStencilResolveKHR.html
		// [3] https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkPipelineStageFlagBits.html
	}

	renderpass root::create_renderpass_from_template(resource_reference<const renderpass_t> aTemplate, std::function<void(renderpass_t&)> aAlterConfigBeforeCreation)
	{
		renderpass_t result;
		result.mAttachmentDescriptions = aTemplate->mAttachmentDescriptions;
		result.mClearValues			   = aTemplate->mClearValues			  ;
		result.mSubpassData			   = aTemplate->mSubpassData			  ;
		rewire_subpass_descriptions(result); // this will set result.mSubpasses
		result.mSubpassDependencies	   = aTemplate->mSubpassDependencies	  ;

		// Maybe alter the config?!
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(result);
		}

		// Finally, create the render pass
		auto createInfo = vk::RenderPassCreateInfo()
			.setAttachmentCount(static_cast<uint32_t>(result.mAttachmentDescriptions.size()))
			.setPAttachments(result.mAttachmentDescriptions.data())
			.setSubpassCount(static_cast<uint32_t>(result.mSubpasses.size()))
			.setPSubpasses(result.mSubpasses.data())
			.setDependencyCount(static_cast<uint32_t>(result.mSubpassDependencies.size()))
			.setPDependencies(result.mSubpassDependencies.data());
		result.mRenderPass = device().createRenderPassUnique(createInfo);
		return result;
	}

	bool renderpass_t::is_input_attachment(uint32_t aSubpassId, size_t aAttachmentIndex) const
	{
		assert(aSubpassId < mSubpassData.size());
		auto& b = mSubpassData[aSubpassId];
		assert(aAttachmentIndex < mAttachmentDescriptions.size());
		return b.mOrderedInputAttachmentRefs.end() != std::find_if(std::begin(b.mOrderedInputAttachmentRefs), std::end(b.mOrderedInputAttachmentRefs),
			[aAttachmentIndex](const vk::AttachmentReference& ref) { return ref.attachment == aAttachmentIndex; });
	}

	bool renderpass_t::is_color_attachment(uint32_t aSubpassId, size_t aAttachmentIndex) const
	{
		assert(aSubpassId < mSubpassData.size());
		auto& b = mSubpassData[aSubpassId];
		assert(aAttachmentIndex < mAttachmentDescriptions.size());
		return b.mOrderedColorAttachmentRefs.end() != std::find_if(std::begin(b.mOrderedColorAttachmentRefs), std::end(b.mOrderedColorAttachmentRefs),
			[aAttachmentIndex](const vk::AttachmentReference& ref) { return ref.attachment == aAttachmentIndex; });
	}

	bool renderpass_t::is_depth_stencil_attachment(uint32_t aSubpassId, size_t aAttachmentIndex) const
	{
		assert(aSubpassId < mSubpassData.size());
		auto& b = mSubpassData[aSubpassId];
		assert(aAttachmentIndex < mAttachmentDescriptions.size());
		return b.mOrderedDepthStencilAttachmentRefs.end() != std::find_if(std::begin(b.mOrderedDepthStencilAttachmentRefs), std::end(b.mOrderedDepthStencilAttachmentRefs),
			[aAttachmentIndex](const vk::AttachmentReference& ref) { return ref.attachment == aAttachmentIndex; });
	}

	bool renderpass_t::is_resolve_attachment(uint32_t aSubpassId, size_t aAttachmentIndex) const
	{
		assert(aSubpassId < mSubpassData.size());
		auto& b = mSubpassData[aSubpassId];
		assert(aAttachmentIndex < mAttachmentDescriptions.size());
		return b.mOrderedResolveAttachmentRefs.end() != std::find_if(std::begin(b.mOrderedResolveAttachmentRefs), std::end(b.mOrderedResolveAttachmentRefs),
			[aAttachmentIndex](const vk::AttachmentReference& ref) { return ref.attachment == aAttachmentIndex; });
	}

	bool renderpass_t::is_preserve_attachment(uint32_t aSubpassId, size_t aAttachmentIndex) const
	{
		assert(aSubpassId < mSubpassData.size());
		auto& b = mSubpassData[aSubpassId];
		assert(aAttachmentIndex < mAttachmentDescriptions.size());
		return b.mPreserveAttachments.end() != std::find_if(std::begin(b.mPreserveAttachments), std::end(b.mPreserveAttachments),
			[aAttachmentIndex](uint32_t idx) { return idx == aAttachmentIndex; });
	}

	const std::vector<vk::AttachmentReference>& renderpass_t::input_attachments_for_subpass(uint32_t aSubpassId)
	{
		assert(aSubpassId < mSubpassData.size());
		auto& b = mSubpassData[aSubpassId];
		return b.mOrderedInputAttachmentRefs;
	}

	const std::vector<vk::AttachmentReference>& renderpass_t::color_attachments_for_subpass(uint32_t aSubpassId)
	{
		assert(aSubpassId < mSubpassData.size());
		auto& b = mSubpassData[aSubpassId];
		return b.mOrderedColorAttachmentRefs;
	}

	const std::vector<vk::AttachmentReference>& renderpass_t::depth_stencil_attachments_for_subpass(uint32_t aSubpassId)
	{
		assert(aSubpassId < mSubpassData.size());
		auto& b = mSubpassData[aSubpassId];
		return b.mOrderedDepthStencilAttachmentRefs;
	}

	const std::vector<vk::AttachmentReference>& renderpass_t::resolve_attachments_for_subpass(uint32_t aSubpassId)
	{
		assert(aSubpassId < mSubpassData.size());
		auto& b = mSubpassData[aSubpassId];
		return b.mOrderedResolveAttachmentRefs;
	}

	const std::vector<uint32_t>& renderpass_t::preserve_attachments_for_subpass(uint32_t aSubpassId)
	{
		assert(aSubpassId < mSubpassData.size());
		auto& b = mSubpassData[aSubpassId];
		return b.mPreserveAttachments;
	}
#pragma endregion

#pragma region semaphore definitions
	semaphore_t::semaphore_t()
		: mCreateInfo{}
		, mSemaphore{}
		, mSemaphoreWaitStageForNextCommand{ vk::PipelineStageFlagBits::eAllCommands }
		, mCustomDeleter{}
	{
	}

	semaphore_t::~semaphore_t()
	{
		if (mCustomDeleter.has_value() && *mCustomDeleter) {
			// If there is a custom deleter => call it now
			(*mCustomDeleter)();
			mCustomDeleter.reset();
		}
		// Destroy the dependant instance before destroying myself
		// ^ This is ensured by the order of the members
		//   See: https://isocpp.org/wiki/faq/dtors#calling-member-dtors
	}

	semaphore_t& semaphore_t::set_semaphore_wait_stage(vk::PipelineStageFlags _Stage)
	{
		mSemaphoreWaitStageForNextCommand = _Stage;
		return *this;
	}

	semaphore root::create_semaphore(vk::Device aDevice, std::function<void(semaphore_t&)> aAlterConfigBeforeCreation)
	{
		semaphore_t result;
		result.mCreateInfo = vk::SemaphoreCreateInfo{};

		// Maybe alter the config?
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(result);
		}

		result.mSemaphore = aDevice.createSemaphoreUnique(result.mCreateInfo);
		return result;
	}

	semaphore root::create_semaphore(std::function<void(semaphore_t&)> aAlterConfigBeforeCreation)
	{
		return create_semaphore(device(), std::move(aAlterConfigBeforeCreation));
	}
#pragma endregion

#pragma region shader definitions
	shader shader::prepare(shader_info pInfo)
	{
		shader result;
		result.mInfo = std::move(pInfo);
		return result;
	}

	vk::UniqueShaderModule root::build_shader_module_from_binary_code(const std::vector<char>& aCode)
	{
		auto createInfo = vk::ShaderModuleCreateInfo()
			.setCodeSize(aCode.size())
			.setPCode(reinterpret_cast<const uint32_t*>(aCode.data()));

		return device().createShaderModuleUnique(createInfo);
	}

	vk::UniqueShaderModule root::build_shader_module_from_file(const std::string& aPath)
	{
		auto binFileContents = avk::load_binary_file(aPath);
		return build_shader_module_from_binary_code(binFileContents);
	}

	shader root::create_shader(shader_info aInfo)
	{
		auto shdr = shader::prepare(std::move(aInfo));

		if (std::filesystem::exists(shdr.info().mPath)) {
			try {
				shdr.mShaderModule = build_shader_module_from_file(shdr.info().mPath);
				shdr.mActualShaderLoadPath = shdr.info().mPath;
				return shdr;
			}
			catch (avk::runtime_error&) {
			}
		}

		const std::string secondTry = shdr.info().mPath + ".spv";
		shdr.mShaderModule = build_shader_module_from_file(secondTry);
		AVK_LOG_INFO("Couldn't load '" + shdr.info().mPath + "' but loading '" + secondTry + "' was successful => going to use the latter, fyi!");
		shdr.mActualShaderLoadPath = secondTry;

		return shdr;
	}

	shader root::create_shader_from_template(const shader& aTemplate)
	{
		return create_shader(aTemplate.info());
	}

	bool shader::has_been_built() const
	{
		return static_cast<bool>(mShaderModule);
	}

	shader_info shader_info::describe(std::string pPath, std::string pEntryPoint, bool pDontMonitorFile, std::optional<shader_type> pShaderType)
	{
		pPath = trim_spaces(pPath);
		if (!pShaderType.has_value()) {
			// "classical" shaders
			if (pPath.ends_with(".vert"))	{ pShaderType = shader_type::vertex; }
			else if (pPath.ends_with(".tesc"))	{ pShaderType = shader_type::tessellation_control; }
			else if (pPath.ends_with(".tese"))	{ pShaderType = shader_type::tessellation_evaluation; }
			else if (pPath.ends_with(".geom"))	{ pShaderType = shader_type::geometry; }
			else if (pPath.ends_with(".frag"))	{ pShaderType = shader_type::fragment; }
			else if (pPath.ends_with(".comp"))	{ pShaderType = shader_type::compute; }
			// ray tracing shaders
			else if (pPath.ends_with(".rgen"))	{ pShaderType = shader_type::ray_generation; }
			else if (pPath.ends_with(".rahit"))	{ pShaderType = shader_type::any_hit; }
			else if (pPath.ends_with(".rchit"))	{ pShaderType = shader_type::closest_hit; }
			else if (pPath.ends_with(".rmiss"))	{ pShaderType = shader_type::miss; }
			else if (pPath.ends_with(".rint"))	{ pShaderType = shader_type::intersection; }
			// callable shader
			else if (pPath.ends_with(".call"))	{ pShaderType = shader_type::callable; }
			// mesh shaders
			else if (pPath.ends_with(".task"))	{ pShaderType = shader_type::task; }
			else if (pPath.ends_with(".mesh"))	{ pShaderType = shader_type::mesh; }
		}

		if (!pShaderType.has_value()) {
			throw avk::runtime_error("No shader type set and could not infer it from the file ending.");
		}

		return shader_info
		{
			std::move(pPath),
			pShaderType.value(),
			std::move(pEntryPoint),
			pDontMonitorFile
		};
	}
#pragma endregion

#pragma region vk_utils2 definitions
	std::optional<command_buffer> copy_image_to_another(avk::resource_reference<image_t> aSrcImage, avk::resource_reference<image_t> aDstImage, sync aSyncHandler, bool aRestoreSrcLayout, bool aRestoreDstLayout)
	{
		const auto originalSrcLayout = aSrcImage->target_layout();
		const auto originalDstLayout = aDstImage->target_layout();

		auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
		// Sync before:
		aSyncHandler.establish_barrier_before_the_operation(pipeline_stage::transfer, read_memory_access{memory_access::transfer_read_access});

		// Citing the specs: "srcImageLayout must be VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, or VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR"
		const auto srcLayoutAfterBarrier = aSrcImage->current_layout();
		const bool suitableSrcLayout = srcLayoutAfterBarrier == vk::ImageLayout::eTransferSrcOptimal; // For optimal performance, only allow eTransferSrcOptimal
									//|| initialSrcLayout == vk::ImageLayout::eGeneral
									//|| initialSrcLayout == vk::ImageLayout::eSharedPresentKHR;
		if (suitableSrcLayout) {
			// Just make sure that is really is in target layout:
			aSrcImage->transition_to_layout({}, sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
		}
		else {
			// Not a suitable src layout => must transform
			aSrcImage->transition_to_layout(vk::ImageLayout::eTransferSrcOptimal, sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
		}

		// Citing the specs: "dstImageLayout must be VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, or VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR"
		const auto dstLayoutAfterBarrier = aDstImage->current_layout();
		const bool suitableDstLayout = dstLayoutAfterBarrier == vk::ImageLayout::eTransferDstOptimal;
									//|| dstLayoutAfterBarrier == vk::ImageLayout::eGeneral
									//|| dstLayoutAfterBarrier == vk::ImageLayout::eSharedPresentKHR;
		if (suitableDstLayout) {
			// Just make sure that is really is in target layout:
			aDstImage->transition_to_layout({}, sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
		}
		else {
			// Not a suitable dst layout => must transform
			aDstImage->transition_to_layout(vk::ImageLayout::eTransferDstOptimal, sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
		}

		// Operation:
		auto copyRegion = vk::ImageCopy{}
			.setExtent(aSrcImage->config().extent) // TODO: Support different ranges/extents
			.setSrcOffset({0, 0})
			.setSrcSubresource(vk::ImageSubresourceLayers{} // TODO: Add support for the other parameters
				.setAspectMask(aSrcImage->aspect_flags())
				.setBaseArrayLayer(0u)
				.setLayerCount(1u)
				.setMipLevel(0u)
			)
			.setDstOffset({0, 0})
			.setDstSubresource(vk::ImageSubresourceLayers{} // TODO: Add support for the other parameters
				.setAspectMask(aDstImage->aspect_flags())
				.setBaseArrayLayer(0u)
				.setLayerCount(1u)
				.setMipLevel(0u));

		commandBuffer.handle().copyImage(
			aSrcImage->handle(),
			aSrcImage->current_layout(),
			aDstImage->handle(),
			aDstImage->current_layout(),
			1u, &copyRegion);

		if (aRestoreSrcLayout) { // => restore original layout of the src image
			aSrcImage->transition_to_layout(originalSrcLayout, sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
		}

		if (aRestoreDstLayout) { // => restore original layout of the dst image
			aDstImage->transition_to_layout(originalDstLayout, sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
		}

		// Sync after:
		aSyncHandler.establish_barrier_after_the_operation(pipeline_stage::transfer, write_memory_access{memory_access::transfer_write_access});

		// Finish him:
		return aSyncHandler.submit_and_sync();
	}

	std::optional<command_buffer> blit_image(avk::resource_reference<image_t> aSrcImage, avk::resource_reference<image_t> aDstImage, sync aSyncHandler, bool aRestoreSrcLayout, bool aRestoreDstLayout)
	{
		const auto originalSrcLayout = aSrcImage->target_layout();
		const auto originalDstLayout = aDstImage->target_layout();

		auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
		// Sync before:
		aSyncHandler.establish_barrier_before_the_operation(pipeline_stage::transfer, read_memory_access{memory_access::transfer_read_access});

		// Citing the specs: "srcImageLayout must be VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, or VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR"
		const auto srcLayoutAfterBarrier = aSrcImage->current_layout();
		const bool suitableSrcLayout = srcLayoutAfterBarrier == vk::ImageLayout::eTransferSrcOptimal; // For optimal performance, only allow eTransferSrcOptimal
									//|| initialSrcLayout == vk::ImageLayout::eGeneral
									//|| initialSrcLayout == vk::ImageLayout::eSharedPresentKHR;
		if (suitableSrcLayout) {
			// Just make sure that is really is in target layout:
			aSrcImage->transition_to_layout({}, sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
		}
		else {
			// Not a suitable src layout => must transform
			aSrcImage->transition_to_layout(vk::ImageLayout::eTransferSrcOptimal, sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
		}

		// Citing the specs: "dstImageLayout must be VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, or VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR"
		const auto dstLayoutAfterBarrier = aDstImage->current_layout();
		const bool suitableDstLayout = dstLayoutAfterBarrier == vk::ImageLayout::eTransferDstOptimal;
									//|| dstLayoutAfterBarrier == vk::ImageLayout::eGeneral
									//|| dstLayoutAfterBarrier == vk::ImageLayout::eSharedPresentKHR;
		if (suitableDstLayout) {
			// Just make sure that is really is in target layout:
			aDstImage->transition_to_layout({}, sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
		}
		else {
			// Not a suitable dst layout => must transform
			aDstImage->transition_to_layout(vk::ImageLayout::eTransferDstOptimal, sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
		}


		std::array<vk::Offset3D, 2> srcOffsets = { vk::Offset3D{ 0, 0, 0 }, vk::Offset3D{ static_cast<int32_t>(aSrcImage->width()), static_cast<int32_t>(aSrcImage->height()), 1 } };
		std::array<vk::Offset3D, 2> dstOffsets = { vk::Offset3D{ 0, 0, 0 }, vk::Offset3D{ static_cast<int32_t>(aDstImage->width()), static_cast<int32_t>(aDstImage->height()), 1 } };

		// Operation:
		auto blitRegion = vk::ImageBlit{}
			.setSrcSubresource(vk::ImageSubresourceLayers{} // TODO: Add support for the other parameters
				.setAspectMask(aSrcImage->aspect_flags())
				.setBaseArrayLayer(0u)
				.setLayerCount(1u)
				.setMipLevel(0u)
			)
			.setSrcOffsets(srcOffsets)
			.setDstSubresource(vk::ImageSubresourceLayers{} // TODO: Add support for the other parameters
				.setAspectMask(aDstImage->aspect_flags())
				.setBaseArrayLayer(0u)
				.setLayerCount(1u)
				.setMipLevel(0u)
			)
			.setDstOffsets(dstOffsets);

		commandBuffer.handle().blitImage(
			aSrcImage->handle(),
			aSrcImage->current_layout(),
			aDstImage->handle(),
			aDstImage->current_layout(),
			1u, &blitRegion,
			vk::Filter::eNearest); // TODO: Support other filters and everything

		if (aRestoreSrcLayout) { // => restore original layout of the src image
			aSrcImage->transition_to_layout(originalSrcLayout, sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
		}

		if (aRestoreDstLayout) { // => restore original layout of the dst image
			aDstImage->transition_to_layout(originalDstLayout, sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
		}

		// Sync after:
		aSyncHandler.establish_barrier_after_the_operation(pipeline_stage::transfer, write_memory_access{memory_access::transfer_write_access});

		// Finish him:
		return aSyncHandler.submit_and_sync();
	}

	std::optional<command_buffer> copy_buffer_to_image_layer_mip_level(resource_reference<const buffer_t> aSrcBuffer, resource_reference<image_t> aDstImage, uint32_t aDstLayer, uint32_t aDstLevel, std::optional<vk::ImageAspectFlags> aAspectFlagsOverride, sync aSyncHandler)
	{

		auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
		// Sync before:
		aSyncHandler.establish_barrier_before_the_operation(pipeline_stage::transfer, read_memory_access{memory_access::transfer_read_access});

		auto extent = aDstImage->config().extent;
		extent.width  = extent.width  > 1u ? extent.width  >> aDstLevel : 1u;
		extent.height = extent.height > 1u ? extent.height >> aDstLevel : 1u;
		extent.depth  = extent.depth  > 1u ? extent.depth  >> aDstLevel : 1u;

		// Operation:
		auto copyRegion = vk::BufferImageCopy()
			.setBufferOffset(0)
			// The bufferRowLength and bufferImageHeight fields specify how the pixels are laid out in memory. For example, you could have some padding 
			// bytes between rows of the image. Specifying 0 for both indicates that the pixels are simply tightly packed like they are in our case. [3]
			.setBufferRowLength(0)
			.setBufferImageHeight(0)
			.setImageSubresource(vk::ImageSubresourceLayers()
				.setAspectMask(aAspectFlagsOverride.value_or(aDstImage->aspect_flags())) // Used to be vk::ImageAspectFlagBits::eColor
				.setMipLevel(aDstLevel)
				.setBaseArrayLayer(aDstLayer)
				.setLayerCount(1u))
			.setImageOffset({ 0u, 0u, 0u })
			.setImageExtent(extent);
		commandBuffer.handle().copyBufferToImage(
			aSrcBuffer->handle(),
			aDstImage->handle(),
			vk::ImageLayout::eTransferDstOptimal, // TODO: Should image layout transitions be handled somehow automatically or so? If not => Document that this function expects the image to be in eTransferDstOptimal Layout.
			{ copyRegion });

		// Sync after:
		aSyncHandler.establish_barrier_after_the_operation(pipeline_stage::transfer, write_memory_access{memory_access::transfer_write_access});

		// Finish him:
		return aSyncHandler.submit_and_sync();
	}

	std::optional<command_buffer> copy_buffer_to_image_mip_level(resource_reference<const buffer_t> aSrcBuffer, resource_reference<image_t> aDstImage, uint32_t aDstLevel, std::optional<vk::ImageAspectFlags> aAspectFlagsOverride, sync aSyncHandler)
	{
		return copy_buffer_to_image_layer_mip_level(std::move(aSrcBuffer), std::move(aDstImage), 0u, aDstLevel, aAspectFlagsOverride, std::move(aSyncHandler));
	}

	std::optional<command_buffer> copy_buffer_to_image(resource_reference<const buffer_t> aSrcBuffer, resource_reference<image_t> aDstImage, std::optional<vk::ImageAspectFlags> aAspectFlagsOverride, sync aSyncHandler)
	{
		return copy_buffer_to_image_mip_level(std::move(aSrcBuffer), std::move(aDstImage), 0u, aAspectFlagsOverride, std::move(aSyncHandler));
	}

	std::optional<command_buffer> copy_buffer_to_another(avk::resource_reference<buffer_t> aSrcBuffer, avk::resource_reference<buffer_t> aDstBuffer, std::optional<vk::DeviceSize> aSrcOffset, std::optional<vk::DeviceSize> aDstOffset, std::optional<vk::DeviceSize> aDataSize, sync aSyncHandler)
	{
		auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
		// Sync before:
		aSyncHandler.establish_barrier_before_the_operation(pipeline_stage::transfer, read_memory_access{memory_access::transfer_read_access});

		vk::DeviceSize dataSize{0};
		if (aDataSize.has_value()) {
			dataSize = aDataSize.value();
		}
		else {
			dataSize = aSrcBuffer->meta_at_index<buffer_meta>().total_size();
		}

#ifdef _DEBUG
		{
			const auto& metaDataSrc = aSrcBuffer->meta_at_index<buffer_meta>();
			const auto& metaDataDst = aDstBuffer->meta_at_index<buffer_meta>();
			assert (aSrcOffset.value_or(0) + dataSize <= metaDataSrc.total_size());
			assert (aDstOffset.value_or(0) + dataSize <= metaDataDst.total_size());
			assert (aSrcOffset.value_or(0) + dataSize <= metaDataDst.total_size());
		}
#endif

		auto copyRegion = vk::BufferCopy{}
			.setSrcOffset(aSrcOffset.value_or(0))
			.setDstOffset(aDstOffset.value_or(0))
			.setSize(dataSize);
		commandBuffer.handle().copyBuffer(aSrcBuffer->handle(), aDstBuffer->handle(), 1u, &copyRegion);

		// Sync after:
		aSyncHandler.establish_barrier_after_the_operation(pipeline_stage::transfer, write_memory_access{memory_access::transfer_write_access});
		// Finish him:
		return aSyncHandler.submit_and_sync();
	}

	std::optional<command_buffer> copy_image_mip_level_to_buffer(avk::resource_reference<image_t> aSrcImage, uint32_t aSrcLevel, avk::resource_reference<buffer_t> aDstBuffer, std::optional<vk::ImageAspectFlags> aAspectFlagsOverride, sync aSyncHandler, bool aRestoreSrcLayout)
	{
		const auto originalSrcLayout = aSrcImage->target_layout();

		auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
		// Sync before:
		aSyncHandler.establish_barrier_before_the_operation(pipeline_stage::transfer, read_memory_access{memory_access::transfer_read_access});

		// Citing the specs: "srcImageLayout must be VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, or VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR"
		const auto srcLayoutAfterBarrier = aSrcImage->current_layout();
		const bool suitableSrcLayout = srcLayoutAfterBarrier == vk::ImageLayout::eTransferSrcOptimal; // For optimal performance, only allow eTransferSrcOptimal
									//|| initialSrcLayout == vk::ImageLayout::eGeneral
									//|| initialSrcLayout == vk::ImageLayout::eSharedPresentKHR;
		if (suitableSrcLayout) {
			// Just make sure that is really is in target layout:
			aSrcImage->transition_to_layout({}, sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
		}
		else {
			// Not a suitable src layout => must transform
			aSrcImage->transition_to_layout(vk::ImageLayout::eTransferSrcOptimal, sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
		}

		auto extent = aSrcImage->config().extent;
		extent.width  = extent.width  > 1u ? extent.width  >> aSrcLevel : 1u;
		extent.height = extent.height > 1u ? extent.height >> aSrcLevel : 1u;
		extent.depth  = extent.depth  > 1u ? extent.depth  >> aSrcLevel : 1u;

		// Operation:
		auto copyRegion = vk::BufferImageCopy()
			.setBufferOffset(0)
			.setBufferRowLength(0)
			.setBufferImageHeight(0)
			.setImageSubresource(vk::ImageSubresourceLayers()
				.setAspectMask(aAspectFlagsOverride.value_or(aSrcImage->aspect_flags()))
				.setMipLevel(aSrcLevel)
				.setBaseArrayLayer(0u)
				.setLayerCount(1u))
			.setImageOffset({ 0u, 0u, 0u })
			.setImageExtent(extent);
		commandBuffer.handle().copyImageToBuffer(
			aSrcImage->handle(),
			vk::ImageLayout::eTransferSrcOptimal, // TODO: Should image layout transitions be handled somehow automatically or so? If not => Document that this function expects the image to be in eTransferSrcOptimal Layout.
			aDstBuffer->handle(),
			{ copyRegion });

		if (aRestoreSrcLayout) { // => restore original layout of the src image
			aSrcImage->transition_to_layout(originalSrcLayout, sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
		}

		// Sync after:
		aSyncHandler.establish_barrier_after_the_operation(pipeline_stage::transfer, write_memory_access{memory_access::transfer_write_access});

		// Finish him:
		return aSyncHandler.submit_and_sync();
	}

	std::optional<command_buffer> copy_image_to_buffer(avk::resource_reference<image_t> aSrcImage, avk::resource_reference<buffer_t> aDstBuffer, std::optional<vk::ImageAspectFlags> aAspectFlagsOverride, sync aSyncHandler, bool aRestoreSrcLayout)
	{
		return copy_image_mip_level_to_buffer(std::move(aSrcImage), 0u, std::move(aDstBuffer), aAspectFlagsOverride, std::move(aSyncHandler), aRestoreSrcLayout);
	}
#pragma endregion

#pragma region pipeline_barrier_data definitions
	void pipeline_barrier_data::make_barrier(command_buffer_t& aIntoCommandBuffer) const
	{
		std::vector<vk::BufferMemoryBarrier> bmbs;
		for (auto& data : mBufferMemoryBarriers) {
			bmbs.emplace_back()
				.setBuffer(data.mBufferRef->handle())
				.setSrcQueueFamilyIndex(data.mSrcQueue.value()->family_index())
				.setDstQueueFamilyIndex(data.mDstQueue.value()->family_index())
				.setOffset(0)
				.setSize(data.mBufferRef->meta_at_index<buffer_meta>().total_size());
		}

		aIntoCommandBuffer.handle().pipelineBarrier(
			to_vk_pipeline_stage_flags(mSrcStage.value()),
			to_vk_pipeline_stage_flags(mDstStage.value()),
			vk::DependencyFlags{},										// TODO: support dependency flags
			0u, nullptr,
			static_cast<uint32_t>(bmbs.size()), bmbs.data(),
			0u, nullptr
		);
	}
#pragma endregion

#pragma region query pool and query definitions
	query_pool root::create_query_pool(vk::QueryType aQueryType, uint32_t aQueryCount, vk::QueryPipelineStatisticFlags aPipelineStatistics)
	{
		query_pool_t result;

		result.mCreateInfo = vk::QueryPoolCreateInfo{}
			.setQueryType( aQueryType )
			.setQueryCount( aQueryCount )
			.setPipelineStatistics( aPipelineStatistics );
		result.mQueryPool = device().createQueryPoolUnique(result.mCreateInfo);

		return result;
	}

	query_pool root::create_query_pool_for_occlusion_queries(uint32_t aQueryCount)
	{
		return create_query_pool(vk::QueryType::eOcclusion, aQueryCount);
	}

	query_pool root::create_query_pool_for_timestamp_queries(uint32_t aQueryCount)
	{
		return create_query_pool(vk::QueryType::eTimestamp, aQueryCount);
	}

	query_pool root::create_query_pool_for_pipeline_statistics_queries(uint32_t aQueryCount, vk::QueryPipelineStatisticFlags aPipelineStatistics)
	{
		return create_query_pool(vk::QueryType::ePipelineStatistics, aQueryCount);
	}

	void query_pool_t::host_reset(uint32_t aFirstQueryIndex, std::optional<uint32_t> aNumQueries)
	{
		mQueryPool.getOwner().resetQueryPool(handle(), aFirstQueryIndex, aNumQueries.value_or(create_info().queryCount - aFirstQueryIndex));
	}

	std::optional<command_buffer> query_pool_t::reset(uint32_t aFirstQueryIndex, std::optional<uint32_t> aNumQueries, avk::sync aSync)
	{
		auto& cmdBfr = aSync.get_or_create_command_buffer();
		cmdBfr.handle().resetQueryPool(handle(), aFirstQueryIndex, aNumQueries.value_or(create_info().queryCount - aFirstQueryIndex));
		return aSync.submit_and_sync();
	}

	std::optional<command_buffer> query_pool_t::write_timestamp(uint32_t aQueryIndex, pipeline_stage aTimestampStage, sync aSync)
	{
		typedef std::underlying_type<pipeline_stage>::type EnumType;
		assert( std::bitset<32>{ static_cast<EnumType>(aTimestampStage) }.count() == 1 );

		auto& cmdBfr = aSync.get_or_create_command_buffer();
		cmdBfr.handle().writeTimestamp(to_vk_pipeline_stage_flag_bits(aTimestampStage), handle(), aQueryIndex);
		return aSync.submit_and_sync();
	}

	std::optional<command_buffer> query_pool_t::begin_query(uint32_t aQueryIndex, vk::QueryControlFlags aFlags, sync aSync)
	{
		auto& cmdBfr = aSync.get_or_create_command_buffer();
		cmdBfr.handle().beginQuery(handle(), aQueryIndex, aFlags);
		return aSync.submit_and_sync();
	}

	std::optional<command_buffer> query_pool_t::end_query(uint32_t aQueryIndex, sync aSync)
	{
		auto& cmdBfr = aSync.get_or_create_command_buffer();
		cmdBfr.handle().endQuery(handle(), aQueryIndex);
		return aSync.submit_and_sync();
	}

	std::optional<command_buffer> query_pool_t::copy_results(uint32_t aFirstQueryIndex, uint32_t aNumQueries, buffer_t& aBuffer, size_t aBufferMetaSkip, vk::QueryResultFlags aFlags, sync aSync)
	{
		const auto& meta = aBuffer.meta<query_results_buffer_meta>(aBufferMetaSkip);
		const auto& member = meta.member_description(content_description::query_result);

		auto& cmdBfr = aSync.get_or_create_command_buffer();
		cmdBfr.handle().copyQueryPoolResults(handle(), aFirstQueryIndex, aNumQueries, aBuffer.handle(), member.mOffset, meta.sizeof_one_element(), aFlags);
		return aSync.submit_and_sync();
	}

	std::optional<command_buffer> query_pool_t::copy_result(uint32_t aFirstQueryIndex, buffer_t& aBuffer, size_t aBufferMetaSkip, vk::QueryResultFlags aFlags, sync aSync)
	{
		return copy_results(aFirstQueryIndex, 1u, aBuffer, aBufferMetaSkip, aFlags, std::move(aSync));
	}
#pragma endregion

}
