#pragma once
#include <ak/ak.hpp>

namespace ak
{
	class descriptor_set_layout;
	
	/** Data about an allocation which is to be passed to a descriptor pool 
	 *	Actually this is only a helper class.
	 */
	class descriptor_alloc_request
	{
	public:
		descriptor_alloc_request(const std::vector<std::reference_wrapper<const descriptor_set_layout>>& aLayouts);
		descriptor_alloc_request(descriptor_alloc_request&&) noexcept = default;
		descriptor_alloc_request(const descriptor_alloc_request&) = default;
		descriptor_alloc_request& operator=(descriptor_alloc_request&&) noexcept = default;
		descriptor_alloc_request& operator=(const descriptor_alloc_request&) = default;
		~descriptor_alloc_request() = default;

		void add_size_requirements(vk::DescriptorPoolSize aToAdd);
		const auto& accumulated_pool_sizes() const { return mAccumulatedSizes; }
		void set_num_sets(uint32_t aNumSets) { mNumSets = aNumSets; }
		auto num_sets() const { return mNumSets; }

		descriptor_alloc_request multiply_size_requirements(uint32_t mFactor) const;

	private:
		std::vector<vk::DescriptorPoolSize> mAccumulatedSizes;
		uint32_t mNumSets;
	};	
}
