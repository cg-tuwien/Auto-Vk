#pragma once
#include <avk/avk.hpp>

namespace avk
{
	/** Descriptor set */
	class descriptor_set
	{
		friend bool operator ==(const descriptor_set& left, const descriptor_set& right);
		friend bool operator !=(const descriptor_set& left, const descriptor_set& right);
		friend struct std::hash<avk::descriptor_set>;
		
	public:
		descriptor_set() = default;
		descriptor_set(descriptor_set&&) noexcept = default;
		descriptor_set(const descriptor_set&) = default;
		descriptor_set& operator=(descriptor_set&&) noexcept = default;
		descriptor_set& operator=(const descriptor_set&) = default;
		~descriptor_set() = default;

		auto number_of_writes() const { return mOrderedDescriptorDataWrites.size(); }
		const auto& write_at(size_t i) const { return mOrderedDescriptorDataWrites[i]; }
		const auto* pool() const { return static_cast<bool>(mPool) ? mPool.get() : nullptr; }
		auto handle() const { return mDescriptorSet; }
		auto set_id() const { return mSetId; }
		void set_set_id(uint32_t aNewSetId) { mSetId = aNewSetId; }

		const auto* store_image_infos(uint32_t aBindingId, std::vector<vk::DescriptorImageInfo> aStoredImageInfos)
		{
			auto back = mStoredImageInfos.emplace_back(aBindingId, std::move(aStoredImageInfos));
			return std::get<std::vector<vk::DescriptorImageInfo>>(back).data();
		}
		
		const auto* store_buffer_infos(uint32_t aBindingId, std::vector<vk::DescriptorBufferInfo> aStoredBufferInfos)
		{
			auto back = mStoredBufferInfos.emplace_back(aBindingId, std::move(aStoredBufferInfos));
			return std::get<std::vector<vk::DescriptorBufferInfo>>(back).data();
		}
		
#if VK_HEADER_VERSION >= 135
		const vk::WriteDescriptorSetAccelerationStructureKHR* store_acceleration_structure_infos(uint32_t aBindingId, std::vector<vk::WriteDescriptorSetAccelerationStructureKHR> aWriteAccelerationStructureInfos)
		{
			// Accumulate all into ONE! (At least I think "This is the way.")
			std::tuple<vk::WriteDescriptorSetAccelerationStructureKHR, std::vector<vk::AccelerationStructureKHR>> oneAndOnlyWrite;

			for (auto& wasi : aWriteAccelerationStructureInfos) {
				for (uint32_t i = 0u; i < wasi.accelerationStructureCount; ++i) {
					std::get<std::vector<vk::AccelerationStructureKHR>>(oneAndOnlyWrite).push_back(wasi.pAccelerationStructures[i]);
				}
			}

			std::get<vk::WriteDescriptorSetAccelerationStructureKHR>(oneAndOnlyWrite).accelerationStructureCount = static_cast<uint32_t>(std::get<std::vector<vk::AccelerationStructureKHR>>(oneAndOnlyWrite).size());
			
			auto back = mStoredAccelerationStructureWrites.emplace_back(aBindingId, std::move(oneAndOnlyWrite));
			return &std::get<vk::WriteDescriptorSetAccelerationStructureKHR>(std::get<1>(back));
		}
#endif

		const auto* store_buffer_views(uint32_t aBindingId, std::vector<vk::BufferView> aStoredBufferViews)
		{
			auto back = mStoredBufferViews.emplace_back(aBindingId, std::move(aStoredBufferViews));
			return std::get<std::vector<vk::BufferView>>(back).data();
		}

		const auto* store_image_info(uint32_t aBindingId, const vk::DescriptorImageInfo& aStoredImageInfo)
		{
			auto back = mStoredImageInfos.emplace_back(aBindingId, avk::make_vector( aStoredImageInfo ));
			return std::get<std::vector<vk::DescriptorImageInfo>>(back).data();
		}
		
		const auto* store_buffer_info(uint32_t aBindingId, const vk::DescriptorBufferInfo& aStoredBufferInfo)
		{
			auto back = mStoredBufferInfos.emplace_back(aBindingId, avk::make_vector( aStoredBufferInfo ));
			return std::get<std::vector<vk::DescriptorBufferInfo>>(back).data();
		}
		
#if VK_HEADER_VERSION >= 135
		const vk::WriteDescriptorSetAccelerationStructureKHR* store_acceleration_structure_info(uint32_t aBindingId, const vk::WriteDescriptorSetAccelerationStructureKHR& aWriteAccelerationStructureInfo)
		{
			std::vector<vk::AccelerationStructureKHR> accStructureHandles;
			for (uint32_t i = 0u; i < aWriteAccelerationStructureInfo.accelerationStructureCount; ++i) {
				accStructureHandles.push_back(aWriteAccelerationStructureInfo.pAccelerationStructures[i]);
			}

			auto theWrite = std::make_tuple<vk::WriteDescriptorSetAccelerationStructureKHR, std::vector<vk::AccelerationStructureKHR>>(
				vk::WriteDescriptorSetAccelerationStructureKHR{aWriteAccelerationStructureInfo}, std::move(accStructureHandles)
			);
			
			auto back = mStoredAccelerationStructureWrites.emplace_back(aBindingId, std::move(theWrite));
			return &std::get<vk::WriteDescriptorSetAccelerationStructureKHR>(std::get<1>(back));
		}
#endif

		const auto* store_buffer_view(uint32_t aBindingId, const vk::BufferView& aStoredBufferView)
		{
			auto back = mStoredBufferViews.emplace_back(aBindingId, avk::make_vector( aStoredBufferView ));
			return std::get<std::vector<vk::BufferView>>(back).data();
		}

		void update_data_pointers();
		
		template <typename It>
		static descriptor_set prepare(It begin, It end)
		{
			descriptor_set result;
			result.mSetId = begin->mSetId;
			
			It it = begin;
			while (it != end) {
				const binding_data& b = *it;
				assert(begin->mSetId == b.mSetId);

				assert((it+1) == end || b.mLayoutBinding.binding != (it+1)->mLayoutBinding.binding);
				assert((it+1) == end || b.mLayoutBinding.binding < (it+1)->mLayoutBinding.binding);

				result.mOrderedDescriptorDataWrites.emplace_back(
					vk::DescriptorSet{}, // To be set before actually writing
					b.mLayoutBinding.binding,
					0u, // TODO: Maybe support other array offsets
					b.descriptor_count(),
					b.mLayoutBinding.descriptorType,
					b.descriptor_image_info(result),
					b.descriptor_buffer_info(result),
					b.texel_buffer_view_info(result)
				);
				result.mOrderedDescriptorDataWrites.back().setPNext(b.next_pointer(result));
				
				++it;
			}

			result.update_data_pointers();
			return result;
		}

		void link_to_handle_and_pool(vk::DescriptorSet aHandle, std::shared_ptr<descriptor_pool> aPool);
		void write_descriptors();
		
	private:
		std::vector<vk::WriteDescriptorSet> mOrderedDescriptorDataWrites;
		std::shared_ptr<descriptor_pool> mPool;
		vk::DescriptorSet mDescriptorSet;
		// TODO: Are there cases where vk::UniqueDescriptorSet would be beneficial? Right now, the pool cleans up all the descriptor sets.
		uint32_t mSetId;
		// TODO: Probably turn all of these vectors into shared_ptrs which is much better when passing around between descriptor_cache and bind_descriptors, etc.!
		std::vector<std::tuple<uint32_t, std::vector<vk::DescriptorImageInfo>>> mStoredImageInfos;
		std::vector<std::tuple<uint32_t, std::vector<vk::DescriptorBufferInfo>>> mStoredBufferInfos;
		std::vector<std::tuple<uint32_t, std::vector<vk::BufferView>>> mStoredBufferViews;
#if VK_HEADER_VERSION >= 135
		std::vector<std::tuple<uint32_t, std::tuple<vk::WriteDescriptorSetAccelerationStructureKHR, std::vector<vk::AccelerationStructureKHR>>>> mStoredAccelerationStructureWrites;
#endif
	};

	extern bool operator ==(const descriptor_set& left, const descriptor_set& right);

	extern bool operator !=(const descriptor_set& left, const descriptor_set& right);
}

namespace std
{
	template<> struct hash<avk::descriptor_set>
	{
		std::size_t operator()(avk::descriptor_set const& o) const noexcept
		{
			std::size_t h = 0;
			for(auto& w : o.mOrderedDescriptorDataWrites)
			{
				avk::hash_combine(h, w.dstBinding, w.dstArrayElement, w.descriptorCount, w.descriptorType);
				// Dont compute a too expensive hash => only take the first elements, each:
				if (nullptr != w.pImageInfo && w.descriptorCount > 0) {
					avk::hash_combine(h, static_cast<VkSampler>(w.pImageInfo[0].sampler), static_cast<VkImageView>(w.pImageInfo[0].imageView), static_cast<VkImageLayout>(w.pImageInfo[0].imageLayout));
				}
				if (nullptr != w.pBufferInfo && w.descriptorCount > 0) {
					avk::hash_combine(h, static_cast<VkBuffer>(w.pBufferInfo[0].buffer), w.pBufferInfo[0].offset, w.pBufferInfo[0].range);
				}
				if (nullptr != w.pTexelBufferView && w.descriptorCount > 0) {
					avk::hash_combine(h, static_cast<VkBufferView>(w.pTexelBufferView[0]));
				}

#if VK_HEADER_VERSION >= 135
				if (nullptr != w.pNext) {
					if (w.descriptorType == vk::DescriptorType::eAccelerationStructureKHR) {
						const auto* asInfo = reinterpret_cast<const VkWriteDescriptorSetAccelerationStructureKHR*>(w.pNext);
						avk::hash_combine(h, asInfo->accelerationStructureCount);
						if (asInfo->accelerationStructureCount > 0) {
							avk::hash_combine(h, static_cast<VkAccelerationStructureKHR>(asInfo->pAccelerationStructures[0]));
						}
					}
					else {
						avk::hash_combine(h, nullptr != w.pNext);
					}
				}
#endif
				// operator== will test for exact equality.
			}
			return h;
		}
	};

}
