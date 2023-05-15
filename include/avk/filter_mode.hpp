#pragma once
#include "avk/avk.hpp"

namespace avk
{
	/** Configuration struct to specify the filtering strategy for texture sampling. */
	enum struct filter_mode
	{
		nearest_neighbor,
		bilinear,
		trilinear,
		cubic,
		anisotropic_2x,
		anisotropic_4x,
		anisotropic_8x,
		anisotropic_16x,
		anisotropic_32x,
		anisotropic_64x
	};
}
