#pragma once
#include <avk/avk.hpp>

namespace avk
{

	/** Represents a Vulkan command pool, holds the native handle and takes
	*	care about lifetime management of the native handles.
	*	Also contains the queue family index it has been created for and is 
	*  intended to be used with:
	*	"All command buffers allocated from this command pool must be submitted 
	*	 on queues from the same queue family." [+]
	*	
	*	[+]: https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkCommandPoolCreateInfo.html
	*/
	class command_pool_t
	{
		friend class root;
		
	public:
		command_pool_t() = default;
		command_pool_t(const command_pool_t&) = delete;
		command_pool_t(command_pool_t&&) noexcept = default;
		command_pool_t& operator=(const command_pool_t&) = delete;
		command_pool_t& operator=(command_pool_t&&) noexcept = default;
		~command_pool_t() = default;

		auto queue_family_index() const { return mQueueFamilyIndex; }
		const auto& create_info() const	{ return mCreateInfo; }
		auto& create_info()				{ return mCreateInfo; }
		const auto& handle() const { return mCommandPool->get(); }
		const auto* handle_ptr() const { return &mCommandPool->get(); }

		std::vector<command_buffer> alloc_command_buffers(uint32_t aCount, vk::CommandBufferUsageFlags aUsageFlags = {}, vk::CommandBufferLevel aLevel = vk::CommandBufferLevel::ePrimary);
			
		command_buffer alloc_command_buffer(vk::CommandBufferUsageFlags aUsageFlags = {}, vk::CommandBufferLevel aLevel = vk::CommandBufferLevel::ePrimary);
		
	private:
		uint32_t mQueueFamilyIndex;
		vk::CommandPoolCreateInfo mCreateInfo;
		std::shared_ptr<vk::UniqueCommandPool> mCommandPool;
	};

	using command_pool = owning_resource<command_pool_t>;
	
}
