#pragma once
#include <avk/avk.hpp>

namespace avk
{
	struct specialization_constants
	{
		uint32_t num_entries() const { return static_cast<uint32_t>(mMapEntries.size()); }
		size_t data_size() const { return mData.size() * sizeof(decltype(mData)::value_type); }
		
		std::vector<vk::SpecializationMapEntry> mMapEntries;
		std::vector<uint8_t> mData;
	};

	static bool operator ==(const specialization_constants& left, const specialization_constants& right)
	{
		if (left.mMapEntries.size() != right.mMapEntries.size())				{ return false; }
		for (size_t i = 0; i < left.mMapEntries.size(); ++i) {
			if (left.mMapEntries[i] != right.mMapEntries[i])					{ return false; }
		}
		if (left.mData.size() != right.mData.size())							{ return false; }
		for (size_t i = 0; i < left.mData.size(); ++i) {
			if (left.mData[i] != right.mData[i])								{ return false; }
		}
		return true;
	}

	static bool operator !=(const specialization_constants& left, const specialization_constants& right)
	{
		return !(left == right);
	}

	
	struct shader_info
	{
		static shader_info describe(std::string pPath, std::string pEntryPoint = "main", bool pDontMonitorFile = false, std::optional<avk::shader_type> pShaderType = {});

		/**	Add one specialization constant to this shader.
		 *	@param	aConstantId		The ID that is used to identify this constant
		 *	@param	aValue			The value of this constant. Pay attention to specify the correct type!
		 *							Example usages:
		 *							 - set_specialization_constant(1u, uint32_t{123})
		 *							 - set_specialization_constant(50u, float{1.1f})
		 *							 - set_specialization_constant(2u, int64_t{456})
		 */
		template <typename T>
		shader_info& set_specialization_constant(uint32_t aConstantId, const T& aValue)
		{
			if (!mSpecializationConstants.has_value()) {
				mSpecializationConstants = specialization_constants{};
			}
			auto& entries = mSpecializationConstants->mMapEntries;
			auto& data = mSpecializationConstants->mData;

			const auto align = sizeof(uint64_t); 
			const auto currentSize = data.size();
			const auto insertSize = sizeof(T);
			const auto numAlloc = std::max(align, insertSize / sizeof(typename decltype(mSpecializationConstants->mData)::value_type));
			assert (insertSize % sizeof(typename decltype(mSpecializationConstants->mData)::value_type) == 0);
			data.resize(currentSize + numAlloc);
			// copy data:
			memcpy(data.data() + currentSize, &aValue, insertSize);
			// make entry:
			entries.emplace_back(aConstantId, static_cast<uint32_t>(currentSize), insertSize);

			return *this;
		}
		
		inline bool direct_spirv() const {
			return mSpVCode.has_value();
		}

		// There are two types available
		// 1. Direct SPIR-V input: Valid fields are mSpVCode, mShaderType, mEntryPoint
		// 2. File SPIR-V input: Valid fields are mPath, mShaderType, mEntryPoint, mDontMonitorFile

		std::optional<std::vector<char>> mSpVCode;
		std::string mPath;
		avk::shader_type mShaderType;
		std::string mEntryPoint;
		bool mDontMonitorFile;

		std::optional<specialization_constants> mSpecializationConstants;
	};

	static bool operator ==(const shader_info& left, const shader_info& right)
	{
		if (left.direct_spirv() != right.direct_spirv()) {
			return false;
		}
		else if (left.direct_spirv()
			&& left.mShaderType == right.mShaderType
			&& trim_spaces(left.mEntryPoint) == trim_spaces(right.mEntryPoint)
			&& left.mSpecializationConstants == right.mSpecializationConstants
			&& left.mSpVCode == right.mSpVCode
			) {
			return true;
		}
		else {
			return !left.direct_spirv()
				&& are_paths_equal(left.mPath, right.mPath)
				&& left.mShaderType == right.mShaderType
				&& trim_spaces(left.mEntryPoint) == trim_spaces(right.mEntryPoint)
				&& left.mSpecializationConstants == right.mSpecializationConstants;
		}

	}

	static bool operator !=(const shader_info& left, const shader_info& right)
	{
		return !(left == right);
	}

	/** @brief Shader source information and shader loading options
	 *
	 *	This information is important especially for shader hot reloading.
	 */
	enum struct shader_source_info : uint32_t
	{
		nothing			= 0x0000,
		/** Shader source is loaded from a file */
		from_file		= 0x0001,
		/** Shader source is loaded from memory (a string most likely) */
		from_memory		= 0x0002,
		/** Load the shader and append a new-line to the source */
		append_newline	= 0x0004,
	};

	inline shader_source_info operator| (shader_source_info a, shader_source_info b)
	{
		typedef std::underlying_type<shader_source_info>::type EnumType;
		return static_cast<shader_source_info>(static_cast<EnumType>(a) | static_cast<EnumType>(b));
	}

	inline shader_source_info operator& (shader_source_info a, shader_source_info b)
	{
		typedef std::underlying_type<shader_source_info>::type EnumType;
		return static_cast<shader_source_info>(static_cast<EnumType>(a) & static_cast<EnumType>(b));
	}

	inline shader_source_info& operator |= (shader_source_info& a, shader_source_info b)
	{
		return a = a | b;
	}

	inline shader_source_info& operator &= (shader_source_info& a, shader_source_info b)
	{
		return a = a & b;
	}

	inline shader_info vertex_shader(std::string pPath, std::string pEntryPoint = "main", bool pDontMonitorFile = false) { return shader_info::describe(std::move(pPath), std::move(pEntryPoint), pDontMonitorFile, avk::shader_type::vertex); }
	inline shader_info tessellation_control_shader(std::string pPath, std::string pEntryPoint = "main", bool pDontMonitorFile = false) { return shader_info::describe(std::move(pPath), std::move(pEntryPoint), pDontMonitorFile, avk::shader_type::tessellation_control); }
	inline shader_info tessellation_evaluation_shader(std::string pPath, std::string pEntryPoint = "main", bool pDontMonitorFile = false) { return shader_info::describe(std::move(pPath), std::move(pEntryPoint), pDontMonitorFile, avk::shader_type::tessellation_evaluation); }
	inline shader_info geometry_shader(std::string pPath, std::string pEntryPoint = "main", bool pDontMonitorFile = false) { return shader_info::describe(std::move(pPath), std::move(pEntryPoint), pDontMonitorFile, avk::shader_type::geometry); }
	inline shader_info fragment_shader(std::string pPath, std::string pEntryPoint = "main", bool pDontMonitorFile = false) { return shader_info::describe(std::move(pPath), std::move(pEntryPoint), pDontMonitorFile, avk::shader_type::fragment); }
	inline shader_info compute_shader(std::string pPath, std::string pEntryPoint = "main", bool pDontMonitorFile = false) { return shader_info::describe(std::move(pPath), std::move(pEntryPoint), pDontMonitorFile, avk::shader_type::compute); }
	inline shader_info ray_generation_shader(std::string pPath, std::string pEntryPoint = "main", bool pDontMonitorFile = false) { return shader_info::describe(std::move(pPath), std::move(pEntryPoint), pDontMonitorFile, avk::shader_type::ray_generation); }
	inline shader_info any_hit_shader(std::string pPath, std::string pEntryPoint = "main", bool pDontMonitorFile = false) { return shader_info::describe(std::move(pPath), std::move(pEntryPoint), pDontMonitorFile, avk::shader_type::any_hit); }
	inline shader_info closest_hit_shader(std::string pPath, std::string pEntryPoint = "main", bool pDontMonitorFile = false) { return shader_info::describe(std::move(pPath), std::move(pEntryPoint), pDontMonitorFile, avk::shader_type::closest_hit); }
	inline shader_info miss_shader(std::string pPath, std::string pEntryPoint = "main", bool pDontMonitorFile = false) { return shader_info::describe(std::move(pPath), std::move(pEntryPoint), pDontMonitorFile, avk::shader_type::miss); }
	inline shader_info intersection_shader(std::string pPath, std::string pEntryPoint = "main", bool pDontMonitorFile = false) { return shader_info::describe(std::move(pPath), std::move(pEntryPoint), pDontMonitorFile, avk::shader_type::intersection); }
	inline shader_info callable_shader(std::string pPath, std::string pEntryPoint = "main", bool pDontMonitorFile = false) { return shader_info::describe(std::move(pPath), std::move(pEntryPoint), pDontMonitorFile, avk::shader_type::callable); }
	inline shader_info task_shader(std::string pPath, std::string pEntryPoint = "main", bool pDontMonitorFile = false) { return shader_info::describe(std::move(pPath), std::move(pEntryPoint), pDontMonitorFile, avk::shader_type::task); }
	inline shader_info mesh_shader(std::string pPath, std::string pEntryPoint = "main", bool pDontMonitorFile = false) { return shader_info::describe(std::move(pPath), std::move(pEntryPoint), pDontMonitorFile, avk::shader_type::mesh); }

}

namespace std // Inject hash for `ak::shader_info` into std::
{
	template<> struct hash<avk::shader_info>
	{
		std::size_t operator()(avk::shader_info const& o) const noexcept
		{
			std::size_t h = 0;
			avk::hash_combine(h,
				avk::transform_path_for_comparison(o.mPath),
				static_cast<std::underlying_type<avk::shader_type>::type>(o.mShaderType),
				avk::trim_spaces(o.mEntryPoint)
			);
			return h;
		}
	};
}
