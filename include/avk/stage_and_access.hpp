#pragma once
#include <avk/avk.hpp>

namespace avk
{
	/**	To define an execution_dependency, use operator>> with two pipeline_stage_flags values!
	 *	There are multiple such pipeline_stage_flags values prepared in the avk::stage namespace.
	 *
	 *	Example:
	 *	  // Create an execution dependency between copy commands and fragment shader stages:
	 *	  avk::stage::copy >> avk::stage::fragment_shader
	 */
	struct stage_and_access
	{
		stage::pipeline_stage_flags mStage;
		access::memory_access_flags mAccess;
	};

	inline avk::stage_and_access operator+ (const avk::stage::pipeline_stage_flags& aStages, const avk::access::memory_access_flags& aAccesses)
	{
		return avk::stage_and_access{ aStages, aAccesses };
	}


	struct stage_and_access_dependency
	{
		stage_and_access mSrc;
		stage_and_access mDst;
	};

	inline static stage_and_access_dependency operator>> (stage_and_access a, stage_and_access b)
	{
		return stage_and_access_dependency{ a, b };
	}


	struct stage_and_access_precisely
	{
		stage_and_access_precisely()
			: mStage{ vk::PipelineStageFlagBits2KHR::eNone }
			, mAccess{ vk::AccessFlagBits2KHR::eNone }
		{}

		stage_and_access_precisely(vk::PipelineStageFlags2KHR aStages, vk::AccessFlags2KHR aAccess)
			: mStage{ aStages }
			, mAccess{ aAccess }
		{}

		stage_and_access_precisely(stage_and_access aInput)
			: mStage{ std::get<vk::PipelineStageFlags2KHR>(aInput.mStage.mFlags) }
			, mAccess{ std::get<vk::AccessFlags2KHR>(aInput.mAccess.mFlags) }
		{
			assert(std::holds_alternative<vk::PipelineStageFlags2KHR>(aInput.mStage.mFlags));
			assert(std::holds_alternative<vk::AccessFlags2KHR>(aInput.mAccess.mFlags));
		}

		stage_and_access_precisely(stage_and_access_precisely&&) noexcept = default;
		stage_and_access_precisely(const stage_and_access_precisely&) = default;

		stage_and_access_precisely& operator=(stage_and_access aInput)
		{
			assert(std::holds_alternative<vk::PipelineStageFlags2KHR>(aInput.mStage.mFlags));
			assert(std::holds_alternative<vk::AccessFlags2KHR>(aInput.mAccess.mFlags));
			mStage  = std::get<vk::PipelineStageFlags2KHR>(aInput.mStage.mFlags);
			mAccess = std::get<vk::AccessFlags2KHR>(aInput.mAccess.mFlags);
			return *this;
		}

		stage_and_access_precisely& operator=(stage_and_access_precisely&&) noexcept = default;
		stage_and_access_precisely& operator=(const stage_and_access_precisely&) = default;
		~stage_and_access_precisely() = default;

		vk::PipelineStageFlags2KHR mStage;
		vk::AccessFlags2KHR        mAccess;
	};


	struct stage_and_access_dependency_precisely
	{
		stage_and_access_dependency_precisely() = default;

		stage_and_access_dependency_precisely(stage_and_access aSrc, stage_and_access aDst)
			: mSrc{ aSrc }
			, mDst{ aDst }
		{}

		stage_and_access_dependency_precisely(stage_and_access_precisely aSrc, stage_and_access_precisely aDst)
			: mSrc{ aSrc }
			, mDst{ aDst }
		{}

		stage_and_access_dependency_precisely(stage_and_access_dependency aInput)
			: mSrc{ aInput.mSrc }
			, mDst{ aInput.mDst }
		{}

		stage_and_access_dependency_precisely(stage_and_access_dependency_precisely&&) noexcept = default;
		stage_and_access_dependency_precisely(const stage_and_access_dependency_precisely&) = default;

		stage_and_access_dependency_precisely& operator=(stage_and_access_dependency aInput)
		{
			mSrc = aInput.mSrc;
			mDst = aInput.mDst;
			return *this;
		}

		stage_and_access_dependency_precisely& operator=(stage_and_access_dependency_precisely&&) noexcept = default;
		stage_and_access_dependency_precisely& operator=(const stage_and_access_dependency_precisely&) = default;
		~stage_and_access_dependency_precisely() = default;

		stage_and_access_precisely mSrc;
		stage_and_access_precisely mDst;
	};
}
