#pragma once
#include <avk/avk.hpp>

class root_example_implementation : public avk::root
{
public:
	vk::Instance vulkan_instance()
	{
		if (!mInstance) {
			mInstance = vk::createInstanceUnique(vk::InstanceCreateInfo{});
		}
		return mInstance.get();
	}
	
	vk::PhysicalDevice physical_device() override
	{
		if (!mPhysicalDevice) {
			mPhysicalDevice = vulkan_instance().enumeratePhysicalDevices().front();
		}
		return mPhysicalDevice;
	}
	
	vk::Device device() override
	{
		if (!mDevice) {
			// Select one queue that can handle everything:
			auto queueFamilyIndex = avk::queue::find_best_queue_family_for(physical_device(), {}, avk::queue_selection_preference::versatile_queue, {});
			auto queues = avk::make_vector(avk::queue::prepare(physical_device(), 0, 0));
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

			// With the device in place, create a dynamic dispatch loader:
			mDynamicDispatch = vk::DispatchLoaderDynamic(
				vulkan_instance(), 
				vkGetInstanceProcAddr,
				device()
			);
		}
		return mDevice.get();
	}

	vk::Queue queue()
	{
		if (!mDevice) {
			device();
		}
		return mQueue.handle();
	}

	vk::DispatchLoaderDynamic dynamic_dispatch() override
	{
		if (!mDevice) {
			device();
		}
		return mDynamicDispatch;
	}

private:
	vk::UniqueInstance mInstance;
	vk::PhysicalDevice mPhysicalDevice;
	vk::UniqueDevice mDevice;
	avk::queue mQueue;
	vk::DispatchLoaderDynamic mDynamicDispatch;
};
