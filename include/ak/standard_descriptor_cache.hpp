#pragma once

namespace ak
{
	class standard_descriptor_cache : public descriptor_cache_interface
	{
		struct pool_id
		{
			std::thread::id mThreadId;
			int mName;
		};
		
	public:
		/** A factor which is applied to the pool size when a new pool is allocated.
		 *	Feel free to modify this factor (BEFORE allocating descriptor sets)!
		 */
		inline static int sDescriptorPoolPreallocFactor = 5;
		
		const descriptor_set_layout& get_or_alloc_layout(root& aRoot, descriptor_set_layout aPreparedLayout) override;
		std::optional<descriptor_set> get_descriptor_set_from_cache(const descriptor_set& aPreparedSet) override;
		std::vector<descriptor_set> alloc_new_descriptor_sets(root& aRoot, const std::vector<std::reference_wrapper<const descriptor_set_layout>>& aLayouts, std::vector<descriptor_set> aPreparedSets) override;
		void cleanup() override;
		
		std::shared_ptr<descriptor_pool> get_descriptor_pool_for_layouts(root& aRoot, const descriptor_alloc_request& aAllocRequest, int aPoolName = 0, bool aRequestNewPool = false);

	private:
		std::unordered_set<descriptor_set_layout> mLayouts;
		std::unordered_set<descriptor_set> mSets;
		
		// Descriptor pools are created/stored per thread and can have a name (an integer-id). 
		// If possible, it is tried to re-use a pool. Even when re-using a pool, it might happen that
		// allocating from it might fail (because out of memory, for instance). In such cases, a new 
		// pool will be created.
		std::unordered_map<pool_id, std::vector<std::weak_ptr<descriptor_pool>>> mDescriptorPools;
	};
}
