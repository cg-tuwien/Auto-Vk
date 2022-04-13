#pragma once
#include <avk/avk.hpp>

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
		std::optional<avk::image_layout::image_layout> mPreviousLayout;

		attachment_load_config& from_previous_layout(avk::image_layout::image_layout aPreviousLayout)
		{
			mPreviousLayout = aPreviousLayout;
			return *this;
		}
	};

	namespace on_load
	{
		inline attachment_load_config dont_care()
		{
			return attachment_load_config{ on_load_behavior::dont_care, {} };
		}

		inline attachment_load_config clear()
		{
			return attachment_load_config{ on_load_behavior::clear, {} };
		}
		
		inline attachment_load_config load()
		{
			return attachment_load_config{ on_load_behavior::load, {} };
		}
	}
}
