//
// Created by ripley on 04.09.20.
//

#include <iostream>

#include "avk/avk.hpp"

class my_root : public avk::root {
 public:
  vk::Instance vulkan_instance()
  {
    if (!mInstance) {
      mInstance = vk::createInstanceUnique(vk::InstanceCreateInfo{});
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

  vk::Device& device() override
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

  vk::DispatchLoaderDynamic& dynamic_dispatch() override
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

int main() {
  std::cout << vk::enumerateInstanceVersion() << '\n';
  try {
    auto root = my_root();
    /*
    auto graphicsPipeline = root.create_graphics_pipeline_for(
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
     */

    std::cout << "Hello World" << '\n';
  } catch (std::runtime_error e) {
    std::cout << e.what() << '\n';
  }
  return 0;
}