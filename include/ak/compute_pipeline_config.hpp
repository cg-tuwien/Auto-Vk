#pragma once

namespace ak
{
	/** Pipeline configuration data: COMPUTE PIPELINE CONFIG STRUCT */
	struct compute_pipeline_config
	{
		compute_pipeline_config() 
			: mPipelineSettings{ cfg::pipeline_settings::nothing }
			, mShaderInfo{}
		{}
		
		compute_pipeline_config(compute_pipeline_config&&) noexcept = default;
		compute_pipeline_config(const compute_pipeline_config&) = delete;
		compute_pipeline_config& operator=(compute_pipeline_config&&) noexcept = default;
		compute_pipeline_config& operator=(const compute_pipeline_config&) = delete;
		~compute_pipeline_config() = default;

		cfg::pipeline_settings mPipelineSettings; // ?
		std::optional<shader_info> mShaderInfo;
		std::vector<binding_data> mResourceBindings;
		std::vector<push_constant_binding_data> mPushConstantsBindings;
	};

	// End of recursive variadic template handling
	inline void add_config(compute_pipeline_config& aConfig, std::function<void(compute_pipeline_t&)>& aFunc) { /* We're done here. */ }

	// Add a specific pipeline setting to the pipeline config
	template <typename... Ts>
	void add_config(compute_pipeline_config& aConfig, std::function<void(compute_pipeline_t&)>& aFunc, cfg::pipeline_settings aSetting, Ts... args)
	{
		aConfig.mPipelineSettings |= aSetting;
		add_config(aConfig, aFunc, std::move(args)...);
	}

	// Add a shader to the pipeline config
	template <typename... Ts>
	void add_config(compute_pipeline_config& aConfig, std::function<void(compute_pipeline_t&)>& aFunc, shader_info aShaderInfo, Ts... args)
	{
		if (aConfig.mShaderInfo.has_value()) {
			throw ak::logic_error("Only one shader is supported for a compute pipeline.");
		}
		if (aShaderInfo.mShaderType != shader_type::compute) {
			throw ak::logic_error("The shader's type is not compute.");
		}
		aConfig.mShaderInfo = std::move(aShaderInfo);
		add_config(aConfig, aFunc, std::move(args)...);
	}

	// Accept a string and assume it refers to a shader file
	template <typename... Ts>
	void add_config(compute_pipeline_config& aConfig, std::function<void(compute_pipeline_t&)>& aFunc, std::string_view aShaderPath, Ts... args)
	{
		if (aConfig.mShaderInfo.has_value()) {
			throw ak::logic_error("Only one shader is supported for a compute pipeline.");
		}
		aConfig.mShaderInfo = shader_info::create(std::string(aShaderPath));
		if (aConfig.mShaderInfo->mShaderType != shader_type::compute) {
			throw ak::logic_error("The shader's type is not compute.");
		}
		add_config(aConfig, aFunc, std::move(args)...);
	}

	// Add a resource binding to the pipeline config
	template <typename... Ts>
	void add_config(compute_pipeline_config& aConfig, std::function<void(compute_pipeline_t&)>& aFunc, binding_data aResourceBinding, Ts... args)
	{
		if ((aResourceBinding.mLayoutBinding.stageFlags & vk::ShaderStageFlagBits::eCompute) != vk::ShaderStageFlagBits::eCompute) {
			throw ak::logic_error("Resource not visible in compute shader, but this is a compute pipeline => that makes no sense.");
		}
		aConfig.mResourceBindings.push_back(std::move(aResourceBinding));
		add_config(aConfig, aFunc, std::move(args)...);
	}

	// Add a push constants binding to the pipeline config
	template <typename... Ts>
	void add_config(compute_pipeline_config& aConfig, std::function<void(compute_pipeline_t&)>& aFunc, push_constant_binding_data aPushConstBinding, Ts... args)
	{
		if ((aPushConstBinding.mShaderStages & shader_type::compute) != shader_type::compute) {
			throw ak::logic_error("Push constants are not visible in compute shader, but this is a compute pipeline => that makes no sense.");
		}
		aConfig.mPushConstantsBindings.push_back(std::move(aPushConstBinding));
		add_config(aConfig, aFunc, std::move(args)...);
	}

	// Add an config-alteration function to the pipeline config
	template <typename... Ts>
	void add_config(compute_pipeline_config& aConfig, std::function<void(compute_pipeline_t&)>& aFunc, std::function<void(compute_pipeline_t&)> aAlterConfigBeforeCreation, Ts... args)
	{
		aFunc = std::move(aAlterConfigBeforeCreation);
		add_config(aConfig, aFunc, std::move(args)...);
	}
}
