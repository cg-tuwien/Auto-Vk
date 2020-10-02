#pragma once
#include <avk/avk.hpp>

namespace avk
{
	/**	Class handling the lifetime of one VMA allocation.
	 *	Also provides some convenience methods.
	 */
	template <typename T>
	struct vma_handle
	{
		/** Construct emptyness */
		vma_handle() : mAllocator{nullptr}, mCreateInfo{}, mAllocation{nullptr}, mAllocationInfo{}, mResource{nullptr}
		{ }

		/** Initialize with VMA structs and the already created resource. */
		vma_handle(VmaAllocator aAllocator, VmaAllocationCreateInfo aAllocInfo, VmaAllocation aAlloc, T aResource)
			: mAllocator{ aAllocator }
			, mCreateInfo{ std::move(aAllocInfo) }
			, mAllocation{ std::move(aAlloc) }
			, mResource{ std::move(aResource) }
		{
			vmaGetAllocationInfo(mAllocator, mAllocation, &mAllocationInfo);
		}

		/** Move-construct a vma_handle */
		vma_handle(vma_handle&& aOther) noexcept : mAllocator{nullptr}, mCreateInfo{}, mAllocation{nullptr}, mAllocationInfo{}, mResource{nullptr}
		{
			std::swap(mAllocator,      aOther.mAllocator);
			std::swap(mCreateInfo,     aOther.mCreateInfo);
			std::swap(mAllocation,     aOther.mAllocation);
			std::swap(mAllocationInfo, aOther.mAllocationInfo);
			std::swap(mResource,       aOther.mResource);
		}

		vma_handle(const vma_handle& aOther) = delete;
		
		/** Move-assign a vma_handle */
		vma_handle& operator=(vma_handle&& aOther) noexcept
		{
			std::swap(mAllocator,      aOther.mAllocator);
			std::swap(mCreateInfo,     aOther.mCreateInfo);
			std::swap(mAllocation,     aOther.mAllocation);
			std::swap(mAllocationInfo, aOther.mAllocationInfo);
			std::swap(mResource,       aOther.mResource);
			return *this;
		}

		vma_handle& operator=(const vma_handle& aOther) = delete;

		/** Destroy the resource and free the allocation */
		~vma_handle();

		/** Get the allocator that was used to allocate this resource */
		auto allocator() const
		{
			return mAllocator;
		}
		
		/** Get the config a.k.a. create-info that was used to allocate this resource. */
		auto config() const
		{
			return mCreateInfo;
		}

		/** Get the VMA-handle of this resource's allocation. */
		auto allocation() const
		{
			return mAllocation;
		}

		/** Get the resource handle. */
		const T& resource() const
		{
			return mResource;
		}

		/** Get the memory properties from the allocation */
		vk::MemoryPropertyFlags memory_properties() const
		{
			VkMemoryPropertyFlags result;
			vmaGetMemoryTypeProperties(mAllocator, mAllocationInfo.memoryType, &result);
			return vk::MemoryPropertyFlags{ result };
		}

		void* map_memory() const
		{
			assert(has_flag(memory_properties(), vk::MemoryPropertyFlagBits::eHostVisible));
			// Allocation ended up in mappable memory. You can map it and access it directly.
		    void* mappedData;
		    vmaMapMemory(mAllocator, mAllocation, &mappedData);
			return mappedData;
		}

		void unmap_memory() const
		{
		    vmaUnmapMemory(mAllocator, mAllocation);
		}

		VmaAllocator mAllocator;
		VmaAllocationCreateInfo mCreateInfo;
		VmaAllocation mAllocation;
		VmaAllocationInfo mAllocationInfo;
		T mResource;
	};

	template <typename T>
	vma_handle<T>::~vma_handle()
	{
		throw avk::runtime_error(std::string("VMA allocation not implemented for type ") + typeid(T).name());
	}

	template <>
	inline vma_handle<vk::Buffer>::~vma_handle()
	{
		if (static_cast<bool>(mResource)) {
			vmaDestroyBuffer(mAllocator, static_cast<VkBuffer>(mResource), mAllocation);
			mAllocator = nullptr;
			mCreateInfo = {};
			mAllocation = nullptr;
			mAllocationInfo = {};
			mResource = nullptr;
		}
	}

	template <>
	inline vma_handle<vk::Image>::~vma_handle()
	{
		if (static_cast<bool>(mResource)) {
			vmaDestroyImage(mAllocator, static_cast<VkImage>(mResource), mAllocation);
			mAllocator = nullptr;
			mCreateInfo = {};
			mAllocation = nullptr;
			mAllocationInfo = {};
			mResource = nullptr;
		}
	}

}
