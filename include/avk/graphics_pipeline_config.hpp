#pragma once
#include <avk/avk.hpp>

namespace avk
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

		/** An operation how to compare values - used for specifying how depth testing compares depth values */
		enum struct compare_operation
		{
			never,
			less,
			equal,
			less_or_equal,
			greater,
			not_equal,
			greater_or_equal,
			always
		};

		/** Pipeline configuration data: Depth Test settings */
		struct depth_test
		{
			static depth_test enabled() { return depth_test{ true, compare_operation::less }; }
			static depth_test disabled() { return depth_test{ false, compare_operation::less }; }

			depth_test& set_compare_operation(compare_operation& aWhichOne) { mCompareOperation = aWhichOne; return *this; }

			auto is_enabled() const { return mEnabled; }
			auto depth_compare_operation() const { return mCompareOperation; }

			bool mEnabled;
			compare_operation mCompareOperation;
		};

		/** Pipeline configuration data: Depth Write settings */
		struct depth_write
		{
			static depth_write enabled() { return depth_write{ true }; }
			static depth_write disabled() { return depth_write{ false }; }
			bool is_enabled() const { return mEnabled; }
			bool mEnabled;
		};

		/** Viewport position and extent */
		struct viewport_depth_scissors_config
		{
			static viewport_depth_scissors_config from_pos_extend_and_depth(int32_t x, int32_t y, uint32_t width, uint32_t height, float minDepth, float maxDepth) 
			{
				return viewport_depth_scissors_config{ 
					std::array<float, 2>{{ static_cast<float>(x), static_cast<float>(y) }},
					std::array<float, 2>{{ static_cast<float>(width), static_cast<float>(height) }}, 
					minDepth, maxDepth,
					vk::Offset2D{ x, y },
					vk::Extent2D{ width, height },
					false,
					false
				}; 
			}

			static viewport_depth_scissors_config from_pos_and_extent(int32_t x, int32_t y, uint32_t width, uint32_t height) 
			{ 
				return viewport_depth_scissors_config{ 
					std::array<float, 2>{{ static_cast<float>(x), static_cast<float>(y) }},
					std::array<float, 2>{{ static_cast<float>(width), static_cast<float>(height) }}, 
					0.0f, 1.0f,
					vk::Offset2D{ x, y },
					vk::Extent2D{ width, height },
					false,
					false
				}; 
			}

			static viewport_depth_scissors_config from_extent(uint32_t width, uint32_t height) 
			{ 
				return viewport_depth_scissors_config{ 
					std::array<float, 2>{{ 0.0f, 0.0f }},
					std::array<float, 2>{{ static_cast<float>(width), static_cast<float>(height) }}, 
					0.0f, 1.0f,
					vk::Offset2D{ 0, 0 },
					vk::Extent2D{ width, height },
					false,
					false
				}; 
			}

			static viewport_depth_scissors_config from_framebuffer(const framebuffer_t& aFramebuffer);

			/** Enables dynamic viewport and scissors. */
			static viewport_depth_scissors_config dynamic(bool aDynamicViewport = true, bool aDynamicScissors = true)
			{
				return viewport_depth_scissors_config{ 
					std::array<float, 2>{{ 0.0f, 0.0f }},
					std::array<float, 2>{{ 0.0f, 0.0f }}, 
					0.0f, 1.0f,
					vk::Offset2D{ 0,  0 },
					vk::Extent2D{ 0u, 0u },
					aDynamicViewport,
					aDynamicScissors
				}; 
			}

			viewport_depth_scissors_config& enable_dynamic_viewport() { mDynamicViewportEnabled = true; return *this; }
			viewport_depth_scissors_config& disable_dynamic_viewport() { mDynamicViewportEnabled = false; return *this; }
			viewport_depth_scissors_config& enable_dynamic_scissor() { mDynamicScissorEnabled = true; return *this; }
			viewport_depth_scissors_config& disable_dynamic_scissor() { mDynamicScissorEnabled = false; return *this; }

			const auto& position() const { return mPosition; }
			auto x() const { return mPosition[0]; }
			auto y() const { return mPosition[1]; }
			const auto& dimensions() const { return mDimensions; }
			auto width() const { return mDimensions[0]; }
			auto height() const { return mDimensions[1]; }
			auto min_depth() const { return mMinDepth; }
			auto max_depth() const { return mMaxDepth; }
			const auto& scissor_offset() const { return mScissorOffset; }
			auto scissor_x() const { return mScissorOffset.x; }
			auto scissor_y() const { return mScissorOffset.y; }
			const auto& scissor_extent() const { return mScissorExtent; }
			auto scissor_width() const { return mScissorExtent.width; }
			auto scissor_height() const { return mScissorExtent.height; }
			auto is_dynamic_viewport_enabled() const { return mDynamicViewportEnabled; }
			auto is_dynamic_scissor_enabled() const { return mDynamicScissorEnabled; }

			std::array<float, 2> mPosition;
			std::array<float, 2> mDimensions;
			float mMinDepth;
			float mMaxDepth;
			vk::Offset2D mScissorOffset;
			vk::Extent2D mScissorExtent;
			bool mDynamicViewportEnabled;
			bool mDynamicScissorEnabled;
		};

		/** Pipeline configuration data: Culling Mode */
		enum struct culling_mode
		{
			 disabled,
			 cull_front_faces,
			 cull_back_faces,
			 cull_front_and_back_faces
		};

		/** Winding order of polygons */
		enum struct winding_order
		{
			counter_clockwise,
			clockwise
		};

		/** Pipeline configuration data: Culling Mode */
		struct front_face
		{
			static front_face define_front_faces_to_be_counter_clockwise() { return front_face{ winding_order::counter_clockwise }; }
			static front_face define_front_faces_to_be_clockwise() { return front_face{ winding_order::clockwise }; }

			winding_order winding_order_of_front_faces() const { return mFrontFaces; }
			winding_order winding_order_of_back_faces() const { return mFrontFaces == winding_order::counter_clockwise ? winding_order::clockwise : winding_order::counter_clockwise; }

			winding_order mFrontFaces;
		};

		/** How to draw polygons */
		enum struct polygon_drawing_mode
		{
			fill,
			line,
			point
		};

		/** Pipeline configuration data: Polygon Drawing Mode (and additional settings) */
		struct polygon_drawing
		{
			static polygon_drawing config_for_filling() 
			{ 
				return { polygon_drawing_mode::fill, 1.0f, false, 1.0f }; 
			}
			
			static polygon_drawing config_for_lines(float aLineWidth = 1.0f) 
			{ 
				return { polygon_drawing_mode::line, aLineWidth, false, 1.0f }; 
			}
			
			static polygon_drawing config_for_points(float aPointSize = 1.0f) 
			{ 
				return { polygon_drawing_mode::point, 1.0f, false, aPointSize }; 
			}

			static polygon_drawing dynamic_for_lines()
			{
				return { polygon_drawing_mode::line, 1.0f, true, 1.0f }; 
			}
			
			auto drawing_mode() const { return mDrawingMode; }
			auto line_width() const { return mLineWidth; }
			auto dynamic_line_width() const { return mDynamicLineWidth; }

			polygon_drawing_mode mDrawingMode;
			float mLineWidth;
			bool mDynamicLineWidth;

			float mPointSize;
		};

		/** How the rasterizer processes geometry */
		enum struct rasterizer_geometry_mode
		{
			rasterize_geometry,
			discard_geometry,
		};

		/** Additional depth-related parameters for the rasterizer */
		struct depth_clamp_bias
		{
			static depth_clamp_bias config_nothing_special() { return { false, false, 0.0f, 0.0f, 0.0f, false }; }
			static depth_clamp_bias config_enable_depth_bias(float pConstantFactor, float pBiasClamp, float pSlopeFactor) { return { false, true, pConstantFactor, pBiasClamp, pSlopeFactor, false }; }
			static depth_clamp_bias config_enable_clamp_and_depth_bias(float pConstantFactor, float pBiasClamp, float pSlopeFactor) { return { true, true, pConstantFactor, pBiasClamp, pSlopeFactor, false }; }
			static depth_clamp_bias dynamic() { return { false, false, 0.0f, 0.0f, 0.0f, true }; }

			auto is_clamp_to_frustum_enabled() const { return mClampDepthToFrustum; }
			auto is_depth_bias_enabled() const { return mEnableDepthBias; }
			auto bias_constant_factor() const { return mDepthBiasConstantFactor; }
			auto bias_clamp_value() const { return mDepthBiasClamp; }
			auto bias_slope_factor() const { return mDepthBiasSlopeFactor; }
			auto is_dynamic_depth_bias_enabled() const { return mEnableDynamicDepthBias; }

			bool mClampDepthToFrustum;
			bool mEnableDepthBias;
			float mDepthBiasConstantFactor;
			float mDepthBiasClamp;
			float mDepthBiasSlopeFactor;
			bool mEnableDynamicDepthBias;
		};

		/** Settings to enable/disable depth bounds and for its range */
		struct depth_bounds
		{
			static depth_bounds disable() { return {false, false, 0.0f, 1.0f}; }
			static depth_bounds enable(float aRangeFrom, float aRangeTo) { return {true, false, aRangeFrom, aRangeTo}; }
			static depth_bounds dynamic() { return {false, true, 0.0f, 1.0f}; }

			auto is_enabled() const { return mEnabled; }
			auto is_dynamic_depth_bounds_enabled() const { return mDynamic; }
			auto min_bounds() const { return mMinDeptBounds; }
			auto max_bounds() const { return mMaxDepthBounds; }

			bool mEnabled;
			bool mDynamic;
			float mMinDeptBounds;
			float mMaxDepthBounds;
		};

		/** Settings to enable/disable and configure stencil tests */
		struct stencil_test
		{
			static stencil_test disable() { return stencil_test{ false, false, {}, {} }; }
			static stencil_test enable(VkStencilOpState aStencilTestActionsFrontAndBack) { return stencil_test{ true, false, aStencilTestActionsFrontAndBack, aStencilTestActionsFrontAndBack }; }
			static stencil_test enable(VkStencilOpState aStencilTestActionsFront, VkStencilOpState aStencilTestActionsBack) { return stencil_test{ true, false, aStencilTestActionsFront, aStencilTestActionsBack }; }
			static stencil_test dynamic() { return stencil_test{ true, true, {}, {} }; }

			auto is_dynamic_enabled() const { return mDynamic; }
			
			bool mEnabled;
			bool mDynamic;
			VkStencilOpState mFrontStencilTestActions;
			VkStencilOpState mBackStencilTestActions;
		};

		/** Reference the separate color channels */
		enum struct color_channel
		{
			none		= 0x0000,
			red			= 0x0001,
			green		= 0x0002,
			blue		= 0x0004,
			alpha		= 0x0008,
			rg			= red | green,
			rgb			= red | green | blue,
			rgba		= red | green | blue | alpha
		};

		inline color_channel operator| (color_channel a, color_channel b)
		{
			typedef std::underlying_type<color_channel>::type EnumType;
			return static_cast<color_channel>(static_cast<EnumType>(a) | static_cast<EnumType>(b));
		}

		inline color_channel operator& (color_channel a, color_channel b)
		{
			typedef std::underlying_type<color_channel>::type EnumType;
			return static_cast<color_channel>(static_cast<EnumType>(a) & static_cast<EnumType>(b));
		}

		inline color_channel& operator |= (color_channel& a, color_channel b)
		{
			return a = a | b;
		}

		inline color_channel& operator &= (color_channel& a, color_channel b)
		{
			return a = a & b;
		}

		/** Different operation types for color blending */
		enum struct color_blending_operation
		{
			add,
			subtract,
			reverse_subtract,
			min,
			max
		};

		/** Different factors for color blending operations */
		enum struct blending_factor
		{
			zero,
			one,
			source_color,
			one_minus_source_color,
			destination_color,
			one_minus_destination_color,
			source_alpha,
			one_minus_source_alpha,
			destination_alpha,
			one_minus_destination_alpha,
			constant_color,
			one_minus_constant_color,
			constant_alpha,
			one_minus_constant_alpha,
			source_alpha_saturate
		};

		/** Different types operation types for `color_blending_mode::logic_operation` mode */
		enum struct blending_logic_operation
		{
			op_clear,
			op_and,
			op_and_reverse,
			op_copy,
			op_and_inverted,
			no_op,
			op_xor,
			op_or,
			op_nor,
			op_equivalent,
			op_invert,
			op_or_reverse,
			op_copy_inverted,
			op_or_inverted,
			op_nand,
			op_set
		};

		/** Color blending settings */
		struct color_blending_settings
		{
			static color_blending_settings disable_logic_operation() { return color_blending_settings{ {}, {} }; }
			static color_blending_settings config_blend_constants(const std::array<float, 4>& aValues) { return color_blending_settings{ {}, aValues }; }
			static color_blending_settings enable_logic_operation(blending_logic_operation aLogicOp) { return color_blending_settings{ aLogicOp, {} }; }

			bool is_logic_operation_enabled() const { return mLogicOpEnabled.has_value(); }
			blending_logic_operation logic_operation() const { return mLogicOpEnabled.value_or(blending_logic_operation::no_op); }
			const auto& blend_constants() const { return mBlendConstants; }

			std::optional<blending_logic_operation> mLogicOpEnabled;
			std::array<float, 4> mBlendConstants;
		};

		/** A specific color blending config for an attachment (or for all attachments) */
		struct color_blending_config
		{
			static color_blending_config disable()
			{
				return color_blending_config{ 
					{},
					false,
					color_channel::rgba,
					blending_factor::one, blending_factor::zero, color_blending_operation::add,
					blending_factor::one, blending_factor::zero, color_blending_operation::add
				};
			}

			static color_blending_config disable_blending_for_attachment(uint32_t pAttachment, color_channel pAffectedColorChannels = color_channel::rgba)
			{
				return color_blending_config{ 
					pAttachment,
					false,
					pAffectedColorChannels,
					blending_factor::one, blending_factor::zero, color_blending_operation::add,
					blending_factor::one, blending_factor::zero, color_blending_operation::add
				};
			}

			static color_blending_config enable_alpha_blending_for_all_attachments(color_channel pAffectedColorChannels = color_channel::rgba)
			{
				return color_blending_config{ 
					{},
					true,
					pAffectedColorChannels,
					blending_factor::source_alpha, blending_factor::one_minus_source_alpha, color_blending_operation::add,
					blending_factor::one, blending_factor::one_minus_source_alpha, color_blending_operation::add
				};
			}

			static color_blending_config enable_alpha_blending_for_attachment(uint32_t pAttachment, color_channel pAffectedColorChannels = color_channel::rgba)
			{
				return color_blending_config{
					pAttachment,
					true,
					pAffectedColorChannels,
					blending_factor::source_alpha, blending_factor::one_minus_source_alpha, color_blending_operation::add,
					blending_factor::one, blending_factor::one_minus_source_alpha, color_blending_operation::add
				};
			}

			static color_blending_config enable_premultiplied_alpha_blending_for_all_attachments(color_channel pAffectedColorChannels = color_channel::rgba)
			{
				return color_blending_config{ 
					{},
					true,
					pAffectedColorChannels,
					blending_factor::one, blending_factor::one_minus_source_alpha, color_blending_operation::add,
					blending_factor::one, blending_factor::one_minus_source_alpha, color_blending_operation::add
				};
			}

			static color_blending_config enable_premultiplied_alpha_blending_for_attachment(uint32_t pAttachment, color_channel pAffectedColorChannels = color_channel::rgba)
			{
				return color_blending_config{
					pAttachment,
					true,
					pAffectedColorChannels,
					blending_factor::one, blending_factor::one_minus_source_alpha, color_blending_operation::add,
					blending_factor::one, blending_factor::one_minus_source_alpha, color_blending_operation::add
				};
			}

			static color_blending_config enable_additive_for_all_attachments(color_channel pAffectedColorChannels = color_channel::rgba)
			{
				return color_blending_config{ 
					{},
					true,
					pAffectedColorChannels,
					blending_factor::one, blending_factor::one, color_blending_operation::add,
					blending_factor::one, blending_factor::one, color_blending_operation::add
				};
			}

			static color_blending_config enable_additive_for_attachment(uint32_t pAttachment, color_channel pAffectedColorChannels = color_channel::rgba)
			{
				return color_blending_config{
					pAttachment,
					true,
					pAffectedColorChannels,
					blending_factor::one, blending_factor::one, color_blending_operation::add,
					blending_factor::one, blending_factor::one, color_blending_operation::add
				};
			}

			bool has_specific_target_attachment()  const { return mTargetAttachment.has_value(); }
			auto target_attachment() const { return mTargetAttachment; }
			auto is_blending_enabled() const { return mEnabled; }
			auto affected_color_channels() const { return mAffectedColorChannels; }
			auto color_source_factor() const { return mIncomingColorFactor; }
			auto color_destination_factor() const { return mExistingColorFactor; }
			auto color_operation() const { return mColorOperation; }
			auto alpha_source_factor() const { return mIncomingAlphaFactor; }
			auto alpha_destination_factor() const { return mExistingAlphaFactor; }
			auto alpha_operation() const { return mAlphaOperation; }

			// affected color attachment
			// This value must equal the colorAttachmentCount for the subpass in which this pipeline is used.
			std::optional<uint32_t> mTargetAttachment;
			// blending enabled
			bool mEnabled;
			// affected color channels a.k.a. write mask
			color_channel mAffectedColorChannels;
			// incoming a.k.a. source color
			blending_factor mIncomingColorFactor;
			// existing a.k.a. destination color
			blending_factor mExistingColorFactor;
			// incoming*factor -operation- existing*factor
			color_blending_operation mColorOperation;
			// incoming a.k.a. source alpha
			blending_factor mIncomingAlphaFactor;
			// existing a.k.a. destination alpha
			blending_factor mExistingAlphaFactor;
			// incoming*factor -operation- existing*factor
			color_blending_operation mAlphaOperation;
		};

		/** Compares two `color_blending_config`s for equality */
		static bool operator== (const color_blending_config& left, const color_blending_config& right)
		{
			return left.mTargetAttachment		== right.mTargetAttachment
				&& left.mEnabled				== right.mEnabled
				&& left.mAffectedColorChannels	== right.mAffectedColorChannels
				&& left.mIncomingColorFactor	== right.mIncomingColorFactor
				&& left.mExistingColorFactor	== right.mExistingColorFactor
				&& left.mColorOperation			== right.mColorOperation
				&& left.mIncomingAlphaFactor	== right.mIncomingAlphaFactor
				&& left.mExistingAlphaFactor	== right.mExistingAlphaFactor
				&& left.mAlphaOperation			== right.mAlphaOperation;
		}

		/** Returns `true` if the two `color_blending_config`s are not equal */
		static bool operator!= (const color_blending_config& left, const color_blending_config& right)
		{
			return !(left == right);
		}

		/** How the vertex input is to be interpreted topology-wise */
		enum struct primitive_topology
		{
			points,
			lines,
			line_strip,
			triangles,
			triangle_strip,
			triangle_fan,
			lines_with_adjacency,
			line_strip_with_adjacency,
			triangles_with_adjacency,
			triangle_strip_with_adjacency,
			patches
		};

		/** Specify the number of control points per patch.
		 *	This setting only makes sense in conjunction with primitive_topology::patches,
		 *	which only makes sense in conjunction with tessellation shaders.
		 */
		struct tessellation_patch_control_points
		{
			uint32_t mPatchControlPoints;
		};

		/** Data about the "Sample Shading" configuration of a graphics pipeline's MSAA section */
		struct per_sample_shading_config
		{
			bool mPerSampleShadingEnabled;
			float mMinFractionOfSamplesShaded;
		};

		/** Indicate that fragment shaders shall be invoked once per fragment. */
		inline per_sample_shading_config shade_per_fragment()
		{
			return per_sample_shading_config { false, 1.0f };
		}

		/** Indicate that fragment shaders shall be invoked once per sample. */
		inline per_sample_shading_config shade_per_sample(float aMinFractionOfSamplesShaded = 1.0f)
		{
			return per_sample_shading_config { true, aMinFractionOfSamplesShaded };
		}
	}

	// Forward declare that the graphics_pipeline_t class for the context_specificaFunction
	class graphics_pipeline_t;
	
	/** Pipeline configuration data: BIG GRAPHICS PIPELINE CONFIG STRUCT */
	struct graphics_pipeline_config
	{
		graphics_pipeline_config();
		graphics_pipeline_config(graphics_pipeline_config&&) noexcept = default;
		graphics_pipeline_config(const graphics_pipeline_config&) = delete;
		graphics_pipeline_config& operator=(graphics_pipeline_config&&) noexcept = default;
		graphics_pipeline_config& operator=(const graphics_pipeline_config&) = delete;
		~graphics_pipeline_config() = default;

		cfg::pipeline_settings mPipelineSettings; // TODO: Handle settings!
		std::optional<std::tuple<renderpass, uint32_t>> mRenderPassSubpass;
		std::vector<input_binding_location_data> mInputBindingLocations;
		cfg::primitive_topology mPrimitiveTopology;
		std::vector<shader_info> mShaderInfos;
		std::vector<cfg::viewport_depth_scissors_config> mViewportDepthConfig;
		cfg::rasterizer_geometry_mode mRasterizerGeometryMode;
		cfg::polygon_drawing mPolygonDrawingModeAndConfig;
		cfg::culling_mode mCullingMode;
		cfg::front_face mFrontFaceWindingOrder;
		cfg::depth_clamp_bias mDepthClampBiasConfig;
		cfg::depth_test mDepthTestConfig;
		cfg::depth_write mDepthWriteConfig;
		cfg::depth_bounds mDepthBoundsConfig;
		std::vector<cfg::color_blending_config> mColorBlendingPerAttachment;
		cfg::color_blending_settings mColorBlendingSettings;
		std::vector<binding_data> mResourceBindings;
		std::vector<push_constant_binding_data> mPushConstantsBindings;
		std::optional<cfg::tessellation_patch_control_points> mTessellationPatchControlPoints;
		std::optional<cfg::per_sample_shading_config> mPerSampleShading;
		std::optional<cfg::stencil_test> mStencilTest;
	};

	// End of recursive variadic template handling
	inline void add_config(graphics_pipeline_config& aConfig, std::vector<avk::attachment>& aAttachments, std::function<void(graphics_pipeline_t&)>& aFunc) { /* We're done here. */ }

	// Add a specific pipeline setting to the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& aConfig, std::vector<avk::attachment>& aAttachments, std::function<void(graphics_pipeline_t&)>& aFunc, cfg::pipeline_settings aSetting, Ts... args)
	{
		aConfig.mPipelineSettings |= aSetting;
		add_config(aConfig, aAttachments, aFunc, std::move(args)...);
	}

	// Add a complete render pass to the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& aConfig, std::vector<avk::attachment>& aAttachments, std::function<void(graphics_pipeline_t&)>& aFunc, renderpass aRenderPass, uint32_t aSubpass, Ts... args)
	{
		aConfig.mRenderPassSubpass = std::move(std::make_tuple(std::move(aRenderPass), aSubpass));
		add_config(aConfig, aAttachments, aFunc, std::move(args)...);
	}

	// Add a complete render pass to the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& aConfig, std::vector<avk::attachment>& aAttachments, std::function<void(graphics_pipeline_t&)>& aFunc, renderpass aRenderPass, Ts... args)
	{
		aConfig.mRenderPassSubpass = std::move(std::make_tuple(std::move(aRenderPass), 0u)); // Default to the first subpass if none is specified
		add_config(aConfig, aAttachments, aFunc, std::move(args)...);
	}

	// Add a renderpass attachment to the (temporary) attachments vector and build renderpass afterwards
	template <typename... Ts>
	void add_config(graphics_pipeline_config& aConfig, std::vector<avk::attachment>& aAttachments, std::function<void(graphics_pipeline_t&)>& aFunc, avk::attachment aAttachment, Ts... args)
	{
		aAttachments.push_back(std::move(aAttachment));
		add_config(aConfig, aAttachments, aFunc, std::move(args)...);
	}

	// Add an input binding location to the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& aConfig, std::vector<avk::attachment>& aAttachments, std::function<void(graphics_pipeline_t&)>& aFunc, input_binding_location_data aInputBinding, Ts... args)
	{
		aConfig.mInputBindingLocations.push_back(std::move(aInputBinding));
		add_config(aConfig, aAttachments, aFunc, std::move(args)...);
	}

	// Set the topology of the input attributes
	template <typename... Ts>
	void add_config(graphics_pipeline_config& aConfig, std::vector<avk::attachment>& aAttachments, std::function<void(graphics_pipeline_t&)>& aFunc, cfg::primitive_topology aTopology, Ts... args)
	{
		aConfig.mPrimitiveTopology = aTopology;
		add_config(aConfig, aAttachments, aFunc, std::move(args)...);
	}

	// Add a shader to the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& aConfig, std::vector<avk::attachment>& aAttachments, std::function<void(graphics_pipeline_t&)>& aFunc, shader_info aShaderInfo, Ts... args)
	{
		aConfig.mShaderInfos.push_back(std::move(aShaderInfo));
		add_config(aConfig, aAttachments, aFunc, std::move(args)...);
	}

	// Accept a string and assume it refers to a shader file
	template <typename... Ts>
	void add_config(graphics_pipeline_config& aConfig, std::vector<avk::attachment>& aAttachments, std::function<void(graphics_pipeline_t&)>& aFunc, std::string_view aShaderPath, Ts... args)
	{
		aConfig.mShaderInfos.push_back(shader_info::describe(std::string(aShaderPath)));
		add_config(aConfig, aAttachments, aFunc, std::move(args)...);
	}

	// Set the depth test behavior in the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& aConfig, std::vector<avk::attachment>& aAttachments, std::function<void(graphics_pipeline_t&)>& aFunc, cfg::depth_test aDepthTestConfig, Ts... args)
	{
		aConfig.mDepthTestConfig = std::move(aDepthTestConfig);
		add_config(aConfig, aAttachments, aFunc, std::move(args)...);
	}

	// Set the depth write behavior in the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& aConfig, std::vector<avk::attachment>& aAttachments, std::function<void(graphics_pipeline_t&)>& aFunc, cfg::depth_write aDepthWriteConfig, Ts... args)
	{
		aConfig.mDepthWriteConfig = std::move(aDepthWriteConfig);
		add_config(aConfig, aAttachments, aFunc, std::move(args)...);
	}

	// Add a viewport, depth, and scissors entry to the pipeline configuration
	template <typename... Ts>
	void add_config(graphics_pipeline_config& aConfig, std::vector<avk::attachment>& aAttachments, std::function<void(graphics_pipeline_t&)>& aFunc, cfg::viewport_depth_scissors_config aViewportDepthScissorsConfig, Ts... args)
	{
		aConfig.mViewportDepthConfig.push_back(std::move(aViewportDepthScissorsConfig));
		add_config(aConfig, aAttachments, aFunc, std::move(args)...);
	}

	// Set the culling mode in the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& aConfig, std::vector<avk::attachment>& aAttachments, std::function<void(graphics_pipeline_t&)>& aFunc, cfg::culling_mode aCullingMode, Ts... args)
	{
		aConfig.mCullingMode = aCullingMode;
		add_config(aConfig, aAttachments, aFunc, std::move(args)...);
	}

	// Set the definition of front faces in the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& aConfig, std::vector<avk::attachment>& aAttachments, std::function<void(graphics_pipeline_t&)>& aFunc, cfg::front_face aFrontFace, Ts... args)
	{
		aConfig.mFrontFaceWindingOrder = std::move(aFrontFace);
		add_config(aConfig, aAttachments, aFunc, std::move(args)...);
	}

	// Set how to draw polygons in the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& aConfig, std::vector<avk::attachment>& aAttachments, std::function<void(graphics_pipeline_t&)>& aFunc, cfg::polygon_drawing aPolygonDrawingConfig, Ts... args)
	{
		aConfig.mPolygonDrawingModeAndConfig = std::move(aPolygonDrawingConfig);
		add_config(aConfig, aAttachments, aFunc, std::move(args)...);
	}

	// Set how the rasterizer handles geometry in the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& aConfig, std::vector<avk::attachment>& aAttachments, std::function<void(graphics_pipeline_t&)>& aFunc, cfg::rasterizer_geometry_mode aRasterizerGeometryMode, Ts... args)
	{
		aConfig.mRasterizerGeometryMode = aRasterizerGeometryMode;
		add_config(aConfig, aAttachments, aFunc, std::move(args)...);
	}

	// Sets if there should be some special depth handling in the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& aConfig, std::vector<avk::attachment>& aAttachments, std::function<void(graphics_pipeline_t&)>& aFunc, cfg::depth_clamp_bias aDepthSettings, Ts... args)
	{
		aConfig.mDepthClampBiasConfig = aDepthSettings;
		add_config(aConfig, aAttachments, aFunc, std::move(args)...);
	}

	// Sets some color blending parameters in the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& aConfig, std::vector<avk::attachment>& aAttachments, std::function<void(graphics_pipeline_t&)>& aFunc, cfg::color_blending_settings aColorBlendingSettings, Ts... args)
	{
		aConfig.mColorBlendingSettings = aColorBlendingSettings;
		add_config(aConfig, aAttachments, aFunc, std::move(args)...);
	}

	// Sets some color blending parameters in the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& aConfig, std::vector<avk::attachment>& aAttachments, std::function<void(graphics_pipeline_t&)>& aFunc, cfg::color_blending_config aColorBlendingConfig, Ts... args)
	{
		aConfig.mColorBlendingPerAttachment.push_back(std::move(aColorBlendingConfig));
		add_config(aConfig, aAttachments, aFunc, std::move(args)...);
	}

	// Sets patch control points for tessellation pipelines
	template <typename... Ts>
	void add_config(graphics_pipeline_config& aConfig, std::vector<avk::attachment>& aAttachments, std::function<void(graphics_pipeline_t&)>& aFunc, cfg::tessellation_patch_control_points aPatchControlPoints, Ts... args)
	{
		aConfig.mTessellationPatchControlPoints = aPatchControlPoints;
		add_config(aConfig, aAttachments, aFunc, std::move(args)...);
	}

	// Configure sample shading parameters
	template <typename... Ts>
	void add_config(graphics_pipeline_config& aConfig, std::vector<avk::attachment>& aAttachments, std::function<void(graphics_pipeline_t&)>& aFunc, cfg::per_sample_shading_config aPerSampleShadingConfig, Ts... args)
	{
		aConfig.mPerSampleShading = aPerSampleShadingConfig;
		add_config(aConfig, aAttachments, aFunc, std::move(args)...);
	}

	// Configure stencil test parameters
	template <typename... Ts>
	void add_config(graphics_pipeline_config& aConfig, std::vector<avk::attachment>& aAttachments, std::function<void(graphics_pipeline_t&)>& aFunc, cfg::stencil_test aStencilTestConfig, Ts... args)
	{
		aConfig.mStencilTest = aStencilTestConfig;
		add_config(aConfig, aAttachments, aFunc, std::move(args)...);
	}

	// Add a resource binding to the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& aConfig, std::vector<avk::attachment>& aAttachments, std::function<void(graphics_pipeline_t&)>& aFunc, binding_data aResourceBinding, Ts... args)
	{
		aConfig.mResourceBindings.push_back(std::move(aResourceBinding));
		add_config(aConfig, aAttachments, aFunc, std::move(args)...);
	}

	// Add a push constants binding to the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& aConfig, std::vector<avk::attachment>& aAttachments, std::function<void(graphics_pipeline_t&)>& aFunc, push_constant_binding_data aPushConstBinding, Ts... args)
	{
		aConfig.mPushConstantsBindings.push_back(std::move(aPushConstBinding));
		add_config(aConfig, aAttachments, aFunc, std::move(args)...);
	}

	// Add an config-alteration function to the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& aConfig, std::vector<avk::attachment>& aAttachments, std::function<void(graphics_pipeline_t&)>& aFunc, std::function<void(graphics_pipeline_t&)> aAlterConfigBeforeCreation, Ts... args)
	{
		aFunc = std::move(aAlterConfigBeforeCreation);
		add_config(aConfig, aAttachments, aFunc, std::move(args)...);
	}
}


namespace std // Inject hash for `ak::cfg::color_blending_config` into std::
{
	template<> struct hash<avk::cfg::color_blending_config>
	{
		std::size_t operator()(avk::cfg::color_blending_config const& o) const noexcept
		{
			std::size_t h = 0;
			avk::hash_combine(h,
				o.mTargetAttachment,
				o.mEnabled,
				static_cast<std::underlying_type<avk::cfg::color_channel>::type>(o.mAffectedColorChannels),
				static_cast<std::underlying_type<avk::cfg::blending_factor>::type>(o.mIncomingColorFactor),
				static_cast<std::underlying_type<avk::cfg::blending_factor>::type>(o.mExistingColorFactor),
				static_cast<std::underlying_type<avk::cfg::color_blending_operation>::type>(o.mColorOperation),
				static_cast<std::underlying_type<avk::cfg::blending_factor>::type>(o.mIncomingAlphaFactor),
				static_cast<std::underlying_type<avk::cfg::blending_factor>::type>(o.mExistingAlphaFactor),
				static_cast<std::underlying_type<avk::cfg::color_blending_operation>::type>(o.mAlphaOperation)
			);
			return h;
		}
	};
}
