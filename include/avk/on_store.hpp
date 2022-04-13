#pragma once
#include <avk/avk.hpp>

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
		std::optional<avk::image_layout::image_layout> mTargetLayout;

		attachment_store_config& in_layout(avk::image_layout::image_layout aTargetLayout)
		{
			mTargetLayout = aTargetLayout;
			return *this;
		}
	};

	namespace on_store
	{
		inline attachment_store_config dont_care()
		{
			return attachment_store_config{ on_store_behavior::dont_care, {} };
		}

		inline attachment_store_config store()
		{
			return attachment_store_config{ on_store_behavior::store, {} };
		}
	}
}
