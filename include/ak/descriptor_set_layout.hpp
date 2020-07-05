#pragma once
#include <ak/ak.hpp>

namespace ak
{
	/**	Represents a descriptor set layout, contains also the bindings
	 *	stored in binding-order, and the accumulated descriptor counts
	 *	per bindings which is information that can be used for configuring
	 *	descriptor pools.
	 */
	class descriptor_set_layout
	{
		friend class root;
		friend static bool operator ==(const descriptor_set_layout& left, const descriptor_set_layout& right);
		friend static bool operator !=(const descriptor_set_layout& left, const descriptor_set_layout& right);
		friend struct std::hash<ak::descriptor_set_layout>;
		
	public:
		descriptor_set_layout() = default;
		descriptor_set_layout(const descriptor_set_layout&) = delete;
		descriptor_set_layout(descriptor_set_layout&&) noexcept = default;
		descriptor_set_layout& operator=(const descriptor_set_layout&) = delete;
		descriptor_set_layout& operator=(descriptor_set_layout&&) noexcept = default;
		~descriptor_set_layout() = default;

		const auto& required_pool_sizes() const { return mBindingRequirements; }
		auto number_of_bindings() const { return mOrderedBindings.size(); }
		const auto& binding_at(size_t i) const { return mOrderedBindings[i]; }
		auto* bindings_data_ptr() const { return mOrderedBindings.data(); }
		auto owner() const { return mLayout.getOwner(); }
		auto has_handle() const { return static_cast<bool>(mLayout); }
		auto handle() const { return mLayout.get(); }

		template <typename It>
		static descriptor_set_layout prepare(It begin, It end)
		{
			// Put elements from initializer list into vector of ORDERED bindings:
			descriptor_set_layout result;
			
			It it = begin;
			while (it != end) {
				const binding_data& b = *it;
				assert(begin->mSetId == b.mSetId);

				{ // Assemble the mBindingRequirements member:
				  // ordered by descriptor type
					auto entry = vk::DescriptorPoolSize{}
						.setType(b.mLayoutBinding.descriptorType)
						.setDescriptorCount(b.mLayoutBinding.descriptorCount);
					// find position where to insert in vector
					auto pos = std::lower_bound(std::begin(result.mBindingRequirements), std::end(result.mBindingRequirements), 
						entry,
						[](const vk::DescriptorPoolSize& first, const vk::DescriptorPoolSize& second) -> bool {
							using EnumType = std::underlying_type<vk::DescriptorType>::type;
							return static_cast<EnumType>(first.type) < static_cast<EnumType>(second.type);
						});
					// Maybe accumulate
					if (pos != std::end(result.mBindingRequirements) && pos->type == entry.type) {
						pos->descriptorCount += entry.descriptorCount;
					}
					else {
						result.mBindingRequirements.insert(pos, entry);
					}
				}

				// Store the ordered binding:
				assert((it+1) == end || b.mLayoutBinding.binding != (it+1)->mLayoutBinding.binding);
				assert((it+1) == end || b.mLayoutBinding.binding < (it+1)->mLayoutBinding.binding);
				result.mOrderedBindings.push_back(b.mLayoutBinding);
				
				it++;
			}

			// Preparation is done
			return result;
		}

		static descriptor_set_layout prepare(std::vector<binding_data> pBindings)
		{
			return prepare(std::begin(pBindings), std::end(pBindings));
		}

	private:
		std::vector<vk::DescriptorPoolSize> mBindingRequirements;
		std::vector<vk::DescriptorSetLayoutBinding> mOrderedBindings;
		vk::UniqueDescriptorSetLayout mLayout;
	};

	static bool operator ==(const descriptor_set_layout& left, const descriptor_set_layout& right) {
		const auto n = left.mOrderedBindings.size();
		if (n != right.mOrderedBindings.size()) {
			return false;
		}
		for (size_t i = 0; i < n; ++i) {
			if (left.mOrderedBindings[i] != right.mOrderedBindings[i]) {
				return false;
			}
		}
		return true;
	}

	static bool operator !=(const descriptor_set_layout& left, const descriptor_set_layout& right) {
		return !(left == right);
	}	
}

namespace std
{
	template<> struct hash<ak::descriptor_set_layout>
	{
		std::size_t operator()(ak::descriptor_set_layout const& o) const noexcept
		{
			std::size_t h = 0;
			for(auto& binding : o.mOrderedBindings)
			{
				ak::hash_combine(h, binding.binding, binding.descriptorType, binding.descriptorCount, static_cast<VkShaderStageFlags>(binding.stageFlags), binding.pImmutableSamplers);
			}
			return h;
		}
	};
}
