#pragma once

namespace ak
{
	class bottom_level_acceleration_structure_t;

	struct geometry_instance
	{
		/** Create a geometry instance for a specific geometry, which is represented by a bottom level acceleration structure.
		 *	@param	_Blas	The bottom level acceleration structure which represents the underlying geometry for this instance
		 */
		geometry_instance(const bottom_level_acceleration_structure_t& aBlas);

		/** Set the transformation matrix of this geometry instance. */
		geometry_instance& set_transform(VkTransformMatrixKHR aTransformationMatrix);
		/** Set the transformation matrix of this geometry instance. */
		geometry_instance& set_transform(std::array<float, 12> aTransformationMatrix);
		/** Set the transformation matrix of this geometry instance. */
		geometry_instance& set_transform(std::array<float, 16> aTransformationMatrix);
		/** Set the custom index assigned to this geometry instance. */
		geometry_instance& set_custom_index(uint32_t aCustomIndex);
		/** Set the mask for this geometry instance. */
		geometry_instance& set_mask(uint32_t aMask);
		/** Set the instance offset parameter assigned to this geometry instance. */
		geometry_instance& set_instance_offset(size_t aOffset);
		/** Set new flags, overwriting any previous value */
		geometry_instance& set_flags(vk::GeometryInstanceFlagsKHR aFlags);
		/** Add the given flags to this instance. */
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

}