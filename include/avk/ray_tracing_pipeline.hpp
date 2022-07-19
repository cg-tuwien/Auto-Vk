#pragma once
#include <avk/avk.hpp>

namespace avk
{
#if VK_HEADER_VERSION >= 135
	/** Represents data for a Vulkan ray tracing pipeline */
	class ray_tracing_pipeline_t
	{
		friend class root;
		
	public:
		ray_tracing_pipeline_t() = default;
		ray_tracing_pipeline_t(ray_tracing_pipeline_t&&) noexcept = default;
		ray_tracing_pipeline_t(const ray_tracing_pipeline_t&) = delete;
		ray_tracing_pipeline_t& operator=(ray_tracing_pipeline_t&&) noexcept = default;
		ray_tracing_pipeline_t& operator=(const ray_tracing_pipeline_t&) = delete;
		~ray_tracing_pipeline_t() = default;

		auto& create_flags() { return mPipelineCreateFlags; }
		auto& shaders() { return mShaders; }
		auto& shader_stage_create_infos() { return mShaderStageCreateInfos; }
		auto& specialization_infos() { return mSpecializationInfos; }
		auto& shader_group_creates_infos() { return mShaderGroupCreateInfos; }
		auto& get_shader_binding_table_groups_info() { return mShaderBindingTableGroupsInfo; }
		auto& max_recursion_depth() { return mMaxRecursionDepth; }
		auto& base_pipeline_index() { return mBasePipelineIndex; }
		auto& descriptor_set_layouts() { return mAllDescriptorSetLayouts; }
		auto& push_constant_ranges() { return mPushConstantRanges; }
		auto& pipeline_layout_create_info() { return mPipelineLayoutCreateInfo; }
		
		const auto& create_flags() const { return mPipelineCreateFlags; }
		const auto& shaders() const { return mShaders; }
		const auto& shader_stage_create_infos() const { return mShaderStageCreateInfos; }
		const auto& specialization_infos() const { return mSpecializationInfos; }
		const auto& shader_group_creates_infos() const { return mShaderGroupCreateInfos; }
		const auto& get_shader_binding_table_groups_info() const { return mShaderBindingTableGroupsInfo; }
		const auto& max_recursion_depth() const { return mMaxRecursionDepth; }
		const auto& base_pipeline_index() const { return mBasePipelineIndex; }
		const auto& descriptor_set_layouts() const { return mAllDescriptorSetLayouts; }
		const auto& push_constant_ranges() const { return mPushConstantRanges; }
		const auto& pipeline_layout_create_info() const { return mPipelineLayoutCreateInfo; }
		const auto& layout_handle() const { return mPipelineLayout.get(); }
		std::tuple<const ray_tracing_pipeline_t*, const vk::PipelineLayout, const std::vector<vk::PushConstantRange>*> layout() const { return std::make_tuple(this, layout_handle(), &mPushConstantRanges); }
		const auto& handle() const { return mPipeline.get(); }
		vk::DeviceSize table_offset_size() const { return static_cast<vk::DeviceSize>(mShaderGroupBaseAlignment); }
		vk::DeviceSize table_entry_size() const { return static_cast<vk::DeviceSize>(mShaderGroupHandleSize); }
		vk::DeviceSize table_size() const { return static_cast<vk::DeviceSize>(mShaderBindingTable->meta_at_index<buffer_meta>(0).total_size()); }
		auto shader_binding_table_handle() const { return mShaderBindingTable->handle(); }
		auto shader_binding_table_device_address() const { return mShaderBindingTable->device_address(); }
		const auto& shader_binding_table_groups() const { return mShaderBindingTableGroupsInfo; }
		shader_binding_table_ref shader_binding_table() const
		{
			return shader_binding_table_ref{
				mRoot,
				shader_binding_table_handle(), 
#if VK_HEADER_VERSION >= 162
				shader_binding_table_device_address(), 
#else 
				0,
#endif
				table_entry_size(), 
				std::cref(shader_binding_table_groups())
			};
		}
		size_t num_raygen_groups_in_shader_binding_table() const;
		size_t num_miss_groups_in_shader_binding_table() const;
		size_t num_hit_groups_in_shader_binding_table() const;
		size_t num_callable_groups_in_shader_binding_table() const;
		void print_shader_binding_table_groups() const;
		
	private:
		// TODO: What to do with flags?
		vk::PipelineCreateFlags mPipelineCreateFlags;

		// Our precious GPU shader programs:
		std::vector<shader> mShaders;
		std::vector<vk::PipelineShaderStageCreateInfo> mShaderStageCreateInfos;
		std::vector<vk::SpecializationInfo> mSpecializationInfos;

		// Shader table a.k.a. shader groups:
		std::vector<vk::RayTracingShaderGroupCreateInfoKHR> mShaderGroupCreateInfos;

		// Info about the groups in the shader binding table:
		shader_binding_table_groups_info mShaderBindingTableGroupsInfo;

		// Maximum recursion depth:
		uint32_t mMaxRecursionDepth;

		// TODO: What to do with the base pipeline index?
		int32_t mBasePipelineIndex;

		// Pipeline layout, i.e. resource bindings
		set_of_descriptor_set_layouts mAllDescriptorSetLayouts;
		std::vector<vk::PushConstantRange> mPushConstantRanges;
		vk::PipelineLayoutCreateInfo mPipelineLayoutCreateInfo;

		// Handles:
		vk::UniqueHandle<vk::PipelineLayout, DISPATCH_LOADER_CORE_TYPE> mPipelineLayout;
		//vk::ResultValueType<vk::UniqueHandle<vk::Pipeline, DISPATCH_LOADER_EXT_TYPE>>::type mPipeline;
		//avk::handle_wrapper<vk::Pipeline> mPipeline;
		vk::UniqueHandle<vk::Pipeline, DISPATCH_LOADER_EXT_TYPE> mPipeline;

		uint32_t mShaderGroupBaseAlignment;
		uint32_t mShaderGroupHandleSize;
		buffer mShaderBindingTable; // TODO: support more than one shader binding table?

		const root* mRoot;
	};

	using ray_tracing_pipeline = avk::owning_resource<ray_tracing_pipeline_t>;

#endif
}
