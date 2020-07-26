#pragma once
#include <avk/avk.hpp>

namespace avk
{
	struct aabb
	{
		std::array<float, 3> mMinBounds;
		std::array<float, 3> mMaxBounds;
	};
}
