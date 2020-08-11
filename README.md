# Auto-Vk

_Auto-Vk_ is a low-level convenience and productivity layer atop [Vulkan-Hpp](https://github.com/KhronosGroup/Vulkan-Hpp). 

# Setup

_Auto-Vk_ requires
* A Vulkan 1.2 SDK (Vulkan 1.1 SDK compatibility w.i.p.)
* [Vulkan-Hpp](https://github.com/KhronosGroup/Vulkan-Hpp)
* A C++17 compiler

_Auto-Vk_ consists of multiple C++ include files and two (soon: one) C++ source file
* Add [`include/`](include/) to the include paths so that your compiler can find include files under paths `avk/*`
* Add [`src/avk.cpp`](src/avk.cpp) (and currently also [`src/sync.cpp`](src/sync.cpp)) as a compiled C++ source code file

# Motivating Example

_Auto-Vk_ aims to hit the sweet spot between full controllability and convenience without having a noticeable impact on performance.

**Creating a graphics pipeline** in Vulkan require hundreds of lines of code. Here is what it can look like using _Auto-Vk_:

```
auto graphicsPipeline = myRoot.create_graphics_pipeline_for(
	// Specify which shaders the pipeline consists of:
	avk::vertex_shader("shaders/vertex_shader.vert"),
	avk::fragment_shader("shaders/fragment_shader.frag"),
	// The next 3 lines define the format and location of the vertex shader inputs:
	avk::from_buffer_binding(0) -> stream_per_vertex<glm::vec3>()   -> to_location(0),
	avk::from_buffer_binding(1) -> stream_per_vertex<glm::vec2>()   -> to_location(1),
	avk::from_buffer_binding(2) -> stream_per_instance<glm::vec3>() -> to_location(5),
	// Some further settings:
	avk::cfg::front_face::define_front_faces_to_be_counter_clockwise(),
	avk::cfg::viewport_depth_scissors_config::from_extent(1920u, 1080u),
	// Declare the renderpass' attachments and the actions to take:
	avk::attachment::declare(vk::Format::eR8G8B8A8Unorm,  avk::on_load::clear, avk::unused()        -> avk::color(0) ->  avk::color(0), avk::on_store::store_in_presentable_format),	
	avk::attachment::declare(vk::Format::eD24UnormS8Uint, avk::on_load::clear, avk::depth_stencil() -> preserve()    -> input(0),       avk::on_store::dont_care                  ),
	// The following define resources that will be bound to the pipeline:
	avk::push_constant_binding_data { avk::shader_type::vertex, 0, sizeof(std::array<float, 16>) },
	avk::descriptor_binding(0, 0, myBuffer->as_uniform_buffer()),
	avk::descriptor_binding(0, 1, myImageView)
);
```

This creates a fully functioning graphics pipeline on the device consisting of two shaders (a vertex shader and a fragment shader), with three vertex/instance input bindings, stating some additional pipeline settings (face orientation, and viewport config), defining the renderpass' attachments (which could correspond to the backbuffer format to render into), and further resource bindings to the pipeline: push constants of the size 64 byte (16 floats, which is enough space for a 4x4 matrix), and two descriptors, namely a buffer bound as uniform buffer (corresponding to descriptor type `vk::DescriptorType::eUniformBuffer`) and an image view (corresponding to descriptor type `vk::DescriptorType::eSampledImage`).

**Vertex input bindings** are specified in an expressive way. For example,       
`avk::from_buffer_binding(2) -> stream_per_instance<glm::vec3>() -> to_location(5)`             
means that from the buffer that is bound at index `2` when invoking `vk::CommandBuffer::bindVertexBuffers`, data is being streamed to vertex shader's `layout (location=5)` and the data pointer is advanced per instance. 

In a similarly expressive manner, the **subpass usages of the renderpass' attachments** are specified. In the example above, there are three subpasses. The color attachment is not used in the first subpass, and is used as color attachment in subpasses `1` and `2`, bound to output location `0` in both of them.        
The depth/stencil attachment is used as depth/stencil attachment in the first subpass, is not used in the second subpass but its contents are preserved, and in the thirds subpass, is is used as an input attachment, i.e. to be read from.

After subpass three, the color attachment shall be stored in a presentable format (`avk::on_store::store_in_presentable_format`), i.e. so that it can be passed on to a presentation engine for being displayed on screen. The depth/stencil attachment is not required any longer after subpass three, therefore we don't care about its contents (`avk::on_store::dont_care`).

# Usage

First of all, include all of _Auto-Vk_:

``` 
#include <avk/avk.hpp>
```

The next essential step is to derive from `avk::root` and implement three virtual methods:
``` 
virtual vk::PhysicalDevice& physical_device() = 0;
virtual vk::Device& device() = 0;
virtual vk::DispatchLoaderDynamic& dynamic_dispatch() = 0;
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

# Owning Resources or Move-Only Types

Many of the types used are implemented as _move-only_ types because they represent resources. Such types are wrapped in the `avk::owning_resource<T>` class and may only be moved to other locations.

For example, images are implemented as such types. The type returned by `myRoot.create_image` is `avk::image`, which is a typedef to `avk::owning_resource<avk::image_t>`. I.e. the concrete image's functionality is implemented within class `avk::image_t`. 

Whenever temporary and non-ownership-transferring access to such types is sufficient, you will see the function or method taking a parameter of the form `const T&` or similar. For the case of `avk::owning_resource<avk::image_t>`, such a parameter would have the type `const image_t&`. Usage is simple, however, since `avk::owning_resource<T>` has implicit cast operators to `const T&`, `T&`, `const T*`, and `T*`.

If you need to have **multiple owners** of a resource type, you can use `avk::owning_resource<T>::enable_shared_ownership` which will move the resource internally into a shared pointers. Storing resources in shared pointers is not the default because it would violate a core principle of C++, namely "don't pay for what you're not using". There is no point in making a heap allocation if an object could spend its lifetime on the stack.

# Sync

Synchronization by the means of 
* waiting for idle,
* semaphores, and
* pipeline barriers

is currently implemented in class `avk::sync`. Many functions/method which perform asynchronous operations take `avk::sync` parameters to allow modification of the synchronization strategy.

_Attention:_ `avk::sync` is ugly and is subject to change. Expect breaking changes soon.
