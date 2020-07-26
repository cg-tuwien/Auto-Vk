#pragma once
#include <avk/avk.hpp>

namespace avk
{
	/** The order of the color channels w.r.t. an image format */
	enum struct image_color_channel_order
	{
		rgba,
		bgra,
		argb,
		abgr,
		rgb = rgba,
		bgr = bgra
	};
}
