#pragma once
#include <avk/avk.hpp>

namespace avk
{
	/** Represents data for a vulkan graphics pipeline */
	class graphics_pipeline_t
	{
		friend class root;
		
	public:
		graphics_pipeline_t() = default;
		graphics_pipeline_t(graphics_pipeline_t&&) noexcept = default;
		graphics_pipeline_t(const graphics_pipeline_t&) = delete;
		graphics_pipeline_t& operator=(graphics_pipeline_t&&) noexcept = default;
		graphics_pipeline_t& operator=(const graphics_pipeline_t&) = delete;
		~graphics_pipeline_t() = default;

		[[nodiscard]] resource_reference<const renderpass_t> get_renderpass() const { return const_referenced(mRenderPass); }
		auto renderpass_handle() const { return mRenderPass->handle(); }
		auto subpass_id() const { return mSubpassIndex; }
		auto& vertex_input_binding_descriptions() { return mOrderedVertexInputBindingDescriptions; }
		auto& vertex_input_attribute_descriptions() { return mVertexInputAttributeDescriptions; }
		auto& vertex_input_state_create_info() { return mPipelineVertexInputStateCreateInfo; }
		auto& input_assembly_state_create_info() { return mInputAssemblyStateCreateInfo; }
		auto& shaders() { return mShaders; }
		auto& shader_stage_create_infos() { return mShaderStageCreateInfos; }
		auto& specialization_infos() { return mSpecializationInfos; }
		auto& viewports() { return mViewports; }
		auto& scissors() { return mScissors; }
		auto& viewport_state_create_info() { return mViewportStateCreateInfo; }
		auto& rasterization_state_create_info() { return mRasterizationStateCreateInfo; }
		auto& depth_stencil_config() { return mDepthStencilConfig; }
		auto& blending_configs_for_color_attachments() { return mBlendingConfigsForColorAttachments; }
		auto& color_blend_state_create_info() { return mColorBlendStateCreateInfo; }
		auto& multisample_state_create_info() { return mMultisampleStateCreateInfo; }
		auto& dynamic_state_entries() { return mDynamicStateEntries; }
		auto& dynamic_state_create_info() { return mDynamicStateCreateInfo; }
		auto& descriptor_set_layouts() { return mAllDescriptorSetLayouts; }
		auto& push_constant_ranges() { return mPushConstantRanges; }
		auto& layout_create_info() { return mPipelineLayoutCreateInfo; }
		auto& tessellation_state_create_info() { return mPipelineTessellationStateCreateInfo; }
		auto& create_flags() { return mPipelineCreateFlags; }
		const auto& vertex_input_binding_descriptions() const { return mOrderedVertexInputBindingDescriptions; }
		const auto& vertex_input_attribute_descriptions() const { return mVertexInputAttributeDescriptions; }
		const auto& vertex_input_state_create_info() const { return mPipelineVertexInputStateCreateInfo; }
		const auto& input_assembly_state_create_info() const { return mInputAssemblyStateCreateInfo; }
		const auto& shaders() const { return mShaders; }
		const auto& shader_stage_create_infos() const { return mShaderStageCreateInfos; }
		const auto& specialization_infos() const { return mSpecializationInfos; }
		const auto& viewports() const { return mViewports; }
		const auto& scissors() const { return mScissors; }
		const auto& viewport_state_create_info() const { return mViewportStateCreateInfo; }
		const auto& rasterization_state_create_info() const { return mRasterizationStateCreateInfo; }
		const auto& depth_stencil_config() const { return mDepthStencilConfig; }
		const auto& blending_configs_for_color_attachments() const { return mBlendingConfigsForColorAttachments; }
		const auto& color_blend_state_create_info() const { return mColorBlendStateCreateInfo; }
		const auto& multisample_state_create_info() const { return mMultisampleStateCreateInfo; }
		const auto& dynamic_state_entries() const { return mDynamicStateEntries; }
		const auto& dynamic_state_create_info() const { return mDynamicStateCreateInfo; }
		const auto& descriptor_set_layouts() const { return mAllDescriptorSetLayouts; }
		const auto& push_constant_ranges() const { return mPushConstantRanges; }
		const auto& layout_create_info() const { return mPipelineLayoutCreateInfo; }
		const auto& tessellation_state_create_info() const { return mPipelineTessellationStateCreateInfo; }
		const auto& create_flags() const { return mPipelineCreateFlags; }
		const auto& layout_handle() const { return mPipelineLayout.get(); }
		std::tuple<const graphics_pipeline_t*, const vk::PipelineLayout, const std::vector<vk::PushConstantRange>*> layout() const { return std::make_tuple(this, layout_handle(), &mPushConstantRanges); }
		const auto& handle() const { return mPipeline.get(); }
		
	private:
		renderpass mRenderPass;
		uint32_t mSubpassIndex;
		// The vertex input data:
		std::vector<vk::VertexInputBindingDescription> mOrderedVertexInputBindingDescriptions;
		std::vector<vk::VertexInputAttributeDescription> mVertexInputAttributeDescriptions;
		vk::PipelineVertexInputStateCreateInfo mPipelineVertexInputStateCreateInfo;
		// How to interpret the vertex input:
		vk::PipelineInputAssemblyStateCreateInfo mInputAssemblyStateCreateInfo;
		// Our precious GPU shader programs:
		std::vector<shader> mShaders;
		std::vector<vk::PipelineShaderStageCreateInfo> mShaderStageCreateInfos;
		std::vector<vk::SpecializationInfo> mSpecializationInfos;
		// Viewport, depth, and scissors configuration
		std::vector<vk::Viewport> mViewports;
		std::vector<vk::Rect2D> mScissors;
		vk::PipelineViewportStateCreateInfo mViewportStateCreateInfo;
		// Rasterization state:
		vk::PipelineRasterizationStateCreateInfo mRasterizationStateCreateInfo;
		// Depth stencil config:
		vk::PipelineDepthStencilStateCreateInfo mDepthStencilConfig;
		// Color blend attachments
		std::vector<vk::PipelineColorBlendAttachmentState> mBlendingConfigsForColorAttachments;
		vk::PipelineColorBlendStateCreateInfo mColorBlendStateCreateInfo;
		// Multisample state
		vk::PipelineMultisampleStateCreateInfo mMultisampleStateCreateInfo;
		// Dynamic state
		std::vector<vk::DynamicState> mDynamicStateEntries;
		vk::PipelineDynamicStateCreateInfo mDynamicStateCreateInfo;
		// Pipeline layout, i.e. resource bindings
		set_of_descriptor_set_layouts mAllDescriptorSetLayouts;
		std::vector<vk::PushConstantRange> mPushConstantRanges;
		vk::PipelineLayoutCreateInfo mPipelineLayoutCreateInfo;
		std::optional<vk::PipelineTessellationStateCreateInfo> mPipelineTessellationStateCreateInfo;
		
		// TODO: What to do with flags?
		vk::PipelineCreateFlags mPipelineCreateFlags;

		// Handles:
		vk::UniqueHandle<vk::PipelineLayout, DISPATCH_LOADER_CORE_TYPE> mPipelineLayout;
		vk::UniqueHandle<vk::Pipeline, DISPATCH_LOADER_CORE_TYPE> mPipeline;
	};
	
	using graphics_pipeline = avk::owning_resource<graphics_pipeline_t>;
	
	template <>
	inline void command_buffer_t::bind_pipeline<resource_reference<const graphics_pipeline_t>>(resource_reference<const graphics_pipeline_t> aPipelineRef)
	{
		handle().bindPipeline(vk::PipelineBindPoint::eGraphics, aPipelineRef->handle());
	}

	template <>
	inline void command_buffer_t::bind_descriptors<std::tuple<const graphics_pipeline_t*, const vk::PipelineLayout, const std::vector<vk::PushConstantRange>*>>
		(std::tuple<const graphics_pipeline_t*, const vk::PipelineLayout, const std::vector<vk::PushConstantRange>*> aPipelineLayout, std::vector<descriptor_set> aDescriptorSets)
	{
		bind_descriptors(vk::PipelineBindPoint::eGraphics, std::get<const graphics_pipeline_t*>(aPipelineLayout)->layout_handle(), std::move(aDescriptorSets));
	}

}
