#pragma once
#include "avk/avk.hpp"

namespace avk
{
	namespace access
	{
		using auto_access_t = uint8_t;

		/**	To define a memory_dependency, use operator>> with two memory_access_flags values!
		 *	There are multiple such memory_access_flags values prepared in the avk::access namespace.
		 *
		 *	Example:
		 *	  // Create a memory dependency between color attachment writes and transform reads:
		 *	  avk::access::color_attachment_write >> avk::access::transfer_read
		 */
		struct memory_dependency 
		{
			std::variant<std::monostate, vk::AccessFlags2KHR, auto_access_t> mSrc;
			std::variant<std::monostate, vk::AccessFlags2KHR, auto_access_t> mDst;
		};

		struct memory_access_flags
		{
			std::variant<std::monostate, vk::AccessFlags2KHR, auto_access_t> mFlags;
		};

		inline memory_dependency operator>> (memory_access_flags a, memory_access_flags b)
		{
			return memory_dependency{ a.mFlags, b.mFlags };
		}

#pragma region memory_access_flags operators
		inline memory_access_flags operator| (memory_access_flags a, memory_access_flags b)
		{
			if (!std::holds_alternative<vk::AccessFlags2KHR>(a.mFlags) || !std::holds_alternative<vk::AccessFlags2KHR>(b.mFlags)) {
				throw avk::runtime_error("operator| may only be used with concrete memory access set, not with auto_access nor with uninitialized values.");
			}
			return memory_access_flags{ std::get<vk::AccessFlags2KHR>(a.mFlags) | std::get<vk::AccessFlags2KHR>(b.mFlags) };
		}

		inline memory_access_flags operator& (memory_access_flags a, memory_access_flags b)
		{
			if (!std::holds_alternative<vk::AccessFlags2KHR>(a.mFlags) || !std::holds_alternative<vk::AccessFlags2KHR>(b.mFlags)) {
				throw avk::runtime_error("operator& may only be used with concrete memory access set, not with auto_access nor with uninitialized values.");
			}
			return memory_access_flags{ std::get<vk::AccessFlags2KHR>(a.mFlags) & std::get<vk::AccessFlags2KHR>(b.mFlags) };
		}

		inline memory_access_flags& operator |= (memory_access_flags& a, memory_access_flags b)
		{
			return a = a | b;
		}

		inline memory_access_flags& operator &= (memory_access_flags& a, memory_access_flags b)
		{
			return a = a & b;
		}

		inline memory_access_flags exclude(memory_access_flags original, memory_access_flags toExclude)
		{
			if (!std::holds_alternative<vk::AccessFlags2KHR>(original.mFlags) || !std::holds_alternative<vk::AccessFlags2KHR>(toExclude.mFlags)) {
				throw avk::runtime_error("exclude may only be used with concrete memory access set, not with auto_access nor with uninitialized values.");
			}
			return memory_access_flags{ std::get<vk::AccessFlags2KHR>(original.mFlags) & ~std::get<vk::AccessFlags2KHR>(toExclude.mFlags) };
		}

		inline bool is_included(const memory_access_flags toTest, const memory_access_flags includee)
		{
			if (!std::holds_alternative<vk::AccessFlags2KHR>(toTest.mFlags) || !std::holds_alternative<vk::AccessFlags2KHR>(includee.mFlags)) {
				throw avk::runtime_error("is_included may only be used with concrete memory access set, not with auto_access nor with uninitialized values.");
			}
			return (std::get<vk::AccessFlags2KHR>(toTest.mFlags) & std::get<vk::AccessFlags2KHR>(includee.mFlags)) == std::get<vk::AccessFlags2KHR>(includee.mFlags);
		}
#pragma endregion

#pragma region memory_access2 | memory_access_flags operators
		inline memory_dependency operator| (memory_dependency a, memory_access_flags b)
		{
			if (!std::holds_alternative<vk::AccessFlags2KHR>(a.mDst) || !std::holds_alternative<vk::AccessFlags2KHR>(b.mFlags)) {
				throw avk::runtime_error("operator| may only be used with concrete memory access set, not with auto_access nor with uninitialized values.");
			}
			return memory_dependency{ a.mSrc, std::get<vk::AccessFlags2KHR>(a.mDst) | std::get<vk::AccessFlags2KHR>(b.mFlags) };
		}

		inline memory_dependency operator& (memory_dependency a, memory_access_flags b)
		{
			if (!std::holds_alternative<vk::AccessFlags2KHR>(a.mDst) || !std::holds_alternative<vk::AccessFlags2KHR>(b.mFlags)) {
				throw avk::runtime_error("operator| may only be used with concrete memory access set, not with auto_access nor with uninitialized values.");
			}
			return memory_dependency{ a.mSrc, std::get<vk::AccessFlags2KHR>(a.mDst) & std::get<vk::AccessFlags2KHR>(b.mFlags) };
		}

		inline memory_dependency& operator |= (memory_dependency& a, memory_access_flags b)
		{
			return a = a | b;
		}

		inline memory_dependency& operator &= (memory_dependency& a, memory_access_flags b)
		{
			return a = a & b;
		}
#pragma endregion

#pragma region memory_access_flags | memory_access2 operators
		inline memory_dependency operator| (memory_access_flags a, memory_dependency b)
		{
			if (!std::holds_alternative<vk::AccessFlags2KHR>(a.mFlags) || !std::holds_alternative<vk::AccessFlags2KHR>(b.mSrc)) {
				throw avk::runtime_error("operator| may only be used with concrete memory access set, not with auto_access nor with uninitialized values.");
			}
			return memory_dependency{ std::get<vk::AccessFlags2KHR>(a.mFlags) | std::get<vk::AccessFlags2KHR>(b.mSrc), b.mDst };
		}

		inline memory_dependency operator& (memory_access_flags a, memory_dependency b)
		{
			if (!std::holds_alternative<vk::AccessFlags2KHR>(a.mFlags) || !std::holds_alternative<vk::AccessFlags2KHR>(b.mSrc)) {
				throw avk::runtime_error("operator& may only be used with concrete memory access set, not with auto_access nor with uninitialized values.");
			}
			return memory_dependency{ std::get<vk::AccessFlags2KHR>(a.mFlags) & std::get<vk::AccessFlags2KHR>(b.mSrc), b.mDst };
		}
#pragma endregion

		static constexpr auto none                                  = memory_access_flags{ vk::AccessFlagBits2KHR::eNone };
		static constexpr auto indirect_command_read                 = memory_access_flags{ vk::AccessFlagBits2KHR::eIndirectCommandRead };
		static constexpr auto index_read                            = memory_access_flags{ vk::AccessFlagBits2KHR::eIndexRead };
		static constexpr auto vertex_attribute_read                 = memory_access_flags{ vk::AccessFlagBits2KHR::eVertexAttributeRead };
		static constexpr auto uniform_read                          = memory_access_flags{ vk::AccessFlagBits2KHR::eUniformRead };
		static constexpr auto input_attachment_read                 = memory_access_flags{ vk::AccessFlagBits2KHR::eInputAttachmentRead };
		static constexpr auto shader_read                           = memory_access_flags{ vk::AccessFlagBits2KHR::eShaderRead };
		static constexpr auto shader_write                          = memory_access_flags{ vk::AccessFlagBits2KHR::eShaderWrite };
		static constexpr auto color_attachment_read                 = memory_access_flags{ vk::AccessFlagBits2KHR::eColorAttachmentRead };
		static constexpr auto color_attachment_write                = memory_access_flags{ vk::AccessFlagBits2KHR::eColorAttachmentWrite };
		static constexpr auto depth_stencil_attachment_read         = memory_access_flags{ vk::AccessFlagBits2KHR::eDepthStencilAttachmentRead };
		static constexpr auto depth_stencil_attachment_write        = memory_access_flags{ vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite };
		static constexpr auto transfer_read                         = memory_access_flags{ vk::AccessFlagBits2KHR::eTransferRead };
		static constexpr auto transfer_write                        = memory_access_flags{ vk::AccessFlagBits2KHR::eTransferWrite };
		static constexpr auto host_read                             = memory_access_flags{ vk::AccessFlagBits2KHR::eHostRead };
		static constexpr auto host_write                            = memory_access_flags{ vk::AccessFlagBits2KHR::eHostWrite };
		static constexpr auto memory_read                           = memory_access_flags{ vk::AccessFlagBits2KHR::eMemoryRead };
		static constexpr auto memory_write                          = memory_access_flags{ vk::AccessFlagBits2KHR::eMemoryWrite };
		static constexpr auto shader_sampled_read                   = memory_access_flags{ vk::AccessFlagBits2KHR::eShaderSampledRead };
		static constexpr auto shader_storage_read                   = memory_access_flags{ vk::AccessFlagBits2KHR::eShaderStorageRead };
		static constexpr auto shader_storage_write                  = memory_access_flags{ vk::AccessFlagBits2KHR::eShaderStorageWrite };
#if defined( VK_ENABLE_BETA_EXTENSIONS )
#if VK_HEADER_VERSION >= 204
		static constexpr auto video_decode_read                     = memory_access_flags{ vk::AccessFlagBits2KHR::eVideoDecodeReadKHR };
		static constexpr auto video_decode_write                    = memory_access_flags{ vk::AccessFlagBits2KHR::eVideoDecodeWriteKHR };
		static constexpr auto video_encode_read                     = memory_access_flags{ vk::AccessFlagBits2KHR::eVideoEncodeReadKHR };
		static constexpr auto video_encode_write                    = memory_access_flags{ vk::AccessFlagBits2KHR::eVideoEncodeWriteKHR };
#else
		static constexpr auto video_decode_read                     = memory_access_flags{ vk::AccessFlagBits2KHR::eVideoDecodeRead };
		static constexpr auto video_decode_write                    = memory_access_flags{ vk::AccessFlagBits2KHR::eVideoDecodeWrite };
		static constexpr auto video_encode_read                     = memory_access_flags{ vk::AccessFlagBits2KHR::eVideoEncodeRead };
		static constexpr auto video_encode_write                    = memory_access_flags{ vk::AccessFlagBits2KHR::eVideoEncodeWrite };
#endif
#endif
#if VK_HEADER_VERSION >= 180
		static constexpr auto transform_feedback_write              = memory_access_flags{ vk::AccessFlagBits2KHR::eTransformFeedbackWriteEXT };
		static constexpr auto transform_feedback_counter_read       = memory_access_flags{ vk::AccessFlagBits2KHR::eTransformFeedbackCounterReadEXT };
		static constexpr auto transform_feedback_counter_write      = memory_access_flags{ vk::AccessFlagBits2KHR::eTransformFeedbackCounterWriteEXT };
		static constexpr auto conditional_rendering_read            = memory_access_flags{ vk::AccessFlagBits2KHR::eConditionalRenderingReadEXT };
		static constexpr auto command_preprocess_read               = memory_access_flags{ vk::AccessFlagBits2KHR::eCommandPreprocessReadNV };
		static constexpr auto command_preprocess_write              = memory_access_flags{ vk::AccessFlagBits2KHR::eCommandPreprocessWriteNV };
		static constexpr auto shading_rate_image_read				= memory_access_flags{ vk::AccessFlagBits2KHR::eShadingRateImageReadNV }; 
#else
		static constexpr auto transform_feedback_write              = memory_access_flags{ vk::AccessFlagBits2KHR::eTransformFeedbackWriteExt };
		static constexpr auto transform_feedback_counter_read       = memory_access_flags{ vk::AccessFlagBits2KHR::eTransformFeedbackCounterReadExt };
		static constexpr auto transform_feedback_counter_write      = memory_access_flags{ vk::AccessFlagBits2KHR::eTransformFeedbackCounterWriteExt };
		static constexpr auto conditional_rendering_read            = memory_access_flags{ vk::AccessFlagBits2KHR::eConditionalRenderingReadExt };
		static constexpr auto command_preprocess_read               = memory_access_flags{ vk::AccessFlagBits2KHR::eCommandPreprocessReadNv };
		static constexpr auto command_preprocess_write              = memory_access_flags{ vk::AccessFlagBits2KHR::eCommandPreprocessWriteNv };
		static constexpr auto shading_rate_image_read				= memory_access_flags{ vk::AccessFlagBits2KHR::eShadingRateImageReadNv }; 
#endif
#if VK_HEADER_VERSION >= 204
		static constexpr auto fragment_shading_rate_attachment_read = memory_access_flags{ vk::AccessFlagBits2KHR::eFragmentShadingRateAttachmentReadKHR };
		static constexpr auto acceleration_structure_read           = memory_access_flags{ vk::AccessFlagBits2KHR::eAccelerationStructureReadKHR };
		static constexpr auto acceleration_structure_write          = memory_access_flags{ vk::AccessFlagBits2KHR::eAccelerationStructureWriteKHR };
#elif VK_HEADER_VERSION >= 180
		static constexpr auto fragment_shading_rate_attachment_read = memory_access_flags{ vk::AccessFlagBits2KHR::eFragmentShadingRateAttachmentRead };
		static constexpr auto acceleration_structure_read           = memory_access_flags{ vk::AccessFlagBits2KHR::eAccelerationStructureRead };
		static constexpr auto acceleration_structure_write          = memory_access_flags{ vk::AccessFlagBits2KHR::eAccelerationStructureWrite };
		static constexpr auto fragment_density_map_read             = memory_access_flags{ vk::AccessFlagBits2KHR::eFragmentDensityMapReadEXT };
		static constexpr auto color_attachment_read_noncoherent     = memory_access_flags{ vk::AccessFlagBits2KHR::eColorAttachmentReadNoncoherentEXT };
#else
		static constexpr auto fragment_shading_rate_attachment_read = memory_access_flags{ vk::AccessFlagBits2KHR::eFragmentShadingRateAttachmentRead };
		static constexpr auto acceleration_structure_read           = memory_access_flags{ vk::AccessFlagBits2KHR::eAccelerationStructureReadNv };
		static constexpr auto acceleration_structure_write          = memory_access_flags{ vk::AccessFlagBits2KHR::eAccelerationStructureWriteNv };
		static constexpr auto fragment_density_map_read             = memory_access_flags{ vk::AccessFlagBits2KHR::eFragmentDensityMapReadExt };
		static constexpr auto color_attachment_read_noncoherent     = memory_access_flags{ vk::AccessFlagBits2KHR::eColorAttachmentReadNoncoherentExt };
#endif
#if VK_HEADER_VERSION >= 204
		static constexpr auto invocation_mask_read                  = memory_access_flags{ vk::AccessFlagBits2KHR::eInvocationMaskReadHUAWEI };
#endif

		/** Automatically try to determine the single immediate preceding/succeeding access and establish a memory dependency to it.
		 *	If a specific access cannot be determined, a rather hefty memory dependency will be installed, so that
		 *	correctness is prioritized over performance.
		 *	Attention: This is equivalent to using stage::auto_accesses(1) and can lead to insufficient synchronization if the command
		 *	           to be synchronized with is not directly adjacent to this sync instruction in the list of recorded commands.
		 */
		static constexpr auto auto_access = memory_access_flags{ auto_access_t{ 1 } };

		/** Automatically try to establish a memory dependency to the given number of preceding/succeeding stages.
		 *	If specific access cannot be determined, a rather hefty memory dependency will be installed, so that
		 *	correctness is prioritized over performance.
		 */
		inline static auto auto_accesses(uint8_t aNumMaxCommands = 20) { return memory_access_flags{ auto_access_t{ aNumMaxCommands } }; }

		
		// Struct which supports stage:: but disallows monostate or auto stages
		struct memory_access_flags_precisely
		{
			memory_access_flags_precisely()
				: mAccess{ vk::AccessFlagBits2KHR::eNone }
			{}

			memory_access_flags_precisely(vk::AccessFlags2KHR aAcces)
				: mAccess{ aAcces }
			{}

			memory_access_flags_precisely(memory_access_flags aInput)
				: mAccess{ std::get<vk::AccessFlags2KHR>(aInput.mFlags) }
			{
				assert(std::holds_alternative<vk::AccessFlags2KHR>(aInput.mFlags));
			}

			memory_access_flags_precisely(memory_access_flags_precisely&&) noexcept = default;
			memory_access_flags_precisely(const memory_access_flags_precisely&) = default;

			memory_access_flags_precisely& operator=(memory_access_flags aInput)
			{
				assert(std::holds_alternative<vk::AccessFlags2KHR>(aInput.mFlags));
				mAccess = std::get<vk::AccessFlags2KHR>(aInput.mFlags);
				return *this;
			}

			memory_access_flags_precisely& operator=(memory_access_flags_precisely&&) noexcept = default;
			memory_access_flags_precisely& operator=(const memory_access_flags_precisely&) = default;
			~memory_access_flags_precisely() = default;

			operator memory_access_flags() const
			{
				return memory_access_flags{ mAccess };
			}

			vk::AccessFlags2KHR mAccess;
		};
	}
}
