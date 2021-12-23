#pragma once
#include <avk/avk.hpp>

namespace avk
{
	namespace subpass
	{
		struct subpass_index2
		{
			uint32_t mSrc;
			uint32_t mDst;
		};

		struct subpass_index
		{
			uint32_t mSubpassIndex;
		};

		inline subpass_index2 operator>> (subpass_index a, subpass_index b)
		{
			return subpass_index2{ a.mSubpassIndex, b.mSubpassIndex };
		}

		static constexpr auto external = subpass_index{ VK_SUBPASS_EXTERNAL };
		static constexpr auto index(uint32_t aSubpassIndex) { return subpass_index{ aSubpassIndex }; }
	}

	struct subpass_dependency
	{
		subpass_dependency(subpass::subpass_index2 aIndices, stage::pipeline_stage2 aStages, access::memory_access2 aAccesses)
			: mIndices{ aIndices }
			, mStages{ aStages }
			, mAccesses{ aAccesses }
		{}

		subpass_dependency(subpass::subpass_index2 aIndices, stage::pipeline_stage2 aStages)
			: mIndices{ aIndices }
			, mStages{ aStages } {
			mAccesses = access::none >> access::none;
		}
		
		subpass_dependency(subpass::subpass_index2 aIndices)
			: mIndices{ aIndices } {
			mStages = stage::none >> stage::none;
			mAccesses = access::none >> access::none;
		}

		subpass::subpass_index2 mIndices;
		stage::pipeline_stage2 mStages;
		access::memory_access2 mAccesses;
	};

	using subpass_dependencies = std::vector<subpass_dependency>;
	
	inline std::optional<std::tuple<vk::SubpassDependency2KHR, vk::MemoryBarrier2KHR>> try_get_subpass_dependency_for_indices(const subpass_dependencies& aDependencies, uint32_t aSrcSubpass, uint32_t aDstSubpass)
	{
		auto result = std::optional<std::tuple<vk::SubpassDependency2KHR, vk::MemoryBarrier2KHR>>{};

		const auto it = std::find_if(std::begin(aDependencies), std::end(aDependencies), [aSrcSubpass, aDstSubpass](const subpass_dependency& lSubDep) {
			return lSubDep.mIndices.mSrc == aSrcSubpass && lSubDep.mIndices.mDst == aDstSubpass;
		});

		if (std::end(aDependencies) != it) {
			result.emplace(std::make_tuple(
				vk::SubpassDependency2KHR{}
					.setSrcSubpass(aSrcSubpass).setDstSubpass(aDstSubpass)
					.setPNext(nullptr) // !!! ATTENTION: This will have to be set by the user of this function to point to the appropriate vk::MemoryBarrier2KHR{} !!!
				, 
				vk::MemoryBarrier2KHR{}
					.setSrcStageMask(it->mStages.mSrc).setDstStageMask(it->mStages.mDst) // No sync specified => no sync established.
					.setSrcAccessMask(it->mAccesses.mSrc).setDstAccessMask(it->mAccesses.mSrc) // No access specified => no access established.
			));
		}

		return result;
	}

	inline std::tuple<vk::SubpassDependency2KHR, vk::MemoryBarrier2KHR> get_subpass_dependency_for_indices(const subpass_dependencies& aDependencies, uint32_t aSrcSubpass, uint32_t aDstSubpass)
	{
		auto result = std::make_tuple(vk::SubpassDependency2KHR{}, vk::MemoryBarrier2KHR{});
		auto& [dependency, barrier] = result;
		dependency.setSrcSubpass(aSrcSubpass).setDstSubpass(aDstSubpass)
			.setPNext(&barrier); // !!! ATTENTION: This will have to be redirected by the user of this function to point to the appropriate vk::MemoryBarrier2KHR{} !!!

		const auto it = std::find_if(std::begin(aDependencies), std::end(aDependencies), [aSrcSubpass, aDstSubpass](const subpass_dependency& lSubDep) {
			return lSubDep.mIndices.mSrc == aSrcSubpass && lSubDep.mIndices.mDst == aDstSubpass;
		});

		if (std::end(aDependencies) == it) {
			barrier
				.setSrcStageMask(vk::PipelineStageFlagBits2KHR::eNone).setDstStageMask(vk::PipelineStageFlagBits2KHR::eNone) // No sync specified => no sync established.
				.setSrcAccessMask(vk::AccessFlagBits2KHR::eNone).setDstAccessMask(vk::AccessFlagBits2KHR::eNone); // No access specified => no access established.
		}
		else { // Found!
			barrier
				.setSrcStageMask(it->mStages.mSrc).setDstStageMask(it->mStages.mDst) // No sync specified => no sync established.
				.setSrcAccessMask(it->mAccesses.mSrc).setDstAccessMask(it->mAccesses.mSrc); // No access specified => no access established.
		}

		return result;
	}

}
