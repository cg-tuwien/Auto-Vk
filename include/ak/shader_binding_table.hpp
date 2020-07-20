#pragma once
#include <ak/ak.hpp>

namespace ak 
{
	struct shader_group_info
	{
		/** Number of shader records in this group */
		size_t mNumEntries;
		/** Entry-offset (not byte-offset) within the Shader Binding Table to the start of this group */
		vk::DeviceSize mOffset;
		/** Byte-offset (not entry-offset) within the Shader Binding Table to the start of this group */
		vk::DeviceSize mByteOffset;
	};
	
	struct shader_binding_table_groups_info
	{
		std::vector<shader_group_info> mRaygenGroupsInfo;
		std::vector<shader_group_info> mMissGroupsInfo;
		std::vector<shader_group_info> mHitGroupsInfo;
		std::vector<shader_group_info> mCallableGroupsInfo;
		vk::DeviceSize mEndOffset;
		vk::DeviceSize mTotalSize;
	};

	struct shader_binding_table_ref
	{
		vk::Buffer mSbtBufferHandle;
		vk::DeviceSize mSbtEntrySize;
		std::reference_wrapper<const shader_binding_table_groups_info> mSbtGroupsInfo;
		vk::DispatchLoaderDynamic mDynamicDispatch;
	};
	
}
