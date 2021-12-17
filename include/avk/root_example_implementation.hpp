#pragma once
#include <avk/avk.hpp>

class root_example_implementation : public avk::root
{
public:
	vk::Instance vulkan_instance()
	{
		if (!mInstance) {
			if constexpr (std::is_same_v<std::remove_cv_t<decltype(mDispatchLoaderCore)>, vk::DispatchLoaderDynamic>) {
				reinterpret_cast<vk::DispatchLoaderDynamic*>(&mDispatchLoaderCore)->init(vkGetInstanceProcAddr);
			}

			mInstance = vk::createInstanceUnique(vk::InstanceCreateInfo{}, nullptr, mDispatchLoaderCore);

			if constexpr (std::is_same_v<std::remove_cv_t<decltype(mDispatchLoaderCore)>, vk::DispatchLoaderDynamic>) {
				reinterpret_cast<vk::DispatchLoaderDynamic*>(&mDispatchLoaderCore)->init(mInstance.get());
			}
		}

		return mInstance.get();
	}
	
	vk::PhysicalDevice& physical_device() override
	{
		if (!mPhysicalDevice) {
			mPhysicalDevice = vulkan_instance().enumeratePhysicalDevices().front();
		}
		return mPhysicalDevice;
	}
	const vk::PhysicalDevice& physical_device() const override
	{
		assert(mPhysicalDevice);
		return mPhysicalDevice;
	}
	
	vk::Device& device() override
	{
		if (!mDevice) {
			// Select one queue that can handle everything:
			auto queueFamilyIndex = avk::queue::find_best_queue_family_for(physical_device(), {}, avk::queue_selection_preference::versatile_queue, {});
			auto queues = avk::make_vector(avk::queue::prepare(physical_device(), mDispatchLoaderCore, 0, 0));
			auto config = avk::queue::get_queue_config_for_DeviceCreateInfo(std::begin(queues), std::end(queues));
			for (auto i = 0; i < std::get<0>(config).size(); ++i) {
				std::get<0>(config)[i].setPQueuePriorities(std::get<1>(config)[i].data());
			}

			// Create the device using the queue information from above:
			mDevice = physical_device().createDeviceUnique(vk::DeviceCreateInfo{}
				.setQueueCreateInfoCount(1u)
				.setPQueueCreateInfos(std::get<0>(config).data())
			);

			// AFTER device creation, the queue handle(s) can be assigned to the queues:
			queues[0].assign_handle(mDevice.get());

			// Store the queue:
			mQueue = std::move(queues[0]);

			if constexpr (std::is_same_v<std::remove_cv_t<decltype(mDispatchLoaderCore)>, vk::DispatchLoaderDynamic>) {
				reinterpret_cast<vk::DispatchLoaderDynamic*>(&mDispatchLoaderCore)->init(mDevice.get());
			}
			if constexpr (std::is_same_v<std::remove_cv_t<decltype(mDispatchLoaderExt)>, vk::DispatchLoaderDynamic>) {
				reinterpret_cast<vk::DispatchLoaderDynamic*>(&mDispatchLoaderExt)->init(mDevice.get());
			}
			
#if defined(AVK_USE_VMA)
			// With everything in place, create the memory allocator:
			VmaAllocatorCreateInfo allocatorInfo = {};
			allocatorInfo.physicalDevice = physical_device();
			allocatorInfo.device = device();
			allocatorInfo.instance = vulkan_instance();
			vmaCreateAllocator(&allocatorInfo, &mMemoryAllocator);
#else
			mMemoryAllocator = std::make_tuple(physical_device(), device());
#endif
		}
		return mDevice.get();
	}
	const vk::Device& device() const override
	{
		assert(mDevice);
		return mDevice.get();
	}

	vk::Queue queue()
	{
		if (!mDevice) {
			device();
		}
		return mQueue.handle();
	}

	DISPATCH_LOADER_CORE_TYPE& dispatch_loader_core() override
	{
		return mDispatchLoaderCore;
	}
	const DISPATCH_LOADER_CORE_TYPE& dispatch_loader_core() const override
	{
		return mDispatchLoaderCore;
	}

	DISPATCH_LOADER_EXT_TYPE& dispatch_loader_ext() override
	{
		return mDispatchLoaderExt;
	}
	const DISPATCH_LOADER_EXT_TYPE& dispatch_loader_ext() const override
	{
		return mDispatchLoaderExt;
	}

	AVK_MEM_ALLOCATOR_TYPE& memory_allocator() override
	{
		if (!mDevice) {
			device();
		}
		return mMemoryAllocator;
	}
	
private:
	vk::UniqueHandle<vk::Instance, DISPATCH_LOADER_CORE_TYPE> mInstance;
	vk::PhysicalDevice mPhysicalDevice;
	vk::UniqueHandle<vk::Device, DISPATCH_LOADER_CORE_TYPE> mDevice;
	avk::queue mQueue;
	DISPATCH_LOADER_CORE_TYPE mDispatchLoaderCore;
	DISPATCH_LOADER_EXT_TYPE mDispatchLoaderExt;
#if defined(AVK_USE_VMA)
	VmaAllocator mMemoryAllocator;
#else
	std::tuple<vk::PhysicalDevice, vk::Device> mMemoryAllocator;
#endif
};
