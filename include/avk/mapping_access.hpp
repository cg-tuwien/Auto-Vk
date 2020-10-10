#pragma once
#include <avk/avk.hpp>

namespace avk
{
	enum struct mapping_access
	{
		read				= 0x00000001,
		write				= 0x00000002,
	};

	inline mapping_access operator| (mapping_access a, mapping_access b)
	{
		typedef std::underlying_type<mapping_access>::type EnumType;
		return static_cast<mapping_access>(static_cast<EnumType>(a) | static_cast<EnumType>(b));
	}

	inline mapping_access operator& (mapping_access a, mapping_access b)
	{
		typedef std::underlying_type<mapping_access>::type EnumType;
		return static_cast<mapping_access>(static_cast<EnumType>(a) & static_cast<EnumType>(b));
	}

	inline mapping_access& operator |= (mapping_access& a, mapping_access b)
	{
		return a = a | b;
	}

	inline mapping_access& operator &= (mapping_access& a, mapping_access b)
	{
		return a = a & b;
	}

	inline mapping_access exclude(mapping_access original, mapping_access toExclude)
	{
		typedef std::underlying_type<mapping_access>::type EnumType;
		return static_cast<mapping_access>(static_cast<EnumType>(original) & (~static_cast<EnumType>(toExclude)));
	}
}
