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
	
	inline std::optional<std::tuple<
		vk::SubpassDependency2KHR,
		vk::MemoryBarrier2KHR
	>> try_get_subpass_dependency_for_indices(const subpass_dependencies& aDependencies, uint32_t aSrcSubpass, uint32_t aDstSubpass)
	{
		auto result = std::optional<std::tuple<vk::SubpassDependency2KHR, vk::MemoryBarrier2KHR>>{};

		const auto it = std::find_if(std::begin(aDependencies), std::end(aDependencies), [aSrcSubpass, aDstSubpass](const subpass_dependency& lSubDep) {
			return lSubDep.mIndices.mSrc == aSrcSubpass && lSubDep.mIndices.mDst == aDstSubpass;
		});

		if (std::end(aDependencies) != it) {
			// See if this is supposed to be an auto-dependency:
			const bool srcStageIsAuto = std::holds_alternative<avk::stage::auto_stage_t>(it->mStages.mSrc);
			const bool dstStageIsAuto = std::holds_alternative<avk::stage::auto_stage_t>(it->mStages.mDst);
			const bool srcAccessIsAuto = std::holds_alternative<avk::access::auto_access_t>(it->mAccesses.mSrc);
			const bool dstAccessIsAuto = std::holds_alternative<avk::access::auto_access_t>(it->mAccesses.mDst);

			result.emplace(std::make_tuple(
				vk::SubpassDependency2KHR{}
					.setSrcSubpass(aSrcSubpass).setDstSubpass(aDstSubpass)
					.setPNext(nullptr) // !!! ATTENTION: This will have to be set by the user of this function to point to the appropriate vk::MemoryBarrier2KHR{} !!!
				, 
				vk::MemoryBarrier2KHR{}
					.setSrcStageMask(srcStageIsAuto 
						? vk::PipelineStageFlags2KHR{
							VK_SUBPASS_EXTERNAL == aSrcSubpass
							? vk::PipelineStageFlagBits2KHR::eAllCommands
							: vk::PipelineStageFlagBits2KHR::eAllGraphics
						}
						: std::get<vk::PipelineStageFlags2KHR>(it->mStages.mSrc)
					)
					.setDstStageMask(dstStageIsAuto 
						? vk::PipelineStageFlags2KHR{
							VK_SUBPASS_EXTERNAL == aDstSubpass
							? vk::PipelineStageFlagBits2KHR::eAllCommands
							: vk::PipelineStageFlagBits2KHR::eAllGraphics
						}
						: std::get<vk::PipelineStageFlags2KHR>(it->mStages.mDst)
					)
					.setSrcAccessMask(srcAccessIsAuto
						? vk::AccessFlags2KHR{ vk::AccessFlagBits2KHR::eMemoryWrite }
						: std::get<vk::AccessFlags2KHR>(it->mAccesses.mSrc)
					)
					.setDstAccessMask(dstAccessIsAuto
						? vk::AccessFlags2KHR{ vk::AccessFlagBits2KHR::eMemoryWrite }
						: std::get<vk::AccessFlags2KHR>(it->mAccesses.mDst)
					)
			));
		}

		return result;
	}

}
