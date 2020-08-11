#pragma once
#include <avk/avk.hpp>

namespace avk
{
#if VK_HEADER_VERSION >= 135
	class bottom_level_acceleration_structure_t;

	struct geometry_instance
	{
		/** Set the transformation matrix of this geometry instance. */
		geometry_instance& set_transform(VkTransformMatrixKHR aTransformationMatrix);
		/** Set the transformation matrix of this geometry instance. */
		geometry_instance& set_transform_row_major(float aTransformationMatrix[3][4]);
		/** Set the transformation matrix of this geometry instance. */
		geometry_instance& set_transform_row_major(std::array<float, 16> aTransformationMatrix);
		/** Set the transformation matrix of this geometry instance. */
		geometry_instance& set_transform_column_major(std::array<float, 16> aTransformationMatrix);
		/** Set the custom index assigned to this geometry instance. */
		geometry_instance& set_custom_index(uint32_t _CustomIndex);
		/** Set the mask for this geometry instance. */
		geometry_instance& set_mask(uint32_t _Mask);
		/** Set the instance offset parameter assigned to this geometry instance. */
		geometry_instance& set_instance_offset(size_t _Offset);
		/** Set the given flag(s) to this instance, overwriting any previous flags. */
		geometry_instance& set_flags(vk::GeometryInstanceFlagsKHR aFlags);
		/** Add the given flag(s) to this instance, that is, logically-or them to the existing value. */
		geometry_instance& add_flags(vk::GeometryInstanceFlagsKHR aFlags);
		/** Add the flag to disable triangle culling for this geometry instance. */
		geometry_instance& disable_culling();
		/** Add the flag which indicates that triangle front faces are given in counter-clockwise order. */
		geometry_instance& define_front_faces_to_be_counter_clockwise();
		/** Add a flag to force this geometry instance to be opaque. */
		geometry_instance& force_opaque();
		/** Add a flag to force this geometry instance to be treated as being non-opaque. */
		geometry_instance& force_non_opaque();
		/** Reset all the flag values. After calling this method, this geometry instance is in a state without any flags set. */
		geometry_instance& reset_flags();

		VkTransformMatrixKHR mTransform;
		uint32_t mInstanceCustomIndex;
		uint32_t mMask;
		size_t mInstanceOffset;
		vk::GeometryInstanceFlagsKHR mFlags;
		vk::DeviceAddress mAccelerationStructureDeviceHandle;
	};
#endif
}