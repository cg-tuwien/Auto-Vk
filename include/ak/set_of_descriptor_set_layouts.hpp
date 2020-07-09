#pragma once
#include <ak/ak.hpp>

namespace ak
{
	/** Basically a vector of descriptor_set_layout instances */
	class set_of_descriptor_set_layouts
	{
		friend class root;
		
	public:
		set_of_descriptor_set_layouts() = default;
		set_of_descriptor_set_layouts(const set_of_descriptor_set_layouts&) = delete;
		set_of_descriptor_set_layouts(set_of_descriptor_set_layouts&&) noexcept = default;
		set_of_descriptor_set_layouts& operator=(const set_of_descriptor_set_layouts&) = delete;
		set_of_descriptor_set_layouts& operator=(set_of_descriptor_set_layouts&&) noexcept = default;
		~set_of_descriptor_set_layouts() = default;

		uint32_t number_of_sets() const { return static_cast<uint32_t>(mLayouts.size()); }
		const auto& set_at(uint32_t pIndex) const { return mLayouts[pIndex]; }
		auto& set_at(uint32_t pIndex) { return mLayouts[pIndex]; }
		const auto& all_sets() const { return mLayouts; }
		uint32_t set_index_for_set_id(uint32_t pSetId) const { return pSetId - mFirstSetId; }
		const auto& set_for_set_id(uint32_t pSetId) const { return set_at(set_index_for_set_id(pSetId)); }
		const auto& required_pool_sizes() const { return mBindingRequirements; }
		std::vector<vk::DescriptorSetLayout> layout_handles() const;

		static set_of_descriptor_set_layouts prepare(std::vector<binding_data> pBindings);
		
	private:
		std::vector<vk::DescriptorPoolSize> mBindingRequirements;
		uint32_t mFirstSetId; // Set-Id of the first set, all the following sets have consecutive ids
		std::vector<descriptor_set_layout> mLayouts;
	};	
}
