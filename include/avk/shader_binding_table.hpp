#pragma once
#include <avk/avk.hpp>

namespace avk 
{
	/** Detail information about one specific group within a shader binding table (SBT)
	 *	See @ref shader_binding_table_groups_info for more details.
	 */
	struct shader_group_info
	{
		/** Number of shader records in this group */
		size_t mNumEntries;
		/** Entry-offset (not byte-offset) within the Shader Binding Table to the start of this group */
		vk::DeviceSize mOffset;
		/** Byte-offset (not entry-offset) within the Shader Binding Table to the start of this group */
		vk::DeviceSize mByteOffset;
	};

	/**	Struct that hold information about the different groups that can occur in shader binding tables (SBT):
	 *   - groups of ray generation shaders
	 *   - groups of miss shaders
	 *   - groups of hit groups (triangle hit groups and procedural hit groups may be combined)
	 *   - groups of callable shaders
	 */
	struct shader_binding_table_groups_info
	{
		/** All the groups of ray generation shaders */
		std::vector<shader_group_info> mRaygenGroupsInfo;
		/** All the groups of miss shaders */
		std::vector<shader_group_info> mMissGroupsInfo;
		/** All the groups of hit groups (might be a combination of triangle hit groups and procedural hit groups) */
		std::vector<shader_group_info> mHitGroupsInfo;
		/** All the groups of callable shaders */
		std::vector<shader_group_info> mCallableGroupsInfo;
		/** One after the last entry-offset (not a byte-offset!) */
		vk::DeviceSize mEndOffset;
		/** Total size of this SBT in bytes */
		vk::DeviceSize mTotalSize;
	};

	/**	A (temporary) object that is used to pass references to a shader binding table (SBT)
	 *	around. Most notably, this is used when passing SBT-information to a trace rays call,
	 *	such as `command_buffer_t::trace_rays`.
	 */
	struct shader_binding_table_ref
	{
		vk::Buffer mSbtBufferHandle;
		vk::DeviceSize mSbtEntrySize;
		std::reference_wrapper<const shader_binding_table_groups_info> mSbtGroupsInfo;
		vk::DispatchLoaderDynamic mDynamicDispatch;
	};
	
}
