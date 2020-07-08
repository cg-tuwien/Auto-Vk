#pragma once

#include <ak/ak.hpp>

class root_example_implementation : public ak::root
{
public:
	vk::Instance vulkan_instance() override
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
			auto queueFamilyIndex = ak::queue::find_best_queue_family_for(physical_device(), {}, ak::queue_selection_preference::versatile_queue, {});
			auto queues = ak::make_vector(ak::queue::prepare(physical_device(), 0, 0));
			auto config = ak::queue::get_queue_config_for_DeviceCreateInfo(std::begin(queues), std::end(queues));
			for (auto i = 0; i < std::get<0>(config).size(); ++i) {
				std::get<0>(config)[i].setPQueuePriorities(std::get<1>(config)[i].data());
			}
			mDevice = physical_device().createDeviceUnique(vk::DeviceCreateInfo{}
				.setQueueCreateInfoCount(1u)
				.setPQueueCreateInfos(std::get<0>(config).data())
			);
			queues[0].assign_handle(mDevice.get());

			mQueue = std::move(queues[0]);
			mQueueFamilyIndex = mQueue.family_index();

			mDynamicDispatch = vk::DispatchLoaderDynamic(
				vulkan_instance(), 
				vkGetInstanceProcAddr,
				device()
			);

			mCommandPool = create_command_pool();
		}
		return mDevice.get();
	}
	
	vk::Queue queue() override
	{
		static auto tmp = device(); // Make sure that the device has been created
		return mQueue.handle();
	}
	
	uint32_t queue_family_index() override
	{
		static auto tmp = device(); // Make sure that the device has been created
		return mQueue.family_index();
	}
	
	vk::DispatchLoaderDynamic dynamic_dispatch() override
	{
		static auto tmp = device(); // Make sure that the device has been created
		return mDynamicDispatch;
	}
	
	vk::CommandPool command_pool_for_flags(vk::CommandPoolCreateFlags aCreateFlags) override
	{
		static auto tmp = device(); // Make sure that the device has been created
		return mCommandPool.handle();
	}
	
	ak::descriptor_cache_interface& descriptor_cache() override
	{
		static auto tmp = device(); // Make sure that the device has been created
		return mDescriptorCache;
	}

private:
	vk::UniqueInstance mInstance;
	vk::PhysicalDevice mPhysicalDevice;
	vk::UniqueDevice mDevice;
	ak::queue mQueue;
	uint32_t mQueueFamilyIndex;
	vk::DispatchLoaderDynamic mDynamicDispatch;
	ak::command_pool mCommandPool;
	ak::standard_descriptor_cache mDescriptorCache;
};