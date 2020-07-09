#pragma once
#include <ak/ak.hpp>

namespace ak
{
	struct push_constant_binding_data
	{
		shader_type mShaderStages;
		size_t mOffset;
		size_t mSize;
	};
}
