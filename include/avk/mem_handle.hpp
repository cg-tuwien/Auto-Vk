#pragma once
#include <avk/avk.hpp>

namespace avk
{
	/**	Class handling the lifetime of one resource + associated memory.
	 *	Also provides some convenience methods.
	 */
	template <typename T>
	struct mem_handle
	{
		/** Construct emptyness */
		mem_handle() : mAllocator{}, mMemoryPropertyFlags{}, mMemory{nullptr}, mResource{nullptr}
		{ }

		/** Initialize with VMA structs and the already created resource. */
		mem_handle(std::tuple<vk::PhysicalDevice, vk::Device> aAllocator, T aResource)
			: mAllocator{ std::move(aAllocator) }
			, mMemoryPropertyFlags{}
			, mMemory{nullptr}
			, mResource{ std::move(aResource) }
		{ }

		/**	Create VmaAllocator, VmaAllocationCreateInfo, and VmaAllocation internally.
		 *	This is only implemented for certain types via template specialization: vk::Buffer, vk::Image
		 */
		template <typename C>
		mem_handle(std::tuple<vk::PhysicalDevice, vk::Device> aAllocator, vk::MemoryPropertyFlags aMemPropFlags, const C& aResourceCreateInfo);
		
		/** Move-construct a mem_handle */
		mem_handle(mem_handle&& aOther) noexcept : mAllocator{}, mMemoryPropertyFlags{}, mMemory{nullptr}, mResource{nullptr}
		{
			std::swap(mAllocator,	        aOther.mAllocator);
			std::swap(mMemoryPropertyFlags,	aOther.mMemoryPropertyFlags);
			std::swap(mMemory,              aOther.mMemory);
			std::swap(mResource,            aOther.mResource);
		}

		mem_handle(const mem_handle& aOther) = delete;
		
		/** Move-assign a mem_handle */
		mem_handle& operator=(mem_handle&& aOther) noexcept
		{
			std::swap(mAllocator,	        aOther.mAllocator);
			std::swap(mMemoryPropertyFlags,	aOther.mMemoryPropertyFlags);
			std::swap(mMemory,              aOther.mMemory);
			std::swap(mResource,            aOther.mResource);
			return *this;
		}

		mem_handle& operator=(const mem_handle& aOther) = delete;

		/** Destroy the resource and free the allocation
		 *	This is only implemented for certain types via template specialization: vk::Buffer, vk::Image
		 *	That also means that this type is only usable with certain resource types.
		 */
		~mem_handle();

		/** Get the allocator that was used to allocate this resource */
		auto allocator() const
		{
			return mAllocator;
		}
		
		/** Get the resource handle. */
		T resource() const
		{
			return mResource;
		}

		/** Get the memory properties from the allocation */
		vk::MemoryPropertyFlags memory_properties() const
		{
			return mMemoryPropertyFlags;
		}

		/**	Map the memory in order to write data into, or read data from it.
		 *	If data shall be read from it and the memory is not host coherent, an invalidate-instruction will be issued.
		 *
		 *	Hint: Consider using avk::scoped_mapping instead of calling this method directly.
		 *
		 *	@param	aAccess		Specify your intent: Are you going to read from the memory, or write into it, or both?
		 *	@return	Pointer to the mapped memory.
		 */
		void* map_memory(mapping_access aAccess) const
		{
			const auto memProps = memory_properties();
			assert(has_flag(memProps, vk::MemoryPropertyFlagBits::eHostVisible)); // => Allocation ended up in mappable memory. You can map it and access it directly.
			
			auto& device = std::get<vk::Device>(mAllocator);
			void* mappedData = device.mapMemory(mMemory, 0, VK_WHOLE_SIZE);

			if (has_flag(aAccess, mapping_access::read) && !has_flag(memProps, vk::MemoryPropertyFlagBits::eHostCoherent)) {
				// Setup the range 
				auto range = vk::MappedMemoryRange{mMemory, 0, VK_WHOLE_SIZE};
				// Invalidate the range
				auto result = device.invalidateMappedMemoryRanges(1, &range);
				assert(static_cast<VkResult>(result) >= 0);
			}
			
			return mappedData;
		}

		/**	Unmap memory that has been mapped before via mem_handle::map_memory.
		 *	If data shall be written to it and the memory is not host coherent, a flush-instruction will be issued.
		 *
		 *	Hint: Consider using avk::scoped_mapping instead of calling this method directly.
		 *
		 *	@param	aAccess		Specify your intent: Are you going to read from the memory, or write into it, or both?
		 */
		void unmap_memory(mapping_access aAccess) const
		{
			const auto memProps = memory_properties();
			assert(has_flag(memProps, vk::MemoryPropertyFlagBits::eHostVisible)); // => Allocation ended up in mappable memory. You can map it and access it directly.
			
			auto& device = std::get<vk::Device>(mAllocator);
			if (has_flag(aAccess, mapping_access::write) && !avk::has_flag(memProps, vk::MemoryPropertyFlagBits::eHostCoherent)) {
				// Setup the range 
				auto range = vk::MappedMemoryRange{mMemory, 0, VK_WHOLE_SIZE};
				// Flush the range
				auto result = device.flushMappedMemoryRanges(1, &range);
				assert(static_cast<VkResult>(result) >= 0);
			}
			
			device.unmapMemory(mMemory);
			// TODO: Handle has_flag(memProps, vk::MemoryPropertyFlagBits::eHostCached) case
		}

		std::tuple<vk::PhysicalDevice, vk::Device> mAllocator;
		vk::MemoryPropertyFlags mMemoryPropertyFlags;
		vk::DeviceMemory mMemory;
		T mResource;
	};

	// Fail if not used with either vk::Buffer or vk::Image
	template <typename T>
	template <typename C>
	mem_handle<T>::mem_handle(std::tuple<vk::PhysicalDevice, vk::Device> aAllocator, vk::MemoryPropertyFlags aMemPropFlags, const C& aResourceCreateInfo)
	{
		throw avk::runtime_error(std::string("Memory allocation not implemented for type ") + typeid(T).name());
	}

	// Constructor's template specialization for vk::Buffer
	template <>
	template <typename C>
	mem_handle<vk::Buffer>::mem_handle(std::tuple<vk::PhysicalDevice, vk::Device> aAllocator, vk::MemoryPropertyFlags aMemPropFlags, const C& aResourceCreateInfo)
		: mAllocator{ aAllocator }
	{
		auto& physicalDevice = std::get<vk::PhysicalDevice>(mAllocator);
		auto& device = std::get<vk::Device>(mAllocator);
		
		// Create the buffer on the logical device
		auto vkBuffer = device.createBuffer(aResourceCreateInfo);

		// The buffer has been created, but it doesn't actually have any memory assigned to it yet. 
		// The first step of allocating memory for the buffer is to query its memory requirements [2]
		const auto memRequirements = device.getBufferMemoryRequirements(vkBuffer);

		// Find suitable memory for this buffer:
		auto tpl = find_memory_type_index_for_device(physicalDevice, memRequirements.memoryTypeBits, aMemPropFlags);
		// The actual memory property flags of the selected memory can be different from the minimum requested flags (which is aMemPropFlags)
		//  => store the ACTUAL memory property flags of this buffer!
		mMemoryPropertyFlags = std::get<vk::MemoryPropertyFlags>(tpl);

		auto allocInfo = vk::MemoryAllocateInfo{}
			.setAllocationSize(memRequirements.size)
			.setMemoryTypeIndex(std::get<uint32_t>(tpl)); // Get the selected memory type index from the result-tuple

#if VK_HEADER_VERSION >= 135
		auto memoryAllocateFlagsInfo = vk::MemoryAllocateFlagsInfo{};
		// If buffer was created with the VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR bit set, memory must have been allocated with the 
		// VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR bit set. The Vulkan spec states: If the VkPhysicalDeviceBufferDeviceAddressFeatures::bufferDeviceAddress
		// feature is enabled and buffer was created with the VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT bit set, memory must have been allocated with the
		// VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT bit set
		if (avk::has_flag(aResourceCreateInfo.usage, vk::BufferUsageFlagBits::eShaderDeviceAddress) || avk::has_flag(aResourceCreateInfo.usage, vk::BufferUsageFlagBits::eShaderDeviceAddressKHR) || avk::has_flag(aResourceCreateInfo.usage, vk::BufferUsageFlagBits::eShaderDeviceAddressEXT)) {
			memoryAllocateFlagsInfo.flags |= vk::MemoryAllocateFlagBits::eDeviceAddress;
			allocInfo.setPNext(&memoryAllocateFlagsInfo);
		}
#endif
		
		// Allocate the memory for the buffer:
		mMemory = device.allocateMemory(allocInfo);

		// If memory allocation was successful, then we can now associate this memory with the buffer
		device.bindBufferMemory(vkBuffer, mMemory, 0);
		
		mResource = vkBuffer;
	}
	
	// Constructor's template specialization for vk::Image
	template <>
	template <typename C>
	mem_handle<vk::Image>::mem_handle(std::tuple<vk::PhysicalDevice, vk::Device> aAllocator, vk::MemoryPropertyFlags aMemPropFlags, const C& aResourceCreateInfo)
		: mAllocator{ aAllocator }
	{
		auto& physicalDevice = std::get<vk::PhysicalDevice>(mAllocator);
		auto& device = std::get<vk::Device>(mAllocator);

		// Create the image...
		auto vkImage = device.createImage(aResourceCreateInfo);

		// ... and the memory:
		auto memRequirements = device.getImageMemoryRequirements(vkImage);
		
		// Find suitable memory for this image:
		auto tpl = find_memory_type_index_for_device(physicalDevice, memRequirements.memoryTypeBits, aMemPropFlags);
		// The actual memory property flags of the selected memory can be different from the minimum requested flags (which is aMemPropFlags)
		//  => store the ACTUAL memory property flags of this buffer!
		mMemoryPropertyFlags = std::get<vk::MemoryPropertyFlags>(tpl);
		
		auto allocInfo = vk::MemoryAllocateInfo{}
			.setAllocationSize(memRequirements.size)
			.setMemoryTypeIndex(std::get<uint32_t>(tpl)); // Get the selected memory type index from the result-tuple
		
		mMemory = device.allocateMemory(allocInfo);

		// bind them together:
		device.bindImageMemory(vkImage, mMemory, 0);
		
		mResource = vkImage;
	}
	
	// Fail if not used with either vk::Buffer or vk::Image
	template <typename T>
	mem_handle<T>::~mem_handle()
	{
		throw avk::runtime_error(std::string("Memory allocation not implemented for type ") + typeid(T).name());
	}

	// Destructor's template specialization for vk::Buffer
	template <>
	inline mem_handle<vk::Buffer>::~mem_handle()
	{
		if (static_cast<bool>(mResource)) {
			auto& device = std::get<vk::Device>(mAllocator);
			device.freeMemory(mMemory);
			mMemory = nullptr;
			device.destroyBuffer(mResource);
			mResource = nullptr;
			mMemoryPropertyFlags = {};
			mAllocator = {};
		}
	}

	// Destructor's template specialzation for vk::Image:
	template <>
	inline mem_handle<vk::Image>::~mem_handle()
	{
		if (static_cast<bool>(mResource)) {
			auto& device = std::get<vk::Device>(mAllocator);
			device.freeMemory(mMemory);
			mMemory = nullptr;
			device.destroyImage(mResource);
			mResource = nullptr;
			mMemoryPropertyFlags = {};
			mAllocator = {};
		}
	}
	
}
