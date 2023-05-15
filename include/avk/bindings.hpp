#pragma once
#include "avk/avk.hpp"
#include <type_traits>

namespace avk
{
	template <typename T>
	vk::DescriptorType descriptor_type_of(const T*);

	template<>
	inline vk::DescriptorType descriptor_type_of<buffer_t>(const buffer_t* aBuffer) { return aBuffer->meta_at_index<buffer_meta>(0).descriptor_type().value(); }
	template<>
	inline vk::DescriptorType descriptor_type_of<buffer>(const buffer* aBuffer) { return (*aBuffer)->meta_at_index<buffer_meta>(0).descriptor_type().value(); }

	template<>
	inline vk::DescriptorType descriptor_type_of<buffer_descriptor>(const buffer_descriptor* aBufferDescriptor) { return aBufferDescriptor->descriptor_type(); }

	template<>
	inline vk::DescriptorType descriptor_type_of<buffer_view_t>(const buffer_view_t* aBufferView) { return aBufferView->descriptor_type(); }
	template<>
	inline vk::DescriptorType descriptor_type_of<buffer_view>(const buffer_view* aBufferView) { return (*aBufferView)->descriptor_type(); }

	template<>
	inline vk::DescriptorType descriptor_type_of<buffer_view_descriptor_info>(const buffer_view_descriptor_info* aBufferViewDescriptor) { return aBufferViewDescriptor->descriptor_type(); }

#if VK_HEADER_VERSION >= 135
	template<>
	inline vk::DescriptorType descriptor_type_of<top_level_acceleration_structure_t>(const top_level_acceleration_structure_t*) { return vk::DescriptorType::eAccelerationStructureKHR; }
	template<>
	inline vk::DescriptorType descriptor_type_of<top_level_acceleration_structure>(const top_level_acceleration_structure*) { return vk::DescriptorType::eAccelerationStructureKHR; }
#endif
	

	template<>
	inline vk::DescriptorType descriptor_type_of<avk::image_view_as_sampled_image>(const avk::image_view_as_sampled_image*) { return vk::DescriptorType::eSampledImage; }

	template<>
	inline vk::DescriptorType descriptor_type_of<avk::image_view_as_input_attachment>(const avk::image_view_as_input_attachment*) { return vk::DescriptorType::eInputAttachment; }

	template<>
	inline vk::DescriptorType descriptor_type_of<avk::image_view_as_storage_image>(const avk::image_view_as_storage_image*) { return vk::DescriptorType::eStorageImage; }

	template<>
	inline vk::DescriptorType descriptor_type_of<avk::sampler_t>(const avk::sampler_t*) { return vk::DescriptorType::eSampler; }
	template<>
	inline vk::DescriptorType descriptor_type_of<avk::sampler>(const avk::sampler*) { return vk::DescriptorType::eSampler; }

	template<>
	inline vk::DescriptorType descriptor_type_of<avk::combined_image_sampler_descriptor_info>(const avk::combined_image_sampler_descriptor_info*) { return vk::DescriptorType::eCombinedImageSampler; }
	
	template<typename T> 
	typename std::enable_if<
		avk::has_size_and_iterators<T>::value && avk::has_nested_value_types<T>::value,
		std::vector<const typename T::value_type::value_type*>
	>::type gather_one_or_multiple_element_pointers(const T& t) {
		std::vector<const typename T::value_type::value_type*> results;
		for (size_t i = 0; i < t.size(); ++i) {
			results.emplace_back(&(*t[i]));
		}
		return results;
	}
	
	template<typename T> 
	typename std::enable_if<
		avk::has_size_and_iterators<T>::value && !avk::has_nested_value_types<T>::value,
		std::vector<const typename T::value_type*>
	>::type gather_one_or_multiple_element_pointers(const T& t) {
		std::vector<const typename T::value_type*> results;
		for (size_t i = 0; i < t.size(); ++i) {
			results.emplace_back(&t[i]);
		}
		return results;
	}

	template<typename T> 
	typename std::enable_if<
		!avk::has_size_and_iterators<T>::value && avk::is_dereferenceable<T>::value,
		const typename T::value_type*
	>::type gather_one_or_multiple_element_pointers(const T& t) {
		return &*t;
	}

	template<typename T> 
	typename std::enable_if<!avk::has_size_and_iterators<T>::value && !avk::is_dereferenceable<T>::value, const T*>::type gather_one_or_multiple_element_pointers(const T& t) {
		return &t;
	}

	template <typename T>
	binding_data descriptor_binding(uint32_t aSet, uint32_t aBinding, const T& aResource, shader_type aShaderStages = shader_type::all)
	{
		binding_data data{
			aSet,
			vk::DescriptorSetLayoutBinding{}
				.setBinding(aBinding)
				.setDescriptorCount(how_many_elements(aResource))
				.setDescriptorType(descriptor_type_of(&first_or_only_element(aResource)))
				.setStageFlags(to_vk_shader_stages(aShaderStages))
				.setPImmutableSamplers(nullptr), // The pImmutableSamplers field is only relevant for image sampling related descriptors [3]
			gather_one_or_multiple_element_pointers(aResource)
		};
		return data;
	}

	template <typename T>
	binding_data descriptor_binding(uint32_t aSet, uint32_t aBinding, uint32_t aCount, shader_type aShaderStages = shader_type::all)
	{
		assert(aCount > 0u);
		binding_data data{
			aSet,
			vk::DescriptorSetLayoutBinding{}
				.setBinding(aBinding)
				.setDescriptorCount(aCount)
				.setDescriptorType(descriptor_type_of<T>(nullptr))
				.setStageFlags(to_vk_shader_stages(aShaderStages))
				.setPImmutableSamplers(nullptr), // The pImmutableSamplers field is only relevant for image sampling related descriptors [3]
		};
		return data;
	}

	template <typename T>
	typename std::enable_if<avk::has_size_and_iterators<T>::value, std::vector<buffer_descriptor>>::type as_uniform_buffers(const T& aCollection)
	{
		std::vector<buffer_descriptor> results;
		for (size_t i = 0; i < aCollection.size(); ++i) {
			results.emplace_back(aCollection[i]->as_uniform_buffer());
		}
		return results;
	}

	template <typename T>
	typename std::enable_if<avk::has_size_and_iterators<T>::value, std::vector<buffer_view_descriptor_info>>::type as_uniform_texel_buffer_views(const T& aCollection)
	{
		std::vector<buffer_view_descriptor_info> results;
		for (size_t i = 0; i < aCollection.size(); ++i) {
			results.emplace_back(aCollection[i]->as_uniform_texel_buffer_view());
		}
		return results;
	}

	template <typename T>
	typename std::enable_if<avk::has_size_and_iterators<T>::value, std::vector<buffer_descriptor>>::type as_storage_buffers(const T& aCollection)
	{
		std::vector<buffer_descriptor> results;
		for (size_t i = 0; i < aCollection.size(); ++i) {
			results.emplace_back(aCollection[i]->as_storage_buffer());
		}
		return results;
	}

	template <typename T>
	typename std::enable_if<avk::has_size_and_iterators<T>::value, std::vector<buffer_view_descriptor_info>>::type as_storage_texel_buffer_views(const T& aCollection)
	{
		std::vector<buffer_view_descriptor_info> results;
		for (size_t i = 0; i < aCollection.size(); ++i) {
			results.emplace_back(aCollection[i]->as_storage_texel_buffer_view());
		}
		return results;
	}

	template <typename T>
	typename std::enable_if<avk::has_size_and_iterators<T>::value, std::vector<image_view_as_sampled_image>>::type as_sampled_images(const T& aCollection, avk::layout::image_layout aImageLayout)
	{
		std::vector<image_view_as_sampled_image> results;
		for (size_t i = 0; i < aCollection.size(); ++i) {
			results.emplace_back(aCollection[i]->as_sampled_image(aImageLayout));
		}
		return results;
	}

	template <typename T>
	typename std::enable_if<avk::has_size_and_iterators<T>::value, std::vector<image_view_as_input_attachment>>::type as_input_attachments(const T& aCollection, avk::layout::image_layout aImageLayout)
	{
		std::vector<image_view_as_input_attachment> results;
		for (size_t i = 0; i < aCollection.size(); ++i) {
			results.emplace_back(aCollection[i]->as_input_attachment(aImageLayout));
		}
		return results;
	}

	template <typename T>
	typename std::enable_if<avk::has_size_and_iterators<T>::value, std::vector<image_view_as_storage_image>>::type as_storage_images(const T& aCollection, avk::layout::image_layout aImageLayout)
	{
		std::vector<image_view_as_storage_image> results;
		for (size_t i = 0; i < aCollection.size(); ++i) {
			results.emplace_back(aCollection[i]->as_storage_image(aImageLayout));
		}
		return results;
	}

	template <typename T>
	typename std::enable_if<avk::has_size_and_iterators<T>::value, std::vector<combined_image_sampler_descriptor_info>>::type as_combined_image_samplers(const T& aCollection, avk::layout::image_layout aImageLayout)
	{
		std::vector<combined_image_sampler_descriptor_info> results;
		for (size_t i = 0; i < aCollection.size(); ++i) {
			results.emplace_back(aCollection[i]->as_combined_image_sampler(aImageLayout));
		}
		return results;
	}

}
