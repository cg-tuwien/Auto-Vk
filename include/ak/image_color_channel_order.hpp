#pragma once
#include <ak/ak.hpp>

namespace ak
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
