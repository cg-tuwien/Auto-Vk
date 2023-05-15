#pragma once
#include "avk/avk.hpp"

namespace avk
{
	enum struct on_load_behavior
	{
		dont_care,
		clear,
		load
	};

	struct attachment_load_config
	{
		on_load_behavior mLoadBehavior;
		std::optional<avk::layout::image_layout> mPreviousLayout;

		attachment_load_config from_previous_layout(avk::layout::image_layout aPreviousLayout) const
		{
			return attachment_load_config{ mLoadBehavior, aPreviousLayout };
		}
	};

	namespace on_load
	{
		static constexpr auto dont_care = attachment_load_config{ on_load_behavior::dont_care, {} };
		static constexpr auto clear     = attachment_load_config{ on_load_behavior::clear, {} };
		static constexpr auto load      = attachment_load_config{ on_load_behavior::load, {} };
	}
}
