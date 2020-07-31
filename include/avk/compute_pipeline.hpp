#pragma once
#include <avk/avk.hpp>

namespace avk
{
	/** Represents data for a vulkan compute pipeline */
	class compute_pipeline_t
	{
		friend class root;
	public:
		compute_pipeline_t() = default;
		compute_pipeline_t(compute_pipeline_t&&) noexcept = default;
		compute_pipeline_t(const compute_pipeline_t&) = delete;
		compute_pipeline_t& operator=(compute_pipeline_t&&) noexcept = default;
		compute_pipeline_t& operator=(const compute_pipeline_t&) = delete;
		~compute_pipeline_t() = default;

		const auto& layout_handle() const { return mPipelineLayout.get(); }
		std::tuple<const compute_pipeline_t*, const vk::PipelineLayout, const std::vector<vk::PushConstantRange>*> layout() const { return std::make_tuple(this, layout_handle(), &mPushConstantRanges); }
		const auto& handle() const { return mPipeline.get(); }

	private:
		// TODO: What to do with flags?
		vk::PipelineCreateFlags mPipelineCreateFlags;

		// Only one shader for compute pipelines:
		shader mShader;
		vk::PipelineShaderStageCreateInfo mShaderStageCreateInfo;
		std::optional<vk::SpecializationInfo> mSpecializationInfo;

		// TODO: What to do with the base pipeline index?
		int32_t mBasePipelineIndex;

		// Pipeline layout, i.e. resource bindings
		set_of_descriptor_set_layouts mAllDescriptorSetLayouts;
		std::vector<vk::PushConstantRange> mPushConstantRanges;
		vk::PipelineLayoutCreateInfo mPipelineLayoutCreateInfo;

		// Handles:
		vk::UniquePipelineLayout mPipelineLayout;
		vk::UniquePipeline mPipeline;
	};
	
	using compute_pipeline = avk::owning_resource<compute_pipeline_t>;

	template <>
	inline void command_buffer_t::bind_pipeline<compute_pipeline_t>(const compute_pipeline_t& aPipeline)
	{
		handle().bindPipeline(vk::PipelineBindPoint::eCompute, aPipeline.handle());
	}

	template <>
	inline void command_buffer_t::bind_pipeline<compute_pipeline>(const compute_pipeline& aPipeline)
	{
		bind_pipeline<compute_pipeline_t>(aPipeline);
	}

	template <>
	inline void command_buffer_t::bind_descriptors<std::tuple<const compute_pipeline_t*,  const vk::PipelineLayout, const std::vector<vk::PushConstantRange>*>>
		(std::tuple<const compute_pipeline_t*, const vk::PipelineLayout, const std::vector<vk::PushConstantRange>*> aPipelineLayout, std::vector<descriptor_set> aDescriptorSets)
	{
		bind_descriptors(vk::PipelineBindPoint::eCompute, std::get<const compute_pipeline_t*>(aPipelineLayout)->layout_handle(), std::move(aDescriptorSets));
	}
}
