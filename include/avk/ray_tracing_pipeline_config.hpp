#pragma once
#include <avk/avk.hpp>

namespace avk
{
#if VK_HEADER_VERSION >= 135
	/** Contains shader infos about a Hit Group which uses the
	 *	ray tracing's standard triangles intersection functionality.
	 */
	struct triangles_hit_group
	{
		static triangles_hit_group create_with_rahit_only(shader_info aAnyHitShader);
		static triangles_hit_group create_with_rchit_only(shader_info aClosestHitShader);
		static triangles_hit_group create_with_rahit_and_rchit(shader_info aAnyHitShader, shader_info aClosestHitShader);
		static triangles_hit_group create_with_rahit_only(std::string aAnyHitShaderPath);
		static triangles_hit_group create_with_rchit_only(std::string aClosestHitShaderPath);
		static triangles_hit_group create_with_rahit_and_rchit(std::string aAnyHitShaderPath, std::string aClosestHitShaderPath);

		std::optional<shader_info> mAnyHitShader;
		std::optional<shader_info> mClosestHitShader;
	};

	/** Contains shader infos about a Hit Group which uses a custom,
	 *	"procedural" intersection shader. Hence, the `mIntersectionShader` 
	 *	member must be set while the other members are optional.
	 */
	struct procedural_hit_group
	{
		static procedural_hit_group create_with_rint_only(shader_info aIntersectionShader);
		static procedural_hit_group create_with_rint_and_rahit(shader_info aIntersectionShader, shader_info aAnyHitShader);
		static procedural_hit_group create_with_rint_and_rchit(shader_info aIntersectionShader, shader_info aClosestHitShader);
		static procedural_hit_group create_with_rint_and_rahit_and_rchit(shader_info aIntersectionShader, shader_info aAnyHitShader, shader_info aClosestHitShader);
		static procedural_hit_group create_with_rint_only(std::string aIntersectionShader);
		static procedural_hit_group create_with_rint_and_rahit(std::string aIntersectionShader, std::string aAnyHitShader);
		static procedural_hit_group create_with_rint_and_rchit(std::string aIntersectionShader, std::string aClosestHitShader);
		static procedural_hit_group create_with_rint_and_rahit_and_rchit(std::string aIntersectionShader, std::string aAnyHitShader, std::string aClosestHitShader);

		shader_info mIntersectionShader;
		std::optional<shader_info> mAnyHitShader;
		std::optional<shader_info> mClosestHitShader;
	};

	/**	Represents one entry of the "shader table" which is used with
	 *	ray tracing pipelines. The "shader table" is also commonly known
	 *	as "shader groups". 
	 *	Every "shader table entry" represents either a single shader,
	 *	or one of the two types of "hit groups", namely `triangles_hit_group`,
	 *	or `procedural_hit_group`.
	 */
	using shader_table_entry_config = std::variant<shader_info, triangles_hit_group, procedural_hit_group>;
	
	/** Represents the entire "shader table" config, which consists
	 *	of multiple "shader table entries", each one represented by
	 *	an element of type `ak::shader_table_entry_config`.
	 *	Depending on specific usages of the shader table, the order 
	 *	of the shader table entries matters and hence, is retained 
	 *	like configured in the vector of shader table entries: `mShaderTableEntries`.
	 */
	struct shader_table_config
	{
		std::vector<shader_table_entry_config> mShaderTableEntries;
	};

	/** Represents the maximum recursion depth supported by a ray tracing pipeline. */
	struct max_recursion_depth
	{
		/** Disable recursions, i.e. set to zero. */
		static max_recursion_depth disable_recursion();
		/** Set the maximum recursion depth to a specific value. */
		static max_recursion_depth set_to(uint32_t aValue);

		uint32_t mMaxRecursionDepth;
	};

	/** Pipeline configuration data: COMPUTE PIPELINE CONFIG STRUCT */
	struct ray_tracing_pipeline_config
	{
		ray_tracing_pipeline_config();
		ray_tracing_pipeline_config(ray_tracing_pipeline_config&&) noexcept = default;
		ray_tracing_pipeline_config(const ray_tracing_pipeline_config&) = delete;
		ray_tracing_pipeline_config& operator=(ray_tracing_pipeline_config&&) noexcept = default;
		ray_tracing_pipeline_config& operator=(const ray_tracing_pipeline_config&) = delete;
		~ray_tracing_pipeline_config() = default;

		cfg::pipeline_settings mPipelineSettings; // ?
		shader_table_config mShaderTableConfig;
		max_recursion_depth mMaxRecursionDepth;
		std::vector<binding_data> mResourceBindings;
		std::vector<push_constant_binding_data> mPushConstantsBindings;
	};

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
		aShaderTableConfig.mShaderTableEntries.push_back(shader_info::describe(std::string(aShaderPath)));
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

	/**	Define a shader table which is to be used with a ray tracing pipeline
	 *
	 *	To define a ray generation shader entry, use:
	 *	 - shader_info
	 *	 - std::string_view (path to shader)
	 *
	 *	To define a miss shader entry, use:
	 *	 - shader_info
	 *	 - std::string_view (path to shader)
	 *
	 *	To define a callable shader entry, use:
	 *	 - shader_info
	 *	 - std::string_view (path to shader)
	 *
	 *	To define a triangles hit group, use:
	 *	 - triangles_hit_group
	 *
	 *	To define a procedural hit group, use:
	 *	 - procedural_hit_group
	 */
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
#pragma endregion
#endif
}
