#pragma once
#include <avk/avk.hpp>

namespace avk
{
	/** A descriptor pool which can allocate storage for descriptor sets from descriptor layouts.
	 */
	class descriptor_pool
	{
		friend class root;
		friend class descriptor_set;
	public:
		descriptor_pool() = default;
		descriptor_pool(descriptor_pool&&) noexcept = default;
		descriptor_pool& operator=(const descriptor_pool&) = delete;
		descriptor_pool& operator=(descriptor_pool&&) noexcept = default;
		~descriptor_pool() = default;

		const auto& handle() const { return mDescriptorPool.get(); }

		bool has_capacity_for(const descriptor_alloc_request& pRequest) const;
		const auto& initial_capacities() const { return mInitialCapacities; }
		const auto& remaining_capacities() const { return mRemainingCapacities; }
		void set_remaining_capacities(std::vector<vk::DescriptorPoolSize> aCapacitiesOverride) { mRemainingCapacities = aCapacitiesOverride; }
		auto initial_sets() const { return mNumInitialSets; }
		auto remaining_sets() const { return mNumRemainingSets; }
		void set_remaining_sets(int aRemainingSetsOverride) { mNumRemainingSets = aRemainingSetsOverride; }
		
		std::vector<vk::DescriptorSet> allocate(const std::vector<std::reference_wrapper<const descriptor_set_layout>>& aLayouts);
		
	private:
		vk::UniqueDescriptorPool mDescriptorPool;
		std::vector<vk::DescriptorPoolSize> mInitialCapacities;
		std::vector<vk::DescriptorPoolSize> mRemainingCapacities;
		int mNumInitialSets;
		int mNumRemainingSets;
	};
}
