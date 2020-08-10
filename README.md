# Auto-Vk

_Auto-Vk_ is a low-level convenience and productivity layer atop [Vulkan-Hpp](https://github.com/KhronosGroup/Vulkan-Hpp). 

# Setup

_Auto-Vk_ requires
* A Vulkan 1.2 SDK (not so sure about that.. TBD.)
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
    my_root() { 
        mInstance = vk::createInstance(vk::InstanceCreateInfo{}); 
        mPhysicalDevice = mInstance.enumeratePhysicalDevices().front();
        
        // Prepare two queues
        mQueues[0] = avk::queue::prepare(mPhysicalDevice, 0u, 0u, 0.5f);
        mQueues[1] = avk::queue::prepare(mPhysicalDevice, 0u, 1u, 0.75f);
        
        auto queueCreateInfos = avk::queue::get_queue_config_for_DeviceCreateInfo(std::begin(mQueues), std::end(mQueues));
        // Iterate over all vk::DeviceQueueCreateInfo entries and set the queue priorities pointers properly (just to be safe!)
        for (auto i = 0; i < std::get<0>(queueCreateInfos).size(); ++i) {
            std::get<0>(queueCreateInfos)[i].setPQueuePriorities(std::get<1>(queueCreateInfos)[i].data());
        }
        
        auto deviceCreateInfo = vk::DeviceCreateInfo{}
            .setQueueCreateInfoCount(static_cast<uint32_t>(std::get<0>(queueCreateInfos).size()))
            .setPQueueCreateInfos(std::get<0>(queueCreateInfos).data());
        // Set further configuration parameters for the device
        mDevice = mPhysicalDevice.createDevice(deviceCreateInfo);

        context().mDynamicDispatch = vk::DispatchLoaderDynamic{ mInstance, vkGetInstanceProcAddr, mDevice };
        
        // Assign handles to the queues to finish their configuration
        for (auto& q : mQueues) {
            q.assign_handle(mDevice);
        }
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

auto myRoot = my_root{};
``` 

Also refer to [`include/avk/root_example_implementation.hpp`](include/avk/root_example_implementation.hpp).

Queues require some special handling because they must be declared prior to creating the logical device. Use `avk::queue::prepare` to prepare a queue and use the convenience method `avk::queue::get_queue_config_for_DeviceCreateInfo` to generate the required entries for `vk::DeviceCreateInfo`.

From this point onwards, the root class (`my_root` in the example) serves as the origin for creating all kinds of things. 

**To create an image** with an image view, for example, you could invoke: 
```
auto myImageView = myRoot.create_image_view(
    myRoot.create_image(1920, 1080, vk::Format::eR8G8B8A8Unorm, 1, avk::memory_usage::device, avk::image_usage::general_storage_image)
);
```
The first parameters state the resolution, the format, and the number of layers of the image. The `memory_usage` parameter states that this image shall live device memory, and the `image_usage` flags state that this image shall be usable as a "general image" and as a "storage image". This will create the the image with usage flags `vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage` and the tiling flag `vk::ImageTiling::eOptimal`.

**To create a buffer** that is usable as both, uniform buffer, and vertex buffer, you could invoke:
```
std::vector<std::array<float, 3>> vertices;

auto myBuffer = myRoot.create_buffer(
    avk::memory_usage::host_coherent, {},
    avk::vertex_buffer_meta::create_from_data(vertices), 
    avk::uniform_buffer_meta::create_from_data(vertices)
);
```
The first parameter states that the buffer shall live in host-coherent memory. The second parameter states no additional usage flags. Parameters three and four state two different usage types of the same buffer, namely as vertex buffer and as uniform buffer which means that the buffer will be created with both, `vk::BufferUsageFlagBits::eVertexBuffer` and `vk::BufferUsageFlagBits::eUniformBuffer`, flags set. The `create_from_data` convenience methods automatically infer size of one element and number of elements from the given `std::vector`.

**To fill a buffer** (and this generally applies to operations after creation), no reference to `myRoot` is needed anymore. Further operations can be invoked on the created instance like follows:
```
myBuffer->fill(vertices, 0, avk::sync::not_required());
```
The first parameter are the data, the second refers to the meta data index to use (in this case to the `vertex_buffer_meta`, but actually the data should be the same for all of the meta data entries anyways), and the third parameter states that "no sync is required" which is only true in this case because `myBuffer` has been created with `avk::memory_usage::host_coherent`.

If the buffer was created with `avk::memory_usage::device`, the fill operation is an asynchronous operation and a different synchronization strategy is required. The most straight forward to be used would be `avk::sync::wait_idle()` where the device waits for idle, meaning that the fill operation must also be completed when `wait_idle()` returns.


