#pragma once

namespace ak
{
	namespace cfg
	{
		/** Pipeline configuration data: General pipeline settings */
		enum struct pipeline_settings
		{
			nothing					= 0x0000,
			force_new_pipe			= 0x0001,
			fail_if_not_reusable	= 0x0002,
			disable_optimization	= 0x0004,
			allow_derivatives		= 0x0008
		};

		inline pipeline_settings operator| (pipeline_settings a, pipeline_settings b)
		{
			typedef std::underlying_type<pipeline_settings>::type EnumType;
			return static_cast<pipeline_settings>(static_cast<EnumType>(a) | static_cast<EnumType>(b));
		}

		inline pipeline_settings operator& (pipeline_settings a, pipeline_settings b)
		{
			typedef std::underlying_type<pipeline_settings>::type EnumType;
			return static_cast<pipeline_settings>(static_cast<EnumType>(a) & static_cast<EnumType>(b));
		}

		inline pipeline_settings& operator |= (pipeline_settings& a, pipeline_settings b)
		{
			return a = a | b;
		}

		inline pipeline_settings& operator &= (pipeline_settings& a, pipeline_settings b)
		{
			return a = a & b;
		}
	}
}
