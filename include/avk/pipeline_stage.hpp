#pragma once
#include "avk/avk.hpp"

namespace avk
{
	class command_buffer_t;

	namespace stage
	{
		using auto_stage_t = uint8_t;

		/**	To define an execution_dependency, use operator>> with two pipeline_stage_flags values!
		 *	There are multiple such pipeline_stage_flags values prepared in the avk::stage namespace.
		 *
		 *	Example:
		 *	  // Create an execution dependency between copy commands and fragment shader stages:
		 *	  avk::stage::copy >> avk::stage::fragment_shader
		 */
		struct execution_dependency
		{
			std::variant<std::monostate, vk::PipelineStageFlags2KHR, auto_stage_t> mSrc;
			std::variant<std::monostate, vk::PipelineStageFlags2KHR, auto_stage_t> mDst;
		};

		struct pipeline_stage_flags
		{
			std::variant<std::monostate, vk::PipelineStageFlags2KHR, auto_stage_t> mFlags;
		};

		inline execution_dependency operator>> (pipeline_stage_flags a, pipeline_stage_flags b)
		{
			return execution_dependency{ a.mFlags, b.mFlags };
		}

#pragma region pipeline_stage_flags operators
		inline pipeline_stage_flags operator| (pipeline_stage_flags a, pipeline_stage_flags b)
		{
			if (!std::holds_alternative<vk::PipelineStageFlags2KHR>(a.mFlags) || !std::holds_alternative<vk::PipelineStageFlags2KHR>(b.mFlags)) {
				throw avk::runtime_error("operator| may only be used with concrete pipeline stages set, not with auto_stage nor with uninitialized values.");
			}
			return pipeline_stage_flags{ std::get<vk::PipelineStageFlags2KHR>(a.mFlags) | std::get<vk::PipelineStageFlags2KHR>(b.mFlags) };
		}

		inline pipeline_stage_flags operator& (pipeline_stage_flags a, pipeline_stage_flags b)
		{
			if (!std::holds_alternative<vk::PipelineStageFlags2KHR>(a.mFlags) || !std::holds_alternative<vk::PipelineStageFlags2KHR>(b.mFlags)) {
				throw avk::runtime_error("operator& may only be used with concrete pipeline stages set, not with auto_stage nor with uninitialized values.");
			}
			return pipeline_stage_flags{ std::get<vk::PipelineStageFlags2KHR>(a.mFlags) & std::get<vk::PipelineStageFlags2KHR>(b.mFlags) };
		}

		inline pipeline_stage_flags& operator |= (pipeline_stage_flags& a, pipeline_stage_flags b)
		{
			return a = a | b;
		}

		inline pipeline_stage_flags& operator &= (pipeline_stage_flags& a, pipeline_stage_flags b)
		{
			return a = a & b;
		}

		inline pipeline_stage_flags exclude(pipeline_stage_flags original, pipeline_stage_flags toExclude)
		{
			if (!std::holds_alternative<vk::PipelineStageFlags2KHR>(original.mFlags) || !std::holds_alternative<vk::PipelineStageFlags2KHR>(toExclude.mFlags)) {
				throw avk::runtime_error("exclude may only be used with concrete pipeline stages set, not with auto_stage nor with uninitialized values.");
			}
			return pipeline_stage_flags{ std::get<vk::PipelineStageFlags2KHR>(original.mFlags) & ~std::get<vk::PipelineStageFlags2KHR>(toExclude.mFlags) };
		}

		inline bool is_included(const pipeline_stage_flags toTest, const pipeline_stage_flags includee)
		{
			if (!std::holds_alternative<vk::PipelineStageFlags2KHR>(toTest.mFlags) || !std::holds_alternative<vk::PipelineStageFlags2KHR>(includee.mFlags)) {
				throw avk::runtime_error("is_included may only be used with concrete pipeline stages set, not with auto_stage nor with uninitialized values.");
			}
			return (std::get<vk::PipelineStageFlags2KHR>(toTest.mFlags) & std::get<vk::PipelineStageFlags2KHR>(includee.mFlags)) == std::get<vk::PipelineStageFlags2KHR>(includee.mFlags);
		}
#pragma endregion

#pragma region pipeline_stage2 | pipeline_stage_flags operators
		inline execution_dependency operator| (execution_dependency a, pipeline_stage_flags b)
		{
			if (!std::holds_alternative<vk::PipelineStageFlags2KHR>(a.mDst) || !std::holds_alternative<vk::PipelineStageFlags2KHR>(b.mFlags)) {
				throw avk::runtime_error("operator| may only be used with concrete pipeline stages set, not with auto_stage nor with uninitialized values.");
			}
			return execution_dependency{ a.mSrc, std::get<vk::PipelineStageFlags2KHR>(a.mDst) | std::get<vk::PipelineStageFlags2KHR>(b.mFlags) };
		}

		inline execution_dependency operator& (execution_dependency a, pipeline_stage_flags b)
		{
			if (!std::holds_alternative<vk::PipelineStageFlags2KHR>(a.mDst) || !std::holds_alternative<vk::PipelineStageFlags2KHR>(b.mFlags)) {
				throw avk::runtime_error("operator& may only be used with concrete pipeline stages set, not with auto_stage nor with uninitialized values.");
			}
			return execution_dependency{ a.mSrc, std::get<vk::PipelineStageFlags2KHR>(a.mDst) & std::get<vk::PipelineStageFlags2KHR>(b.mFlags) };
		}

		inline execution_dependency& operator |= (execution_dependency& a, pipeline_stage_flags b)
		{
			return a = a | b;
		}

		inline execution_dependency& operator &= (execution_dependency& a, pipeline_stage_flags b)
		{
			return a = a & b;
		}
#pragma endregion

#pragma region pipeline_stage_flags | pipeline_stage2 operators
		inline execution_dependency operator| (pipeline_stage_flags a, execution_dependency b)
		{
			if (!std::holds_alternative<vk::PipelineStageFlags2KHR>(a.mFlags) || !std::holds_alternative<vk::PipelineStageFlags2KHR>(b.mSrc)) {
				throw avk::runtime_error("operator| may only be used with concrete pipeline stages set, not with auto_stage nor with uninitialized values.");
			}
			return execution_dependency{ std::get<vk::PipelineStageFlags2KHR>(a.mFlags) | std::get<vk::PipelineStageFlags2KHR>(b.mSrc), b.mDst };
		}

		inline execution_dependency operator& (pipeline_stage_flags a, execution_dependency b)
		{
			if (!std::holds_alternative<vk::PipelineStageFlags2KHR>(a.mFlags) || !std::holds_alternative<vk::PipelineStageFlags2KHR>(b.mSrc)) {
				throw avk::runtime_error("operator& may only be used with concrete pipeline stages set, not with auto_stage nor with uninitialized values.");
			}
			return execution_dependency{ std::get<vk::PipelineStageFlags2KHR>(a.mFlags) & std::get<vk::PipelineStageFlags2KHR>(b.mSrc), b.mDst };
		}
#pragma endregion

		static constexpr auto none                             = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eNone };
		static constexpr auto top_of_pipe                      = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eTopOfPipe };
		static constexpr auto draw_indirect                    = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eDrawIndirect };
		static constexpr auto vertex_input                     = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eVertexInput };
		static constexpr auto vertex_shader                    = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eVertexShader };
		static constexpr auto tessellation_control_shader      = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eTessellationControlShader };
		static constexpr auto tessellation_evaluation_shader   = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eTessellationEvaluationShader };
		static constexpr auto geometry_shader                  = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eGeometryShader };
		static constexpr auto fragment_shader                  = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eFragmentShader };
		static constexpr auto early_fragment_tests             = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eEarlyFragmentTests };
		static constexpr auto late_fragment_tests              = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eLateFragmentTests };
		static constexpr auto color_attachment_output          = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eColorAttachmentOutput };
		static constexpr auto compute_shader                   = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eComputeShader };
		static constexpr auto all_transfer                     = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eAllTransfer };
		static constexpr auto transfer                         = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eTransfer };
		static constexpr auto bottom_of_pipe                   = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eBottomOfPipe };
		static constexpr auto host                             = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eHost };
		static constexpr auto all_graphics                     = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eAllGraphics };
		static constexpr auto all_commands                     = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eAllCommands };
		static constexpr auto copy                             = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eCopy };
		static constexpr auto resolve                          = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eResolve };
		static constexpr auto blit                             = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eBlit };
		static constexpr auto clear                            = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eClear };
		static constexpr auto index_input                      = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eIndexInput };
		static constexpr auto vertex_attribute_input           = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eVertexAttributeInput };
		static constexpr auto pre_rasterization_shaders        = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::ePreRasterizationShaders };
#if defined( VK_ENABLE_BETA_EXTENSIONS )
#if VK_HEADER_VERSION >= 204
		static constexpr auto video_decode                     = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eVideoDecodeKHR };
		static constexpr auto video_encode                     = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eVideoEncodeKHR };
#else
		static constexpr auto video_decode                     = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eVideoDecode };
		static constexpr auto video_encode                     = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eVideoEncode };
#endif
#endif
#if VK_HEADER_VERSION >= 180
		static constexpr auto transform_feedback               = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eTransformFeedbackEXT };
		static constexpr auto conditional_rendering            = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eConditionalRenderingEXT };
		static constexpr auto command_preprocess               = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eCommandPreprocessNV };
		static constexpr auto shading_rate_image               = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eShadingRateImageNV };
#else
		static constexpr auto transform_feedback               = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eTransformFeedbackExt };
		static constexpr auto conditional_rendering            = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eConditionalRenderingExt };
		static constexpr auto command_preprocess               = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eCommandPreprocessNv };
		static constexpr auto shading_rate_image               = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eShadingRateImageNv };
#endif 
#if VK_HEADER_VERSION >= 204
		static constexpr auto fragment_shading_rate_attachment = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eFragmentShadingRateAttachmentKHR };
		static constexpr auto acceleration_structure_build     = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eAccelerationStructureBuildKHR };
		static constexpr auto ray_tracing_shader               = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eRayTracingShaderKHR };
#else
		static constexpr auto fragment_shading_rate_attachment = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eFragmentShadingRateAttachment };
		static constexpr auto acceleration_structure_build     = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eAccelerationStructureBuild };
		static constexpr auto ray_tracing_shader               = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eRayTracingShader };
#endif
#if VK_HEADER_VERSION >= 180
		static constexpr auto fragment_density_process         = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eFragmentDensityProcessEXT };
		static constexpr auto task_shader                      = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eTaskShaderNV };
		static constexpr auto mesh_shader                      = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eMeshShaderNV };
#else
		static constexpr auto fragment_density_process         = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eFragmentDensityProcessExt };
		static constexpr auto task_shader                      = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eTaskShaderNv };
		static constexpr auto mesh_shader                      = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eMeshShaderNv };
#endif
#if VK_HEADER_VERSION >= 204
		static constexpr auto subpass_shading                  = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eSubpassShadingHUAWEI };
		static constexpr auto invocation_mask                  = pipeline_stage_flags{ vk::PipelineStageFlagBits2KHR::eInvocationMaskHUAWEI };
#endif

		/** Automatically try to determine the single immediate preceding/succeeding stage and establish a synchronization dependency to it.
		 *	If a specific stage cannot be determined, a rather hefty synchronization dependency will be installed, so that
		 *	correctness is prioritized over performance.
		 *	Attention: This is equivalent to using stage::auto_stages(1) and can lead to insufficient synchronization if the command
		 *	           to be synchronized with is not directly adjacent to this sync instruction in the list of recorded commands.
		 */
		static constexpr auto auto_stage                       = pipeline_stage_flags{ auto_stage_t{ 1 } };

		/** Automatically try to establish a synchronization dependency to the given number of preceding/succeeding stages.
		 *	If specific stages cannot be determined, a rather hefty synchronization dependency will be installed, so that
		 *	correctness is prioritized over performance.
		 */
		inline static auto auto_stages(uint8_t aNumMaxCommands = 20) { return pipeline_stage_flags{ auto_stage_t{ aNumMaxCommands } }; }


		// Struct which supports stage:: but disallows monostate or auto stages
		struct pipeline_stage_flags_precisely
		{
			pipeline_stage_flags_precisely()
				: mStage{ vk::PipelineStageFlagBits2KHR::eNone }
			{}

			pipeline_stage_flags_precisely(vk::PipelineStageFlags2KHR aStages)
				: mStage{ aStages }
			{}

			pipeline_stage_flags_precisely(pipeline_stage_flags aInput)
				: mStage{ std::get<vk::PipelineStageFlags2KHR>(aInput.mFlags) }
			{
				assert(std::holds_alternative<vk::PipelineStageFlags2KHR>(aInput.mFlags));
			}

			pipeline_stage_flags_precisely(pipeline_stage_flags_precisely&&) noexcept = default;
			pipeline_stage_flags_precisely(const pipeline_stage_flags_precisely&) = default;

			pipeline_stage_flags_precisely& operator=(pipeline_stage_flags aInput)
			{
				assert(std::holds_alternative<vk::PipelineStageFlags2KHR>(aInput.mFlags));
				mStage = std::get<vk::PipelineStageFlags2KHR>(aInput.mFlags);
				return *this;
			}

			pipeline_stage_flags_precisely& operator=(pipeline_stage_flags_precisely&&) noexcept = default;
			pipeline_stage_flags_precisely& operator=(const pipeline_stage_flags_precisely&) = default;
			~pipeline_stage_flags_precisely() = default;

			operator pipeline_stage_flags() const
			{
				return pipeline_stage_flags{ mStage };
			}

			vk::PipelineStageFlags2KHR mStage;
		};

	}

}
