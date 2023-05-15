#pragma once
#include "avk/avk.hpp"

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
		subpass_dependency(subpass::subpass_index2 aIndices, stage::execution_dependency aStages, access::memory_dependency aAccesses)
			: mIndices{ aIndices }
			, mStages{ aStages }
			, mAccesses{ aAccesses }
		{}

		subpass_dependency(subpass::subpass_index2 aIndices, stage::execution_dependency aStages)
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
		stage::execution_dependency mStages;
		access::memory_dependency mAccesses;
	};

	using subpass_dependencies = std::vector<subpass_dependency>;
}
