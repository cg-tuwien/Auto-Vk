# Auto-Vk

_Auto-Vk_ is a low-level convenience and productivity layer atop [Vulkan-Hpp](https://github.com/KhronosGroup/Vulkan-Hpp). 

# Setup

_Auto-Vk_ requires
* A Vulkan 1.2 SDK
* [Vulkan-Hpp](https://github.com/KhronosGroup/Vulkan-Hpp)

_Auto-Vk_ consists of multiple C++ include files and two (soon: one) C++ source file
* Add [`include/`](include/) to the include paths so that your compiler can find include files under paths `avk/*`
* Add [`src/avk.cpp`](src/avk.cpp) (and currently also [`src/sync.cpp`](src/sync.cpp)) as a compiled C++ source code file

# Usage

The first step is to derive from `avk::root` and implement three virtual methods:
``` 
virtual vk::PhysicalDevice physical_device() = 0;
virtual vk::Device device() = 0;
virtual vk::DispatchLoaderDynamic dynamic_dispatch() = 0;
```

Example implementation:
```
class my_root : avk::root
{
public:
    my_root(std::vector<avk::queue> queues) { 
        mInstance = vk::createInstance(vk::InstanceCreateInfo{}); 
        mPhysicalDevice = mInstance.enumeratePhysicalDevices().front();
        
        auto queueCreateInfos = avk::queue::get_queue_config_for_DeviceCreateInfo(std::begin(queues), std::end(queues));
        // Iterate over all vk::DeviceQueueCreateInfo entries and set the queue priorities pointers properly (just to be safe!)
        for (auto i = 0; i < std::get<0>(queueCreateInfos).size(); ++i) {
            std::get<0>(queueCreateInfos)[i].setPQueuePriorities(std::get<1>(queueCreateInfos)[i].data());
        }
        
        auto deviceCreateInfo = vk::DeviceCreateInfo()
            .setQueueCreateInfoCount(static_cast<uint32_t>(std::get<0>(queueCreateInfos).size()))
            .setPQueueCreateInfos(std::get<0>(queueCreateInfos).data());
        // Set further configuration parameters for the device
        mDevice = mPhysicalDevice.createDevice(deviceCreateInfo);

        context().mDynamicDispatch = vk::DispatchLoaderDynamic{ mInstance, vkGetInstanceProcAddr, mDevice };
    }
    
    vk::PhysicalDevice& physical_device() override { return mPhysicalDevice; }
    vk::Device& device() override { return mDevice; }
    vk::DispatchLoaderDynamic& dynamic_dispatch() override { return mDispatchLoaderDynamic; }
    
private:
    vk::Instance mInstance;
    vk::PhysicalDevice mPhysicalDevice;
    std::array<avk::queue, 2> mQueues;
    vk::Device mDevice;
    vk::DispatchLoaderDynamic mDispatchLoaderDynamic;
}
``` 

```
        // Prepare two queues
        mQueues[0] = avk::queue::prepare(mPhysicalDevice, 0u, 0u, 0.5f);
        mQueues[1] = avk::queue::prepare(mPhysicalDevice, 0u, 1u, 0.75f);
```
