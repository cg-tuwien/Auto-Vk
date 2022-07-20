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
		aBufferViewToBeFinished.mCreateInfo = vk::BufferViewCreateInfo{}
			.setBuffer(aBufferViewToBeFinished.buffer_handle())
			.setFormat(aViewFormat)
			.setOffset(0) // TODO: Support offsets
			.setRange(VK_WHOLE_SIZE); // TODO: Support ranges

		// Maybe alter the config?!
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(aBufferViewToBeFinished);
		}

		aBufferViewToBeFinished.mBufferView = device().createBufferViewUnique(aBufferViewToBeFinished.mCreateInfo, nullptr, dispatch_loader_core());

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
		AVK_LOG_INFO("========== MEMORY PROPERTIES OF DEVICE '" + deviceName + "'  ");
		AVK_LOG_INFO("-------------------------------------------------------------");
		AVK_LOG_INFO(" HEAP TYPES:                                                 ");
		AVK_LOG_INFO(" heap-idx |               bytes | heap flags                 ");
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
			auto combined = heapIndex + " | " + heapSize + " | " + heapFlags;
			auto spacesToAdd = static_cast<int>(std::string("                                                             ").size()) - static_cast<int>(combined.size());
			combined += std::string("                                                             ").substr(0, std::max(0, spacesToAdd));
			AVK_LOG_INFO(combined);
		}
		AVK_LOG_INFO("=============================================================");
		AVK_LOG_INFO(" MEMORY TYPES:                                               ");
		AVK_LOG_INFO(" mem-idx | heap-idx | memory propety flags                   ");
		AVK_LOG_INFO("-------------------------------------------------------------");
		for (auto i = 0u; i < memProperties.memoryTypeCount; ++i) {
			std::string memIndex =  "       " + std::to_string(i);
			memIndex = memIndex.substr(memIndex.length() - 8);
			std::string heapIndex = "        " + std::to_string(memProperties.memoryTypes[i].heapIndex);
			heapIndex = heapIndex.substr(heapIndex.length() - 9);
			std::string propFlags = vk::to_string(memProperties.memoryTypes[i].propertyFlags);
			auto combined = memIndex + " | " + heapIndex + " | " + propFlags;
			auto spacesToAdd = static_cast<int>(std::string("                                                             ").size()) - static_cast<int>(combined.size());
			combined += std::string("                                                             ").substr(0, std::max(0, spacesToAdd));
			AVK_LOG_INFO(combined);
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

	bool is_int_format(const vk::Format& aImageFormat)
	{
		return is_int8_format(aImageFormat) || is_int16_format(aImageFormat) || is_int32_format(aImageFormat);
	}

	bool is_uint_format(const vk::Format& aImageFormat)
	{
		return is_uint8_format(aImageFormat) || is_uint16_format(aImageFormat) || is_uint32_format(aImageFormat);
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
			vk::Format::eBc5UnormBlock,
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
			vk::Format::eR16G16B16A16Snorm,
			vk::Format::eBc4SnormBlock,
			vk::Format::eBc5SnormBlock,
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

	vk::AttachmentLoadOp to_vk_load_op(on_load_behavior aValue)
	{
		switch (aValue) {
		case on_load_behavior::dont_care:
			return vk::AttachmentLoadOp::eDontCare;
		case on_load_behavior::clear:
			return vk::AttachmentLoadOp::eClear;
		case on_load_behavior::load:
			return vk::AttachmentLoadOp::eLoad;
		default:
			throw std::invalid_argument("Invalid attachment load operation.");
		}
	}

	vk::AttachmentStoreOp to_vk_store_op(on_store_behavior aValue)
	{
		switch (aValue) {
		case on_store_behavior::dont_care:
			return vk::AttachmentStoreOp::eDontCare;
		case on_store_behavior::store:
			return vk::AttachmentStoreOp::eStore;
		default:
			throw std::invalid_argument("Invalid attachment store operation.");
		}
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
	attachment attachment::declare(std::tuple<vk::Format, vk::SampleCountFlagBits> aFormatAndSamples, attachment_load_config aLoadOp, subpass_usages aUsageInSubpasses, attachment_store_config aStoreOp)
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

	attachment attachment::declare(vk::Format aFormat, attachment_load_config aLoadOp, subpass_usages aUsageInSubpasses, attachment_store_config aStoreOp)
	{
		return declare({aFormat, vk::SampleCountFlagBits::e1}, aLoadOp, std::move(aUsageInSubpasses), aStoreOp);
	}

	attachment attachment::declare_for(const image_view_t& aImageView, attachment_load_config aLoadOp, avk::subpass_usages aUsageInSubpasses, attachment_store_config aStoreOp)
	{
		const auto& imageConfig = aImageView.get_image().create_info();
		const auto format = imageConfig.format;
		auto result = declare({format, imageConfig.samples}, aLoadOp, std::move(aUsageInSubpasses), aStoreOp);
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
			.setGeometryCount(static_cast<uint32_t>(result.mAccStructureGeometries.size()))
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

	avk::buffer bottom_level_acceleration_structure_t::get_and_possibly_create_scratch_buffer()
	{
		if (!mScratchBuffer.has_value()) {
			mScratchBuffer = root::create_buffer(
				*mRoot,
				avk::memory_usage::device,
#if VK_HEADER_VERSION >= 189
				vk::BufferUsageFlagBits::eStorageBuffer |
#endif
#if VK_HEADER_VERSION >= 162
				vk::BufferUsageFlagBits::eShaderDeviceAddressKHR
				| vk::BufferUsageFlagBits::eStorageBuffer,
#else
				vk::BufferUsageFlagBits::eRayTracingKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR,
#endif
				avk::generic_buffer_meta::create_from_size(std::max(required_scratch_buffer_build_size(), required_scratch_buffer_update_size()))
			);
			mScratchBuffer->enable_shared_ownership();
		}
		assert(mScratchBuffer.has_value());
		return mScratchBuffer.value();
	}

	avk::command::action_type_command bottom_level_acceleration_structure_t::build_or_update(std::vector<vertex_index_buffer_pair> aGeometries, std::optional<avk::buffer> aScratchBuffer, blas_action aBuildAction)
	{
		// Set the aScratchBuffer parameter to an internal scratch buffer, if none has been passed:
		std::vector<avk::buffer> lifetimeHandledBuffers;
		lifetimeHandledBuffers.push_back(std::move( // Scratch buffer is always the first in the vector
			aScratchBuffer.value_or(get_and_possibly_create_scratch_buffer())
		));
		auto getScratchBuffer = [&]() { return lifetimeHandledBuffers.front(); };

		// Construct before, then pass to the action_type_command:
		std::vector<std::tuple<std::variant<vk::Image, vk::Buffer>, avk::sync::sync_hint>> resSpecificSyncHints;
		resSpecificSyncHints.push_back( // For the scratch buffer
			std::make_tuple(getScratchBuffer()->handle(), avk::sync::sync_hint{
				// As the specification has it:
				//   Accesses to the acceleration structure scratch buffers as identified by the VkAccelerationStructureBuildGeometryInfoKHR::scratchData buffer device addresses must be synchronized with the 
				//   VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR pipeline stage and an access type of VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR or VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR
				stage::acceleration_structure_build + (access::acceleration_structure_read | access::acceleration_structure_write),
				stage::acceleration_structure_build +                                        access::acceleration_structure_write
			})
		);
		// Later, also fill in the dependencies for ALL the buffers referenced by aGeometries.

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
			auto& vertexBuffer = pair.vertex_buffer();
			const auto& vertexBufferMeta = vertexBuffer->meta<vertex_buffer_meta>();
			auto& indexBuffer = pair.index_buffer();
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

			// Create sync hint for each one of the buffers
			// As the specification has it:
			//   Accesses to other input buffers [...] must be synchronized with the VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR pipeline stageand an access type of VK_ACCESS_SHADER_READ_BIT:
			resSpecificSyncHints.push_back(std::make_tuple(vertexBuffer->handle(), avk::sync::sync_hint{ stage::acceleration_structure_build + access::shader_read, stage::acceleration_structure_build + access::none }));
			resSpecificSyncHints.push_back(std::make_tuple(indexBuffer->handle(),  avk::sync::sync_hint{ stage::acceleration_structure_build + access::shader_read, stage::acceleration_structure_build + access::none }));

			// See if we must handle the lifetime of the two buffers:
			if (vertexBuffer.is_ownership()) {
				lifetimeHandledBuffers.push_back(std::move(vertexBuffer.get_ownership()));
			}
			if (indexBuffer.is_ownership()) {
				lifetimeHandledBuffers.push_back(std::move(indexBuffer.get_ownership()));
			}
		}

		//const auto* pointerToAnArray = accStructureGeometries.data();

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
			//.setPpGeometries(&pointerToAnArray)
			.setScratchData(vk::DeviceOrHostAddressKHR{ getScratchBuffer()->device_address() });

		auto actionTypeCommand = avk::command::action_type_command{
			{}, // Let the sync hint be inferred afterwards. For the acceleration structure, it should be exactly the same as the scratch buffer's => so, inferring is fine.
			std::move(resSpecificSyncHints),
			[
				lRoot = mRoot,
				lAccStructureGeometries = std::move(accStructureGeometries),
				lBuildGeometryInfos = std::move(buildGeometryInfos),
				lBuildRangeInfos = std::move(buildRangeInfos),
				lBuildRangeInfoPtrs = std::move(buildRangeInfoPtrs),
				lLifetimeHandledBuffers = std::move(lifetimeHandledBuffers)
			] (avk::command_buffer_t& cb) mutable {
				// It requires pointer to a pointer => set here, inside the lambda:
				const auto* pointerToAnArray = lAccStructureGeometries.data();
				lBuildGeometryInfos[0].setPpGeometries(&pointerToAnArray);
				// Actually, after the moves, all the pointers should still be intact!
#ifdef _DEBUG
				// But let's check in DEBUG mode:
				for (size_t i = 0; i < lBuildRangeInfoPtrs.size(); ++i) {
					if (lBuildRangeInfoPtrs[i] != &lBuildRangeInfos[i]) {
						AVK_LOG_WARNING("Strange: lBuildRangeInfoPtrs[" + std::to_string(i) + "] != &lBuildRangeInfos[" + std::to_string(i) + "] after having moved lBuildRangeInfos.");
						lBuildRangeInfoPtrs[i] = &lBuildRangeInfos[i]; // fixed, but still strange; And in RELEASE mode it won't be fixed.
					}
				}
#endif

#if VK_HEADER_VERSION >= 162
				cb.handle().buildAccelerationStructuresKHR(
					static_cast<uint32_t>(lBuildGeometryInfos.size()),
					lBuildGeometryInfos.data(),
					lBuildRangeInfoPtrs.data(),
					lRoot->dispatch_loader_ext()
				);
#else
				cb.handle().buildAccelerationStructureKHR(
					static_cast<uint32_t>(lBuildGeometryInfos.size()),
					lBuildGeometryInfos.data(),
					lBuildRangeInfoPtrs.data(),
					lRoot->dispatch_loader_ext()
				);
#endif

				// Take care of the the buffers' lifetimes:
				for (auto& b : lLifetimeHandledBuffers) {
					let_it_handle_lifetime_of(cb, b);
				}
			}
		};

		actionTypeCommand.infer_sync_hint_from_resource_sync_hints();

		return actionTypeCommand;
	}

	avk::command::action_type_command bottom_level_acceleration_structure_t::build(const std::vector<vertex_index_buffer_pair>& aGeometries, std::optional<avk::buffer> aScratchBuffer)
	{
		return build_or_update(aGeometries, std::move(aScratchBuffer), blas_action::build);
	}

	avk::command::action_type_command bottom_level_acceleration_structure_t::update(const std::vector<vertex_index_buffer_pair>& aGeometries, std::optional<avk::buffer> aScratchBuffer)
	{
		return build_or_update(aGeometries, std::move(aScratchBuffer), blas_action::update);
	}

	avk::command::action_type_command bottom_level_acceleration_structure_t::build_or_update(const std::vector<VkAabbPositionsKHR>& aGeometries, std::optional<avk::buffer> aScratchBuffer, blas_action aBuildAction)
	{
		// Create buffer for the AABBs:
		auto aabbDataBuffer = root::create_buffer(
			*mRoot,
			memory_usage::device, {},
			aabb_buffer_meta::create_from_data(aGeometries)
		);
		// TODO: ^ Probably better to NOT create an entirely new buffer at every invocation ^^

		auto result = avk::command::action_type_command{};
		result.mNestedCommandsAndSyncInstructions.push_back(aabbDataBuffer->fill(aGeometries.data(), 0));
		result.mNestedCommandsAndSyncInstructions.push_back(sync::buffer_memory_barrier(aabbDataBuffer.as_reference(), stage::auto_stage >> stage::auto_stage, access::auto_access >> access::auto_access));
		result.mNestedCommandsAndSyncInstructions.push_back(build_or_update(std::move(aabbDataBuffer), std::move(aScratchBuffer), aBuildAction));
		result.infer_sync_hint_from_nested_commands();
		
		return result;
	}

	avk::command::action_type_command bottom_level_acceleration_structure_t::build_or_update(resource_argument<buffer_t> aGeometriesBuffer, std::optional<avk::buffer> aScratchBuffer, blas_action aBuildAction)
	{
		// Set the aScratchBuffer parameter to an internal scratch buffer, if none has been passed:
		avk::buffer scratchBuffer = aScratchBuffer.value_or(get_and_possibly_create_scratch_buffer());

		// Construct before, then pass to the action_type_command:
		std::vector<std::tuple<std::variant<vk::Image, vk::Buffer>, avk::sync::sync_hint>> resSpecificSyncHints;
		resSpecificSyncHints.push_back( // For the scratch buffer
			std::make_tuple(scratchBuffer->handle(), avk::sync::sync_hint{
				// As the specification has it:
				//   Accesses to the acceleration structure scratch buffers as identified by the VkAccelerationStructureBuildGeometryInfoKHR::scratchData buffer device addresses must be synchronized with the 
				//   VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR pipeline stage and an access type of VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR or VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR
				stage::acceleration_structure_build + (access::acceleration_structure_read | access::acceleration_structure_write),
				stage::acceleration_structure_build + access::acceleration_structure_write
				})
		);
		// Let's additionally also fill the dependencies for the geometries buffer:
		// As the specification has it:
		//   Accesses to other input buffers [...] must be synchronized with the VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR pipeline stage and an access type of VK_ACCESS_SHADER_READ_BIT:
		resSpecificSyncHints.push_back(std::make_tuple(aGeometriesBuffer->handle(), avk::sync::sync_hint{ stage::acceleration_structure_build + access::shader_read, stage::acceleration_structure_build + access::none }));

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
		//vk::AccelerationStructureBuildRangeInfoKHR* buildRangeInfoPtr = &buildRangeInfo;
#else
		auto buildOffsetInfo = vk::AccelerationStructureBuildOffsetInfoKHR{}
			.setPrimitiveCount(static_cast<uint32_t>(aabbMeta.num_elements()))
			.setPrimitiveOffset(0u)
			.setFirstVertex(0u)
			.setTransformOffset(0u); // TODO: Support different values for all these parameters?!
		vk::AccelerationStructureBuildOffsetInfoKHR* buildOffsetInfoPtr = &buildOffsetInfo;
#endif

		//const auto* pointerToAnArray = &accStructureGeometry;

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
			//.setPpGeometries(&pointerToAnArray)
			.setScratchData(vk::DeviceOrHostAddressKHR{ scratchBuffer->device_address() });

		auto actionTypeCommand = avk::command::action_type_command{
			{}, // Let the sync hint be inferred afterwards. For the acceleration structure, it should be exactly the same as the scratch buffer's => so, inferring is fine.
			std::move(resSpecificSyncHints),
			[
				lRoot = mRoot,
				lScratchBuffer = std::move(scratchBuffer),
				lGeometriesBuffer = aGeometriesBuffer.move_ownership_or_get_empty(),
				lAccStructureGeometry = std::move(accStructureGeometry),
				lBuildGeometryInfos = std::move(buildGeometryInfos),
				lBuildRangeInfo = std::move(buildRangeInfo)
			] (avk::command_buffer_t& cb) mutable {
				// Set all the config pointers here inside the lambda:
				vk::AccelerationStructureBuildRangeInfoKHR* buildRangeInfoPtr = &lBuildRangeInfo;
				const auto* pointerToAnArray = &lAccStructureGeometry;
				lBuildGeometryInfos.setPpGeometries(&pointerToAnArray);

#if VK_HEADER_VERSION >= 162
				cb.handle().buildAccelerationStructuresKHR(
					1u,
					&lBuildGeometryInfos,
					&buildRangeInfoPtr,
					lRoot->dispatch_loader_ext()
				);
#else
				cb.handle().buildAccelerationStructureKHR(
					1u,
					&lBuildGeometryInfos,
					&buildRangeInfoPtr,
					lRoot->dispatch_loader_ext()
				);
#endif

				// Take care of the buffers' lifetimes:
				let_it_handle_lifetime_of(cb, lScratchBuffer);
				let_it_handle_lifetime_of(cb, lGeometriesBuffer);
			}
		};

		actionTypeCommand.infer_sync_hint_from_resource_sync_hints();

		return actionTypeCommand;
	}

	avk::command::action_type_command bottom_level_acceleration_structure_t::build(const std::vector<VkAabbPositionsKHR>& aGeometries, std::optional<avk::buffer> aScratchBuffer)
	{
		return build_or_update(aGeometries, std::move(aScratchBuffer), blas_action::build);
	}

	avk::command::action_type_command bottom_level_acceleration_structure_t::update(const std::vector<VkAabbPositionsKHR>& aGeometries, std::optional<avk::buffer> aScratchBuffer)
	{
		return build_or_update(aGeometries, std::move(aScratchBuffer), blas_action::update);
	}

	avk::command::action_type_command bottom_level_acceleration_structure_t::build(const buffer& aGeometriesBuffer, std::optional<avk::buffer> aScratchBuffer)
	{
		return build_or_update(aGeometriesBuffer, std::move(aScratchBuffer), blas_action::build);
	}

	avk::command::action_type_command bottom_level_acceleration_structure_t::update(const buffer& aGeometriesBuffer, std::optional<avk::buffer> aScratchBuffer)
	{
		return build_or_update(aGeometriesBuffer, std::move(aScratchBuffer), blas_action::update);
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
			.setGeometryCount(static_cast<uint32_t>(result.mAccStructureGeometries.size()))
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
	
	avk::buffer top_level_acceleration_structure_t::get_and_possibly_create_scratch_buffer()
	{
		if (!mScratchBuffer.has_value()) {
			mScratchBuffer = root::create_buffer(
				*mRoot,
				avk::memory_usage::device,
#if VK_HEADER_VERSION >= 189
				vk::BufferUsageFlagBits::eStorageBuffer |
#endif
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
		return mScratchBuffer.value();
	}

	avk::command::action_type_command top_level_acceleration_structure_t::build_or_update(const std::vector<geometry_instance>& aGeometryInstances, std::optional<avk::buffer> aScratchBuffer, tlas_action aBuildAction)
	{
		auto geomInstances = convert_for_gpu_usage(aGeometryInstances);

		auto geomInstBuffer = root::create_buffer(
			*mRoot,
			AVK_STAGING_BUFFER_MEMORY_USAGE,
#if VK_HEADER_VERSION >= 182
			vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
#else 
			{},
#endif
			geometry_instance_buffer_meta::create_from_data(geomInstances)
		);

		auto result = avk::command::action_type_command{};
		result.mNestedCommandsAndSyncInstructions.push_back(geomInstBuffer->fill(geomInstances.data(), 0));
		result.mNestedCommandsAndSyncInstructions.push_back(sync::buffer_memory_barrier(geomInstBuffer.as_reference(), stage::auto_stage >> stage::auto_stage, access::auto_access >> access::auto_access));
		result.mNestedCommandsAndSyncInstructions.push_back(build_or_update(std::move(geomInstBuffer), aScratchBuffer, aBuildAction));
		result.infer_sync_hint_from_nested_commands();
		
		return result;
	}

	avk::command::action_type_command top_level_acceleration_structure_t::build_or_update(avk::resource_argument<avk::buffer_t> aGeometryInstancesBuffer, std::optional<avk::buffer> aScratchBuffer, tlas_action aBuildAction)
	{
		// Set the aScratchBuffer parameter to an internal scratch buffer, if none has been passed:
		avk::buffer scratchBuffer = std::move(aScratchBuffer.value_or(get_and_possibly_create_scratch_buffer()));

		// Construct before, then pass to the action_type_command:
		std::vector<std::tuple<std::variant<vk::Image, vk::Buffer>, avk::sync::sync_hint>> resSpecificSyncHints;
		resSpecificSyncHints.push_back( // For the scratch buffer
			std::make_tuple(scratchBuffer->handle(), avk::sync::sync_hint{
				// As the specification has it:
				//   Accesses to the acceleration structure scratch buffers as identified by the VkAccelerationStructureBuildGeometryInfoKHR::scratchData buffer device addresses must be synchronized with the 
				//   VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR pipeline stage and an access type of VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR or VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR
				stage::acceleration_structure_build + (access::acceleration_structure_read | access::acceleration_structure_write),
				stage::acceleration_structure_build + access::acceleration_structure_write
			})
		);
		// Let's additionally also fill the dependencies for the geometries buffer:
		// As the specification has it:
		//   Accesses to other input buffers [...] must be synchronized with the VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR pipeline stage and an access type of VK_ACCESS_SHADER_READ_BIT:
		resSpecificSyncHints.push_back(std::make_tuple(aGeometryInstancesBuffer->handle(), avk::sync::sync_hint{ stage::acceleration_structure_build + access::shader_read, stage::acceleration_structure_build + access::none }));

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
		//vk::AccelerationStructureBuildRangeInfoKHR* buildRangeInfoPtr = &bri;
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
			//.setPpGeometries(&pointerToAnArray)
			.setScratchData(vk::DeviceOrHostAddressKHR{ scratchBuffer->device_address() });

		auto actionTypeCommand = avk::command::action_type_command{
			{}, // Let the sync hint be inferred afterwards. For the acceleration structure, it should be exactly the same as the scratch buffer's => so, inferring is fine.
			std::move(resSpecificSyncHints),
			[
				lRoot = mRoot,
				lScratchBuffer = std::move(scratchBuffer),
				lAccStructureGeometries = std::move(accStructureGeometries),
				lBuildGeometryInfo = std::move(buildGeometryInfo),
				lBuildRangeInfo = std::move(bri),
				lGeometryInstancesBuffer = aGeometryInstancesBuffer.move_ownership_or_get_empty()
			] (avk::command_buffer_t& cb) mutable {
				// It requires pointer to a pointer => set here, inside the lambda:
				vk::AccelerationStructureBuildRangeInfoKHR* buildRangeInfoPtr = &lBuildRangeInfo;

				// Set pointer to acceleration structure geometries here:
				const auto* pointerToAnArray = &lAccStructureGeometries;
				lBuildGeometryInfo.setPpGeometries(&pointerToAnArray);

#if VK_HEADER_VERSION >= 162
				cb.handle().buildAccelerationStructuresKHR(
					1u,
					&lBuildGeometryInfo,
					&buildRangeInfoPtr,
					lRoot->dispatch_loader_ext()
				);
#else
				cb.handle().buildAccelerationStructureKHR(
					1u,
					&buildGeometryInfo,
					&buildOffsetInfoPtr,
					lRoot->dispatch_loader_ext()
				);
#endif

				// Take care of the scratch buffer's lifetime:
				let_it_handle_lifetime_of(cb, lScratchBuffer);
				let_it_handle_lifetime_of(cb, lGeometryInstancesBuffer);
			}
		};

		actionTypeCommand.infer_sync_hint_from_resource_sync_hints();

		return actionTypeCommand;
	}

	avk::command::action_type_command top_level_acceleration_structure_t::build(const std::vector<geometry_instance>& aGeometryInstances, std::optional<avk::buffer> aScratchBuffer)
	{
		return build_or_update(aGeometryInstances, std::move(aScratchBuffer), tlas_action::build);
	}

	avk::command::action_type_command top_level_acceleration_structure_t::build(const buffer& aGeometryInstancesBuffer, std::optional<avk::buffer> aScratchBuffer)
	{
		return build_or_update(aGeometryInstancesBuffer, std::move(aScratchBuffer), tlas_action::build);
	}

	avk::command::action_type_command top_level_acceleration_structure_t::update(const std::vector<geometry_instance>& aGeometryInstances, std::optional<avk::buffer> aScratchBuffer)
	{
		return build_or_update(aGeometryInstances, std::move(aScratchBuffer), tlas_action::update);
	}

	avk::command::action_type_command top_level_acceleration_structure_t::update(const buffer& aGeometryInstancesBuffer, std::optional<avk::buffer> aScratchBuffer)
	{
		return build_or_update(aGeometryInstancesBuffer, std::move(aScratchBuffer), tlas_action::update);
	}
#endif
#pragma endregion

#pragma region binding_data definitions
	uint32_t binding_data::descriptor_count() const
	{
		if (std::holds_alternative<std::vector<const buffer_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<const buffer_t*>>(mResourcePtr).size()); }
		if (std::holds_alternative<std::vector<const buffer_descriptor*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<const buffer_descriptor*>>(mResourcePtr).size()); }
		if (std::holds_alternative<std::vector<const buffer_view_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<const buffer_view_t*>>(mResourcePtr).size()); }
		if (std::holds_alternative<std::vector<const buffer_view_descriptor_info*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<const buffer_view_descriptor_info*>>(mResourcePtr).size()); }

		//                                                                         vvv There can only be ONE pNext (at least I think so) vvv
		if (std::holds_alternative<std::vector<const top_level_acceleration_structure_t*>>(mResourcePtr)) { return 1u; }

		if (std::holds_alternative<std::vector<const image_view_as_sampled_image*>>(mResourcePtr))    { return static_cast<uint32_t>(std::get<std::vector<const image_view_as_sampled_image*>>   (mResourcePtr).size()); }
		if (std::holds_alternative<std::vector<const image_view_as_input_attachment*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<const image_view_as_input_attachment*>>(mResourcePtr).size()); }
		if (std::holds_alternative<std::vector<const image_view_as_storage_image*>>(mResourcePtr))    { return static_cast<uint32_t>(std::get<std::vector<const image_view_as_storage_image*>>   (mResourcePtr).size()); }
		if (std::holds_alternative<std::vector<const sampler_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<const sampler_t*>>(mResourcePtr).size()); }
		if (std::holds_alternative<std::vector<const combined_image_sampler_descriptor_info*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<const combined_image_sampler_descriptor_info*>>(mResourcePtr).size()); }

		return 1u;
	}

	const vk::DescriptorImageInfo* binding_data::descriptor_image_info(descriptor_set& aDescriptorSet) const
	{
		if (std::holds_alternative<const buffer_t*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const buffer_descriptor*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const buffer_view_t*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const buffer_view_descriptor_info*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const top_level_acceleration_structure_t*>(mResourcePtr)) { return nullptr; }

		if (std::holds_alternative<const image_view_as_sampled_image*>(mResourcePtr)) {
			return aDescriptorSet.store_image_info(mLayoutBinding.binding, std::get<const image_view_as_sampled_image*>(mResourcePtr)->descriptor_info());
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
		if (std::holds_alternative<const combined_image_sampler_descriptor_info*>(mResourcePtr)) {
			return aDescriptorSet.store_image_info(mLayoutBinding.binding, std::get<const combined_image_sampler_descriptor_info*>(mResourcePtr)->descriptor_info());
		}


		if (std::holds_alternative<std::vector<const buffer_t*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const buffer_descriptor*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const buffer_view_t*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const buffer_view_descriptor_info*>>(mResourcePtr)) { return nullptr; }

		if (std::holds_alternative<std::vector<const top_level_acceleration_structure_t*>>(mResourcePtr)) { return nullptr; }

		if (std::holds_alternative<std::vector<const image_view_as_sampled_image*>>(mResourcePtr)) {
			return aDescriptorSet.store_image_infos(mLayoutBinding.binding, gather_image_infos(std::get<std::vector<const image_view_as_sampled_image*>>(mResourcePtr)));
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
		if (std::holds_alternative<std::vector<const combined_image_sampler_descriptor_info*>>(mResourcePtr)) {
			return aDescriptorSet.store_image_infos(mLayoutBinding.binding, gather_image_infos(std::get<std::vector<const combined_image_sampler_descriptor_info*>>(mResourcePtr)));
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
		if (std::holds_alternative<const buffer_view_descriptor_info*>(mResourcePtr)) { return nullptr; }

		if (std::holds_alternative<const top_level_acceleration_structure_t*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const image_view_as_sampled_image*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const image_view_as_input_attachment*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const image_view_as_storage_image*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const sampler_t*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const combined_image_sampler_descriptor_info*>(mResourcePtr)) { return nullptr; }


		if (std::holds_alternative<std::vector<const buffer_t*>>(mResourcePtr)) {
			return aDescriptorSet.store_buffer_infos(mLayoutBinding.binding, gather_buffer_infos(std::get<std::vector<const buffer_t*>>(mResourcePtr)));
		}
		if (std::holds_alternative<std::vector<const buffer_descriptor*>>(mResourcePtr)) {
			return aDescriptorSet.store_buffer_infos(mLayoutBinding.binding, gather_buffer_infos(std::get<std::vector<const buffer_descriptor*>>(mResourcePtr)));
		}
		if (std::holds_alternative<std::vector<const buffer_view_t*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const buffer_view_descriptor_info*>>(mResourcePtr)) { return nullptr; }

		if (std::holds_alternative<std::vector<const top_level_acceleration_structure_t*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const image_view_as_sampled_image*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const image_view_as_input_attachment*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const image_view_as_storage_image*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const sampler_t*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const combined_image_sampler_descriptor_info*>>(mResourcePtr)) { return nullptr; }

		throw runtime_error("Some holds_alternative calls are not implemented.");
	}

	const void* binding_data::next_pointer(descriptor_set& aDescriptorSet) const
	{
		if (std::holds_alternative<const buffer_t*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const buffer_descriptor*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const buffer_view_t*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const buffer_view_descriptor_info*>(mResourcePtr)) { return nullptr; }

#if VK_HEADER_VERSION >= 135
		if (std::holds_alternative<const top_level_acceleration_structure_t*>(mResourcePtr)) {
			return aDescriptorSet.store_acceleration_structure_info(mLayoutBinding.binding, std::get<const top_level_acceleration_structure_t*>(mResourcePtr)->descriptor_info());
		}
#endif

		if (std::holds_alternative<const image_view_as_sampled_image*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const image_view_as_input_attachment*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const image_view_as_storage_image*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const sampler_t*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const combined_image_sampler_descriptor_info*>(mResourcePtr)) { return nullptr; }


		if (std::holds_alternative<std::vector<const buffer_t*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const buffer_descriptor*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const buffer_view_t*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const buffer_view_descriptor_info*>>(mResourcePtr)) { return nullptr; }

#if VK_HEADER_VERSION >= 135
		if (std::holds_alternative<std::vector<const top_level_acceleration_structure_t*>>(mResourcePtr)) {
			return aDescriptorSet.store_acceleration_structure_infos(mLayoutBinding.binding, gather_acceleration_structure_infos(std::get<std::vector<const top_level_acceleration_structure_t*>>(mResourcePtr)));
		}
#endif

		if (std::holds_alternative<std::vector<const image_view_as_sampled_image*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const image_view_as_input_attachment*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const image_view_as_storage_image*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const sampler_t*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const combined_image_sampler_descriptor_info*>>(mResourcePtr)) { return nullptr; }

		throw runtime_error("Some holds_alternative calls are not implemented.");
	}

	const vk::BufferView* binding_data::texel_buffer_view_info(descriptor_set& aDescriptorSet) const
	{
		if (std::holds_alternative<const buffer_t*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const buffer_descriptor*>(mResourcePtr)) { return nullptr; }

		if (std::holds_alternative<const buffer_view_t*>(mResourcePtr)) {
			return aDescriptorSet.store_buffer_view(mLayoutBinding.binding, std::get<const buffer_view_t*>(mResourcePtr)->view_handle());
		}
		if (std::holds_alternative<const buffer_view_descriptor_info*>(mResourcePtr)) {
			return aDescriptorSet.store_buffer_view(mLayoutBinding.binding, std::get<const buffer_view_descriptor_info*>(mResourcePtr)->view_handle());
		}

		if (std::holds_alternative<const top_level_acceleration_structure_t*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const image_view_as_sampled_image*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const image_view_as_input_attachment*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const image_view_as_storage_image*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const sampler_t*>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<const combined_image_sampler_descriptor_info*>(mResourcePtr)) { return nullptr; }


		if (std::holds_alternative<std::vector<const buffer_t*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const buffer_descriptor*>>(mResourcePtr)) { return nullptr; }

		if (std::holds_alternative<std::vector<const buffer_view_t*>>(mResourcePtr)) {
			return aDescriptorSet.store_buffer_views(mLayoutBinding.binding, gather_buffer_views(std::get<std::vector<const buffer_view_t*>>(mResourcePtr)));
		}
		if (std::holds_alternative<std::vector<const buffer_view_descriptor_info*>>(mResourcePtr)) {
			return aDescriptorSet.store_buffer_views(mLayoutBinding.binding, gather_buffer_views(std::get<std::vector<const buffer_view_descriptor_info*>>(mResourcePtr)));
		}

		if (std::holds_alternative<std::vector<const top_level_acceleration_structure_t*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const image_view_as_sampled_image*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const image_view_as_input_attachment*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const image_view_as_storage_image*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const sampler_t*>>(mResourcePtr)) { return nullptr; }
		if (std::holds_alternative<std::vector<const combined_image_sampler_descriptor_info*>>(mResourcePtr)) { return nullptr; }

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
		const root& aRoot,
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
		result.mBuffer = AVK_MEM_BUFFER_HANDLE{ aRoot.memory_allocator(), aMemoryProperties, result.mCreateInfo };
		result.mRoot = &aRoot;

#if VK_HEADER_VERSION >= 135
		if (   avk::has_flag(result.usage_flags(), vk::BufferUsageFlagBits::eShaderDeviceAddress)
			|| avk::has_flag(result.usage_flags(), vk::BufferUsageFlagBits::eShaderDeviceAddressKHR)
			|| avk::has_flag(result.usage_flags(), vk::BufferUsageFlagBits::eShaderDeviceAddressEXT)
			//|| avk::has_flag(result.usage_flags(), vk::BufferUsageFlagBits::eShaderBindingTableKHR)
			//|| avk::has_flag(result.usage_flags(), vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR)
			//|| avk::has_flag(result.usage_flags(), vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR)
			) {
			result.mDeviceAddress = get_buffer_address(aRoot.device(), result.handle());
		}
#endif

		return result;
	}

	avk::command::action_type_command buffer_t::fill(const void* aDataPtr, size_t aMetaDataIndex)
	{
		const auto metaData = meta_at_index<buffer_meta>(aMetaDataIndex);
		const auto bufferSize = static_cast<vk::DeviceSize>(metaData.total_size());
		return fill(aDataPtr, aMetaDataIndex, 0u, bufferSize);
	}

	command::action_type_command buffer_t::fill(const void* aDataPtr, size_t aMetaDataIndex, size_t aOffsetInBytes, size_t aDataSizeInBytes)
	{
		auto dstOffset = static_cast<vk::DeviceSize>(aOffsetInBytes);
		auto dataSize = static_cast<vk::DeviceSize>(aDataSizeInBytes);
		const auto memProps = memory_properties();

#ifdef _DEBUG
		const auto& metaData = meta_at_index<buffer_meta>(aMetaDataIndex);
		assert(dstOffset + dataSize <= metaData.total_size()); // The fill operation would write beyond the buffer's size.
#endif

		// Prepare a result which defines no sync hints and no instructions to be executed for some cases:
		auto actionTypeCommand = command::action_type_command{
			{}, // Define a resource-specific sync hint here and let the general sync hint be inferred afterwards (because it is supposed to be exactly the same)
			{
				std::make_tuple(handle(), avk::sync::sync_hint{
					stage::none + access::none, // No need to wait on anything nor to make anything available
					// Set defaults for the host-visible-only case, overwrite them for device buffers further down:
					stage::none + access::none, // <-- This is okay for host-visible buffers, because the queue submit transfers the memory
				})
			}
		};
		actionTypeCommand.infer_sync_hint_from_resource_sync_hints();

		// #0: Sanity check
		if (dataSize == 0) {
			// Nothing to do here
			return actionTypeCommand;
		}

		// #1: Is our memory accessible from the CPU-SIDE?
		if (avk::has_flag(memProps, vk::MemoryPropertyFlagBits::eHostVisible)) {
			auto mapped = scoped_mapping{mBuffer, mapping_access::write};
			// Memcpy doesn't have to wait on anything, no sync required.
			memcpy(static_cast<uint8_t *>(mapped.get()) + dstOffset, aDataPtr, dataSize);
			// Since this is a host-write, no need for any barrier, because of implicit host write guarantee.
			return actionTypeCommand;
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

			auto stagingBuffer = root::create_buffer(
				*mRoot,
				AVK_STAGING_BUFFER_MEMORY_USAGE,
				vk::BufferUsageFlagBits::eTransferSrc,
				generic_buffer_meta::create_from_size(dataSize)
			);
			stagingBuffer.enable_shared_ownership(); // TODO: Why does it not work WITHOUT shared_ownership? (Fails when assigning it to mBeginFun)
			stagingBuffer->fill(aDataPtr, 0); // Recurse into the other if-branch

			// Whatever comes after must synchronize with the device-local copy:
			std::get<avk::sync::sync_hint>(actionTypeCommand.mResourceSpecificSyncHints.front()).mSrcForSubsequentCmds = stage::copy + access::transfer_write;
			actionTypeCommand.infer_sync_hint_from_resource_sync_hints();
						
			actionTypeCommand.mBeginFun = [
				lRoot = mRoot,
				lOwnedStagingBuffer = std::move(stagingBuffer),
				lDstBufferHandle = handle(),
				dstOffset, dataSize
			](avk::command_buffer_t& cb) mutable {
				//const auto copyRegion = vk::BufferCopy2KHR{ 0u, 0u, dataSize };
				//const auto copyBufferInfo = vk::CopyBufferInfo2KHR{ lOwnedStagingBuffer->handle(), lDstBufferHandle, 1u, &copyRegion };
				//cb.handle().copyBuffer2KHR(&copyBufferInfo);
				// TODO: No idea why copyBuffer2KHR fails with an access violation

				const auto copyRegion = vk::BufferCopy{ 0u, 0u, dataSize };
				cb.handle().copyBuffer(lOwnedStagingBuffer->handle(), lDstBufferHandle, 1u, &copyRegion, lRoot->dispatch_loader_core());

				// Take care of the lifetime handling of the stagingBuffer, it might still be in use when this method returns:
				cb.handle_lifetime_of(std::move(lOwnedStagingBuffer));
			};

			return actionTypeCommand;
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
	//			stagingBuffer->fill(aDataPtr, 0, old_sync::wait_idle()); // Recurse into the other if-branch

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

	avk::command::action_type_command buffer_t::read_into(void* aDataPtr, size_t aMetaDataIndex) const
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
			//						after the transfer operation has completed => handle via avk::old_sync!
			auto stagingBuffer = root::create_buffer( // Need it in shared ownership (default), because we do not know how often the user of this function will execute the commands
				*mRoot,
				AVK_STAGING_BUFFER_READBACK_MEMORY_USAGE,
				vk::BufferUsageFlagBits::eTransferDst,
				generic_buffer_meta::create_from_size(bufferSize)
			);

			// TODO: Creating a staging buffer in every read()-call is probably not optimal. => Think about alternative ways!

			auto actionTypeCommand = avk::command::action_type_command{
				{}, // Define a resource-specific sync hint here and let the general sync hint be inferred afterwards (because it is supposed to be exactly the same)
				{
					std::make_tuple(handle(), avk::sync::sync_hint{
						stage::copy + access::transfer_read,
						stage::copy + access::none
					})
					// No need for any dependencies for the staging buffer
				},
				[
					lBufferSize = bufferSize,
					lBufferHandle = handle(),
					lStagingBuffer = std::move(stagingBuffer),
					aMetaDataIndex, aDataPtr
				] (avk::command_buffer_t& cb) {
					auto copyRegion = vk::BufferCopy{}
						.setSrcOffset(0u)
						.setDstOffset(0u)
						.setSize(lBufferSize);
					cb.handle().copyBuffer(lBufferHandle, lStagingBuffer->handle(), { copyRegion });

					// Don't need to handle ownership here, because we're storing it in the post execution handler

					cb.set_post_execution_handler([
						lStagingBuffer, // enabled shared ownership anyways, so just pass by value
						aMetaDataIndex,
						aDataPtr
					]() {
						lStagingBuffer->read_into(aDataPtr, aMetaDataIndex); // This one will return an empty action_type_command{}
					});
				}
			};

			actionTypeCommand.infer_sync_hint_from_resource_sync_hints();

			return actionTypeCommand;
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

	const vk::BufferCreateInfo& buffer_view_t::buffer_create_info() const
	{
		if (std::holds_alternative<buffer>(mBuffer)) {
			return std::get<buffer>(mBuffer)->create_info();
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

	buffer_view root::create_buffer_view(buffer aBufferToOwn, vk::Format aViewFormat, std::function<void(buffer_view_t&)> aAlterConfigBeforeCreation)
	{
		buffer_view_t result;
		result.mBuffer = std::move(aBufferToOwn);
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
		result.mRoot = this;
		result.mCreateInfo = createInfo;
		result.mCommandPool = std::make_shared<vk::UniqueHandle<vk::CommandPool, DISPATCH_LOADER_CORE_TYPE>>(device().createCommandPoolUnique(createInfo, nullptr, dispatch_loader_core()));
		return result;
	}

	std::vector<command_buffer> command_pool_t::alloc_command_buffers(uint32_t aCount, vk::CommandBufferUsageFlags aUsageFlags, vk::CommandBufferLevel aLevel)
	{
		auto bufferAllocInfo = vk::CommandBufferAllocateInfo()
			.setCommandPool(handle())
			.setLevel(aLevel)
			.setCommandBufferCount(aCount);

		auto tmp = mCommandPool->getOwner().allocateCommandBuffersUnique(bufferAllocInfo, mRoot->dispatch_loader_core());

		// Iterate over all the "raw"-Vk objects in `tmp` and...
		std::vector<command_buffer> buffers;
		buffers.reserve(aCount);
		std::transform(std::begin(tmp), std::end(tmp),
			std::back_inserter(buffers),
			// ...transform them into `ak::command_buffer_t` objects:
			[lUsageFlags = aUsageFlags, poolPtr = mCommandPool, lRoot = mRoot](auto& vkCb) -> command_buffer {
				command_buffer_t result;
				result.mBeginInfo = vk::CommandBufferBeginInfo()
					.setFlags(lUsageFlags)
					.setPInheritanceInfo(nullptr);
				result.mCommandBuffer = std::move(vkCb);
				result.mCommandPool = std::move(poolPtr);
				result.mRoot = lRoot;
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
		mLifetimeHandledResources.clear();
	}

	void command_buffer_t::reset()
	{
		prepare_for_reuse();
		handle().reset();
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

	command_buffer_t& command_buffer_t::handle_lifetime_of(any_owning_resource_t aResource)
	{


		mLifetimeHandledResources.push_back(std::move(aResource));
		return *this;
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
		aPreparedPipeline.mPipelineLayout = device().createPipelineLayoutUnique(aPreparedPipeline.mPipelineLayoutCreateInfo, nullptr, dispatch_loader_core());
		assert(static_cast<bool>(aPreparedPipeline.layout_handle()));

		// Create the PIPELINE, a.k.a. putting it all together:
		auto pipelineInfo = vk::ComputePipelineCreateInfo{}
			.setFlags(aPreparedPipeline.mPipelineCreateFlags)
			.setStage(aPreparedPipeline.mShaderStageCreateInfo)
			.setLayout(aPreparedPipeline.layout_handle())
			.setBasePipelineHandle(nullptr) // Optional
			.setBasePipelineIndex(-1); // Optional
#if VK_HEADER_VERSION >= 141
		auto result = device().createComputePipelineUnique(nullptr, pipelineInfo, nullptr, dispatch_loader_core());
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

	compute_pipeline root::create_compute_pipeline_from_template(const compute_pipeline_t& aTemplate, std::function<void(compute_pipeline_t&)> aAlterConfigBeforeCreation)
	{
		compute_pipeline_t result;
		result.mPipelineCreateFlags			= aTemplate.mPipelineCreateFlags;
		result.mShader						= create_shader_from_template(aTemplate.mShader);
		result.mShaderStageCreateInfo		= aTemplate.mShaderStageCreateInfo;
		result.mSpecializationInfo			= aTemplate.mSpecializationInfo;
		result.mBasePipelineIndex			= aTemplate.mBasePipelineIndex;
		result.mAllDescriptorSetLayouts		= create_set_of_descriptor_set_layouts_from_template(aTemplate.mAllDescriptorSetLayouts);
		result.mPushConstantRanges			= aTemplate.mPushConstantRanges;
		result.mPipelineLayoutCreateInfo	= aTemplate.mPipelineLayoutCreateInfo;

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
	descriptor_pool root::create_descriptor_pool(vk::Device aDevice, const DISPATCH_LOADER_CORE_TYPE& aDispatchLoader, const std::vector<vk::DescriptorPoolSize>& aSizeRequirements, int aNumSets)
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
		result.mDescriptorPool = aDevice.createDescriptorPoolUnique(createInfo, nullptr, aDispatchLoader);

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
		return create_descriptor_pool(device(), dispatch_loader_core(), aSizeRequirements, aNumSets);
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

	void descriptor_pool::reset()
	{
		mDescriptorPool.getOwner().resetDescriptorPool(mDescriptorPool.get());
		mRemainingCapacities = mInitialCapacities;
		mNumRemainingSets = mNumInitialSets;
	}

	descriptor_cache root::create_descriptor_cache(std::string aName)
	{
		if (aName.empty()) {
			static int sDescCacheId = 1;
			aName = "Descriptor Cache #" + std::to_string(sDescCacheId++);
		}

		descriptor_cache_t result;
		result.mName = std::move(aName);
		result.mRoot = this;
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

	void root::allocate_descriptor_set_layout(vk::Device aDevice, const DISPATCH_LOADER_CORE_TYPE& aDispatchLoader, descriptor_set_layout& aLayoutToBeAllocated)
	{
		if (!aLayoutToBeAllocated.mLayout) {
			// Allocate the layout and return the result:
			auto createInfo = vk::DescriptorSetLayoutCreateInfo()
				.setBindingCount(static_cast<uint32_t>(aLayoutToBeAllocated.mOrderedBindings.size()))
				.setPBindings(aLayoutToBeAllocated.mOrderedBindings.data());
			aLayoutToBeAllocated.mLayout = aDevice.createDescriptorSetLayoutUnique(createInfo, nullptr, aDispatchLoader);
		}
		else {
			AVK_LOG_ERROR("descriptor_set_layout's handle already has a value => it most likely has already been allocated. Won't do it again.");
		}
	}

	void root::allocate_descriptor_set_layout(descriptor_set_layout& aLayoutToBeAllocated)
	{
		return allocate_descriptor_set_layout(device(), dispatch_loader_core(), aLayoutToBeAllocated);
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

	const descriptor_set_layout& descriptor_cache_t::get_or_alloc_layout(descriptor_set_layout aPreparedLayout)
	{
		const auto it = mLayouts.find(aPreparedLayout);
		if (mLayouts.end() != it) {
			assert(it->handle());
			return *it;
		}

		root::allocate_descriptor_set_layout(mRoot->device(), mRoot->dispatch_loader_core(), aPreparedLayout);

		const auto result = mLayouts.insert(std::move(aPreparedLayout));
		assert(result.second);
		return *result.first;
	}

	std::optional<descriptor_set> descriptor_cache_t::get_descriptor_set_from_cache(const descriptor_set& aPreparedSet)
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

	std::vector<descriptor_set> descriptor_cache_t::alloc_new_descriptor_sets(const std::vector<std::reference_wrapper<const descriptor_set_layout>>& aLayouts, std::vector<descriptor_set> aPreparedSets)
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
					break;
				default:
					AVK_LOG_INFO("Trying again with new pool..."); // and possibly doubled size requirements, depending on whether maxTries is 2 or 0
					poolToTry = get_descriptor_pool_for_layouts(allocRequest, true);
					break;
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

	void descriptor_cache_t::cleanup()
	{
		mSets.clear();
		mLayouts.clear();
	}

	std::shared_ptr<descriptor_pool> descriptor_cache_t::get_descriptor_pool_for_layouts(const descriptor_alloc_request& aAllocRequest, bool aRequestNewPool)
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

		const bool isNvidia = 0x12d2 == mRoot->physical_device().getProperties().vendorID;
		auto amplifiedAllocRequest = aAllocRequest.multiply_size_requirements(prealloc_factor());
		//if (!isNvidia) { // Let's 'if' on the vendor and see what happens...
		//}

		auto newPool = root::create_descriptor_pool(mRoot->device(), mRoot->dispatch_loader_core(),
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

	std::vector<descriptor_set> descriptor_cache_t::get_or_create_descriptor_sets(std::initializer_list<binding_data> aBindings)
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

	int descriptor_cache_t::remove_sets_with_handle(vk::ImageView aHandle)
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

	int descriptor_cache_t::remove_sets_with_handle(vk::Buffer aHandle)
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

	int descriptor_cache_t::remove_sets_with_handle(vk::Sampler aHandle)
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

	int descriptor_cache_t::remove_sets_with_handle(vk::BufferView aHandle)
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

	fence_t& fence_t::handle_lifetime_of(any_owning_resource_t aResource)
	{
		mLifetimeHandledResources.push_back(std::move(aResource));
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

	fence root::create_fence(vk::Device aDevice, const DISPATCH_LOADER_CORE_TYPE& aDispatchLoader, bool aCreateInSignalledState, std::function<void(fence_t&)> aAlterConfigBeforeCreation)
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

		result.mFence = aDevice.createFenceUnique(result.mCreateInfo, nullptr, aDispatchLoader);
		return result;
	}

	fence root::create_fence(bool aCreateInSignalledState, std::function<void(fence_t&)> aAlterConfigBeforeCreation)
	{
		return create_fence(device(), dispatch_loader_core(), aCreateInSignalledState, std::move(aAlterConfigBeforeCreation));
	}
#pragma endregion

#pragma region framebuffer definitions
	void root::check_and_config_attachments_based_on_views(std::vector<attachment>& aAttachments, std::vector<image_view>& aImageViews)
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
		}
	}

	framebuffer root::create_framebuffer(renderpass aRenderpass, std::vector<image_view> aImageViews, uint32_t aWidth, uint32_t aHeight, std::function<void(framebuffer_t&)> aAlterConfigBeforeCreation)
	{
		framebuffer_t result;
		result.mRenderpass = std::move(aRenderpass);
		result.mImageViews = std::move(aImageViews);

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

		result.mFramebuffer = device().createFramebufferUnique(result.mCreateInfo, nullptr, dispatch_loader_core());
		return result;
	}

	framebuffer root::create_framebuffer(std::vector<attachment> aAttachments, std::vector<image_view> aImageViews, uint32_t aWidth, uint32_t aHeight, std::function<void(framebuffer_t&)> aAlterConfigBeforeCreation)
	{
		check_and_config_attachments_based_on_views(aAttachments, aImageViews);
		return create_framebuffer(
			create_renderpass(std::move(aAttachments)),
			std::move(aImageViews),
			aWidth, aHeight,
			std::move(aAlterConfigBeforeCreation)
		);
	}

	framebuffer root::create_framebuffer(renderpass aRenderpass, std::vector<image_view> aImageViews, std::function<void(framebuffer_t&)> aAlterConfigBeforeCreation)
	{
		assert(!aImageViews.empty());
		auto extent = aImageViews.front()->get_image().create_info().extent;
		return create_framebuffer(std::move(aRenderpass), std::move(aImageViews), extent.width, extent.height, std::move(aAlterConfigBeforeCreation));
	}

	framebuffer root::create_framebuffer(std::vector<avk::attachment> aAttachments, std::vector<image_view> aImageViews, std::function<void(framebuffer_t&)> aAlterConfigBeforeCreation)
	{
		check_and_config_attachments_based_on_views(aAttachments, aImageViews);
		return create_framebuffer(
			create_renderpass(std::move(aAttachments)),
			std::move(aImageViews),
			std::move(aAlterConfigBeforeCreation)
		);
	}

	framebuffer root::create_framebuffer_from_template(const framebuffer_t& aTemplate, std::function<void(image_t&)> aAlterImageConfigBeforeCreation,
		std::function<void(image_view_t&)> aAlterImageViewConfigBeforeCreation, std::function<void(framebuffer_t&)> aAlterFramebufferConfigBeforeCreation)
	{
		const auto& templateImageViews = aTemplate.image_views();
		std::vector<image_view> imageViews;

		for (auto& imView : templateImageViews)	{
			auto imageView = create_image_view_from_template(*imView, aAlterImageConfigBeforeCreation, aAlterImageViewConfigBeforeCreation);
			imageViews.emplace_back(std::move(imageView));
		}

		return create_framebuffer(
			create_renderpass_from_template(aTemplate.renderpass_reference()),
			std::move(imageViews),
			aTemplate.mCreateInfo.width,
			aTemplate.mCreateInfo.height,
			aAlterFramebufferConfigBeforeCreation
		);
	}
#pragma endregion

#pragma region geometry instance definitions
#if VK_HEADER_VERSION >= 135
	geometry_instance root::create_geometry_instance(const bottom_level_acceleration_structure_t& aBlas)
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
			aBlas.device_address()
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
		viewport_depth_scissors_config viewport_depth_scissors_config::from_framebuffer(const framebuffer_t& aFramebuffer)
		{
			const auto width = aFramebuffer.create_info().width;
			const auto height = aFramebuffer.create_info().height;
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
			.setRasterizationSamples(aPreparedPipeline.renderpass_reference().num_samples_for_subpass(aPreparedPipeline.subpass_id()))
			.setPSampleMask(nullptr);

		aPreparedPipeline.mDynamicStateCreateInfo
			.setDynamicStateCount(static_cast<uint32_t>(aPreparedPipeline.mDynamicStateEntries.size()))
			.setPDynamicStates(aPreparedPipeline.mDynamicStateEntries.data());

		// Pipeline Layout must be rewired already before calling this function

		// Create the PIPELINE LAYOUT
		aPreparedPipeline.mPipelineLayout = device().createPipelineLayoutUnique(aPreparedPipeline.mPipelineLayoutCreateInfo, nullptr, dispatch_loader_core());
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
		auto result = device().createGraphicsPipelineUnique(nullptr, pipelineInfo, nullptr, dispatch_loader_core());
		aPreparedPipeline.mPipeline = std::move(result.value);
#else
		aPreparedPipeline.mPipeline = device().createGraphicsPipelineUnique(nullptr, pipelineInfo);
#endif
	}

	// Unfortunately C++20 does not have support to easily convert a range into a vector through chaining, so we have to
	// either pass the ranges begin and end members to the vector constructor every time or implement a helper ourself
	// until something similar is standardized (There is a to<Container> proposal for C++23).
	// Using a helper struct to invoke operator| for ranges was inspired by https://stackoverflow.com/a/65635762
	// This helper implementation converts a range into a vector by piping the range into to_vector():
	//
	// auto nums = std::vector{1, 2, 3, 4};
	// auto num_strings = nums | std::ranges::transform([](auto num) { return std::to_string(num); }) | to_vector();
	// 
	namespace to_vector_impl
	{
		// Helper type to find the operator| overload below
		struct to_vector_helper {};

		// Operator| overload to convert a range into a vector
		template <std::ranges::range R>
		auto operator|(R&& r, to_vector_helper)
		{
			std::vector<std::ranges::range_value_t<decltype(r)>> v;
			if constexpr (std::ranges::sized_range<decltype(r)>) {
				v.reserve(std::ranges::size(r));
			}
			std::ranges::copy(r, std::back_inserter(v));
			return v;
		}
	}

	auto to_vector()
	{
		return to_vector_impl::to_vector_helper{};
	}

	graphics_pipeline root::create_graphics_pipeline(graphics_pipeline_config aConfig, std::function<void(graphics_pipeline_t&)> aAlterConfigBeforeCreation)
	{
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
			// Select bindings as vector:
			auto bindings = aConfig.mInputBindingLocations
				| std::views::transform([](const auto& bindingData) { return bindingData.mGeneralData; })
				| to_vector();

			// Sort bindings:
			std::ranges::sort(bindings);
			// Remove duplicates of both vertex and instance inputs (this will invoke operator ==(const vertex_input_buffer_binding& left, const vertex_input_buffer_binding& right):
			auto [ret, last] = std::ranges::unique(bindings);
			bindings.erase(ret, last);
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
			auto universalConfig = aConfig.mColorBlendingPerAttachment
				| std::views::filter([](const color_blending_config& config) { return !config.mTargetAttachment.has_value(); })
				| to_vector();

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
				auto configForI = aConfig.mColorBlendingPerAttachment
					| std::views::filter([i](const color_blending_config& config) { return config.mTargetAttachment.has_value() && config.mTargetAttachment.value() == i; })
					| to_vector();
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
			vk::SampleCountFlagBits numSamples = (*result.mRenderPass).num_samples_for_subpass(result.subpass_id());
			
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

	graphics_pipeline root::create_graphics_pipeline_from_template(const graphics_pipeline_t& aTemplate, renderpass aNewRenderpass, std::optional<cfg::subpass_index> aSubpassIndex, std::function<void(graphics_pipeline_t&)> aAlterConfigBeforeCreation)
	{
		graphics_pipeline_t result;
		result.mRenderPass = std::move(aNewRenderpass);
		result.mSubpassIndex = aSubpassIndex.value_or(cfg::subpass_index{ aTemplate.mSubpassIndex }).mSubpassIndex;

		result.mOrderedVertexInputBindingDescriptions	= aTemplate.mOrderedVertexInputBindingDescriptions;
		result.mVertexInputAttributeDescriptions		= aTemplate.mVertexInputAttributeDescriptions	   ;
		result.mPipelineVertexInputStateCreateInfo		= aTemplate.mPipelineVertexInputStateCreateInfo   ;
		result.mInputAssemblyStateCreateInfo			= aTemplate.mInputAssemblyStateCreateInfo		   ;

		for (const auto& shdr : aTemplate.mShaders) {
			result.mShaders.push_back(create_shader_from_template(shdr));
		}

		result.mShaderStageCreateInfos					= aTemplate.mShaderStageCreateInfos					;
		result.mSpecializationInfos						= aTemplate.mSpecializationInfos				   ;
		result.mViewports								= aTemplate.mViewports							   ;
		result.mScissors								= aTemplate.mScissors							   ;
		result.mViewportStateCreateInfo					= aTemplate.mViewportStateCreateInfo			   ;
		result.mRasterizationStateCreateInfo			= aTemplate.mRasterizationStateCreateInfo		   ;
		result.mDepthStencilConfig						= aTemplate.mDepthStencilConfig						;
		result.mBlendingConfigsForColorAttachments		= aTemplate.mBlendingConfigsForColorAttachments   ;
		result.mColorBlendStateCreateInfo				= aTemplate.mColorBlendStateCreateInfo			   ;
		result.mMultisampleStateCreateInfo				= aTemplate.mMultisampleStateCreateInfo				;
		result.mDynamicStateEntries						= aTemplate.mDynamicStateEntries				   ;
		result.mDynamicStateCreateInfo					= aTemplate.mDynamicStateCreateInfo					;

		result.mAllDescriptorSetLayouts = create_set_of_descriptor_set_layouts_from_template(aTemplate.mAllDescriptorSetLayouts);

		result.mPushConstantRanges						= aTemplate.mPushConstantRanges						;
		result.mPipelineTessellationStateCreateInfo		= aTemplate.mPipelineTessellationStateCreateInfo  ;

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

	graphics_pipeline root::create_graphics_pipeline_from_template(const graphics_pipeline_t& aTemplate, std::function<void(graphics_pipeline_t&)> aAlterConfigBeforeCreation)
	{
		renderpass renderpassForPipeline;
		if (aTemplate.mRenderPass.is_shared_ownership_enabled()) {
			renderpassForPipeline = aTemplate.mRenderPass;
		}
		else {
			renderpassForPipeline = create_renderpass_from_template(*aTemplate.mRenderPass, {});
		}
		return create_graphics_pipeline_from_template(aTemplate, std::move(renderpassForPipeline), std::nullopt, std::move(aAlterConfigBeforeCreation));
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
			mImageUsage = aOther.mImageUsage;
			mAspectFlags = aOther.mAspectFlags;
		}
		else {
			throw avk::runtime_error("Can not copy this image instance!");
		}
	}

	image root::create_image_from_template(const image_t& aTemplate, std::function<void(image_t&)> aAlterConfigBeforeCreation)
	{
		image_t result;
		result.mRoot = aTemplate.mRoot;
		result.mCreateInfo = aTemplate.mCreateInfo;
		result.mImageUsage = aTemplate.mImageUsage;
		result.mAspectFlags = aTemplate.mAspectFlags;

		if (std::holds_alternative<vk::Image>(aTemplate.mImage)) {
			// If the template is a wrapped image, there's not a lot we can do.
			//result.mImage = std::get<vk::Image>(aTemplate.mImage);
			return result;
		}

		assert(!std::holds_alternative<std::monostate>(aTemplate.mImage));

		// Maybe alter the config?!
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(result);
		}

		result.mImage = AVK_MEM_IMAGE_HANDLE{ memory_allocator(), aTemplate.memory_properties(), result.mCreateInfo };

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
		result.mRoot = this;
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
		result.mRoot = this;
		result.mCreateInfo = aImageCreateInfo;
		result.mImage = aImageToWrap;
		result.mImageUsage = aImageUsage;
		result.mAspectFlags = aImageAspectFlags;
		return result;
	}

	vk::ImageSubresourceRange image_t::entire_subresource_range() const
	{
		return vk::ImageSubresourceRange{
			mAspectFlags,
			0u, VK_REMAINING_MIP_LEVELS,	// MIP info
			0u, VK_REMAINING_ARRAY_LAYERS	// Layers info
		};
	}

	avk::command::action_type_command image_t::generate_mip_maps(avk::layout::image_layout_transition aLayoutTransition)
	{
		if (create_info().mipLevels <= 1u) {
			return {};
		}
		
		auto actionTypeCommand = avk::command::action_type_command{
			{}, // Define a resource-specific sync hint here and let the general sync hint be inferred afterwards (because it is supposed to be exactly the same)
			{
				std::make_tuple(handle(), avk::sync::sync_hint{
					stage::transfer + (access::transfer_read | access::transfer_write), // This one is actually only here to establish a dependency chain to the first image layout transition
					stage::transfer +                          access::transfer_write   // This one as well
				})
			},
			[
				lRoot = root_ptr(),
				lImageHandle = handle(),
				lArrayLayers = create_info().arrayLayers,
				lWidth = static_cast<int32_t>(width()),
				lHeight = static_cast<int32_t>(height()),
				lAspectFlags = mAspectFlags,
				lMipLevels = create_info().mipLevels,
				aLayoutTransition
			](avk::command_buffer_t& cb) {

				auto w = lWidth;
				auto h = lHeight;

				for (uint32_t l = 0u; l < lArrayLayers; ++l) {
					
					std::array layoutTransitions = { // during the loop, we'll use 1 or 2 of these
						vk::ImageMemoryBarrier{
							vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eTransferWrite | vk::AccessFlagBits::eTransferRead, // Memory is available AND already visible for transfer read because that has been established in establish_barrier_before_the_operation above.
							aLayoutTransition.mOld.mLayout, vk::ImageLayout::eTransferSrcOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, lImageHandle, vk::ImageSubresourceRange{ lAspectFlags, 0u, 1u, l, 1u }},
						vk::ImageMemoryBarrier{
							vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eTransferWrite | vk::AccessFlagBits::eTransferRead, // This is the first mip-level we're going to write to
							aLayoutTransition.mOld.mLayout, vk::ImageLayout::eTransferDstOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, lImageHandle, vk::ImageSubresourceRange{ lAspectFlags, 1u, 1u, l, 1u }},
						vk::ImageMemoryBarrier{} // To be used in loop
					};

					cb.handle().pipelineBarrier(
						vk::PipelineStageFlagBits::eTransfer,
						vk::PipelineStageFlagBits::eTransfer,
						vk::DependencyFlags{},
						0u, nullptr,
						0u, nullptr,
						2u /* initially, only 2 required */, layoutTransitions.data()
					);

					for (uint32_t i = 1u; i < lMipLevels; ++i) {

						cb.handle().blitImage(
							lImageHandle, vk::ImageLayout::eTransferSrcOptimal,
							lImageHandle, vk::ImageLayout::eTransferDstOptimal,
							{ vk::ImageBlit{
								vk::ImageSubresourceLayers{ lAspectFlags, i - 1, l, 1u }, { vk::Offset3D{ 0, 0, 0 }, vk::Offset3D{ w                , h                , 1 } },
								vk::ImageSubresourceLayers{ lAspectFlags, i    , l, 1u }, { vk::Offset3D{ 0, 0, 0 }, vk::Offset3D{ w > 1 ? w / 2 : 1, h > 1 ? h / 2 : 1, 1 } }
							  }
							},
							vk::Filter::eLinear
						);

						// mip-level  i-1  is done:
						layoutTransitions[0] = vk::ImageMemoryBarrier{
							{}, {}, // Blit Read -> Done
							vk::ImageLayout::eTransferSrcOptimal, aLayoutTransition.mNew.mLayout, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, lImageHandle, vk::ImageSubresourceRange{ lAspectFlags, i - 1, 1u, l, 1u } };
						// mip-level   i   has been transfer destination, but is going to be transfer source:
						layoutTransitions[1] = vk::ImageMemoryBarrier{
							vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eTransferRead, // Blit Write -> Blit Read
							vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, lImageHandle, vk::ImageSubresourceRange{ lAspectFlags, i, 1u, l, 1u } };
						// mip-level  i+1  is entering the game:
						layoutTransitions[2] = vk::ImageMemoryBarrier{
							{}, vk::AccessFlagBits::eTransferWrite, // make visible to Blit Write
							aLayoutTransition.mOld.mLayout, vk::ImageLayout::eTransferDstOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, lImageHandle, vk::ImageSubresourceRange{ lAspectFlags, i + 1, 1u, l, 1u } };

						uint32_t numBarriersRequired = std::min(3u, lMipLevels - i + 1);
						if (lMipLevels - 1 == i) {
							layoutTransitions[1].newLayout = aLayoutTransition.mNew.mLayout; // Last one => done
						}

						cb.handle().pipelineBarrier(
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
			}
		};

		actionTypeCommand.infer_sync_hint_from_resource_sync_hints();

		return actionTypeCommand;
	}
#pragma endregion

#pragma region image view definitions
	image_view root::create_image_view_from_template(const image_view_t& aTemplate, std::function<void(image_t&)> aAlterImageConfigBeforeCreation, std::function<void(image_view_t&)> aAlterImageViewConfigBeforeCreation)
	{
		image_view_t result;

		// Transfer ownership:
		if (std::holds_alternative<image>(aTemplate.mImage)) {
			auto& im = std::get<image>(aTemplate.mImage);
			result.mImage = create_image_from_template(*im, std::move(aAlterImageConfigBeforeCreation));
			if (im.is_shared_ownership_enabled()) {
				std::get<image>(result.mImage).enable_shared_ownership();
			}
		}
		else {
			AVK_LOG_ERROR("Can not create an image_view from a template which wraps an image_t.");
		}

		result.mCreateInfo = aTemplate.mCreateInfo;
		result.mCreateInfo.setImage(result.get_image().handle());
		result.mCreateInfo.subresourceRange
			.setLevelCount(result.get_image().create_info().mipLevels)
			.setLayerCount(result.get_image().create_info().arrayLayers);

		result.mUsageInfo = aTemplate.mUsageInfo;

		// Maybe alter the config?!
		if (aAlterImageViewConfigBeforeCreation) {
			aAlterImageViewConfigBeforeCreation(result);
		}

		result.mImageView = device().createImageViewUnique(result.mCreateInfo, nullptr, dispatch_loader_core());

		return result;
	}


	image_view root::create_image_view(avk::image aImage, std::optional<vk::Format> aViewFormat, std::optional<avk::image_usage> aImageViewUsage, std::function<void(image_view_t&)> aAlterConfigBeforeCreation)
	{
		image_view_t result;

		// Transfer ownership:
		result.mImage = std::move(aImage);

		// What's the format of the image view?
		if (!aViewFormat.has_value()) {
			aViewFormat = result.get_image().format();
		}

		finish_configuration(result, aViewFormat.value(), {}, aImageViewUsage, std::move(aAlterConfigBeforeCreation));

		return result;
	}

	image_view root::create_depth_image_view(avk::image aImage, std::optional<vk::Format> aViewFormat, std::optional<avk::image_usage> aImageViewUsage, std::function<void(image_view_t&)> aAlterConfigBeforeCreation)
	{
		image_view_t result;

		// Transfer ownership:
		result.mImage = std::move(aImage);

		// What's the format of the image view?
		if (!aViewFormat.has_value()) {
			aViewFormat = result.get_image().format();
		}

		finish_configuration(result, aViewFormat.value(), vk::ImageAspectFlagBits::eDepth, aImageViewUsage, std::move(aAlterConfigBeforeCreation));

		return result;
	}

	image_view root::create_stencil_image_view(avk::image aImage, std::optional<vk::Format> aViewFormat, std::optional<avk::image_usage> aImageViewUsage, std::function<void(image_view_t&)> aAlterConfigBeforeCreation)
	{
		image_view_t result;

		// Transfer ownership:
		result.mImage = std::move(aImage);

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
			const auto imageFormat = aImageView.get_image().create_info().format;
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
		aImageView.mCreateInfo = vk::ImageViewCreateInfo{}
			.setImage(aImageView.get_image().handle())
			.setViewType(to_image_view_type(aImageView.get_image().create_info()))
			.setFormat(aViewFormat)
			.setComponents(vk::ComponentMapping() // The components field allows you to swizzle the color channels around. In our case we'll stick to the default mapping. [3]
							  .setR(vk::ComponentSwizzle::eIdentity)
							  .setG(vk::ComponentSwizzle::eIdentity)
							  .setB(vk::ComponentSwizzle::eIdentity)
							  .setA(vk::ComponentSwizzle::eIdentity))
			.setSubresourceRange(vk::ImageSubresourceRange() // The subresourceRange field describes what the image's purpose is and which part of the image should be accessed. Our images will be used as color targets without any mipmapping levels or multiple layers. [3]
				.setAspectMask(aImageAspectFlags.value())
				.setBaseMipLevel(0u)
				.setLevelCount(aImageView.get_image().create_info().mipLevels)
				.setBaseArrayLayer(0u)
				.setLayerCount(aImageView.get_image().create_info().arrayLayers));

		if (aImageViewUsage.has_value()) {
			auto [imageUsage, imageLayout, imageTiling, imageCreateFlags] = determine_usage_layout_tiling_flags_based_on_image_usage(aImageViewUsage.value());
			aImageView.mUsageInfo = vk::ImageViewUsageCreateInfo()
				.setUsage(imageUsage);
			aImageView.mCreateInfo.setPNext(&aImageView.mUsageInfo);
		}

		// Maybe alter the config?!
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(aImageView);
		}

		aImageView.mImageView = device().createImageViewUnique(aImageView.mCreateInfo, nullptr, dispatch_loader_core());
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
		result.mCreateInfo = vk::SamplerCreateInfo()
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

		result.mSampler = device().createSamplerUnique(result.create_info(), nullptr, dispatch_loader_core());
		result.mDescriptorInfo = vk::DescriptorImageInfo{}
			.setSampler(result.handle());
		result.mDescriptorType = vk::DescriptorType::eSampler;
		return result;
	}

	image_sampler root::create_image_sampler(image_view aImageView, sampler aSampler)
	{
		image_sampler_t result;
		result.mImageView = std::move(aImageView);
		result.mSampler = std::move(aSampler);
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
		avk::root* aRoot,
		uint32_t aQueueFamilyIndex,
		uint32_t aQueueIndex,
		float aQueuePriority
	)
	{
		auto queueFamilies = aRoot->physical_device().getQueueFamilyProperties();
		if (queueFamilies.size() <= aQueueFamilyIndex) {
			throw avk::runtime_error("Invalid queue family index in queue::prepare");
		}
		if (queueFamilies[aQueueFamilyIndex].queueCount <= aQueueIndex) {
			throw avk::runtime_error("Queue family #" + std::to_string(aQueueFamilyIndex) + " does not provide enough queues (requested index: " + std::to_string(aQueueIndex) + ")");
		}

		queue result;
		result.mRoot = aRoot;
		result.mQueueFamilyIndex = aQueueFamilyIndex;
		result.mQueueIndex = aQueueIndex;
		result.mPriority = aQueuePriority;
		result.mQueue = nullptr;
		return result;
	}

	void queue::assign_handle()
	{
		mQueue = mRoot->device().getQueue(mQueueFamilyIndex, mQueueIndex);
	}

	avk::submission_data queue::submit(avk::command_buffer_t& aCommandBuffer) const
	{
		return avk::submission_data(mRoot, aCommandBuffer, this);
	}

	bool queue::is_prepared() const
	{
		return nullptr != mRoot && static_cast<bool>(mRoot->physical_device());
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

		aPreparedPipeline.mPipelineLayout = device().createPipelineLayoutUnique(aPreparedPipeline.mPipelineLayoutCreateInfo, nullptr, dispatch_loader_core());
		assert(aPreparedPipeline.layout_handle());


		// Create the PIPELINE LAYOUT
		aPreparedPipeline.mPipelineLayout = device().createPipelineLayoutUnique(aPreparedPipeline.mPipelineLayoutCreateInfo, nullptr, dispatch_loader_core());
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
		auto pipeCreationResult = device().createRayTracingPipelineKHRUnique(
			{}, {},
			pipelineCreateInfo,
			nullptr,
			dispatch_loader_ext());
#else
		auto pipeCreationResult = device().createRayTracingPipelineKHRUnique(
			nullptr,
			pipelineCreateInfo,
			nullptr,
			dispatch_loader_ext());
#endif

		aPreparedPipeline.mPipeline = std::move(pipeCreationResult.value);
	}

	void root::build_shader_binding_table(ray_tracing_pipeline_t& aPipeline)
	{
		// According to https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/#shaderbindingtable this is the way:
		const uint32_t groupCount = static_cast<uint32_t>(aPipeline.mShaderGroupCreateInfos.size());
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
		auto result = device().getRayTracingShaderGroupHandlesKHR(aPipeline.handle(), 0, groupCount, shaderBindingTableSize, shaderHandleStorage.data(), dispatch_loader_ext());
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
		result.mRoot = this;

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

	ray_tracing_pipeline root::create_ray_tracing_pipeline_from_template(const ray_tracing_pipeline_t& aTemplate, std::function<void(ray_tracing_pipeline_t&)> aAlterConfigBeforeCreation)
	{
		ray_tracing_pipeline_t result;

		result.mPipelineCreateFlags						= aTemplate.mPipelineCreateFlags;
		for (const auto& shdr : aTemplate.mShaders) {
			result.mShaders.push_back(create_shader_from_template(shdr));
		}
		result.mShaderStageCreateInfos					= aTemplate.mShaderStageCreateInfos;
		result.mSpecializationInfos						= aTemplate.mSpecializationInfos;
		result.mShaderGroupCreateInfos					= aTemplate.mShaderGroupCreateInfos;
		result.mShaderBindingTableGroupsInfo			= aTemplate.mShaderBindingTableGroupsInfo;
		result.mMaxRecursionDepth						= aTemplate.mMaxRecursionDepth;
		result.mBasePipelineIndex						= aTemplate.mBasePipelineIndex;
		result.mAllDescriptorSetLayouts = create_set_of_descriptor_set_layouts_from_template(aTemplate.mAllDescriptorSetLayouts);
		result.mPushConstantRanges						= aTemplate.mPushConstantRanges;

		auto descriptorSetLayoutHandles = result.mAllDescriptorSetLayouts.layout_handles();
		result.mPipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo{}
			.setSetLayoutCount(static_cast<uint32_t>(descriptorSetLayoutHandles.size()))
			.setPSetLayouts(descriptorSetLayoutHandles.data())
			.setPushConstantRangeCount(static_cast<uint32_t>(result.mPushConstantRanges.size()))
			.setPPushConstantRanges(result.mPushConstantRanges.data());

		result.mShaderGroupBaseAlignment				= aTemplate.mShaderGroupBaseAlignment;
		result.mShaderGroupHandleSize					= aTemplate.mShaderGroupHandleSize;

		result.mRoot = this;

		// 15. Maybe alter the config?!
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(result);
		}

		rewire_config_and_create_ray_tracing_pipeline(result);
		build_shader_binding_table(result);
		return result;
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

	struct subpass_desc_helper
	{
		size_t mSubpassId;
		std::map<uint32_t, vk::AttachmentReference2KHR> mSpecificInputLocations;
		std::queue<vk::AttachmentReference2KHR> mUnspecifiedInputLocations;
		int mInputMaxLoc;
		std::map<uint32_t, vk::AttachmentReference2KHR> mSpecificColorLocations;
		std::queue<vk::AttachmentReference2KHR> mUnspecifiedColorLocations;
		int mColorMaxLoc;
		std::queue<vk::AttachmentReference2KHR> mUnspecifiedDepthStencilLocations;
		int mDepthStencilMaxLoc;
		std::map<uint32_t, vk::AttachmentReference2KHR> mSpecificColorResolveLocations;
		std::queue<vk::AttachmentReference2KHR> mUnspecifiedColorResolveLocations;
		std::queue<vk::SubpassDescriptionDepthStencilResolve> mUnspecifiedDepthStencilResolveData;
		std::queue<vk::AttachmentReference2KHR> mUnspecifiedDepthStencilResolveRefs;
		std::vector<uint32_t> mPreserveAttachments;
	};

	void root::rewire_subpass_descriptions(renderpass_t& aRenderpass)
	{
		const auto nSubpasses = aRenderpass.mSubpassData.size();

		aRenderpass.mSubpasses.clear(); // Start with a clean state
		aRenderpass.mSubpasses.reserve(nSubpasses);

		for (size_t i = 0; i < nSubpasses; ++i) {
			auto& b = aRenderpass.mSubpassData[i];

			assert(b.mOrderedDepthStencilResolveAttachmentRefs.size() == b.mOrderedDepthStencilResolveAttachmentData.size());
			const auto nDepthStencilResolve = b.mOrderedDepthStencilResolveAttachmentRefs.size();
			for (size_t ids = 0; ids < nDepthStencilResolve; ++ids) {
				b.mOrderedDepthStencilResolveAttachmentData[ids].setPDepthStencilResolveAttachment(
					&b.mOrderedDepthStencilResolveAttachmentRefs[ids]
				);
			}

			aRenderpass.mSubpasses.push_back(vk::SubpassDescription2KHR{}
				// pipelineBindPoint must be VK_PIPELINE_BIND_POINT_GRAPHICS [1] because subpasses are only relevant for graphics at the moment
				.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
				.setColorAttachmentCount(static_cast<uint32_t>(b.mOrderedColorAttachmentRefs.size()))
				.setPColorAttachments(b.mOrderedColorAttachmentRefs.data())
				// If pResolveAttachments is not NULL, each of its elements corresponds to a color attachment
				//  (the element in pColorAttachments at the same index), and a multisample resolve operation
				//  is defined for each attachment. [1]
				.setPResolveAttachments(b.mOrderedColorResolveAttachmentRefs.empty() ? nullptr : b.mOrderedColorResolveAttachmentRefs.data())
				// If pDepthStencilAttachment is NULL, or if its attachment index is VK_ATTACHMENT_UNUSED, it
				//  indicates that no depth/stencil attachment will be used in the subpass. [1]
				.setPDepthStencilAttachment(b.mOrderedDepthStencilAttachmentRefs.empty() ? nullptr : &b.mOrderedDepthStencilAttachmentRefs[0])
				// The following two attachment types are probably totally irrelevant if we only have one subpass
				.setInputAttachmentCount(static_cast<uint32_t>(b.mOrderedInputAttachmentRefs.size()))
				.setPInputAttachments(b.mOrderedInputAttachmentRefs.data())
				.setPreserveAttachmentCount(static_cast<uint32_t>(b.mPreserveAttachments.size()))
				.setPPreserveAttachments(b.mPreserveAttachments.data())
				// Add depth/stencil resolve attachment:
				.setPNext(b.mOrderedDepthStencilResolveAttachmentData.empty() ? nullptr : b.mOrderedDepthStencilResolveAttachmentData.data())
			);				
		}
	}


	/**	Creates memory barrier data which is appropriate for the given subpass dependency.
	 *	@param	aSubpassDependency	Subpass dependency to create memory barrier data for.
	 *	@return	Memory barrier data including stage and access masks.
	 */
	static inline vk::MemoryBarrier2KHR get_memory_barrier_for_subpass_dependency(const subpass_dependency& aSubpassDependency)
	{
		const bool srcStageIsAuto = std::holds_alternative<avk::stage::auto_stage_t>(aSubpassDependency.mStages.mSrc);
		const bool dstStageIsAuto = std::holds_alternative<avk::stage::auto_stage_t>(aSubpassDependency.mStages.mDst);
		const bool srcAccessIsAuto = std::holds_alternative<avk::access::auto_access_t>(aSubpassDependency.mAccesses.mSrc);
		const bool dstAccessIsAuto = std::holds_alternative<avk::access::auto_access_t>(aSubpassDependency.mAccesses.mDst);

		return vk::MemoryBarrier2KHR{}
			.setSrcStageMask(srcStageIsAuto
				? vk::PipelineStageFlags2KHR{
					VK_SUBPASS_EXTERNAL == aSubpassDependency.mIndices.mSrc
					? vk::PipelineStageFlagBits2KHR::eAllCommands // TODO: Try to determine tighter stage mask; needs more information about external commands, though
					: vk::PipelineStageFlagBits2KHR::eAllGraphics
				}
				: std::get<vk::PipelineStageFlags2KHR>(aSubpassDependency.mStages.mSrc)
			)
			.setDstStageMask(dstStageIsAuto
				? vk::PipelineStageFlags2KHR{
					VK_SUBPASS_EXTERNAL == aSubpassDependency.mIndices.mDst
					? vk::PipelineStageFlagBits2KHR::eAllCommands // TODO: Try to determine tighter stage mask; needs more information about external commands, though
					: vk::PipelineStageFlagBits2KHR::eAllGraphics
				}
				: std::get<vk::PipelineStageFlags2KHR>(aSubpassDependency.mStages.mDst)
			)
			.setSrcAccessMask(srcAccessIsAuto
				? vk::AccessFlags2KHR{ vk::AccessFlagBits2KHR::eMemoryWrite }  // TODO: Try to determine tighter access mask; needs more information about external commands, though
				: std::get<vk::AccessFlags2KHR>(aSubpassDependency.mAccesses.mSrc)
			)
			.setDstAccessMask(dstAccessIsAuto
				? vk::AccessFlags2KHR{ vk::AccessFlagBits2KHR::eMemoryWrite }  // TODO: Try to determine tighter access mask; needs more information about external commands, though
				: std::get<vk::AccessFlags2KHR>(aSubpassDependency.mAccesses.mDst)
			);
	}

	std::tuple<std::vector<vk::SubpassDependency2KHR>, std::vector<vk::MemoryBarrier2KHR>> root::compile_subpass_dependencies(const renderpass_t& aRenderpass)
	{
		const auto numSubpassesFirst = aRenderpass.mSubpassData.size();
		assert(numSubpassesFirst > 0);

		// And construct the actual dependency-info from it:
		std::vector<vk::SubpassDependency2KHR> subpassDependencies2;
		std::vector<vk::MemoryBarrier2KHR> memoryBarriers2;

		// Just turn them all into subpass dependencies:
		for (auto& subDep : aRenderpass.mSubpassDependencies) {
			subpassDependencies2.push_back(vk::SubpassDependency2KHR{}
				.setSrcSubpass(subDep.mIndices.mSrc).setDstSubpass(subDep.mIndices.mDst) // If a VkMemoryBarrier2 is included in the pNext chain, srcStageMask, dstStageMask, srcAccessMask, and dstAccessMask parameters are ignored.
				.setPNext(nullptr) // !!! ATTENTION: This will have to be set by the user of this function to point to the appropriate vk::MemoryBarrier2KHR{} !!!
			);
			memoryBarriers2.push_back(get_memory_barrier_for_subpass_dependency(subDep));
		}

		assert(subpassDependencies2.size() == memoryBarriers2.size());

		return std::make_tuple(std::move(subpassDependencies2), std::move(memoryBarriers2));
	}

	void root::rewire_subpass_dependencies(std::vector<vk::SubpassDependency2KHR>& aAlignedSubpassDependencies, std::vector<vk::MemoryBarrier2KHR>& aAlignedMemoryBarriers)
	{
		// Set pNext pointers in every vk::SubpassDependency2KHR
		for (size_t i = 0; i < aAlignedSubpassDependencies.size(); ++i) {
			aAlignedSubpassDependencies[i].setPNext(&aAlignedMemoryBarriers[i]);
			//aAlignedSubpassDependencies[i]
			//	.setPNext(nullptr)
			//	.setSrcStageMask(aAlignedMemoryBarriers[i].srcStageMask == vk::PipelineStageFlagBits2KHR::eNone ? (vk::PipelineStageFlags)vk::PipelineStageFlagBits::eTopOfPipe : (vk::PipelineStageFlags)(VkPipelineStageFlags)(VkPipelineStageFlags2KHR)aAlignedMemoryBarriers[i].srcStageMask)
			//	.setSrcAccessMask((vk::AccessFlags)(VkAccessFlags)(VkAccessFlags2KHR)aAlignedMemoryBarriers[i].srcAccessMask)
			//	.setDstStageMask(aAlignedMemoryBarriers[i].dstStageMask == vk::PipelineStageFlagBits2KHR::eNone ? (vk::PipelineStageFlags)vk::PipelineStageFlagBits::eBottomOfPipe : (vk::PipelineStageFlags)(VkPipelineStageFlags)(VkPipelineStageFlags2KHR)aAlignedMemoryBarriers[i].dstStageMask)
			//	.setDstAccessMask((vk::AccessFlags)(VkAccessFlags)(VkAccessFlags2KHR)aAlignedMemoryBarriers[i].dstAccessMask);
		}
	}

	renderpass root::create_renderpass(std::vector<avk::attachment> aAttachments, subpass_dependencies aSubpassDependencies, std::function<void(renderpass_t&)> aAlterConfigBeforeCreation)
	{
		renderpass_t result;
		result.mRoot = this;

		std::vector<subpass_desc_helper> subpasses;

		if (aAttachments.empty()) {
			throw avk::runtime_error("No attachments have been passed to the creation of a renderpass.");
		}
		const auto numSubpassesFirst = aAttachments.front().mSubpassUsages.num_subpasses();
		if (0 == numSubpassesFirst) {
			throw avk::runtime_error("The first subpass has no usages specified. Subpass usages are not optional.");
		}
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
			//// Try to infer initial and final image layouts (If this isn't cool => user must use aAlterConfigBeforeCreation)
			//vk::ImageLayout initialLayout = vk::ImageLayout::eUndefined;
			//vk::ImageLayout finalLayout = vk::ImageLayout::eUndefined;

			const auto isLoad = avk::on_load_behavior::load == a.mLoadOperation.mLoadBehavior;
			const auto isClear = avk::on_load_behavior::clear == a.mLoadOperation.mLoadBehavior;
			const auto isStore  = avk::on_store_behavior::store == a.mStoreOperation.mStoreBehavior;

			const auto hasSeparateStencilLoad = a.mStencilLoadOperation.has_value();
			const auto hasSeparateStencilStore = a.mStencilStoreOperation.has_value();
			const auto isStencilLoad = avk::on_load_behavior::load == a.get_stencil_load_op().mLoadBehavior;
			const auto isStencilClear = avk::on_load_behavior::clear == a.get_stencil_load_op().mLoadBehavior;
			const auto isStencilStore  = avk::on_store_behavior::store == a.get_stencil_store_op().mStoreBehavior;
			const auto hasStencilComponent = has_stencil_component(a.format());

			const auto attachmentAspect = is_depth_format(a.format())
				? vk::ImageAspectFlagBits::eDepth
				: vk::ImageAspectFlagBits::eColor;

			//bool initialLayoutFixed = false;
			//auto firstUsage = a.get_first_color_depth_input();
			//if (firstUsage.as_input()) {
			//	if (isLoad) {
			//		initialLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			//		initialLayoutFixed = true;
			//	}
			//	if (isClear) {
			//		initialLayoutFixed = true;
			//	}
			//}
			//if (firstUsage.as_color()) { // this potentially overwrites the above
			//	if (isLoad) {
			//		initialLayout = vk::ImageLayout::eColorAttachmentOptimal;
			//		initialLayoutFixed = true;
			//	}
			//	if (isClear) {
			//		initialLayoutFixed = true;
			//	}
			//}
			//if (firstUsage.as_depth_stencil()) {
			//	if (isLoad) {
			//		initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
			//		{
			//			// TODO: Set other depth/stencil-specific formats
			//			//       - vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal
			//			//       - vk::ImageLayout::eDepthStencilReadOnlyOptimal
			//			//       - vk::ImageLayout::eDepthReadOnlyOptimal
			//			//       - vk::ImageLayout::eStencilAttachmentOptimal
			//			//       - vk::ImageLayout::eStencilReadOnlyOptimal
			//		}
			//		initialLayoutFixed = true;
			//	}
			//	if (isClear) {
			//		initialLayoutFixed = true;
			//	}
			//}

			//auto lastUsage = a.get_last_color_depth_input();
			//if (lastUsage.as_input()) {
			//	finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			//}
			//if (lastUsage.as_color()) { // This potentially overwrites the above
			//	finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
			//}
			//if (lastUsage.as_depth_stencil()) {
			//	finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
			//	{
			//		// TODO: Set other depth/stencil-specific formats
			//		//       - vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal
			//		//       - vk::ImageLayout::eDepthStencilReadOnlyOptimal
			//		//       - vk::ImageLayout::eDepthReadOnlyOptimal
			//		//       - vk::ImageLayout::eStencilAttachmentOptimal
			//		//       - vk::ImageLayout::eStencilReadOnlyOptimal
			//	}
			//}
			//if (isStore && vk::ImageLayout::eUndefined == finalLayout) {
			//	if (a.is_used_as_color_attachment()) {
			//		finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
			//	}
			//	else if (a.is_used_as_depth_stencil_attachment()) {
			//		finalLayout = vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal;
			//	}
			//	else if (a.is_used_as_input_attachment()) {
			//		finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			//	}
			//}
			//if (vk::ImageLayout::eUndefined == finalLayout) {
			//	// We can just guess:
			//	finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			//}

			//if (a.shall_be_presentable()) {
			//	finalLayout = vk::ImageLayout::ePresentSrcKHR;
			//}

			//if (!initialLayoutFixed && isLoad) {
			//	initialLayout = finalLayout;
			//}
			//// ^^^ I have no idea what I'm assuming ^^^

			// 1. Create the attachment descriptions
			auto& newAttachmentDesc = result.mAttachmentDescriptions.emplace_back(vk::AttachmentDescription2KHR{}
				.setFormat(a.format())
				.setSamples(a.sample_count())
				.setLoadOp(to_vk_load_op(a.mLoadOperation.mLoadBehavior))
				.setStoreOp(to_vk_store_op(a.mStoreOperation.mStoreBehavior))
				.setStencilLoadOp(to_vk_load_op(a.get_stencil_load_op().mLoadBehavior))
				.setStencilStoreOp(to_vk_store_op(a.get_stencil_store_op().mStoreBehavior))
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

			// Now go over all the sub passes and remember initial and final layouts along the way
			std::optional<vk::ImageLayout> initialLayout{};
			std::optional<vk::ImageLayout> finalLayout{};
			auto useLayout = [&initialLayout, &finalLayout](vk::ImageLayout aLayout) {
				if (!initialLayout.has_value()) {
					initialLayout = aLayout;
				}
				finalLayout = aLayout;
				return aLayout;
			};

			for (size_t i = 0; i < nSubpasses; ++i) {
				auto& sp = subpasses[i];
				auto subpassUsage = a.mSubpassUsages.get_subpass_usage(i);
				auto subpassAspect = static_cast<bool>(subpassUsage.mAspectToBeUsed) ? subpassUsage.mAspectToBeUsed : vk::ImageAspectFlags{ attachmentAspect };
				if (subpassUsage.as_input()) {
					assert(!subpassUsage.has_resolve() || subpassUsage.as_color()); // Can not resolve input attachments, it's fine if it's also used as color attachment
					if (subpassUsage.has_input_location()) {
						auto loc = subpassUsage.input_location();
						if (sp.mSpecificInputLocations.count(loc) != 0) {
							throw avk::runtime_error("Layout location " + std::to_string(loc) + " is used multiple times for an input attachments in subpass " + std::to_string(i) + ". This is not allowed.");
						}
						sp.mSpecificInputLocations[loc] = vk::AttachmentReference2KHR{}
							.setAttachment(attachmentIndex)
							.setLayout(useLayout(vk::ImageLayout::eShaderReadOnlyOptimal))
							.setAspectMask(subpassAspect);
						sp.mInputMaxLoc = std::max(sp.mInputMaxLoc, loc);
					}
					else {
						AVK_LOG_WARNING("No layout location is specified for an input attachment in subpass " + std::to_string(i) + ". This might be problematic. Consider declaring it 'unused'.");
						sp.mUnspecifiedInputLocations.push(vk::AttachmentReference2KHR{}
							.setAttachment(attachmentIndex)
							.setLayout(useLayout(vk::ImageLayout::eShaderReadOnlyOptimal))
							.setAspectMask(subpassAspect)
						);
					}
				}
				if (subpassUsage.as_color()) {
					auto resolve = subpassUsage.has_resolve();
					if (subpassUsage.has_color_location()) {
						auto loc = subpassUsage.color_location();
						if (sp.mSpecificColorLocations.count(loc) != 0) {
							throw avk::runtime_error("Layout location " + std::to_string(loc) + " is used multiple times for a color attachments in subpass " + std::to_string(i) + ". This is not allowed.");
						}
						sp.mSpecificColorLocations[loc] =	vk::AttachmentReference2KHR{}
							.setAttachment(attachmentIndex)
							.setLayout(useLayout(vk::ImageLayout::eColorAttachmentOptimal))
							.setAspectMask(subpassAspect);
						sp.mSpecificColorResolveLocations[loc] =	vk::AttachmentReference2KHR{}
							.setAttachment(resolve ? subpassUsage.resolve_target_index() : VK_ATTACHMENT_UNUSED)
							.setLayout(useLayout(vk::ImageLayout::eColorAttachmentOptimal))
							.setAspectMask(subpassAspect);
						sp.mColorMaxLoc = std::max(sp.mColorMaxLoc, loc);
					}
					else {
						AVK_LOG_WARNING("No layout location is specified for a color attachment in subpass " + std::to_string(i) + ". This might be problematic. Consider declaring it 'unused'.");
						sp.mUnspecifiedColorLocations.push(vk::AttachmentReference2KHR{}
							.setAttachment(attachmentIndex)
							.setLayout(useLayout(vk::ImageLayout::eColorAttachmentOptimal))
							.setAspectMask(subpassAspect)
						);
						sp.mUnspecifiedColorResolveLocations.push(vk::AttachmentReference2KHR{}
							.setAttachment(resolve ? subpassUsage.resolve_target_index() : VK_ATTACHMENT_UNUSED)
							.setLayout(useLayout(vk::ImageLayout::eColorAttachmentOptimal))
							.setAspectMask(subpassAspect)
						);
					}
				}
				if (subpassUsage.as_depth_stencil()) {
					auto resolve = subpassUsage.has_resolve();
					sp.mUnspecifiedDepthStencilLocations.push(vk::AttachmentReference2KHR{}
						.setAttachment(attachmentIndex)
						.setLayout(useLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal))
						.setAspectMask(subpassAspect)
					);
					if (resolve) {
						sp.mUnspecifiedDepthStencilResolveData.push(vk::SubpassDescriptionDepthStencilResolve{}
							.setDepthResolveMode(  static_cast<bool>(subpassUsage.mResolveModeDepth)   ? static_cast<vk::ResolveModeFlagBits>(static_cast<vk::ResolveModeFlags::MaskType>(subpassUsage.mResolveModeDepth)  ) : vk::ResolveModeFlagBits::eSampleZero)
							.setStencilResolveMode(static_cast<bool>(subpassUsage.mResolveModeStencil) ? static_cast<vk::ResolveModeFlagBits>(static_cast<vk::ResolveModeFlags::MaskType>(subpassUsage.mResolveModeStencil)) : vk::ResolveModeFlagBits::eSampleZero)
							// pDepthStencilResolveAttachment to be set later to aligned vector   vvv   never forgetti!
						);
						sp.mUnspecifiedDepthStencilResolveRefs.push(vk::AttachmentReference2KHR{}
							.setAttachment(resolve ? subpassUsage.resolve_target_index() : VK_ATTACHMENT_UNUSED)
							.setLayout(useLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal))
							.setAspectMask(subpassAspect)
						);
					}
				}
				if (subpassUsage.as_preserve()) {
					assert(!subpassUsage.has_resolve() || subpassUsage.as_color()); // Can not resolve input attachments, it's fine if it's also used as color attachment
					assert(!subpassUsage.as_input() && !subpassUsage.as_color() && !subpassUsage.as_depth_stencil()); // Makes no sense to preserve and use as something else
					sp.mPreserveAttachments.push_back(attachmentIndex);
				}
				if (subpassUsage.as_unused()) {
					for (const auto& aaaarr : aAttachments) {
						if (aaaarr.mSubpassUsages.get_subpass_usage(i).resolve_target_index() == attachmentIndex) { // Talkin 'bout me, bro?
							if (aaaarr.mSubpassUsages.get_subpass_usage(i).as_color()) {
								useLayout(vk::ImageLayout::eColorAttachmentOptimal);
							}
							else if (aaaarr.mSubpassUsages.get_subpass_usage(i).as_depth_stencil()) {
								useLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
							}
						}
					}
				}
			}

			// Now that we have found all the subpass data => set initial and final layouts
			newAttachmentDesc
				.setInitialLayout(a.mLoadOperation.mPreviousLayout.has_value() ? a.mLoadOperation.mPreviousLayout.value().mLayout : initialLayout.value_or(vk::ImageLayout::eUndefined))
				.setFinalLayout(  a.mStoreOperation.mTargetLayout.has_value()  ? a.mStoreOperation.mTargetLayout.value().mLayout  : finalLayout.value_or(  vk::ImageLayout::eUndefined));
		}

		// 3. Fill all the vectors in the right order:
		const auto unusedAttachmentRef = vk::AttachmentReference2KHR{}.setAttachment(VK_ATTACHMENT_UNUSED);
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
					assert (a.mSpecificColorResolveLocations.count(loc) == 1);
					b.mOrderedColorAttachmentRefs.push_back(a.mSpecificColorLocations[loc]);
					b.mOrderedColorResolveAttachmentRefs.push_back(a.mSpecificColorResolveLocations[loc]);
				}
				else {
					if (!a.mUnspecifiedColorLocations.empty()) {
						assert(a.mUnspecifiedColorLocations.size() == a.mUnspecifiedColorResolveLocations.size());
						b.mOrderedColorAttachmentRefs.push_back(a.mUnspecifiedColorLocations.front());
						a.mUnspecifiedColorLocations.pop();
						b.mOrderedColorResolveAttachmentRefs.push_back(a.mUnspecifiedColorResolveLocations.front());
						a.mUnspecifiedColorResolveLocations.pop();
					}
					else {
						b.mOrderedColorAttachmentRefs.push_back(unusedAttachmentRef);
						b.mOrderedColorResolveAttachmentRefs.push_back(unusedAttachmentRef);
					}
				}
			}
			// DEPTH/STENCIL ATTACHMENTS
			for (int loc = 0; loc <= a.mDepthStencilMaxLoc || !a.mUnspecifiedDepthStencilLocations.empty(); ++loc) {
				if (!a.mUnspecifiedDepthStencilLocations.empty()) {
					b.mOrderedDepthStencilAttachmentRefs.push_back(a.mUnspecifiedDepthStencilLocations.front());
					a.mUnspecifiedDepthStencilLocations.pop();
					if (!a.mUnspecifiedDepthStencilResolveRefs.empty()) {
						assert(a.mUnspecifiedDepthStencilResolveRefs.size() == a.mUnspecifiedDepthStencilResolveData.size());
						b.mOrderedDepthStencilResolveAttachmentRefs.push_back(a.mUnspecifiedDepthStencilResolveRefs.front());
						a.mUnspecifiedDepthStencilResolveRefs.pop();
						b.mOrderedDepthStencilResolveAttachmentData.push_back(a.mUnspecifiedDepthStencilResolveData.front());
						a.mUnspecifiedDepthStencilResolveData.pop();
					}
				}
				else {
					b.mOrderedDepthStencilAttachmentRefs.push_back(unusedAttachmentRef);
				}
			}
			b.mPreserveAttachments = std::move(a.mPreserveAttachments);

			// SOME SANITY CHECKS:
			// - The color resolve attachments must either be empty or there must be a entry for each color attachment
			assert(b.mOrderedColorResolveAttachmentRefs.empty() || b.mOrderedColorResolveAttachmentRefs.size() == b.mOrderedColorAttachmentRefs.size());
			// - The depth/stencil resolve attachments must either be empty or there must be a entry for each depth/stencil attachment
			assert(b.mOrderedDepthStencilResolveAttachmentRefs.empty() || b.mOrderedDepthStencilResolveAttachmentRefs.size() == b.mOrderedDepthStencilAttachmentRefs.size());
			// - There must not be more than 1 depth/stencil attachements
			assert(b.mOrderedDepthStencilAttachmentRefs.size() <= 1);
		}

		// Done with the helper structure:
		subpasses.clear();

		// 4. Now we can fill the subpass description
		rewire_subpass_descriptions(result);

		// ======== Subpass Dependencies ==========
		// Store whatever the user has passed to not lose any data:
		result.mSubpassDependencies = std::move(aSubpassDependencies);
		auto [subpassDependencies, memoryBarriers] = compile_subpass_dependencies(result);
		rewire_subpass_dependencies(subpassDependencies, memoryBarriers);

		// Finally, create the render pass
		result.mCreateInfo = vk::RenderPassCreateInfo2KHR()
			.setAttachmentCount(static_cast<uint32_t>(result.mAttachmentDescriptions.size()))
			.setPAttachments(result.mAttachmentDescriptions.data())
			.setSubpassCount(static_cast<uint32_t>(result.mSubpasses.size()))
			.setPSubpasses(result.mSubpasses.data())
			.setDependencyCount(static_cast<uint32_t>(subpassDependencies.size()))
			.setPDependencies(subpassDependencies.data());

		// Maybe alter the config?!
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(result);
		}

		result.mRenderPass = device().createRenderPass2KHRUnique(result.mCreateInfo, nullptr, dispatch_loader_ext());
		return result;

		// TODO: Support VkSubpassDescriptionDepthStencilResolveKHR in order to enable resolve-settings for the depth attachment (see [1] and [2] for more details)

		// References:
		// [1] https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkSubpassDescription.html
		// [2] https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkSubpassDescriptionDepthStencilResolveKHR.html
		// [3] https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkPipelineStageFlagBits.html
	}

	renderpass root::create_renderpass_from_template(const renderpass_t& aTemplate, std::function<void(renderpass_t&)> aAlterConfigBeforeCreation)
	{
		renderpass_t result;
		result.mRoot                   = aTemplate.mRoot;
		result.mAttachmentDescriptions = aTemplate.mAttachmentDescriptions;
		result.mClearValues			   = aTemplate.mClearValues			  ;
		result.mSubpassData			   = aTemplate.mSubpassData			  ;
		rewire_subpass_descriptions(result); // this will set result.mSubpasses
		result.mSubpassDependencies	   = aTemplate.mSubpassDependencies	  ;

		// Maybe alter the config?!
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(result);
		}

		auto [subpassDependencies, memoryBarriers] = compile_subpass_dependencies(result);
		rewire_subpass_dependencies(subpassDependencies, memoryBarriers);

		// Finally, create the render pass
		auto createInfo = vk::RenderPassCreateInfo2KHR{}
			.setAttachmentCount(static_cast<uint32_t>(result.mAttachmentDescriptions.size()))
			.setPAttachments(result.mAttachmentDescriptions.data())
			.setSubpassCount(static_cast<uint32_t>(result.mSubpasses.size()))
			.setPSubpasses(result.mSubpasses.data())
			.setDependencyCount(static_cast<uint32_t>(subpassDependencies.size()))
			.setPDependencies(subpassDependencies.data());
		
		result.mRenderPass = device().createRenderPass2KHRUnique(createInfo, nullptr, dispatch_loader_ext());
		return result;
	}

	bool renderpass_t::is_input_attachment(uint32_t aSubpassId, size_t aAttachmentIndex) const
	{
		assert(aSubpassId < mSubpassData.size());
		auto& b = mSubpassData[aSubpassId];
		assert(aAttachmentIndex < mAttachmentDescriptions.size());
		return b.mOrderedInputAttachmentRefs.end() != std::find_if(std::begin(b.mOrderedInputAttachmentRefs), std::end(b.mOrderedInputAttachmentRefs),
			[aAttachmentIndex](const vk::AttachmentReference2KHR& ref) { return ref.attachment == aAttachmentIndex; });
	}

	bool renderpass_t::is_color_attachment(uint32_t aSubpassId, size_t aAttachmentIndex) const
	{
		assert(aSubpassId < mSubpassData.size());
		auto& b = mSubpassData[aSubpassId];
		assert(aAttachmentIndex < mAttachmentDescriptions.size());
		return b.mOrderedColorAttachmentRefs.end() != std::find_if(std::begin(b.mOrderedColorAttachmentRefs), std::end(b.mOrderedColorAttachmentRefs),
			[aAttachmentIndex](const vk::AttachmentReference2KHR& ref) { return ref.attachment == aAttachmentIndex; });
	}

	bool renderpass_t::is_depth_stencil_attachment(uint32_t aSubpassId, size_t aAttachmentIndex) const
	{
		assert(aSubpassId < mSubpassData.size());
		auto& b = mSubpassData[aSubpassId];
		assert(aAttachmentIndex < mAttachmentDescriptions.size());
		return b.mOrderedDepthStencilAttachmentRefs.end() != std::find_if(std::begin(b.mOrderedDepthStencilAttachmentRefs), std::end(b.mOrderedDepthStencilAttachmentRefs),
			[aAttachmentIndex](const vk::AttachmentReference2KHR& ref) { return ref.attachment == aAttachmentIndex; });
	}

	bool renderpass_t::is_resolve_attachment(uint32_t aSubpassId, size_t aAttachmentIndex) const
	{
		assert(aSubpassId < mSubpassData.size());
		auto& b = mSubpassData[aSubpassId];
		assert(aAttachmentIndex < mAttachmentDescriptions.size());
		return b.mOrderedColorResolveAttachmentRefs.end() != std::find_if(std::begin(b.mOrderedColorResolveAttachmentRefs), std::end(b.mOrderedColorResolveAttachmentRefs),
			[aAttachmentIndex](const vk::AttachmentReference2KHR& ref) { return ref.attachment == aAttachmentIndex; });
	}

	bool renderpass_t::is_preserve_attachment(uint32_t aSubpassId, size_t aAttachmentIndex) const
	{
		assert(aSubpassId < mSubpassData.size());
		auto& b = mSubpassData[aSubpassId];
		assert(aAttachmentIndex < mAttachmentDescriptions.size());
		return b.mPreserveAttachments.end() != std::find_if(std::begin(b.mPreserveAttachments), std::end(b.mPreserveAttachments),
			[aAttachmentIndex](uint32_t idx) { return idx == aAttachmentIndex; });
	}

	const std::vector<vk::AttachmentReference2KHR>& renderpass_t::input_attachments_for_subpass(uint32_t aSubpassId) const
	{
		assert(aSubpassId < mSubpassData.size());
		auto& b = mSubpassData[aSubpassId];
		return b.mOrderedInputAttachmentRefs;
	}

	const std::vector<vk::AttachmentReference2KHR>& renderpass_t::color_attachments_for_subpass(uint32_t aSubpassId) const
	{
		assert(aSubpassId < mSubpassData.size());
		auto& b = mSubpassData[aSubpassId];
		return b.mOrderedColorAttachmentRefs;
	}

	const std::vector<vk::AttachmentReference2KHR>& renderpass_t::depth_stencil_attachments_for_subpass(uint32_t aSubpassId) const
	{
		assert(aSubpassId < mSubpassData.size());
		auto& b = mSubpassData[aSubpassId];
		return b.mOrderedDepthStencilAttachmentRefs;
	}

	const std::vector<vk::AttachmentReference2KHR>& renderpass_t::resolve_attachments_for_subpass(uint32_t aSubpassId) const
	{
		assert(aSubpassId < mSubpassData.size());
		auto& b = mSubpassData[aSubpassId];
		return b.mOrderedColorResolveAttachmentRefs;
	}

	const std::vector<uint32_t>& renderpass_t::preserve_attachments_for_subpass(uint32_t aSubpassId) const
	{
		assert(aSubpassId < mSubpassData.size());
		auto& b = mSubpassData[aSubpassId];
		return b.mPreserveAttachments;
	}

	vk::SampleCountFlagBits renderpass_t::num_samples_for_subpass(uint32_t aSubpassId) const
	{
		vk::SampleCountFlagBits numSamples = vk::SampleCountFlagBits::e1;

		// See what is configured in the render pass
		auto colorAttConfigs = color_attachments_for_subpass(aSubpassId)
			| std::views::filter([](const vk::AttachmentReference2KHR& colorAttachment) { return colorAttachment.attachment != VK_ATTACHMENT_UNUSED; })
			| std::views::transform([this](const vk::AttachmentReference2KHR& colorAttachment) { return attachment_descriptions()[colorAttachment.attachment]; });

		for (const vk::AttachmentDescription2KHR& config : colorAttConfigs) {
			typedef std::underlying_type<vk::SampleCountFlagBits>::type EnumType;
			numSamples = static_cast<vk::SampleCountFlagBits>(std::max(static_cast<EnumType>(config.samples), static_cast<EnumType>(numSamples)));
		}

#if defined(_DEBUG)
		for (const vk::AttachmentDescription2KHR& config : colorAttConfigs) {
			if (config.samples != numSamples) {
				AVK_LOG_DEBUG("Not all of the color target attachments have the same number of samples configured, fyi. This might be fine, though.");
			}
		}
#endif

		if (vk::SampleCountFlagBits::e1 == numSamples) {
			auto depthAttConfigs = depth_stencil_attachments_for_subpass(aSubpassId)
				| std::views::filter([](const vk::AttachmentReference2KHR& depthStencilAttachment) { return depthStencilAttachment.attachment != VK_ATTACHMENT_UNUSED; })
				| std::views::transform([this](const vk::AttachmentReference2KHR& depthStencilAttachment) { return attachment_descriptions()[depthStencilAttachment.attachment]; });

			for (const vk::AttachmentDescription2KHR& config : depthAttConfigs) {
				typedef std::underlying_type<vk::SampleCountFlagBits>::type EnumType;
				numSamples = static_cast<vk::SampleCountFlagBits>(std::max(static_cast<EnumType>(config.samples), static_cast<EnumType>(numSamples)));
			}

#if defined(_DEBUG)
			for (const vk::AttachmentDescription2KHR& config : depthAttConfigs) {
				if (config.samples != numSamples) {
					AVK_LOG_DEBUG("Not all of the depth/stencil target attachments have the same number of samples configured, fyi. This might be fine, though.");
				}
			}
#endif

#if defined(_DEBUG)
			for (const vk::AttachmentDescription2KHR& config : colorAttConfigs) {
				if (config.samples != numSamples) {
					AVK_LOG_DEBUG("Some of the color target attachments have different numbers of samples configured as the depth/stencil attachments, fyi. This might be fine, though.");
				}
			}
#endif
		}

		return numSamples;
	}
#pragma endregion

#pragma region semaphore definitions
	semaphore_t::semaphore_t()
		: mCreateInfo{}
		, mSemaphore{}
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

	semaphore root::create_semaphore(vk::Device aDevice, const DISPATCH_LOADER_CORE_TYPE& aDispatchLoader, std::function<void(semaphore_t&)> aAlterConfigBeforeCreation)
	{
		semaphore_t result;
		result.mCreateInfo = vk::SemaphoreCreateInfo{};

		// Maybe alter the config?
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(result);
		}

		result.mSemaphore = aDevice.createSemaphoreUnique(result.mCreateInfo, nullptr, aDispatchLoader);
		return result;
	}

	semaphore root::create_semaphore(std::function<void(semaphore_t&)> aAlterConfigBeforeCreation)
	{
		return create_semaphore(device(), dispatch_loader_core(), std::move(aAlterConfigBeforeCreation));
	}

	semaphore_t& semaphore_t::handle_lifetime_of(any_owning_resource_t aResource)
	{
		mLifetimeHandledResources.push_back(std::move(aResource));
		return *this;
	}
#pragma endregion

#pragma region shader definitions
	shader shader::prepare(shader_info pInfo)
	{
		shader result;
		result.mInfo = std::move(pInfo);
		return result;
	}

	vk::UniqueHandle<vk::ShaderModule, DISPATCH_LOADER_CORE_TYPE> root::build_shader_module_from_binary_code(const std::vector<char>& aCode)
	{
		auto createInfo = vk::ShaderModuleCreateInfo()
			.setCodeSize(aCode.size())
			.setPCode(reinterpret_cast<const uint32_t*>(aCode.data()));

		return device().createShaderModuleUnique(createInfo, nullptr, dispatch_loader_core());
	}

	vk::UniqueHandle<vk::ShaderModule, DISPATCH_LOADER_CORE_TYPE> root::build_shader_module_from_file(const std::string& aPath)
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
	avk::command::action_type_command copy_image_to_another(avk::resource_argument<image_t> aSrcImage, avk::layout::image_layout aSrcImageLayout, avk::resource_argument<image_t> aDstImage, avk::layout::image_layout aDstImageLayout, vk::ImageAspectFlags aImageAspectFlags)
	{
		auto actionTypeCommand = avk::command::action_type_command {
			{}, // Define a resource-specific sync hint here and let the general sync hint be inferred afterwards (because it is supposed to be exactly the same)
			{
				std::make_tuple(aSrcImage->handle(), avk::sync::sync_hint{ stage::copy + access::transfer_read,  stage::copy + access::none           }),
				std::make_tuple(aDstImage->handle(), avk::sync::sync_hint{ stage::copy + access::transfer_write, stage::copy + access::transfer_write })
			},
			[
				lRoot = aSrcImage->root_ptr(),
				lSrcHandle = aSrcImage->handle(),
				lDstHandle = aDstImage->handle(),
				aSrcImageLayout, aDstImageLayout, aImageAspectFlags,
				lExtent = vk::Extent3D{ aSrcImage->width(), aSrcImage->height(), 1u }
			](avk::command_buffer_t& cb) {
				const vk::ImageCopy region {
					vk::ImageSubresourceLayers{ aImageAspectFlags, 0u, 0u, 1u}, vk::Offset3D{ 0, 0, 0 },
					vk::ImageSubresourceLayers{ aImageAspectFlags, 0u, 0u, 1u }, vk::Offset3D{ 0, 0, 0 }, lExtent
				};

				cb.handle().copyImage(
					lSrcHandle, aSrcImageLayout.mLayout,
					lDstHandle, aDstImageLayout.mLayout,
					1u, &region,
					lRoot->dispatch_loader_core()
				);
			}
		};

		if (aSrcImage.is_ownership() || aDstImage.is_ownership()) {
			actionTypeCommand.mEndFun = [
				lSrcImage = aSrcImage.move_ownership_or_get_empty(),
				lDstImage = aDstImage.move_ownership_or_get_empty()
			](avk::command_buffer_t& cb) mutable {
				let_it_handle_lifetime_of(cb, lSrcImage);
				let_it_handle_lifetime_of(cb, lDstImage);
			};
		};

		actionTypeCommand.infer_sync_hint_from_resource_sync_hints();

		return actionTypeCommand;
	}

	avk::command::action_type_command blit_image(avk::resource_argument<image_t> aSrcImage, avk::layout::image_layout aSrcImageLayout, avk::resource_argument<image_t> aDstImage, avk::layout::image_layout aDstImageLayout, vk::ImageAspectFlags aImageAspectFlags, vk::Filter aFilter)
	{
		auto actionTypeCommand = avk::command::action_type_command{
			{}, // Define a resource-specific sync hint here and let the general sync hint be inferred afterwards (because it is supposed to be exactly the same)
			{
				std::make_tuple(aSrcImage->handle(), avk::sync::sync_hint{ stage::blit + access::transfer_read,  stage::blit + access::none           }),
				std::make_tuple(aDstImage->handle(), avk::sync::sync_hint{ stage::blit + access::transfer_write, stage::blit + access::transfer_write })
			},
			[
				lRoot = aSrcImage->root_ptr(),
				lSrcHandle = aSrcImage->handle(),
				lDstHandle = aDstImage->handle(),
				aSrcImageLayout, aDstImageLayout, aImageAspectFlags, aFilter,
				lExtent = vk::Extent3D{ aSrcImage->width(), aSrcImage->height(), 1u }
			](avk::command_buffer_t& cb) {
				const std::array srcOffsets{ vk::Offset3D{ 0, 0, 0 }, vk::Offset3D{ static_cast<int32_t>(lExtent.width), static_cast<int32_t>(lExtent.height), 1 }};
				const std::array dstOffsets{ vk::Offset3D{ 0, 0, 0 }, vk::Offset3D{ static_cast<int32_t>(lExtent.width), static_cast<int32_t>(lExtent.height), 1 }};
				const vk::ImageBlit region {
					vk::ImageSubresourceLayers{ aImageAspectFlags, 0u, 0u, 1u }, srcOffsets,
					vk::ImageSubresourceLayers{ aImageAspectFlags, 0u, 0u, 1u }, dstOffsets
				};

				cb.handle().blitImage(
					lSrcHandle, aSrcImageLayout.mLayout,
					lDstHandle, aDstImageLayout.mLayout,
					1u, &region,
					aFilter,
					lRoot->dispatch_loader_core()
				);
			}
		};

		if (aSrcImage.is_ownership() || aDstImage.is_ownership()) {
			actionTypeCommand.mEndFun = [
				lSrcImage = aSrcImage.move_ownership_or_get_empty(),
				lDstImage = aDstImage.move_ownership_or_get_empty()
			](avk::command_buffer_t& cb) mutable {
				let_it_handle_lifetime_of(cb, lSrcImage);
				let_it_handle_lifetime_of(cb, lDstImage);
			};
		};

		actionTypeCommand.infer_sync_hint_from_resource_sync_hints();

		return actionTypeCommand;
	}

	avk::command::action_type_command copy_buffer_to_image_layer_mip_level(avk::resource_argument<buffer_t> aSrcBuffer, avk::resource_argument<image_t> aDstImage, uint32_t aDstLayer, uint32_t aDstLevel, avk::layout::image_layout aDstImageLayout, vk::ImageAspectFlags aImageAspectFlags)
	{
		auto extent = aDstImage->create_info().extent;
		extent.width  = extent.width  > 1u ? extent.width  >> aDstLevel : 1u;
		extent.height = extent.height > 1u ? extent.height >> aDstLevel : 1u;
		extent.depth  = extent.depth  > 1u ? extent.depth  >> aDstLevel : 1u;

		auto actionTypeCommand = avk::command::action_type_command{
			{}, // Define a resource-specific sync hint here and let the general sync hint be inferred afterwards (because it is supposed to be exactly the same)
			{
				std::make_tuple(aSrcBuffer->handle(), avk::sync::sync_hint{ stage::copy + access::transfer_read,  stage::copy + access::none           }),
				std::make_tuple(aDstImage->handle(),  avk::sync::sync_hint{ stage::copy + access::transfer_write, stage::copy + access::transfer_write })
			},
			[
				lRoot = aSrcBuffer->root_ptr(),
				lSrcHandle = aSrcBuffer->handle(),
				lDstHandle = aDstImage->handle(),
				aDstLayer, aDstLevel, aDstImageLayout, aImageAspectFlags,
				extent
			](avk::command_buffer_t& cb) {
				// The bufferRowLength and bufferImageHeight fields specify how the pixels are laid out in memory. For example, you could have some padding 
				// bytes between rows of the image. Specifying 0 for both indicates that the pixels are simply tightly packed like they are in our case. [3]
				const vk::BufferImageCopy region {
					0, 0u, 0u,
					vk::ImageSubresourceLayers{ aImageAspectFlags, aDstLevel, aDstLayer, 1u },
					vk::Offset3D{ 0, 0, 0 }, extent
				};
				cb.handle().copyBufferToImage(
					lSrcHandle, 
					lDstHandle, aDstImageLayout.mLayout,
					1u, &region,
					lRoot->dispatch_loader_core()
				);
			}
		};

		if (aSrcBuffer.is_ownership() || aDstImage.is_ownership()) {
			actionTypeCommand.mEndFun = [
				lSrcBuffer = aSrcBuffer.move_ownership_or_get_empty(),
				lDstImage = aDstImage.move_ownership_or_get_empty()
			](avk::command_buffer_t& cb) mutable {
				let_it_handle_lifetime_of(cb, lSrcBuffer);
				let_it_handle_lifetime_of(cb, lDstImage);
			};
		};

		actionTypeCommand.infer_sync_hint_from_resource_sync_hints();

		return actionTypeCommand;
	}

	avk::command::action_type_command copy_buffer_to_image_mip_level(avk::resource_argument<buffer_t> aSrcBuffer, avk::resource_argument<image_t> aDstImage, uint32_t aDstLevel, avk::layout::image_layout aDstImageLayout, vk::ImageAspectFlags aImageAspectFlags)
	{
		return copy_buffer_to_image_layer_mip_level(std::move(aSrcBuffer), std::move(aDstImage), 0u, aDstLevel, aDstImageLayout, aImageAspectFlags);
	}

	avk::command::action_type_command copy_buffer_to_image(avk::resource_argument<buffer_t> aSrcBuffer, avk::resource_argument<image_t> aDstImage, avk::layout::image_layout aDstImageLayout, vk::ImageAspectFlags aImageAspectFlags)
	{
		return copy_buffer_to_image_mip_level(std::move(aSrcBuffer), std::move(aDstImage), 0u, aDstImageLayout, aImageAspectFlags);
	}

	avk::command::action_type_command copy_buffer_to_another(avk::resource_argument<buffer_t> aSrcBuffer, avk::resource_argument<buffer_t> aDstBuffer, std::optional<vk::DeviceSize> aSrcOffset, std::optional<vk::DeviceSize> aDstOffset, std::optional<vk::DeviceSize> aDataSize)
	{
		vk::DeviceSize dataSize{ 0 };
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
			assert(aSrcOffset.value_or(0) + dataSize <= metaDataSrc.total_size());
			assert(aDstOffset.value_or(0) + dataSize <= metaDataDst.total_size());
			assert(aSrcOffset.value_or(0) + dataSize <= metaDataDst.total_size());
		}
#endif

		auto actionTypeCommand = avk::command::action_type_command{
			{}, // Define a resource-specific sync hint here and let the general sync hint be inferred afterwards (because it is supposed to be exactly the same)
			{
				std::make_tuple(aSrcBuffer->handle(), avk::sync::sync_hint{ stage::copy + access::transfer_read , stage::copy + access::none           }),
				std::make_tuple(aDstBuffer->handle(), avk::sync::sync_hint{ stage::copy + access::transfer_write, stage::copy + access::transfer_write })
			},
			[
				lRoot = aSrcBuffer->root_ptr(),
				lSrcHandle = aSrcBuffer->handle(),
				lDstHandle = aDstBuffer->handle(),
				lSrcOffset = aSrcOffset.value_or(0),
				lDstOffset = aDstOffset.value_or(0),
				dataSize
			](avk::command_buffer_t& cb) {
				const vk::BufferCopy region {
					lSrcOffset, lDstOffset, dataSize
				};
				cb.handle().copyBuffer(
					lSrcHandle, 
					lDstHandle,
					1u, &region, 
					lRoot->dispatch_loader_core()
				);
			}
		};

		if (aSrcBuffer.is_ownership() || aDstBuffer.is_ownership()) {
			actionTypeCommand.mEndFun = [
				lSrcBuffer = aSrcBuffer.move_ownership_or_get_empty(),
				lDstBuffer = aDstBuffer.move_ownership_or_get_empty()
			](avk::command_buffer_t& cb) mutable {
				let_it_handle_lifetime_of(cb, lSrcBuffer);
				let_it_handle_lifetime_of(cb, lDstBuffer);
			};
		};

		actionTypeCommand.infer_sync_hint_from_resource_sync_hints();

		return actionTypeCommand;
	}

	avk::command::action_type_command copy_image_layer_mip_level_to_buffer(avk::resource_argument<image_t> aSrcImage, avk::layout::image_layout aSrcImageLayout, uint32_t aSrcLayer, uint32_t aSrcLevel, vk::ImageAspectFlags aImageAspectFlags, avk::resource_argument<buffer_t> aDstBuffer, std::optional<vk::DeviceSize> aDstOffset)
	{
		auto extent = aSrcImage->create_info().extent;
		extent.width = extent.width > 1u ? extent.width >> aSrcLevel : 1u;
		extent.height = extent.height > 1u ? extent.height >> aSrcLevel : 1u;
		extent.depth = extent.depth > 1u ? extent.depth >> aSrcLevel : 1u;

		auto actionTypeCommand = avk::command::action_type_command{
			{}, // Define a resource-specific sync hint here and let the general sync hint be inferred afterwards (because it is supposed to be exactly the same)
			{
				std::make_tuple(aSrcImage->handle() , avk::sync::sync_hint{ stage::copy + access::transfer_read , stage::copy + access::none           }),
				std::make_tuple(aDstBuffer->handle(), avk::sync::sync_hint{ stage::copy + access::transfer_write, stage::copy + access::transfer_write })
			},
			[
				lRoot = aSrcImage->root_ptr(),
				lSrcHandle = aSrcImage->handle(), aSrcImageLayout,
				lDstHandle = aDstBuffer->handle(),
				lDstOffset = aDstOffset.value_or(0),
				aSrcLayer, aSrcLevel, aImageAspectFlags,
				extent
			](avk::command_buffer_t& cb) {
				// The bufferRowLength and bufferImageHeight fields specify how the pixels are laid out in memory. For example, you could have some padding 
				// bytes between rows of the image. Specifying 0 for both indicates that the pixels are simply tightly packed like they are in our case. [3]
				const vk::BufferImageCopy region {
					lDstOffset, 0u, 0u,
					vk::ImageSubresourceLayers{ aImageAspectFlags, aSrcLevel, aSrcLayer, 1u },
					vk::Offset3D{ 0, 0, 0 }, extent
				};
				cb.handle().copyImageToBuffer(
					lSrcHandle, aSrcImageLayout.mLayout,
					lDstHandle,
					1u, &region, 
					lRoot->dispatch_loader_ext()
				);
			}
		};

		if (aSrcImage.is_ownership() || aDstBuffer.is_ownership()) {
			actionTypeCommand.mEndFun = [
				lSrcImage = aSrcImage.move_ownership_or_get_empty(),
				lDstBuffer = aDstBuffer.move_ownership_or_get_empty()
			](avk::command_buffer_t& cb) mutable {
				let_it_handle_lifetime_of(cb, lSrcImage);
				let_it_handle_lifetime_of(cb, lDstBuffer);
			};
		};

		actionTypeCommand.infer_sync_hint_from_resource_sync_hints();

		return actionTypeCommand;
	}

	avk::command::action_type_command copy_image_mip_level_to_buffer(avk::resource_argument<image_t> aSrcImage, avk::layout::image_layout aSrcImageLayout, uint32_t aSrcLevel, vk::ImageAspectFlags aImageAspectFlags, avk::resource_argument<buffer_t> aDstBuffer, std::optional<vk::DeviceSize> aDstOffset)
	{
		return copy_image_layer_mip_level_to_buffer(std::move(aSrcImage), aSrcImageLayout, 0u, aSrcLevel, aImageAspectFlags, std::move(aDstBuffer), aDstOffset);
	}

	avk::command::action_type_command copy_image_to_buffer(avk::resource_argument<image_t> aSrcImage, avk::layout::image_layout aSrcImageLayout, vk::ImageAspectFlags aImageAspectFlags, avk::resource_argument<buffer_t> aDstBuffer, std::optional<vk::DeviceSize> aDstOffset)
	{
		return copy_image_mip_level_to_buffer(std::move(aSrcImage), aSrcImageLayout, 0u, aImageAspectFlags, std::move(aDstBuffer), aDstOffset);
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
		result.mQueryPool = device().createQueryPoolUnique(result.mCreateInfo, nullptr, dispatch_loader_core());

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
		return create_query_pool(vk::QueryType::ePipelineStatistics, aQueryCount, aPipelineStatistics);
	}

	void query_pool_t::host_reset(uint32_t aFirstQueryIndex, std::optional<uint32_t> aNumQueries)
	{
		mQueryPool.getOwner().resetQueryPool(handle(), aFirstQueryIndex, aNumQueries.value_or(create_info().queryCount - aFirstQueryIndex));
	}

	avk::command::action_type_command query_pool_t::reset(uint32_t aFirstQueryIndex, std::optional<uint32_t> aNumQueries)
	{
		return avk::command::action_type_command{
			{}, {}, // Sync not applicable here, I guess
			[
				lHandle = handle(),
				aFirstQueryIndex,
				lNumQueries = aNumQueries.value_or(create_info().queryCount - aFirstQueryIndex)
			](avk::command_buffer_t& cb) {
				cb.handle().resetQueryPool(lHandle, aFirstQueryIndex, lNumQueries, cb.root_ptr()->dispatch_loader_core());
			}
		};
	}

	avk::command::action_type_command query_pool_t::write_timestamp(uint32_t aQueryIndex, stage::pipeline_stage_flags_precisely aTimestampStage)
	{
		return avk::command::action_type_command{
			{ aTimestampStage + access::none, aTimestampStage + access::none }, // <-- Guess, that's fine. // TODO: Is it?
			{}, 
			[
				lTimestampStage = aTimestampStage.mStage,
				lHandle = handle(),
				aQueryIndex
			](avk::command_buffer_t& cb) {
				cb.handle().writeTimestamp2KHR(lTimestampStage, lHandle, aQueryIndex, cb.root_ptr()->dispatch_loader_core());
			}
		};
	}

	avk::command::action_type_command query_pool_t::begin_query(uint32_t aQueryIndex, vk::QueryControlFlags aFlags)
	{
		return avk::command::action_type_command{
			{}, {}, // Sync not applicable here, I guess
			[
				lHandle = handle(),
				aQueryIndex,
				aFlags
			](avk::command_buffer_t& cb) {
				cb.handle().beginQuery(lHandle, aQueryIndex, aFlags, cb.root_ptr()->dispatch_loader_core());
			}
		};
	}

	avk::command::action_type_command query_pool_t::end_query(uint32_t aQueryIndex)
	{
		return avk::command::action_type_command{
			{}, {}, // Sync not applicable here, I guess
			[
				lHandle = handle(),
				aQueryIndex
			](avk::command_buffer_t& cb) {
				cb.handle().endQuery(lHandle, aQueryIndex, cb.root_ptr()->dispatch_loader_core());
			}
		};
	}

	avk::command::action_type_command query_pool_t::copy_results(uint32_t aFirstQueryIndex, uint32_t aNumQueries, const buffer_t& aBuffer, size_t aBufferMetaSkip, vk::QueryResultFlags aFlags)
	{
		auto actionTypeCommand = avk::command::action_type_command{
			{}, // Define a resource-specific sync hint here and let the general sync hint be inferred afterwards (because it is supposed to be exactly the same)
			{
				// As per the specification:
				//   vkCmdCopyQueryPoolResults is considered to be a transfer operation, and its writes to buffer memory must be
				//   synchronized using VK_PIPELINE_STAGE_TRANSFER_BIT and VK_ACCESS_TRANSFER_WRITE_BIT before using the results.
				std::make_tuple(aBuffer.handle(), avk::sync::sync_hint{
					stage::transfer + access::transfer_write,
					stage::transfer + access::transfer_write
				})
			},
			[
				lHandle = handle(),
				aFirstQueryIndex, aNumQueries, aFlags,
				lBufferHandle = aBuffer.handle(),
				lMeta = aBuffer.meta<query_results_buffer_meta>(aBufferMetaSkip)
			](avk::command_buffer_t& cb) {
				cb.handle().copyQueryPoolResults(lHandle, aFirstQueryIndex, aNumQueries, lBufferHandle, lMeta.member_description(content_description::query_result).mOffset, lMeta.sizeof_one_element(), aFlags, cb.root_ptr()->dispatch_loader_core());
			}
		};

		actionTypeCommand.infer_sync_hint_from_resource_sync_hints();

		return actionTypeCommand;
	}

	avk::command::action_type_command query_pool_t::copy_result(uint32_t aOnlyQueryIndex, const buffer_t& aBuffer, size_t aBufferMetaSkip, vk::QueryResultFlags aFlags)
	{
		return copy_results(aOnlyQueryIndex, 1u, aBuffer, aBufferMetaSkip, aFlags);
	}
#pragma endregion

#pragma region commands and sync
	inline static void record_into_command_buffer(command_buffer_t& aCommandBuffer, const DISPATCH_LOADER_EXT_TYPE& aDispatchLoader, const std::vector<recorded_commands_t>& aRecordedCommandsAndSyncInstructions);

	template <typename T>
	inline static T accumulate_sync_details(
		const std::vector<recorded_commands_t>& aRecordedCommandsAndSyncInstructions,
		const int aStartIndex,
		uint32_t aNumSteps,
		const int aStepDirection,
		T aDefaultValue,
		// If the following parameter is provided, it means that we are looking for sync-dependencies w.r.t. this resource only.
		// If it is not, we are looking for ANY sync-dependencies (e.g., relevant for semaphore signals)
		std::optional<std::variant<vk::Image, vk::Buffer>> aWrtResource 
	) {
		assert(aStartIndex >= 0);
		assert(aStartIndex < static_cast<int>(aRecordedCommandsAndSyncInstructions.size()));
		assert(!std::holds_alternative<command::action_type_command>(aRecordedCommandsAndSyncInstructions[aStartIndex]));
		assert(aNumSteps >= 0);
		assert(aStepDirection == -1 || aStepDirection == 1);
		
		T result{};

		const int ub = static_cast<int>(aRecordedCommandsAndSyncInstructions.size());

		// Doesn't make sense if aNumSteps is less than 1, but the user could pass it (e.g., thorugh stage::auto_stages(0)) => just max it:
		aNumSteps = std::max(aNumSteps, 1u);
		uint32_t accSoFar = 0; // This must become equal to aNumSteps, then we're done

		for (int i = aStartIndex; i >= 0 && i < ub && accSoFar < aNumSteps; i += aStepDirection) {
			if (std::holds_alternative<command::action_type_command>(aRecordedCommandsAndSyncInstructions[i])) { // Only regard action_type_commands here!
																											     // TODO: Do we also have to regard image layout transitions here????!?!?!?!
				// We used the general sync hint, except if we are looking for a specific resource:
				auto* syncHint = &std::get<command::action_type_command>(aRecordedCommandsAndSyncInstructions[i]).mSyncHint;
				if (aWrtResource.has_value()) {
					// See if we can find a resource-specific one (otherwise do not regard this action_type_command):
					syncHint = nullptr;
					for (const auto& [res, resSyncHint] : std::get<command::action_type_command>(aRecordedCommandsAndSyncInstructions[i]).mResourceSpecificSyncHints) {
						if (res == aWrtResource.value()) {
							syncHint = &resSyncHint;
							break;
						}
					}
				}

				if (nullptr == syncHint) {
					continue;
				}

				switch (aStepDirection) {
				case -1:
					// Moving backwards, i.e. the previous command(s) "AFTER" values are relevant:
					if constexpr (std::is_same_v<T, vk::PipelineStageFlags2KHR>) {
						if (syncHint->mSrcForSubsequentCmds.has_value()) {
							result |= syncHint->mSrcForSubsequentCmds.value().mStage;
						}
						else {
							result |= aDefaultValue;
						}
						accSoFar += 1;
					}
					else if constexpr (std::is_same_v<T, vk::AccessFlags2KHR>) {
						if (syncHint->mSrcForSubsequentCmds.has_value()) {
							result |= syncHint->mSrcForSubsequentCmds.value().mAccess;
						}
						else {
							result |= aDefaultValue;
						}
						accSoFar += 1;
					}
					else {
						throw avk::logic_error("Unsupported T in function accumulate_sync_details.");
					}
					break;
				case  1:
					// Moving forwards, i.e. the subsequent command(s) "BEFORE" values are relevant:
					if constexpr (std::is_same_v<T, vk::PipelineStageFlags2KHR>) {
						if (syncHint->mDstForPreviousCmds.has_value()) {
							result |= syncHint->mDstForPreviousCmds.value().mStage;
						}
						else {
							result |= aDefaultValue;
						}
						accSoFar += 1;
					}
					else if constexpr (std::is_same_v<T, vk::AccessFlags2KHR>) {
						if (syncHint->mDstForPreviousCmds.has_value()) {
							result |= syncHint->mDstForPreviousCmds.value().mAccess;
						}
						else {
							result |= aDefaultValue;
						}
						accSoFar += 1;
					}
					else {
						throw avk::logic_error("Unsupported T in function accumulate_sync_details.");
					}
					break;
				default:
					throw avk::logic_error("Invalid value for aStepDirection.");
				}
			}
		}

		// If we were unable to find anything to sync with, just return the default:
		if (0 == accSoFar) {
			result = aDefaultValue;
		}
		return result;
	}

	// Internal helper function to assemble all the data for a barrier, based on:
	//  - A given sync_type_command (aBarrierData)
	//  - All the sync_hints of action_type_commands aRecordedCommandsAndSyncInstructions
	template <typename T>
	inline static T assemble_barrier_data(
		const sync::sync_type_command& aBarrierData, 
		const std::vector<recorded_commands_t>& aRecordedCommandsAndSyncInstructions,
		int aRecordedStuffIndex
	) {
		// Sanity check: Does T and aBarrierData fit together?
		assert(
			   (std::is_same_v<T, vk::MemoryBarrier2KHR>       && (aBarrierData.is_global_execution_barrier() || aBarrierData.is_global_memory_barrier()))
			|| (std::is_same_v<T, vk::ImageMemoryBarrier2KHR>  && (aBarrierData.is_image_memory_barrier()                                               ))
			|| (std::is_same_v<T, vk::BufferMemoryBarrier2KHR> && (aBarrierData.is_buffer_memory_barrier()                                              ))
		);

		if (!aRecordedCommandsAndSyncInstructions.empty()) {
			assert(std::holds_alternative<sync::sync_type_command>(aRecordedCommandsAndSyncInstructions[aRecordedStuffIndex]));
			if (!std::holds_alternative<sync::sync_type_command>(aRecordedCommandsAndSyncInstructions[aRecordedStuffIndex])) {
				throw avk::logic_error("The element at aRecordedStuffIndex[" + std::to_string(aRecordedStuffIndex) + "] is not of type sync_type_command.");
			}
		}

		// We're definitely going to establish a barrier:
		auto barrier = T{};

		// The barrier can be restricted to a specific resource only, in which case, we should only accumulate
		// sync data from relevant sync hints (i.e., relevant means: restricted to the same resource):
		std::optional<std::variant<vk::Image, vk::Buffer>> restrictedToSpecificResource;
		if constexpr (std::is_same_v<T, vk::ImageMemoryBarrier2KHR>) {
			restrictedToSpecificResource = aBarrierData.image_memory_barrier_data().mImage;
		}
		if constexpr (std::is_same_v<T, vk::BufferMemoryBarrier2KHR>) {
			restrictedToSpecificResource = aBarrierData.buffer_memory_barrier_data().mBuffer;
		}
		
		// Handle source stage:
		std::visit(lambda_overload{
			[&barrier                                                            ](const std::monostate&){
				barrier.setSrcStageMask(vk::PipelineStageFlagBits2KHR::eNone);
			},
			[&barrier                                                            ](const vk::PipelineStageFlags2KHR& bFixedStage){
				barrier.setSrcStageMask(bFixedStage);
			},
			[&barrier, &aRecordedCommandsAndSyncInstructions, aRecordedStuffIndex, &restrictedToSpecificResource](const avk::stage::auto_stage_t& bAutoStage){
				if (aRecordedCommandsAndSyncInstructions.empty()) {
					barrier.setSrcStageMask(vk::PipelineStageFlagBits2KHR::eAllCommands);
				}
				else {
					// Gotta determine which stage:
					barrier.setSrcStageMask(accumulate_sync_details<vk::PipelineStageFlags2KHR>(
						aRecordedCommandsAndSyncInstructions, 
						/* Start index: within std::vector<recorded_commands_t>: */ aRecordedStuffIndex,
						/* How many steps to accumulate: */ static_cast<int>(bAutoStage),
						/* before-wards: */ -1,
						/* If we can't determine something specific, employ a heavy barrier to ensure correctness: */ vk::PipelineStageFlagBits2KHR::eAllCommands,
						restrictedToSpecificResource
					));
				}
			},
		}, aBarrierData.src_stage());

		// Handle destination stage:
		std::visit(lambda_overload{
			[&barrier                                                            ](const std::monostate&){
				barrier.setDstStageMask(vk::PipelineStageFlagBits2KHR::eNone);
			},
			[&barrier                                                            ](const vk::PipelineStageFlags2KHR& bFixedStage){
				barrier.setDstStageMask(bFixedStage);
			},
			[&barrier, &aRecordedCommandsAndSyncInstructions, aRecordedStuffIndex, &restrictedToSpecificResource](const avk::stage::auto_stage_t& bAutoStage){
				if (aRecordedCommandsAndSyncInstructions.empty()) {
					barrier.setDstStageMask(vk::PipelineStageFlagBits2KHR::eAllCommands);
				}
				else {
					// Gotta determine which stage:
					barrier.setDstStageMask(accumulate_sync_details<vk::PipelineStageFlags2KHR>(
						aRecordedCommandsAndSyncInstructions, 
						/* Start index: within std::vector<recorded_commands_t>: */ aRecordedStuffIndex,
						/* How many steps to accumulate: */ static_cast<int>(bAutoStage),
						/* after-wards: */  1,
						/* If we can't determine something specific, employ a heavy barrier to ensure correctness: */ vk::PipelineStageFlagBits2KHR::eAllCommands,
						restrictedToSpecificResource
					));
				}
			},
		}, aBarrierData.dst_stage());

		// Handle source access:
		std::visit(lambda_overload{
			[&barrier                                                            ](const std::monostate&){
				barrier.setSrcAccessMask(vk::AccessFlagBits2KHR::eNone);
			},
			[&barrier                                                            ](const vk::AccessFlags2KHR& bFixedAccess){
				barrier.setSrcAccessMask(bFixedAccess);
			},
			[&barrier, &aRecordedCommandsAndSyncInstructions, aRecordedStuffIndex, &restrictedToSpecificResource](const avk::access::auto_access_t& bAutoAccess){
				if (aRecordedCommandsAndSyncInstructions.empty()) {
					barrier.setSrcAccessMask(vk::AccessFlagBits2KHR::eMemoryWrite);
				}
				else {
					// Gotta determine which access:
					barrier.setSrcAccessMask(accumulate_sync_details<vk::AccessFlags2KHR>(
						aRecordedCommandsAndSyncInstructions, 
						/* Start index: within std::vector<recorded_commands_t>: */ aRecordedStuffIndex,
						/* How many steps to accumulate: */ static_cast<int>(bAutoAccess),
						/* before-wards: */ -1,
						/* If we can't determine something specific, employ a heavy access mask to ensure correctness: */ vk::AccessFlagBits2KHR::eMemoryWrite,
						restrictedToSpecificResource
					));
				}
			},
		}, aBarrierData.src_access());

		// Handle destination access:
		std::visit(lambda_overload{
			[&barrier                                                            ](const std::monostate&){
				barrier.setDstAccessMask(vk::AccessFlagBits2KHR::eNone);
			},
			[&barrier                                                            ](const vk::AccessFlags2KHR& bFixedAccess){
				barrier.setDstAccessMask(bFixedAccess);
			},
			[&barrier, &aRecordedCommandsAndSyncInstructions, aRecordedStuffIndex, &restrictedToSpecificResource](const avk::access::auto_access_t& bAutoAccess){
				if (aRecordedCommandsAndSyncInstructions.empty()) {
					barrier.setSrcAccessMask(vk::AccessFlagBits2KHR::eMemoryWrite | vk::AccessFlagBits2KHR::eMemoryRead);
				}
				else {
					// Gotta determine which access:
					barrier.setDstAccessMask(accumulate_sync_details<vk::AccessFlags2KHR>(
						aRecordedCommandsAndSyncInstructions, 
						/* Start index: within std::vector<recorded_commands_t>: */ aRecordedStuffIndex,
						/* How many steps to accumulate: */ static_cast<uint32_t>(bAutoAccess),
						/* after-wards: */  1,
						/* If we can't determine something specific, employ a heavy access mask to ensure correctness: */ vk::AccessFlagBits2KHR::eMemoryWrite | vk::AccessFlagBits2KHR::eMemoryRead,
						restrictedToSpecificResource
					));
				}
			},
		}, aBarrierData.dst_access());

		// For T = vk::MemoryBarrier2KHR, we are done.
		// But for image memory barriers or buffer memory barriers, there could be more sync data to be filled-in:
		
		if constexpr (std::is_same_v<T, vk::ImageMemoryBarrier2KHR>) {
			auto imageSyncData = aBarrierData.image_memory_barrier_data();

			barrier.setImage(imageSyncData.mImage);
			barrier.setSubresourceRange(imageSyncData.mSubresourceRange);

			// Specification goes like this:
			// > When the old and new layout are equal, the layout values are ignored - data is preserved
			// > no matter what values are specified, or what layout the image is currently in.
			if (imageSyncData.mLayoutTransition.has_value()) {
				barrier.setOldLayout(imageSyncData.mLayoutTransition.value().mOld.mLayout);
				barrier.setNewLayout(imageSyncData.mLayoutTransition.value().mNew.mLayout);
			}
			// else leave both set to 0 which corresponds to eUndefined -> eUndefined, a.k.a. no layout transition
		}

		if constexpr (std::is_same_v<T, vk::BufferMemoryBarrier2KHR>) {
			auto bufferSyncData = aBarrierData.buffer_memory_barrier_data();
			barrier.setBuffer(bufferSyncData.mBuffer);
			barrier.setOffset(bufferSyncData.mOffset);
			barrier.setSize(bufferSyncData.mSize);
		}

		// For both, buffer memory barriers and image memory barriers, queue family o	wnership transfers are relevant:
		if constexpr (std::is_same_v<T, vk::ImageMemoryBarrier2KHR> || std::is_same_v<T, vk::BufferMemoryBarrier2KHR>) {
			auto qfot = aBarrierData.queue_family_ownership_transfer();
			barrier.setSrcQueueFamilyIndex(qfot.has_value() ? qfot.value().mSrcQueueFamilyIndex : VK_QUEUE_FAMILY_IGNORED);
			barrier.setDstQueueFamilyIndex(qfot.has_value() ? qfot.value().mDstQueueFamilyIndex : VK_QUEUE_FAMILY_IGNORED);
		}

		return barrier;
	}

	inline static void record_into_command_buffer(command_buffer_t& aCommandBuffer, const DISPATCH_LOADER_EXT_TYPE& aDispatchLoader, const command::state_type_command& aStateCmd)
	{
		if (aStateCmd.mFun) {
			aStateCmd.mFun(aCommandBuffer);
		}
	}

	inline static void record_into_command_buffer(command_buffer_t& aCommandBuffer, const DISPATCH_LOADER_EXT_TYPE& aDispatchLoader, const command::action_type_command& aActionCmd)
	{
		if (aActionCmd.mBeginFun) {
			aActionCmd.mBeginFun(aCommandBuffer);
		}
		if (!aActionCmd.mNestedCommandsAndSyncInstructions.empty()) {
			record_into_command_buffer(aCommandBuffer, aDispatchLoader, aActionCmd.mNestedCommandsAndSyncInstructions);
		}
		if (aActionCmd.mEndFun) {
			aActionCmd.mEndFun(aCommandBuffer);
		}
	}

	inline static void record_into_command_buffer(
		command_buffer_t& aCommandBuffer, 
		const DISPATCH_LOADER_EXT_TYPE& aDispatchLoader, 
		const sync::sync_type_command& aSyncCmd, 
		const std::vector<recorded_commands_t>& aRecordedCommandsAndSyncInstructions, 
		int aRecordedStuffIndex)
	{
		if (aSyncCmd.is_global_execution_barrier() || aSyncCmd.is_global_memory_barrier()) {
			auto barrier = assemble_barrier_data<vk::MemoryBarrier2KHR>(aSyncCmd, aRecordedCommandsAndSyncInstructions, aRecordedStuffIndex);
			auto dependencyInfo = vk::DependencyInfoKHR{}
				.setMemoryBarrierCount(1u)
				.setPMemoryBarriers(&barrier);
			aCommandBuffer.handle().pipelineBarrier2KHR(dependencyInfo, aDispatchLoader);
		}
		else if (aSyncCmd.is_image_memory_barrier()) {
			auto barrier = assemble_barrier_data<vk::ImageMemoryBarrier2KHR>(aSyncCmd, aRecordedCommandsAndSyncInstructions, aRecordedStuffIndex);
			auto dependencyInfo = vk::DependencyInfoKHR{}
				.setImageMemoryBarrierCount(1u)
				.setPImageMemoryBarriers(&barrier);
			aCommandBuffer.handle().pipelineBarrier2KHR(dependencyInfo, aDispatchLoader);
		}
		else if (aSyncCmd.is_buffer_memory_barrier()) {
			auto barrier = assemble_barrier_data<vk::BufferMemoryBarrier2KHR>(aSyncCmd, aRecordedCommandsAndSyncInstructions, aRecordedStuffIndex);
			auto dependencyInfo = vk::DependencyInfoKHR{}
				.setBufferMemoryBarrierCount(1u)
				.setPBufferMemoryBarriers(&barrier);
			aCommandBuffer.handle().pipelineBarrier2KHR(dependencyInfo, aDispatchLoader);
		}
	}


	void command_buffer_t::record(const avk::command::state_type_command& aToBeRecorded)
	{
		record_into_command_buffer(*this, root_ptr()->dispatch_loader_ext(), aToBeRecorded);
	}

	void command_buffer_t::record(const avk::command::action_type_command& aToBeRecorded)
	{
		record_into_command_buffer(*this, root_ptr()->dispatch_loader_ext(), aToBeRecorded);
	}

	void command_buffer_t::record(const avk::sync::sync_type_command& aToBeRecorded)
	{
		record_into_command_buffer(*this, root_ptr()->dispatch_loader_ext(), aToBeRecorded, std::vector<recorded_commands_t>{}, 0);
	}


	struct recordee_visitors
	{
		void operator()(const command::state_type_command& vStateCmd) const {
			record_into_command_buffer(mCommandBuffer, mDispatchLoaderExt, vStateCmd);
		}
		void operator()(const command::action_type_command& vActionCmd) const {
			record_into_command_buffer(mCommandBuffer, mDispatchLoaderExt, vActionCmd);
			
		}
		void operator()(const sync::sync_type_command& vSyncCmd) const {
			record_into_command_buffer(mCommandBuffer, mDispatchLoaderExt, vSyncCmd, mRecordedStuff, mCurrentIndexIntoRecordedStuff);
		}

		command_buffer_t& mCommandBuffer;
		const DISPATCH_LOADER_EXT_TYPE& mDispatchLoaderExt;
		const std::vector<recorded_commands_t>& mRecordedStuff;
		int mCurrentIndexIntoRecordedStuff;
	};
	
	inline static void record_into_command_buffer(command_buffer_t& aCommandBuffer, const DISPATCH_LOADER_EXT_TYPE& aDispatchLoader, const std::vector<recorded_commands_t>& aRecordedCommandsAndSyncInstructions)
	{
		recordee_visitors visitState{ aCommandBuffer, aDispatchLoader, aRecordedCommandsAndSyncInstructions, /* Current index: */ 0 };
		
		const int n = static_cast<int>(aRecordedCommandsAndSyncInstructions.size());
		for (int i = 0; i < n; ++i) {
			// Get current element:
			auto& recordee = aRecordedCommandsAndSyncInstructions[i];
			// Update current index:
			visitState.mCurrentIndexIntoRecordedStuff = i;
			// Handle current element:
			std::visit(visitState, recordee);
		}
	}
	
	submission_data recorded_command_buffer::then_waiting_for(avk::semaphore_wait_info aWaitInfo)
	{
		return submission_data{ mRoot, mCommandBufferToRecordInto, std::move(aWaitInfo) };
	}

	submission_data recorded_command_buffer::then_submit_to(const queue* aQueue)
	{
		return submission_data{ mRoot, mCommandBufferToRecordInto, aQueue, this };
	}

	recorded_command_buffer::recorded_command_buffer(const root* aRoot, const std::vector<recorded_commands_t>& aRecordedCommandsAndSyncInstructions, avk::resource_argument<avk::command_buffer_t> aCommandBuffer, const avk::recorded_commands* aDangerousRecordedCommandsPointer)
		: mRoot{ aRoot }
		, mCommandBufferToRecordInto{ std::move(aCommandBuffer) }
		, mDangerousRecordedComandsPointer{ aDangerousRecordedCommandsPointer }
	{
		mCommandBufferToRecordInto.get().begin_recording();
		record_into_command_buffer(mCommandBufferToRecordInto.get(), mRoot->dispatch_loader_ext(), aRecordedCommandsAndSyncInstructions);
		mCommandBufferToRecordInto.get().end_recording();
	}

	recorded_commands::recorded_commands(const root* aRoot, std::vector<recorded_commands_t> aRecordedCommandsAndSyncInstructions)
		: mRoot{ aRoot }
		, mRecordedCommandsAndSyncInstructions{ std::move(aRecordedCommandsAndSyncInstructions) }
	{
		for (auto& recordee : mRecordedCommandsAndSyncInstructions) {
			if (std::holds_alternative<avk::command::action_type_command>(recordee)) {
				for (auto& lifetime : std::get<avk::command::action_type_command>(recordee).mLifetimeHandledResources) {
					handle_lifetime_of(std::move(lifetime));
				}
				std::get<avk::command::action_type_command>(recordee).mLifetimeHandledResources.clear();
			}
		}
	}
	
	recorded_commands& recorded_commands::move_into(std::vector<recorded_commands_t>& aTarget)
	{
		aTarget = std::move(mRecordedCommandsAndSyncInstructions);
		return *this;
	}

	recorded_commands& recorded_commands::prepend_by(std::vector<recorded_commands_t>& aCommands)
	{
		mRecordedCommandsAndSyncInstructions.insert(std::begin(mRecordedCommandsAndSyncInstructions), std::begin(aCommands), std::end(aCommands));
		return *this;
	}

	recorded_commands& recorded_commands::append_by(std::vector<recorded_commands_t>& aTarget)
	{
		mRecordedCommandsAndSyncInstructions.insert(std::end(mRecordedCommandsAndSyncInstructions), std::begin(aTarget), std::end(aTarget));
		return *this;
	}

	recorded_commands& recorded_commands::handle_lifetime_of(any_owning_resource_t aResource)
	{
		mLifetimeHandledResources.push_back(std::move(aResource));
		return *this;
	}

	std::vector<recorded_commands_t> recorded_commands::and_store()
	{
		return std::move(mRecordedCommandsAndSyncInstructions);
	}

	recorded_command_buffer recorded_commands::into_command_buffer(avk::resource_argument<avk::command_buffer_t> aCommandBuffer)
	{
		recorded_command_buffer result(mRoot, mRecordedCommandsAndSyncInstructions, std::move(aCommandBuffer), this);

		for (int i = static_cast<int>(mLifetimeHandledResources.size() - 1); i > 0; --i) {
			if (std::visit(lambda_overload{
				[](const bottom_level_acceleration_structure& a) { return a.is_shared_ownership_enabled(); },
				[](const buffer&                              a) { return a.is_shared_ownership_enabled(); },
				[](const buffer_view&                         a) { return a.is_shared_ownership_enabled(); },
				[](const command_buffer&                      a) { return a.is_shared_ownership_enabled(); },
				[](const command_pool&                        a) { return a.is_shared_ownership_enabled(); },
				[](const compute_pipeline&                    a) { return a.is_shared_ownership_enabled(); },
				[](const fence&                               a) { return a.is_shared_ownership_enabled(); },
				[](const framebuffer&                         a) { return a.is_shared_ownership_enabled(); },
				[](const graphics_pipeline&                   a) { return a.is_shared_ownership_enabled(); },
				[](const image&                               a) { return a.is_shared_ownership_enabled(); },
				[](const image_sampler&                       a) { return a.is_shared_ownership_enabled(); },
				[](const image_view&                          a) { return a.is_shared_ownership_enabled(); },
				[](const query_pool&                          a) { return a.is_shared_ownership_enabled(); },
				[](const ray_tracing_pipeline&                a) { return a.is_shared_ownership_enabled(); },
				[](const renderpass&                          a) { return a.is_shared_ownership_enabled(); },
				[](const sampler&                             a) { return a.is_shared_ownership_enabled(); },
				[](const semaphore&                           a) { return a.is_shared_ownership_enabled(); },
				[](const top_level_acceleration_structure&    a) { return a.is_shared_ownership_enabled(); }
			}, mLifetimeHandledResources[i])) {
				// Copy and possibly reuse in future:
				result.handling_lifetime_of(mLifetimeHandledResources[i]);
			}
			else {
				// Move and remove:
				result.handling_lifetime_of(std::move(mLifetimeHandledResources[i]));
				mLifetimeHandledResources.erase(std::begin(mLifetimeHandledResources) + i);
			}
		}

		return result;
	}

	namespace command
	{
		action_type_command begin_render_pass_for_framebuffer(const renderpass_t& aRenderpass, const framebuffer_t& aFramebuffer, vk::Offset2D aRenderAreaOffset, std::optional<vk::Extent2D> aRenderAreaExtent, bool aSubpassesInline)
		{
			return action_type_command{
				// Define a sync hint that corresponds to the implicit subpass dependencies (see specification chapter 8.1)
				avk::sync::sync_hint {
					{{ // What previous commands must synchronize with:
						vk::PipelineStageFlagBits2KHR::eAllCommands, // eAllGraphics does not include new stages or ext-stages. Therefore, eAllCommands!
						vk::AccessFlagBits2KHR::eInputAttachmentRead | vk::AccessFlagBits2KHR::eColorAttachmentRead | vk::AccessFlagBits2KHR::eColorAttachmentWrite | vk::AccessFlagBits2KHR::eDepthStencilAttachmentRead | vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite
					}},
					{{ // What subsequent commands must synchronize with:
						vk::PipelineStageFlagBits2KHR::eAllCommands, // Same comment as above regarding eAllCommands vs. eAllGraphics
						vk::AccessFlagBits2KHR::eColorAttachmentWrite | vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite
					}}
				},
				{},
				[
					lRoot = aRenderpass.root_ptr(),
					lExtent = aFramebuffer.image_view_at(0)->get_image().create_info().extent,
					lClearValues = aRenderpass.clear_values(),
					lRenderPassHandle = aRenderpass.handle(),
					lFramebufferHandle = aFramebuffer.handle(),
					aRenderAreaOffset, aRenderAreaExtent, aSubpassesInline
				](avk::command_buffer_t& cb) {
					// TODO: make vk::SubpassContentscontents state explicit
					cb.save_subpass_contents_state(aSubpassesInline ? vk::SubpassContents::eInline : vk::SubpassContents::eSecondaryCommandBuffers);

					auto renderPassBeginInfo = vk::RenderPassBeginInfo()
						.setRenderPass(lRenderPassHandle)
						.setFramebuffer(lFramebufferHandle)
						.setRenderArea(vk::Rect2D{}
							.setOffset(vk::Offset2D{ aRenderAreaOffset.x, aRenderAreaOffset.y })
							.setExtent(aRenderAreaExtent.has_value()
										? vk::Extent2D{ aRenderAreaExtent.value() }
										: vk::Extent2D{ lExtent.width, lExtent.height }
								)
							)
						.setClearValueCount(static_cast<uint32_t>(lClearValues.size()))
						.setPClearValues(lClearValues.data());

					cb.handle().beginRenderPass2KHR(
						renderPassBeginInfo,
						vk::SubpassBeginInfo{ aSubpassesInline ? vk::SubpassContents::eInline : vk::SubpassContents::eSecondaryCommandBuffers },
						lRoot->dispatch_loader_ext()
					);
				}
			};
		}

		action_type_command end_render_pass()
		{
			return action_type_command{
				// Define a sync hint that corresponds to the implicit subpass dependencies (see specification chapter 8.1)
				avk::sync::sync_hint {
					{{ // What previous commands must synchronize with:
						vk::PipelineStageFlagBits2KHR::eAllCommands, // eAllGraphics does not include new stages or ext-stages. Therefore, eAllCommands!
						vk::AccessFlagBits2KHR::eInputAttachmentRead | vk::AccessFlagBits2KHR::eColorAttachmentRead | vk::AccessFlagBits2KHR::eColorAttachmentWrite | vk::AccessFlagBits2KHR::eDepthStencilAttachmentRead | vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite
					}},
					{{ // What subsequent commands must synchronize with:
						vk::PipelineStageFlagBits2KHR::eAllCommands, // Same comment as above regarding eAllCommands vs. eAllGraphics
						vk::AccessFlagBits2KHR::eColorAttachmentWrite | vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite
					}}
				},
				{},
				[](avk::command_buffer_t& cb) {
					cb.handle().endRenderPass2KHR(vk::SubpassEndInfo{}, cb.root_ptr()->dispatch_loader_ext());
				}
			};
		}
		
		action_type_command render_pass(
			const renderpass_t& aRenderpass,
			const framebuffer_t& aFramebuffer,
			std::vector<recorded_commands_t> aNestedCommands,
			vk::Offset2D aRenderAreaOffset,
			std::optional<vk::Extent2D> aRenderAreaExtent,
			bool aSubpassesInline)
		{
			auto tmpBeginRenderPass = begin_render_pass_for_framebuffer(aRenderpass, aFramebuffer, aRenderAreaOffset, aRenderAreaExtent, aSubpassesInline);
			auto tmpEndRenderPass = end_render_pass();

			return action_type_command{
				// Define a sync hint that corresponds to the implicit subpass dependencies (see specification chapter 8.1)
				avk::sync::sync_hint {
					tmpBeginRenderPass.mSyncHint.mDstForPreviousCmds,
					tmpEndRenderPass.mSyncHint.mSrcForSubsequentCmds
				},
				std::move(tmpBeginRenderPass.mResourceSpecificSyncHints),
				std::move(tmpBeginRenderPass.mBeginFun),
				std::move(aNestedCommands),
				std::move(tmpEndRenderPass.mBeginFun)
			};
		}

		action_type_command next_subpass(bool aSubpassesInline)
		{
			return action_type_command{
				{}, {}, // Sync hints not applicable here, I guess
				[aSubpassesInline](avk::command_buffer_t& cb) {
					cb.handle().nextSubpass2KHR(
						vk::SubpassBeginInfo{ aSubpassesInline ? vk::SubpassContents::eInline : vk::SubpassContents::eSecondaryCommandBuffers },
						vk::SubpassEndInfo{},
						cb.root_ptr()->dispatch_loader_ext()
					);
				}
			};
		}

		state_type_command bind_pipeline(const graphics_pipeline_t& aPipeline)
		{
			return state_type_command{
				[
					lPipelineHandle = aPipeline.handle()
				] (avk::command_buffer_t& cb) {
					cb.handle().bindPipeline(vk::PipelineBindPoint::eGraphics, lPipelineHandle);
				}
			};
		}

		state_type_command bind_pipeline(const compute_pipeline_t& aPipeline)
		{
			return state_type_command{
				[
					lPipelineHandle = aPipeline.handle()
				] (avk::command_buffer_t& cb) {
					cb.handle().bindPipeline(vk::PipelineBindPoint::eCompute, lPipelineHandle);
				}
			};
		}

#if VK_HEADER_VERSION >= 135
		state_type_command bind_pipeline(const ray_tracing_pipeline_t& aPipeline)
		{
			return state_type_command{
				[
					lPipelineHandle = aPipeline.handle()
				] (avk::command_buffer_t& cb) {
					cb.handle().bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, lPipelineHandle);
				}
			};
		}
#endif

		state_type_command bind_descriptors(std::tuple<const graphics_pipeline_t*, const vk::PipelineLayout, const std::vector<vk::PushConstantRange>*> aPipelineLayout, std::vector<descriptor_set> aDescriptorSets)
		{
			return state_type_command{
				[
					lLayoutHandle = std::get<const graphics_pipeline_t*>(aPipelineLayout)->layout_handle(),
					lDescriptorSets = std::move(aDescriptorSets)
				] (avk::command_buffer_t& cb) {
					cb.bind_descriptors(
						vk::PipelineBindPoint::eGraphics,
						lLayoutHandle,
						lDescriptorSets // Attention: Copy! => Potentially expensive?! TODO: What was the reason for bind_descriptors requiring std::vector<descriptor_set> being passed by value?
					);
				}
			};
		}

		state_type_command bind_descriptors(std::tuple<const compute_pipeline_t*, const vk::PipelineLayout, const std::vector<vk::PushConstantRange>*> aPipelineLayout, std::vector<descriptor_set> aDescriptorSets)
		{
			return state_type_command{
				[
					lLayoutHandle = std::get<const compute_pipeline_t*>(aPipelineLayout)->layout_handle(),
					lDescriptorSets = std::move(aDescriptorSets)
				] (avk::command_buffer_t& cb) {
					cb.bind_descriptors(
						vk::PipelineBindPoint::eCompute,
						lLayoutHandle,
						lDescriptorSets // Attention: Copy! => Potentially expensive?! TODO: What was the reason for bind_descriptors requiring std::vector<descriptor_set> being passed by value?
					);
				}
			};
		}

#if VK_HEADER_VERSION >= 135
		state_type_command bind_descriptors(std::tuple<const ray_tracing_pipeline_t*, const vk::PipelineLayout, const std::vector<vk::PushConstantRange>*> aPipelineLayout, std::vector<descriptor_set> aDescriptorSets)
		{
			return state_type_command{
				[
					lLayoutHandle = std::get<const ray_tracing_pipeline_t*>(aPipelineLayout)->layout_handle(),
					lDescriptorSets = std::move(aDescriptorSets)
				] (avk::command_buffer_t& cb) {
					cb.bind_descriptors(
						vk::PipelineBindPoint::eRayTracingKHR,
						lLayoutHandle,
						lDescriptorSets // Attention: Copy! => Potentially expensive?! TODO: What was the reason for bind_descriptors requiring std::vector<descriptor_set> being passed by value?
					);
				}
			};
		}
#endif 

		action_type_command draw(uint32_t aVertexCount, uint32_t aInstanceCount, uint32_t aFirstVertex, uint32_t aFirstInstance)
		{
			return action_type_command{
				avk::sync::sync_hint {
					{{ // What previous commands must synchronize with:
						vk::PipelineStageFlagBits2KHR::eAllGraphics,
						vk::AccessFlagBits2KHR::eInputAttachmentRead | vk::AccessFlagBits2KHR::eColorAttachmentRead | vk::AccessFlagBits2KHR::eColorAttachmentWrite | vk::AccessFlagBits2KHR::eDepthStencilAttachmentRead | vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite
					}},
					{{ // What subsequent commands must synchronize with:
						vk::PipelineStageFlagBits2KHR::eAllGraphics,
						vk::AccessFlagBits2KHR::eColorAttachmentWrite | vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite
					}}
				},
				{},
				[aVertexCount, aInstanceCount, aFirstVertex, aFirstInstance](avk::command_buffer_t& cb) {
					cb.handle().draw(aVertexCount, aInstanceCount, aFirstVertex, aFirstInstance);
				}
			};
		}

#if VK_HEADER_VERSION >= 135
		action_type_command trace_rays(
			vk::Extent3D aRaygenDimensions,
			const shader_binding_table_ref& aShaderBindingTableRef,
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
			return action_type_command{
				avk::sync::sync_hint {
					stage::ray_tracing_shader + access::acceleration_structure_read,
					stage::ray_tracing_shader + access::none
				},
				{}, // no resource-specific sync hints
				[
					lSbtHandle = aShaderBindingTableRef.mSbtBufferHandle,
					lEntrySize = aShaderBindingTableRef.mSbtEntrySize,
					lRaygenSbtRef = aRaygenSbtRef,
					lRaymissSbtRef = aRaymissSbtRef,
					lRayhitSbtRef = aRayhitSbtRef,
					lCallableSbtRef = aCallableSbtRef,
					aRaygenDimensions
				](avk::command_buffer_t& cb) {
					cb.handle().traceRaysKHR(
						&lRaygenSbtRef, &lRaymissSbtRef, &lRayhitSbtRef, &lCallableSbtRef,
						aRaygenDimensions.width, aRaygenDimensions.height, aRaygenDimensions.depth,
						cb.root_ptr()->dispatch_loader_ext()
					);
				}
			};
		}
#endif
	}

	submission_data::submission_data(submission_data&& aOther) noexcept
		: mRoot{ std::move(aOther.mRoot) }
		, mCommandBufferToSubmit{ std::move(aOther.mCommandBufferToSubmit) }
		, mQueueToSubmitTo{ std::move(aOther.mQueueToSubmitTo) }
		, mSemaphoreWaits{ std::move(aOther.mSemaphoreWaits) }
		, mSemaphoreSignals{ std::move(aOther.mSemaphoreSignals) }
		, mFence{ std::move(aOther.mFence) }
		, mSubmissionCount{ std::move(aOther.mSubmissionCount) }
	{
		aOther.mRoot = nullptr;
		aOther.mQueueToSubmitTo = nullptr;
		aOther.mSemaphoreWaits.clear();
		aOther.mSemaphoreSignals.clear();
		aOther.mFence.reset();
		aOther.mSubmissionCount = 0u;
	}

	submission_data& submission_data::operator=(submission_data&& aOther) noexcept
	{
		mRoot = std::move(aOther.mRoot);
		mCommandBufferToSubmit = std::move(aOther.mCommandBufferToSubmit);
		mQueueToSubmitTo = std::move(aOther.mQueueToSubmitTo);
		mSemaphoreWaits = std::move(aOther.mSemaphoreWaits);
		mSemaphoreSignals = std::move(aOther.mSemaphoreSignals);
		mFence = std::move(aOther.mFence);
		mSubmissionCount = std::move(aOther.mSubmissionCount);

		aOther.mRoot = nullptr;
		aOther.mQueueToSubmitTo = nullptr;
		aOther.mSemaphoreWaits.clear();
		aOther.mSemaphoreSignals.clear();
		aOther.mFence.reset();
		aOther.mSubmissionCount = 0u;

		return *this;
	}

	submission_data::~submission_data() noexcept(false)
	{
		if (is_sane() && 0 == mSubmissionCount) { // TODO: PROBLEM HERE due to auto submission = .... in imgui_manager.cpp#L337
			submit();
		}
	}

	submission_data& submission_data::submit_to(const queue* aQueue)
	{
		mQueueToSubmitTo = aQueue;
		return *this;
	}

	submission_data& submission_data::waiting_for(avk::semaphore_wait_info aWaitInfo)
	{
		mSemaphoreWaits.push_back(std::move(aWaitInfo));
		return *this;
	}

	submission_data& submission_data::signaling_upon_completion(semaphore_signal_info aSignalInfo)
	{
		mSemaphoreSignals.push_back(std::move(aSignalInfo));
		return *this;
	}

	submission_data& submission_data::signaling_upon_completion(avk::resource_argument<avk::fence_t> aFence)
	{
		mFence = std::move(aFence);
		return *this;
	}

	submission_data&& submission_data::store_for_now() noexcept
	{
		return std::move(*this);
	}

	void submission_data::submit()
	{
		// Gather config for wait semaphores:
		std::vector<vk::SemaphoreSubmitInfoKHR> waitSem;
		for (auto& semWait : mSemaphoreWaits) {
			auto& subInfo = waitSem.emplace_back(semWait.mWaitSemaphore->handle()); // TODO: What about timeline semaphores? (see 'value' param!)
			std::visit(lambda_overload{
				[&subInfo](const std::monostate&) {
					subInfo.setStageMask(vk::PipelineStageFlagBits2KHR::eNone);
				},
				[&subInfo](const vk::PipelineStageFlags2KHR& aFixedStage) {
					subInfo.setStageMask(aFixedStage);
				},
				[&subInfo, this](const avk::stage::auto_stage_t& aAutoStage) {
					// Set something:
					subInfo.setStageMask(vk::PipelineStageFlagBits2KHR::eAllCommands);
					// But now try to find a tighter auto-stage:
					auto* prevPtr = recorded_command_buffer_ptr();
					if (nullptr != prevPtr) {
						auto* prevPrevPtr = prevPtr->recorded_commands_ptr();
						if (nullptr != prevPrevPtr) {
							subInfo.setStageMask(accumulate_sync_details<vk::PipelineStageFlags2KHR>(
								prevPrevPtr->recorded_commands_and_sync_instructions(),
								/* Start index: within std::vector<recorded_commands_t>: */ 0,
								/* How many steps to accumulate: */ static_cast<uint32_t>(aAutoStage),
								/* after-wards: */  1,
								/* If we can't determine something specific, employ a heavy access mask to ensure correctness: */ vk::PipelineStageFlagBits2KHR::eAllCommands,
								{} // <-- For the semaphore signal, we do not want to restrict looking for specific images or buffers; everything is relevant
							));
						}
					}
				}
			}, semWait.mDstStage.mFlags);
		}

		// Gather config for signal semaphores:
		std::vector<vk::SemaphoreSubmitInfoKHR> signalSem;
		for (auto& semSig : mSemaphoreSignals) {
			auto& subInfo = signalSem.emplace_back(semSig.mSignalSemaphore->handle()); // TODO: What about timeline semaphores? (see 'value' param!)
			std::visit(lambda_overload{
				[&subInfo](const std::monostate&) {
					subInfo.setStageMask(vk::PipelineStageFlagBits2KHR::eNone);
				},
				[&subInfo](const vk::PipelineStageFlags2KHR& lFixedStage) {
					subInfo.setStageMask(lFixedStage);
				},
				[&subInfo, this](const avk::stage::auto_stage_t& lAutoStage) {
					// Set something:
					subInfo.setStageMask(vk::PipelineStageFlagBits2KHR::eAllCommands);
					// But now try to find a tighter auto-stage:
					auto* prevPtr = recorded_command_buffer_ptr();
					if (nullptr != prevPtr) {
						auto* prevPrevPtr = prevPtr->recorded_commands_ptr();
						if (nullptr != prevPrevPtr) {
							subInfo.setStageMask(accumulate_sync_details<vk::PipelineStageFlags2KHR>(
								prevPrevPtr->recorded_commands_and_sync_instructions(),
								/* Start index: within std::vector<recorded_commands_t>: */ static_cast<int>(prevPrevPtr->recorded_commands_and_sync_instructions().size()) - 1,
								/* How many steps to accumulate: */ static_cast<uint32_t>(lAutoStage),
								/* before-wards: */  -1,
								/* If we can't determine something specific, employ a heavy access mask to ensure correctness: */ vk::PipelineStageFlagBits2KHR::eAllCommands,
								{} // <-- For the semaphore signal, we do not want to restrict looking for specific images or buffers; everything is relevant
							));
						}
					}
				}
				}, semSig.mSrcStage.mFlags);
		}

		auto cmdBfrSubmitInfo = vk::CommandBufferSubmitInfoKHR{}
		.setCommandBuffer(mCommandBufferToSubmit->handle());

		auto submitInfo = vk::SubmitInfo2KHR{}
			.setWaitSemaphoreInfoCount(static_cast<uint32_t>(waitSem.size()))
			.setPWaitSemaphoreInfos(waitSem.data())
			.setCommandBufferInfoCount(1u)
			.setPCommandBufferInfos(&cmdBfrSubmitInfo)
			.setSignalSemaphoreInfoCount(static_cast<uint32_t>(signalSem.size()))
			.setPSignalSemaphoreInfos(signalSem.data());

		auto fenceHandle = mFence.has_value() ? mFence.value()->handle() : vk::Fence{};
		auto result = mQueueToSubmitTo->handle().submit2KHR(1u, &submitInfo, fenceHandle, mRoot->dispatch_loader_ext());

		++mSubmissionCount;
	}

#pragma endregion
	
	avk::recorded_commands root::record(std::vector<recorded_commands_t> aRecordedCommands) const
	{
		return avk::recorded_commands{ this, std::move(aRecordedCommands) };
	}

}
