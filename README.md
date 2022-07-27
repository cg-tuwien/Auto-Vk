# Auto-Vk v0.98

_Auto-Vk_ is a low-level convenience and productivity layer for the best graphics API there is, namely
<p align="center">
  <img src="Vulkan_170px_Dec16.png" alt="Vulkan Logo"/>
</p>
(...psst: and also the graphics API that most desperately needs such a layer.)    

_Auto-Vk_ is written in modern C++ (using C++17 and C++20 features) and is built atop the Khronos Group's very own [Vulkan-Hpp](https://github.com/KhronosGroup/Vulkan-Hpp). It aims to add more clarity, efficiency, and expressiveness to writing Vulkan code, while not abstracting-away any (or let's say: most) of its important concepts. 

I.e., the big, important concepts, which make Vulkan as performant as it can be---such as fine-grained synchronization, parallel command recording on the host-side, usage of multiple queues, etc.---are all still there, but _Auto-Vk_ can help to not spend days implementing them, and to not lose track of the big picture of your source code. Just a few parts are abstracted a bit more heavily by _Auto-Vk_, in particular: descriptor set allocation and handling, memory allocation, and the handling of certain configuration parameters and flags. Should any of these parts fail suit your needs, you're still free to use raw Vulkan, or even more better: We're always happy to receive pull requests which extend the functionality and versatility of _Auto-Vk_.

# Setup

_Auto-Vk_ requires
* A Vulkan 1.2 SDK or a Vulkan 1.3 SDK
* [Vulkan-Hpp](https://github.com/KhronosGroup/Vulkan-Hpp)
* A C++20 compiler

_Auto-Vk_ consists of multiple C++ include files, one mandatory C++ source file, and one optional C++ source file:
* Add [`include/`](include/) to the include paths so that your compiler can find include files under paths `avk/*`
* Add [`src/avk.cpp`](src/avk.cpp) as a compiled C++ source code file
* *Optional:* Add [`src/vk_mem_alloc.cpp`](src/vk_mem_alloc.cpp) if you want to use [Vulkan Memory Allocator (VMA)](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) for handling memory allocations. For configuration instructions, see section [Memory Allocation](#memory-allocation).

#### Caveats
* On `clang` (at least version <= 12) _Auto-Vk_ does not compile when using `libstdc++` version 11 or higher, because `clang` doesn't yet support "Down with `typename`!" ([P0634R3](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0634r3.html)), which is used in the `libstdc++` `ranges`-header.

# Motivating Examples

## Creating a graphics pipeline

The task of creating a graphics pipeline in Vulkan might require several hundreds of lines of code. Here is what it can look like with _Auto-Vk_:

```cpp
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
	avk::attachment::declare(vk::Format::eR8G8B8A8Unorm,  avk::on_load::clear, avk::usage::unused >> avk::usage::color(0) >> avk::usage::input(0), avk::on_store::dont_care),	
	avk::attachment::declare(vk::Format::eD24UnormS8Uint, avk::on_load::clear, avk::usage::depth_stencil >> avk::usage::preserve >> avk::usage::input(1),       avk::on_store::dont_care),
	avk::attachment::declare(vk::Format::eR8G8B8A8Unorm,  avk::on_load::clear, avk::usage::unused >> avk::usage::unused >> avk::usage::color(0), avk::on_store.in_layout(avk::layout::color_attachment_optimal)),	
	// The following define resources that will be bound to the pipeline:
	avk::push_constant_binding_data { avk::shader_type::vertex, 0, sizeof(std::array<float, 16>) },
	avk::descriptor_binding(0, 0, myBuffer->as_uniform_buffer()),
	avk::descriptor_binding(0, 1, myImageView->as_sampled_image(avk::layout::shader_read_only_optimal))
);
```

This creates a fully functioning graphics pipeline on the device consisting of two shaders (a vertex shader and a fragment shader), with three vertex/instance input bindings, stating some additional pipeline settings (face orientation, and viewport config), defining the renderpass' attachments, and further resource bindings to the pipeline: push constants of the size 64 byte (16 floats, which is enough space for a 4x4 matrix), and two descriptors, namely a buffer bound as uniform buffer (corresponding to descriptor type `vk::DescriptorType::eUniformBuffer`) and an image view as sampled image (corresponding to descriptor type `vk::DescriptorType::eSampledImage`) in a given layout.

**Vertex input bindings** are specified in an expressive way: For example,       
`avk::from_buffer_binding(2) -> stream_per_instance<glm::vec3>() -> to_location(5)`             
means that from the buffer that is bound at index `2` when invoking `vk::CommandBuffer::bindVertexBuffers`, data is being streamed to vertex shader's `layout (location=5)` and the data pointer is advanced per instance. 

In a similarly expressive manner, the **subpass usages of the renderpass' attachments** are specified. In the example above, there are three subpasses. The first color attachment is not used in the first subpass, it is used as color attachment in subpass one (bound to output location `0`), and as input attachment in subpass two (bound to input attachment index `0`).        
The depth/stencil attachment is used as depth/stencil attachment in the first subpass, is not used in the second subpass but its contents are preserved, and in the third subpass, is is used as an input attachment (bound to input attachment index `1`).      
The second color attachment is only used in subpass three (bound to output location `0`), and its contents are stored in an image layout optimal for color attachments, while the contents of the other two attachments are no longer needed.

## Recording and submitting batches of commands

Recording batches of work and submitting that to a queue can require a lot of code---especially if additional synchronization to other work shall be established. With _Auto-Vk_, it can look like follows:

```cpp 
avk::queue& graphicsQueue = myRoot.create_queue(vk::QueueFlagBits::eGraphics, avk::queue_selection_preference::versatile_queue);
...
avk::command_pool& commandPool = myRoot.get_command_pool_for_single_use_command_buffers(graphicsQueue);
avk::command_buffer cmdBfr = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
avk::graphics_pipeline graphicsPipeline = myRoot.create_graphics_pipeline_for(...);
avk::framebuffer framebuffer = myRoot.create_framebuffer(...);
avk::semaphore imageAvailableSemaphore = ...;

mRoot.record({
      avk::command::render_pass(graphicsPipeline->renderpass_reference(), framebuffer.as_reference(), {
          avk::command::bind_pipeline(graphicsPipeline.as_reference()),
          avk::command::draw(3u, 1u, 0u, 0u)
      })
  })
  .into_command_buffer(cmdBfr)
  .then_submit_to(graphicsQueue)
  .waiting_for(imageAvailableSemaphore >> avk::stage::color_attachment_output)
  .submit();
```

Getting handles to a queue (`avk::queue&`) and to a command pool (`avk::command_pool&`), and creating a command buffer (`avk::command_buffer`) from a command pool are one-liners. The creation of a graphics queue has been described above under [Creating a graphics pipeline](#creating-a-graphics-pipeline). Framebuffers (`avk::framebuffer`) and semaphores (`avk::semaphore`) can be efficiently created too. 

In the example, let us assume that we have received the `avk::semaphore imageAvailableSemaphore` from our swap chain, indicatig when the image is available and can be rendered into. We can express such a dependency for a new batch of work (as recorded via `root::record`) simply by appending `.waiting_for(imageAvailableSemaphore >> avk::stage::color_attachment_output)`. It says that the batch's color attachment output stage must wait until the given semaphore has been signaled.

The batch that is recorded into `avk::command_buffer cmdBfr` (through `.into_command_buffer(cmdBfr)`), describes the rendering of a renderpass, and within the renderpass, the `graphicsPipeline` is bound, and one draw call (rendering `3u` vertices and `1u` instance) is recorded.

The batch is submitted to a specific queue, configured through `.then_submit_to(graphicsQueue)`, and executed at `.submit()`-time. If `.submit()` is not called explicitly, the returned `avk::submission_data`'s destructor will ensure to submit it to the queue (yeah, in its destructor ^^).

## Recording barriers efficiently

Also "simple" tasks like recording barriers can take so much code in Vulkan. See how a global memory barrier can be recorded in _Auto-Vk_:

```cpp
avk::sync::global_memory_barrier(
    avk::stage::transfer        >> avk::stage::acceleration_structure_build, 
    avk::access::transfer_write >> avk::access::shader_read
)
``` 

It expresses that transfer stages and transfer writes must have completed and been made available, before an acceleration build structure may proceed, reading the data that has been written by the preceding transfer operations. Such a barrier can be recorded into a batch (e.g.: `root::record`). There's an alternative (maybe even more convenient) syntax for recording such a barrier, but it requires `using namespace avk;` s.t. the compiler can find the necessary `operator+`:

```cpp
using namespace avk;

sync::global_memory_barrier(
    stage::transfer + access::transfer_write  >>  stage::acceleration_structure_build + access::shader_read
)
``` 

Buffer memory barriers and image memory barriers additionally offer the option to conveniently and expressively add queue family ownership transfers; image memory barriers also support the specification of image layout transitions as shown in the following example:

```cpp
using namespace avk;

queue& transferQueue = myRoot.create_queue(vk::QueueFlagBits::eTransfer, queue_selection_preference::specialized_queue);
queue& graphicsQueue = myRoot.create_queue(vk::QueueFlagBits::eGraphics, queue_selection_preference::specialized_queue);
...
image myImage;
...
avk::sync::image_memory_barrier(myImage.as_reference(), stage::copy + access::transfer_write >> stage::fragment_shader + access::shader_read)
    .with_layout_transition(layout::transfer_dst >> layout::shader_read_only_optimal)
    .with_queue_family_ownership_transfer(transferQueue.family_index(), graphicsQueue.family_index())
``` 

Finally, as a special convenience feature, _Auto-Vk_ is able to automatically determine the stages and access types of barriers for many cases:

```cpp
using namespace avk;

std::vector<glm::vec3> vertices;
buffer vertexBuffer = myRoot.create_buffer(memory_usage::device, {}, vertex_buffer_meta::create_from_data(vertices));
bottom_level_acceleration_structure blas = ...;

mRoot.record({
      myBuffer->fill(myDataPointer, 0),
      sync::buffer_memory_barrier(vertexBuffer.as_reference(), stage::auto_stage + access::auto_access >> stage::auto_stage + access::auto_access),
      blas->build(...)
  })
  ...
```

In such a case, a barrier corresponding to `stage::copy + access::transfer_write >> stage::acceleration_structure_build + access::shader_read` should be established. 

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

# Resource Management

Resource management in _Auto-Vk_ is managed in a way which strongly relies on _move-only_ types. That means: Whenever a resource's destructor is being invoked, the resource is destroyed and its memory freed. Furthermore, where resources "live" can be totally controlled by the programmer: be it on the stack or on the heap, enabling both, efficient and versatile usage patterns. These policies require explicit and precise handling of _how_ resources are stored and passed around, especially when passing them as arguments to functions/methods.

The **TL;DR**-version of resource-passing guidelines reads like follows:
- When a reference to a resource is requested, the argument's type will have the form `avk::resource_reference<T>` => Pass a resource `r` like follows: `avk::referenced(r)`     
  (You can think of this as an analog to `std::reference_wrapper` created via `std::ref`.)
- When a const-reference to a resource is requested, the argument's type will have the form `avk::resource_reference<const T>` => Pass a resource `r` like follows: `avk::const_referenced(r)`    
  (You can think of this as an analog to `std::reference_wrapper` created via `std::cref`.)
- When an argument indicates that ownership of a resource must be taken, the argument's type will have the form `avk::resource_ownership<T>` => Pass a resource `r` in one of two ways:
    - Use `avk::owned(r)` if you no longer need `r` anywhere else. (You can think of this as an analog to `std::move`.)
    - Use `avk::shared(r)` if you still need `r` in other places. (This corresponds to transferring `r` into a `std::shared_ptr<T>` and passing the shared pointer around.)
- When it is not obvious which kind of resource-passing is requested (as it can happen with some templated methods/functions), please consult the method's/function's documentation. Usually, such arguments will be captured by value (i.e. not by lvalue reference or rvalue reference) and the arguments' description will state whether you are expected to pass via `avk::referenced`, `avk::const_referenced`, or `avk::owned`/`avk::shared`.

The resource type `T` always refers to the "inner resource type" which, for example, means `avk::image_t`, when there's also a `avk::image`. I.e. don't let yourself be confused by the `_t`-version and the non-`_t`-version, they actually mean the same thing, only that the non-`_t`-versions represent the `_t`-versions with an additional lifetime-convenience-wrapper around them which adds some features w.r.t. lifetime-handling of the "inner resource types" (i.e. the `_t`-versions). Whenever you call one of the `root::create_*` methods, you'll get back a resource _with_ that convenience-wrapper.

The following is a list of _Auto-Vk_ resource types that are implemented and handled as described above:
- Buffer type `avk::buffer_t` and its wrapper type `avk::buffer`
- Buffer-View type `avk::buffer_view_t` and its wrapper type `avk::buffer_view`
- Image type `avk::image_t` and its wrapper type `avk::image`
- Image-View type `avk::image_view_t` and its wrapper type `avk::image_view`
- Sampler type `avk::sampler_t` and its wrapper type `avk::sampler`
- Image Sampler type `avk::image_sampler_t` and its wrapper type `avk::image_sampler`
- Framebuffer type `avk::framebuffer_t` and its wrapper type `avk::framebuffer`
- Renderpass type `avk::renderpass_t` and its wrapper type `avk::renderpass`
- Graphics Pipeline type `avk::graphics_pipeline_t` and its wrapper type `avk::graphics_pipeline`
- Compute Pipeline type `avk::compute_pipeline_t` and its wrapper type `avk::compute_pipeline`
- Ray-Tracing Pipeline type `avk::ray_tracing_pipeline_t` and its wrapper type `avk::ray_tracing_pipeline`
- Ray-Tracing Bottom Level Acceleration Structure type `avk::bottom_level_acceleration_structure_t` and its wrapper type `avk::bottom_level_acceleration_structure`
- Ray-Tracing Top Level Acceleration Structure type `avk::top_level_acceleration_structure_t` and its wrapper type `avk::top_level_acceleration_structure_t`
- Command Pool type `avk::command_pool_t` and its wrapper type `avk::command_pool`
- Command Buffer type `avk::command_buffer_t` and its wrapper type `avk::command_buffer`
- Fence type `avk::fence_t` and its wrapper type `avk::fence`
- Semaphore type `avk::semaphore_t` and its wrapper type `avk::semaphore`
- Query Pool type `avk::query_pool_t` and its wrapper type `avk::query_pool`

The wrapper types are defined as `avk::owning_resource<T>`, where `T` refers to one of the "inner resource types" and collude with `avk::resource_reference` (as created via `avk::referenced` and `avk::const_referenced`) and `avk::resource_ownership` (as created via `avk::owned` and `avk::shared`) types. For ordinary framework usage, you should probably not really require this information. View it as additional background information. 

One of `avk::owning_resource<T>`'s features shall be pointed out explicitly, namely **enabling shared ownership**:     
If you need to have **multiple owners** of a resource, you can use `avk::owning_resource<T>::enable_shared_ownership` which will move the resource internally into a shared pointer. Storing resources in shared pointers is not the default because it would violate one of our and C++'s core principles: "don't pay for what you're not using". There is no point in making a heap allocation if an object could spend its lifetime on the stack. But sometimes, there must be multiple owners of a resource, and this feature can come in handy. `avk::owning_resource<T>::enable_shared_ownership` is actually what is automatically internally invoked on resource `r` by a call to `avk::shared(r)`.

# Sync

Synchronization by the means of 
* waiting for idle,
* semaphores, and
* pipeline barriers

is currently implemented in class `avk::sync`. Many functions/method which perform asynchronous operations take `avk::sync` parameters to allow modification of the synchronization strategy.

_Attention:_ `avk::sync` is ugly and is subject to change. Expect breaking changes soon.

# Memory Allocation

By default _Auto-Vk_ uses a very straight-forward, but for most cases probably also suboptimal, way of handling memory allocations: One allocation per resource. This is especially suboptimal if many small resources are used. Implementation-wise, [`avk::mem_handle`](include/avk/mem_handle.hpp) is used in this case. 

_Auto-Vk_, however, allows to easily swap this straight-froward way of memory handling with using the well-established [Vulkan Memory Allocator (VMA) library](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator). Only a small config-change is necessary to switch from [`avk::mem_handle`](include/avk/mem_handle.hpp) to [`avk::vma_handle`](include/avk/vma_handle.hpp), which uses VMA to alloc memory for all resources.

**To enable VMA**, define `AVK_USE_VMA` *before* including `<avk/avk.hpp>`:
```
#define AVK_USE_VMA
#include <avk/avk.hpp>
```
Furthermore, add [`src/vk_mem_alloc.cpp`](src/vk_mem_alloc.cpp) to your source files. If you're using `CMake` you can do this by setting the `avk_UseVMA` option to `ON`.

This is all that is required to set-up VMA to handle all internal memory allocations.

**Advanced usage:**

By defining `AVK_USE_VMA`, internally the following definitions are set:
```
#define AVK_MEM_ALLOCATOR_TYPE       VmaAllocator
#define AVK_MEM_IMAGE_HANDLE         avk::vma_handle<vk::Image>
#define AVK_MEM_BUFFER_HANDLE        avk::vma_handle<vk::Buffer>
```

By defining them by yourself *before* including `<avk/avk.hpp>`, you can plug in custom memory allocation behavior into _Auto-Vk_. 
