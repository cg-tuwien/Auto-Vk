#pragma once

namespace ak // ========================== TODO/WIP =================================
{
	struct shader_binding_table
	{
		friend class root;
		
		shader_binding_table();
		shader_binding_table(size_t, const vk::BufferUsageFlags&, const vk::Buffer&, const vk::MemoryPropertyFlags&, const vk::DeviceMemory&) noexcept;
		shader_binding_table(const sampler&) = delete;
		shader_binding_table(shader_binding_table&&) noexcept;
		shader_binding_table& operator=(const shader_binding_table&) = delete;
		shader_binding_table& operator=(shader_binding_table&&) noexcept;
		~shader_binding_table();

	};
}
