#pragma once
#include <avk/avk.hpp>

namespace avk
{
	enum struct memory_access
	{
		indirect_command_data_read_access			= 0x00000001,
		index_buffer_read_access					= 0x00000002,
		vertex_buffer_read_access					= 0x00000004,
		uniform_buffer_read_access					= 0x00000008,
		input_attachment_read_access				= 0x00000010,
		shader_buffers_and_images_read_access		= 0x00000020,
		shader_buffers_and_images_write_access		= 0x00000040,
		color_attachment_read_access				= 0x00000080,
		color_attachment_write_access				= 0x00000100,
		depth_stencil_attachment_read_access		= 0x00000200,
		depth_stencil_attachment_write_access		= 0x00000400,
		transfer_read_access						= 0x00000800,
		transfer_write_access						= 0x00001000,
		host_read_access						    = 0x00002000,
		host_write_access						    = 0x00004000,
		any_read_access								= 0x00008000,
		any_write_access							= 0x00010000,
		transform_feedback_write_access				= 0x00020000,
		transform_feedback_counter_read_access		= 0x00040000,
		transform_feedback_counter_write_access		= 0x00080000,
		conditional_rendering_predicate_read_access = 0x00100000,
		command_preprocess_read_access				= 0x00200000,
		command_preprocess_write_access				= 0x00400000,
		color_attachment_noncoherent_read_access	= 0x00800000,
		shading_rate_image_read_access				= 0x01000000,
		acceleration_structure_read_access			= 0x02000000,
		acceleration_structure_write_access			= 0x04000000,
		fragment_density_map_attachment_read_access = 0x08000000,

		any_vertex_input_read_access				= index_buffer_read_access | vertex_buffer_read_access,
		any_shader_input_read_access				= any_vertex_input_read_access | input_attachment_read_access | uniform_buffer_read_access | shader_buffers_and_images_read_access,
		any_graphics_read_access					= any_shader_input_read_access | indirect_command_data_read_access | color_attachment_read_access | color_attachment_noncoherent_read_access | depth_stencil_attachment_read_access,
		any_graphics_basic_write_access				= color_attachment_write_access | depth_stencil_attachment_write_access,
		any_graphics_extended_write_access			= any_graphics_basic_write_access | shader_buffers_and_images_write_access | transform_feedback_write_access | transform_feedback_counter_write_access,
		shader_buffers_and_images_any_access		= shader_buffers_and_images_read_access | shader_buffers_and_images_write_access,
		any_buffer_read_access						= index_buffer_read_access | vertex_buffer_read_access | uniform_buffer_read_access | shader_buffers_and_images_read_access,
		color_attachment_any_access					= color_attachment_read_access | color_attachment_write_access,
		depth_stencil_attachment_any_access			= depth_stencil_attachment_read_access | depth_stencil_attachment_write_access,
		transfer_any_access							= transfer_read_access | transfer_write_access,
		host_any_access								= host_read_access | host_write_access,
		any_access									= any_read_access | any_write_access,
		any_device_read_access_to_image				= shader_buffers_and_images_read_access | color_attachment_read_access | depth_stencil_attachment_read_access | transfer_read_access | color_attachment_noncoherent_read_access | shading_rate_image_read_access,
		any_device_write_access_to_image			= shader_buffers_and_images_write_access | color_attachment_write_access | depth_stencil_attachment_write_access | transfer_write_access,
		any_device_access_to_image					= shader_buffers_and_images_any_access | color_attachment_any_access | depth_stencil_attachment_any_access | transfer_any_access | color_attachment_noncoherent_read_access | shading_rate_image_read_access,
		command_process_any_access					= command_preprocess_read_access | command_preprocess_write_access,
		acceleration_structure_any_access			= acceleration_structure_read_access | acceleration_structure_write_access
	};

	inline memory_access operator| (memory_access a, memory_access b)
	{
		typedef std::underlying_type<memory_access>::type EnumType;
		return static_cast<memory_access>(static_cast<EnumType>(a) | static_cast<EnumType>(b));
	}

	inline memory_access operator& (memory_access a, memory_access b)
	{
		typedef std::underlying_type<memory_access>::type EnumType;
		return static_cast<memory_access>(static_cast<EnumType>(a) & static_cast<EnumType>(b));
	}

	inline memory_access& operator |= (memory_access& a, memory_access b)
	{
		return a = a | b;
	}

	inline memory_access& operator &= (memory_access& a, memory_access b)
	{
		return a = a & b;
	}

	inline memory_access exclude(memory_access original, memory_access toExclude)
	{
		typedef std::underlying_type<memory_access>::type EnumType;
		return static_cast<memory_access>(static_cast<EnumType>(original) & (~static_cast<EnumType>(toExclude)));
	}

	inline bool is_included(const memory_access toTest, const memory_access includee)
	{
		return (toTest & includee) == includee;
	}

	inline bool is_read_access(memory_access aValue)
	{
		return (aValue & (memory_access::indirect_command_data_read_access			
						| memory_access::index_buffer_read_access					
						| memory_access::vertex_buffer_read_access					
						| memory_access::uniform_buffer_read_access					
						| memory_access::input_attachment_read_access				
						| memory_access::shader_buffers_and_images_read_access		
						| memory_access::color_attachment_read_access				
						| memory_access::depth_stencil_attachment_read_access		
						| memory_access::transfer_read_access						
						| memory_access::host_read_access						    
						| memory_access::any_read_access							
						| memory_access::transform_feedback_counter_read_access		
						| memory_access::conditional_rendering_predicate_read_access
						| memory_access::command_preprocess_read_access				
						| memory_access::color_attachment_noncoherent_read_access	
						| memory_access::shading_rate_image_read_access				
						| memory_access::acceleration_structure_read_access			
						| memory_access::fragment_density_map_attachment_read_access)
				) == aValue;
	}

	// Constructur is noexcept => throws on usage if the value is invalid
	class read_memory_access
	{
	public:
		read_memory_access(memory_access aValue) noexcept : mMemoryAccess{aValue} {}
		read_memory_access(const read_memory_access&) noexcept = default;
		read_memory_access(read_memory_access&&) noexcept = default;
		read_memory_access& operator=(memory_access aValue) noexcept { mMemoryAccess = aValue; return *this; }
		read_memory_access& operator=(const read_memory_access&) noexcept = default;
		read_memory_access& operator=(read_memory_access&&) noexcept = default;
		~read_memory_access() = default;

		explicit operator memory_access() const;
		memory_access value() const;
		
	private:
		void validate_or_throw() const;
		
		memory_access mMemoryAccess;
	};

	// Constructur is noexcept => throws on usage if the value is invalid
	class write_memory_access
	{
	public:
		write_memory_access(memory_access aValue) noexcept : mMemoryAccess{aValue} {}
		write_memory_access(const write_memory_access&) noexcept = default;
		write_memory_access(write_memory_access&&) noexcept = default;
		write_memory_access& operator=(memory_access aValue) noexcept { mMemoryAccess = aValue; return *this; }
		write_memory_access& operator=(const write_memory_access&) noexcept = default;
		write_memory_access& operator=(write_memory_access&&) noexcept = default;
		~write_memory_access() = default;

		explicit operator memory_access() const;
		memory_access value() const;
		
	private:
		void validate_or_throw() const;
		
		memory_access mMemoryAccess;
	};

	namespace access
	{
		using auto_access_t = uint8_t;

		struct memory_access2 
		{
			std::variant<std::monostate, vk::AccessFlags2KHR, auto_access_t> mSrc;
			std::variant<std::monostate, vk::AccessFlags2KHR, auto_access_t> mDst;
		};

		struct memory_access_flags
		{
			std::variant<std::monostate, vk::AccessFlags2KHR, auto_access_t> mFlags;
		};

		inline memory_access2 operator>> (memory_access_flags a, memory_access_flags b)
		{
			return memory_access2{ a.mFlags, b.mFlags };
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
		inline memory_access2 operator| (memory_access2 a, memory_access_flags b)
		{
			if (!std::holds_alternative<vk::AccessFlags2KHR>(a.mDst) || !std::holds_alternative<vk::AccessFlags2KHR>(b.mFlags)) {
				throw avk::runtime_error("operator| may only be used with concrete memory access set, not with auto_access nor with uninitialized values.");
			}
			return memory_access2{ a.mSrc, std::get<vk::AccessFlags2KHR>(a.mDst) | std::get<vk::AccessFlags2KHR>(b.mFlags) };
		}

		inline memory_access2 operator& (memory_access2 a, memory_access_flags b)
		{
			if (!std::holds_alternative<vk::AccessFlags2KHR>(a.mDst) || !std::holds_alternative<vk::AccessFlags2KHR>(b.mFlags)) {
				throw avk::runtime_error("operator| may only be used with concrete memory access set, not with auto_access nor with uninitialized values.");
			}
			return memory_access2{ a.mSrc, std::get<vk::AccessFlags2KHR>(a.mDst) & std::get<vk::AccessFlags2KHR>(b.mFlags) };
		}

		inline memory_access2& operator |= (memory_access2& a, memory_access_flags b)
		{
			return a = a | b;
		}

		inline memory_access2& operator &= (memory_access2& a, memory_access_flags b)
		{
			return a = a & b;
		}
#pragma endregion

#pragma region memory_access_flags | memory_access2 operators
		inline memory_access2 operator| (memory_access_flags a, memory_access2 b)
		{
			if (!std::holds_alternative<vk::AccessFlags2KHR>(a.mFlags) || !std::holds_alternative<vk::AccessFlags2KHR>(b.mSrc)) {
				throw avk::runtime_error("operator| may only be used with concrete memory access set, not with auto_access nor with uninitialized values.");
			}
			return memory_access2{ std::get<vk::AccessFlags2KHR>(a.mFlags) | std::get<vk::AccessFlags2KHR>(b.mSrc), b.mDst };
		}

		inline memory_access2 operator& (memory_access_flags a, memory_access2 b)
		{
			if (!std::holds_alternative<vk::AccessFlags2KHR>(a.mFlags) || !std::holds_alternative<vk::AccessFlags2KHR>(b.mSrc)) {
				throw avk::runtime_error("operator& may only be used with concrete memory access set, not with auto_access nor with uninitialized values.");
			}
			return memory_access2{ std::get<vk::AccessFlags2KHR>(a.mFlags) & std::get<vk::AccessFlags2KHR>(b.mSrc), b.mDst };
		}
#pragma endregion

		static const auto none                                  = memory_access_flags{ vk::AccessFlagBits2KHR::eNone };
		static const auto indirect_command_read                 = memory_access_flags{ vk::AccessFlagBits2KHR::eIndirectCommandRead };
		static const auto index_read                            = memory_access_flags{ vk::AccessFlagBits2KHR::eIndexRead };
		static const auto vertex_attribute_read                 = memory_access_flags{ vk::AccessFlagBits2KHR::eVertexAttributeRead };
		static const auto uniform_read                          = memory_access_flags{ vk::AccessFlagBits2KHR::eUniformRead };
		static const auto input_attachment_read                 = memory_access_flags{ vk::AccessFlagBits2KHR::eInputAttachmentRead };
		static const auto shader_read                           = memory_access_flags{ vk::AccessFlagBits2KHR::eShaderRead };
		static const auto shader_write                          = memory_access_flags{ vk::AccessFlagBits2KHR::eShaderWrite };
		static const auto color_attachment_read                 = memory_access_flags{ vk::AccessFlagBits2KHR::eColorAttachmentRead };
		static const auto color_attachment_write                = memory_access_flags{ vk::AccessFlagBits2KHR::eColorAttachmentWrite };
		static const auto transform_read                        = memory_access_flags{ vk::AccessFlagBits2KHR::eTransferRead };
		static const auto transform_write                       = memory_access_flags{ vk::AccessFlagBits2KHR::eTransferWrite };
		static const auto host_read                             = memory_access_flags{ vk::AccessFlagBits2KHR::eHostRead };
		static const auto host_write                            = memory_access_flags{ vk::AccessFlagBits2KHR::eHostWrite };
		static const auto memory_read                           = memory_access_flags{ vk::AccessFlagBits2KHR::eMemoryRead };
		static const auto memory_write                          = memory_access_flags{ vk::AccessFlagBits2KHR::eMemoryWrite };
		static const auto shader_sampled_read                   = memory_access_flags{ vk::AccessFlagBits2KHR::eShaderSampledRead };
		static const auto shader_storage_read                   = memory_access_flags{ vk::AccessFlagBits2KHR::eShaderStorageRead };
		static const auto shader_storage_write                  = memory_access_flags{ vk::AccessFlagBits2KHR::eShaderStorageWrite };
		static const auto video_decode_read                     = memory_access_flags{ vk::AccessFlagBits2KHR::eVideoDecodeRead };
		static const auto video_decode_write                    = memory_access_flags{ vk::AccessFlagBits2KHR::eVideoDecodeWrite };
		static const auto video_encode_read                     = memory_access_flags{ vk::AccessFlagBits2KHR::eVideoEncodeRead };
		static const auto video_encode_write                    = memory_access_flags{ vk::AccessFlagBits2KHR::eVideoEncodeWrite };
		static const auto transform_feedback_write              = memory_access_flags{ vk::AccessFlagBits2KHR::eTransformFeedbackWriteEXT };
		static const auto transform_feedback_counter_read       = memory_access_flags{ vk::AccessFlagBits2KHR::eTransformFeedbackCounterReadEXT };
		static const auto transform_feedback_counter_write      = memory_access_flags{ vk::AccessFlagBits2KHR::eTransformFeedbackCounterWriteEXT };
		static const auto conditional_rendering_read            = memory_access_flags{ vk::AccessFlagBits2KHR::eConditionalRenderingReadEXT };
		static const auto command_preprocess_read               = memory_access_flags{ vk::AccessFlagBits2KHR::eCommandPreprocessReadNV };
		static const auto command_preprocess_write              = memory_access_flags{ vk::AccessFlagBits2KHR::eCommandPreprocessWriteNV };
		static const auto fragment_shading_rate_attachment_read = memory_access_flags{ vk::AccessFlagBits2KHR::eFragmentShadingRateAttachmentRead };
		static const auto shading_rate_image_read               = memory_access_flags{ vk::AccessFlagBits2KHR::eShadingRateImageReadNV };
		static const auto acceleration_structure_read           = memory_access_flags{ vk::AccessFlagBits2KHR::eAccelerationStructureRead };
		static const auto acceleration_structure_write          = memory_access_flags{ vk::AccessFlagBits2KHR::eAccelerationStructureWrite };
		static const auto fragment_density_map_read             = memory_access_flags{ vk::AccessFlagBits2KHR::eFragmentDensityMapReadEXT };
		static const auto color_attachment_read_noncoherent     = memory_access_flags{ vk::AccessFlagBits2KHR::eColorAttachmentReadNoncoherentEXT };
#if VK_HEADER_VERSION > 182
		static const auto invocation_mask_read                  = memory_access_flags{ vk::AccessFlagBits2KHR::eInvocationMaskReadHUAWEI };
#endif

		/** Automatically try to determine the preceding/succeeding access and establish a memory dependency to it.
		 *	If a specific access cannot be determined, a rather hefty memory dependency will be installed, so that
		 *	correctness is prioritized over performance.
		 */
		static const auto auto_access = memory_access_flags{ auto_access_t{ 0 } };

		/** Automatically try to establish a memory dependency to the given number of preceding/succeeding stages.
		 *	If specific access cannot be determined, a rather hefty memory dependency will be installed, so that
		 *	correctness is prioritized over performance.
		 */
		inline static auto auto_accesses(uint8_t aNumMaxCommands = 100) { return memory_access_flags{ auto_access_t{ aNumMaxCommands } }; }

	}
}
