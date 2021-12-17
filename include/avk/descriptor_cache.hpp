#pragma once
#include <avk/avk.hpp>

namespace avk
{
	/**	This is a ready-to-use implementation for a descriptor cache.
	 *  The cache is prepared for concurrent access from multiple threads
	 *  and it will create one or multiple descriptor pools per thread.
	 *
	 *  Descriptor pools are not shared across threads, but always exclusive
	 *  for a certain thread.
	 *
	 *  The allocated pools are rather tightly sized and fit to incoming requests.
	 *  This might or might not be the desired behavior. If the descriptors that
	 *  you pass to shaders do not change a lot during the runtime of the program,
	 *  this behavior might be well suited.
	 *  If, however, you have to pass constantly changing descriptors, you might
	 *  experience too many descriptor pool allocations which can lead to bad
	 *  performance.
	 *
	 *  You can control the pool size with set_prealloc_factor().
	 *  However, the pool still only allocates descriptor types according to the
	 *  incoming request (just prealloc_factor-times many of them).
	 *  If your application's requirements diverge too much from the assumptions
	 *  of this descriptor_cache, consider implementing a different descriptor
	 *  cache class or in general, handle it manually.
	 *
	 */
	class descriptor_cache
	{
		friend class root;
		
	public:
		auto prealloc_factor() const { return mPreallocFactor; }
		void set_prealloc_factor(int aFactor) { mPreallocFactor = aFactor; }
		
		const descriptor_set_layout& get_or_alloc_layout(descriptor_set_layout aPreparedLayout);
		std::optional<descriptor_set> get_descriptor_set_from_cache(const descriptor_set& aPreparedSet);
		std::vector<descriptor_set> alloc_new_descriptor_sets(const std::vector<std::reference_wrapper<const descriptor_set_layout>>& aLayouts, std::vector<descriptor_set> aPreparedSets);
		void cleanup();
		
		std::shared_ptr<descriptor_pool> get_descriptor_pool_for_layouts(const descriptor_alloc_request& aAllocRequest, bool aRequestNewPool = false);

		std::vector<descriptor_set> get_or_create_descriptor_sets(std::initializer_list<binding_data> aBindings);

		int remove_sets_with_handle(vk::ImageView aHandle);
		int remove_sets_with_handle(vk::Buffer aHandle);
		int remove_sets_with_handle(vk::Sampler aHandle);
		int remove_sets_with_handle(vk::BufferView aHandle);
		
	private:
		std::string mName = "descriptor cache";
		int mPreallocFactor = 5;
		const root* mRoot;
		
		std::unordered_set<descriptor_set_layout> mLayouts;
		std::unordered_set<descriptor_set> mSets;
		
		// Descriptor pools are created/stored per thread and can have a name (an integer-id). 
		// If possible, it is tried to re-use a pool. Even when re-using a pool, it might happen that
		// allocating from it might fail (because out of memory, for instance). In such cases, a new 
		// pool will be created.
		std::unordered_map<std::thread::id, std::vector<std::weak_ptr<descriptor_pool>>> mDescriptorPools;
	};
}
