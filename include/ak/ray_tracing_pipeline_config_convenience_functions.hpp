#pragma once

namespace ak
{
#pragma region shader_table_config convenience functions
	// End of recursive variadic template handling
	inline void add_shader_table_entry(shader_table_config& aShaderTableConfig) { /* We're done here. */ }

	// Add a single shader (without hit group) to the shader table definition
	template <typename... Ts>
	void add_shader_table_entry(shader_table_config& aShaderTableConfig, shader_info aShaderInfo, Ts... args)
	{
		aShaderTableConfig.mShaderTableEntries.push_back(std::move(aShaderInfo));
		add_shader_table_entry(aShaderTableConfig, std::move(args)...);
	}

	// Add a single shader (without hit group) to the shader table definition
	template <typename... Ts>
	void add_shader_table_entry(shader_table_config& aShaderTableConfig, std::string_view aShaderPath, Ts... args)
	{
		aShaderTableConfig.mShaderTableEntries.push_back(shader_info::create(std::string(aShaderPath)));
		add_shader_table_entry(aShaderTableConfig, std::move(args)...);
	}

	// Add a triangles-intersection-type hit group to the shader table definition
	template <typename... Ts>
	void add_shader_table_entry(shader_table_config& aShaderTableConfig, triangles_hit_group aHitGroup, Ts... args)
	{
		aShaderTableConfig.mShaderTableEntries.push_back(std::move(aHitGroup));
		add_shader_table_entry(aShaderTableConfig, std::move(args)...);
	}

	// Add a procedural-type hit group to the shader table definition
	template <typename... Ts>
	void add_shader_table_entry(shader_table_config& aShaderTableConfig, procedural_hit_group aHitGroup, Ts... args)
	{
		aShaderTableConfig.mShaderTableEntries.push_back(std::move(aHitGroup));
		add_shader_table_entry(aShaderTableConfig, std::move(args)...);
	}

	// Define a shader table which is to be used with a ray tracing pipeline
	template <typename... Ts>
	shader_table_config define_shader_table(Ts... args)
	{
		shader_table_config shaderTableConfig;
		add_shader_table_entry(shaderTableConfig, std::move(args)...);
		return shaderTableConfig;
	}
#pragma endregion

#pragma region ray_tracing_pipeline_config convenience functions
	// End of recursive variadic template handling
	inline void add_config(ray_tracing_pipeline_config& aConfig, std::function<void(ray_tracing_pipeline_t&)>& aFunc) { /* We're done here. */ }

	// Add a specific pipeline setting to the pipeline config
	template <typename... Ts>
	void add_config(ray_tracing_pipeline_config& aConfig, std::function<void(ray_tracing_pipeline_t&)>& aFunc, cfg::pipeline_settings aSetting, Ts... args)
	{
		aConfig.mPipelineSettings |= aSetting;
		add_config(aConfig, aFunc, std::move(args)...);
	}

	// Add the shader table to the pipeline config
	template <typename... Ts>
	void add_config(ray_tracing_pipeline_config& aConfig, std::function<void(ray_tracing_pipeline_t&)>& aFunc, shader_table_config aShaderTable, Ts... args)
	{
		aConfig.mShaderTableConfig = std::move(aShaderTable);
		add_config(aConfig, aFunc, std::move(args)...);
	}

	// Add the maximum recursion setting to the pipeline config
	template <typename... Ts>
	void add_config(ray_tracing_pipeline_config& aConfig, std::function<void(ray_tracing_pipeline_t&)>& aFunc, max_recursion_depth aMaxRecursionDepth, Ts... args)
	{
		aConfig.mMaxRecursionDepth = std::move(aMaxRecursionDepth);
		add_config(aConfig, aFunc, std::move(args)...);
	}

	// Add a resource binding to the pipeline config
	template <typename... Ts>
	void add_config(ray_tracing_pipeline_config& aConfig, std::function<void(ray_tracing_pipeline_t&)>& aFunc, binding_data aResourceBinding, Ts... args)
	{
		aConfig.mResourceBindings.push_back(std::move(aResourceBinding));
		add_config(aConfig, aFunc, std::move(args)...);
	}

	// Add a push constants binding to the pipeline config
	template <typename... Ts>
	void add_config(ray_tracing_pipeline_config& aConfig, std::function<void(ray_tracing_pipeline_t&)>& aFunc, push_constant_binding_data aPushConstBinding, Ts... args)
	{
		aConfig.mPushConstantsBindings.push_back(std::move(aPushConstBinding));
		add_config(aConfig, aFunc, std::move(args)...);
	}

	// Add an config-alteration function to the pipeline config
	template <typename... Ts>
	void add_config(ray_tracing_pipeline_config& aConfig, std::function<void(ray_tracing_pipeline_t&)>& aFunc, std::function<void(ray_tracing_pipeline_t&)> aAlterConfigBeforeCreation, Ts... args)
	{
		aFunc = std::move(aAlterConfigBeforeCreation);
		add_config(aConfig, aFunc, std::move(args)...);
	}

	/**	Convenience function for gathering the ray tracing pipeline's configuration.
	 *
	 *	It supports the following types:
	 *		- ak::cfg::pipeline_settings
	 *		- ak::shader_table_config (hint: use `ak::define_shader_table`)
	 *		- ak::max_recursion_depth
	 *		- ak::binding_data
	 *		- ak::push_constant_binding_data
	 *		- ak::std::function<void(ray_tracing_pipeline_t&)>
	 *
	 *	For building the shader table in a convenient fashion, use the `ak::define_shader_table` function!
	 *	
	 *	For the actual Vulkan-calls which finally create the pipeline, please refer to @ref ray_tracing_pipeline_t::create
	 */
	template <typename... Ts>
	ak::owning_resource<ray_tracing_pipeline_t> ray_tracing_pipeline_for(Ts... args)
	{
		// 1. GATHER CONFIG
		std::function<void(ray_tracing_pipeline_t&)> alterConfigFunction;
		ray_tracing_pipeline_config config;
		add_config(config, alterConfigFunction, std::move(args)...);

		// 2. CREATE PIPELINE according to the config
		// ============================================ Vk ============================================ 
		//    => VULKAN CODE HERE:
		return ray_tracing_pipeline_t::create(std::move(config), std::move(alterConfigFunction));
		// ============================================================================================ 
	}
#pragma endregion 
}
