#pragma once
#include <ak/ak.hpp>

namespace ak
{
	struct aabb
	{
		std::array<float, 3> mMinBounds;
		std::array<float, 3> mMaxBounds;
	};
}
