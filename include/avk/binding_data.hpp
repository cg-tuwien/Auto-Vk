#pragma once
#include <avk/avk.hpp>

namespace avk
{
	class buffer_t;
	class buffer_descriptor;
	class buffer_view_t;
	class top_level_acceleration_structure_t;
	class image_view_t;
	class image_view_as_input_attachment;
	class image_view_as_storage_image;
	class sampler_t;
	class image_sampler_t;
	class descriptor_set_t;
	class descriptor_set;
	
	/** Configuration data for a binding, containing a set-index, binding data, 
	*	and the shader stages where the bound resource might be used.
	*/
	struct binding_data
	{
		uint32_t mSetId;
		vk::DescriptorSetLayoutBinding mLayoutBinding;
		std::variant<
			std::monostate,
			const buffer_t*,
			const buffer_descriptor*,
			const buffer_view_t*,
			const top_level_acceleration_structure_t*,
			const image_view_t*,
			const image_view_as_input_attachment*,
			const image_view_as_storage_image*,
			const sampler_t*,
			const image_sampler_t*,
			std::vector<const buffer_t*>,
			std::vector<const buffer_descriptor*>,
			std::vector<const buffer_view_t*>,
			std::vector<const top_level_acceleration_structure_t*>,
			std::vector<const image_view_t*>,
			std::vector<const image_view_as_input_attachment*>,
			std::vector<const image_view_as_storage_image*>,
			std::vector<const sampler_t*>,
			std::vector<const image_sampler_t*>
		> mResourcePtr;


		template <typename T>
		std::vector<vk::DescriptorImageInfo> gather_image_infos(const std::vector<T*>& vec) const
		{
			std::vector<vk::DescriptorImageInfo> dataForImageInfos;
			for (auto& v : vec) {
				dataForImageInfos.push_back(v->descriptor_info());
			}
			return dataForImageInfos;
		}

		template <typename T>
		std::vector<vk::DescriptorBufferInfo> gather_buffer_infos(const std::vector<T*>& vec) const
		{
			std::vector<vk::DescriptorBufferInfo> dataForBufferInfos;
			for (auto& v : vec) {
				dataForBufferInfos.push_back(v->descriptor_info());
			}
			return dataForBufferInfos;
		}

#if VK_HEADER_VERSION >= 135
		template <typename T>
		std::vector<vk::WriteDescriptorSetAccelerationStructureKHR> gather_acceleration_structure_infos(const std::vector<T*>& vec) const
		{
			std::vector<vk::WriteDescriptorSetAccelerationStructureKHR> dataForAccStructures;
			for (auto& v : vec) {
				dataForAccStructures.push_back(v->descriptor_info());
			}
			return dataForAccStructures;
		}
#endif

		template <typename T>
		std::vector<vk::BufferView> gather_buffer_views(const std::vector<T*>& vec) const
		{
			std::vector<vk::BufferView> dataForBufferViews;
			for (auto& v : vec) {
				dataForBufferViews.push_back(v->view_handle());
			}
			return dataForBufferViews;
		}

		uint32_t descriptor_count() const;

		const vk::DescriptorImageInfo* descriptor_image_info(descriptor_set& aDescriptorSet) const;

		const vk::DescriptorBufferInfo* descriptor_buffer_info(descriptor_set& aDescriptorSet) const;

		const void* next_pointer(descriptor_set& aDescriptorSet) const;

		const vk::BufferView* texel_buffer_view_info(descriptor_set& aDescriptorSet) const;
	};

	/** Compares two `binding_data` instances for equality, but only in
	*	in terms of their set-ids and binding-ids. 
	*	It does not consider equality or inequality of other members 
	*  (like of the `mLayoutBinding` or the `mShaderStages` members - they are simply ignored)
	*/
	inline bool operator ==(const binding_data& first, const binding_data& second)
	{
		return first.mSetId == second.mSetId
			&& first.mLayoutBinding.binding == second.mLayoutBinding.binding;
	}

	/** Compares two `binding_data` instances for inequality, but only in
	*	in terms of their set-ids and binding-ids. 
	*	It does not consider equality or inequality of other members 
	*  (like of the `mLayoutBinding` or the `mShaderStages` members - they are simply ignored)
	*/
	inline bool operator !=(const binding_data& first, const binding_data& second)
	{
		return !(first == second);
	}

	/** Returns true if the first binding is less than the second binding
	*	in terms of their set-ids and binding-ids. 
	*	It does not consider other members (like of the `mLayoutBinding` 
	*	or the `mShaderStages` members - they are simply ignored)
	*/
	inline bool operator <(const binding_data& first, const binding_data& second)
	{
		return	first.mSetId < second.mSetId
			|| (first.mSetId == second.mSetId && first.mLayoutBinding.binding < second.mLayoutBinding.binding);
	}

	/** Returns true if the first binding is less than or equal to the second binding
	*	in terms of their set-ids and binding-ids. 
	*	It does not consider other members (like of the `mLayoutBinding` 
	*	or the `mShaderStages` members - they are simply ignored)
	*/
	inline bool operator <=(const binding_data& first, const binding_data& second)
	{
		return first < second || first == second;
	}

	/** Returns true if the first binding is greater than the second binding
	*	in terms of their set-ids and binding-ids. 
	*	It does not consider other members (like of the `mLayoutBinding` 
	*	or the `mShaderStages` members - they are simply ignored)
	*/
	inline bool operator >(const binding_data& first, const binding_data& second)
	{
		return !(first <= second);
	}

	/** Returns true if the first binding is greater than or equal to the second binding
	*	in terms of their set-ids and binding-ids. 
	*	It does not consider other members (like of the `mLayoutBinding` 
	*	or the `mShaderStages` members - they are simply ignored)
	*/
	inline bool operator >=(const binding_data& first, const binding_data& second)
	{
		return !(first < second);
	}
}
