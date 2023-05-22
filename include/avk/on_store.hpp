#pragma once
#include "avk/avk.hpp"

namespace avk
{
	enum struct on_store_behavior
	{
		dont_care,
		store
	};

	struct attachment_store_config
	{
		on_store_behavior mStoreBehavior;
		std::optional<avk::layout::image_layout> mTargetLayout;

		attachment_store_config in_layout(avk::layout::image_layout aTargetLayout) const
		{
			return attachment_store_config{ mStoreBehavior, aTargetLayout };
		}
	};

	namespace on_store
	{
		static constexpr auto dont_care = attachment_store_config{ on_store_behavior::dont_care, {} };
		static constexpr auto store     = attachment_store_config{ on_store_behavior::store, {} };

	}
}
