#pragma once
#include <ak/ak.hpp>

namespace ak
{
	class descriptor_cache_interface
	{
	public:
		virtual ~descriptor_cache_interface() = default;
	
		virtual const descriptor_set_layout& get_or_alloc_layout(root& aRoot, descriptor_set_layout aPreparedLayout) = 0;

		virtual std::optional<descriptor_set> get_descriptor_set_from_cache(const descriptor_set& aPreparedSet) = 0;

		virtual std::vector<descriptor_set> alloc_new_descriptor_sets(root& aRoot, const std::vector<std::reference_wrapper<const descriptor_set_layout>>& aLayouts, std::vector<descriptor_set> aPreparedSets) = 0;

		virtual void cleanup() = 0;
	};
}
