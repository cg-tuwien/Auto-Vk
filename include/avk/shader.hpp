#pragma once
#include <avk/avk.hpp>

namespace avk
{
	/** Represents a shader program handle for the Vulkan context */
	class shader
	{
		friend class root;
		
	public:
		shader() = default;
		shader(shader&&) noexcept = default;
		shader(const shader&) = delete;
		shader& operator=(shader&&) noexcept = default;
		shader& operator=(const shader&) = delete;
		~shader() = default;

		const auto& handle() const { return mShaderModule.get(); }
		const auto* handle_addr() const { return &mShaderModule.get(); }
		const auto& info() const { return mInfo; }
		const auto& actual_load_path() const { return mActualShaderLoadPath; }

		bool has_been_built() const;

		static shader prepare(shader_info pInfo);

	private:
		shader_info mInfo;
		vk::UniqueShaderModule mShaderModule;
		std::string mActualShaderLoadPath;
	};

}
