#pragma once
#include "avk/avk.hpp"

namespace avk
{
	/** Describes an attachment to a framebuffer or a renderpass.
	 *	It can describe color attachments as well as depth/stencil attachments
	 *	and holds some additional config parameters for these attachments.
	 */
	struct attachment
	{
		/**	Declare multisampled format and usages of an attachment across all subpasses:
		 *	@param	aFormatAndSamples	Multisampled format definition: A tuple with the format of the attachment in its first element, and with the number of samples in its second element.
		 *	@param	aLoadOp				What shall happen to the contents of the attachment during begin_renderpass?
		 *                              Possible values in namespace avk::on_load::
		 *	@param	aUsageInSubpasses	How is this attachment being used in the different subpasses of a renderpass?
		 *                              Possible values in namespace avk::usage::
		 *                              Usages for different subpasses can be defined by concatenating them using operator>>.
		 *                              Example 1: avk::usage::color(0) >> avk::usage::input(5) // Indicates that this attachment is used as color attachment at location=0 in the first subpass,
		 *                                                                                      // and as input attachment at location=5 in the second subpass
		 *                              Example 2: usage::unused >> usage::color(0)+usage::resolve_to(3) // Indicates that this attachment is unused in the first subpass,
		 *                                                                                               // and used as color attachment at location=0 in the second subpass.
		 *                                                                                               // Additionally, at the end of the second subpass, its contents are resolved into the attachment at index 3.
		 *	@param	aStoreOp			What shall happen to the contents of the attachment at end_renderpass?
		 *                              Possible values in namespace avk::on_store::
		 */
		static attachment declare(std::tuple<vk::Format, vk::SampleCountFlagBits> aFormatAndSamples, attachment_load_config aLoadOp, subpass_usages aUsageInSubpasses, attachment_store_config aStoreOp);

		/**	Declare format and usages of an attachment across all subpasses:
		 *	@param	aFormat				The format of the attachment
		 *	@param	aLoadOp				What shall happen to the contents of the attachment during begin_renderpass?
		 *                              Possible values in namespace avk::on_load::
		 *	@param	aUsageInSubpasses	How is this attachment being used in the different subpasses of a renderpass?
		 *                              Possible values in namespace avk::usage::
		 *                              Usages for different subpasses can be defined by concatenating them using operator>>.
		 *                              Example 1: avk::usage::color(0) >> avk::usage::input(5) // Indicates that this attachment is used as color attachment at location=0 in the first subpass,
		 *                                                                                      // and as input attachment at location=5 in the second subpass
		 *                              Example 2: usage::unused >> usage::color(0)+usage::resolve_to(3) // Indicates that this attachment is unused in the first subpass,
		 *                                                                                               // and used as color attachment at location=0 in the second subpass.
		 *                                                                                               // Additionally, at the end of the second subpass, its contents are resolved into the attachment at index 3.
		 *	@param	aStoreOp			What shall happen to the contents of the attachment at end_renderpass?
		 *                              Possible values in namespace avk::on_store::
		 */
		static attachment declare(vk::Format aFormat, attachment_load_config aLoadOp, subpass_usages aUsageInSubpasses, attachment_store_config aStoreOp);

		/**	Declare format and usages of an attachment across all subpasses:
		 *	@param	aImageView			The format of the attachment is copied from the given image view.
		 *	@param	aLoadOp				What shall happen to the contents of the attachment during begin_renderpass?
		 *                              Possible values in namespace avk::on_load::
		 *	@param	aUsageInSubpasses	How is this attachment being used in the different subpasses of a renderpass?
		 *                              Possible values in namespace avk::usage::
		 *                              Usages for different subpasses can be defined by concatenating them using operator>>.
		 *                              Example 1: avk::usage::color(0) >> avk::usage::input(5) // Indicates that this attachment is used as color attachment at location=0 in the first subpass,
		 *                                                                                      // and as input attachment at location=5 in the second subpass
		 *                              Example 2: usage::unused >> usage::color(0)+usage::resolve_to(3) // Indicates that this attachment is unused in the first subpass,
		 *                                                                                               // and used as color attachment at location=0 in the second subpass.
		 *                                                                                               // Additionally, at the end of the second subpass, its contents are resolved into the attachment at index 3.
		 *	@param	aStoreOp			What shall happen to the contents of the attachment at end_renderpass?
		 *                              Possible values in namespace avk::on_store::
		 */
		static attachment declare_for(const image_view_t& aImageView, attachment_load_config aLoadOp, subpass_usages aUsageInSubpasses, attachment_store_config aStoreOp);

		/**	Declare multisampled format of an attachment for a dynamic rendering pipeline. This attachment can only be used with pipeline that has dynamic rendering enabled.
	 	 *  It has fewer parameters than regular attachment since some of its values (load/store ops etc...) are set when starting the dynamic render pass
		 *  as opposed to being declared beforehand..
		 *	@param	aFormatAndSamples	Multisampled format definition: A tuple with the format of the attachment in its first element, and with the number of samples in its second element.
		 */
		static attachment declare_dynamic(std::tuple<vk::Format, vk::SampleCountFlagBits> aFormatAndSamples, subpass_usage_type aUsage);

		/**	Declare multisampled format of an attachment for a dynamic rendering pipeline. This attachment can only be used with pipeline that has dynamic rendering enabled.
	 	 *  It has fewer parameters than regular attachment since some of its values (load/store ops etc...) are set when starting the dynamic render pass
		 *  as opposed to being declared beforehand..
		 *	@param	aFormat	 The format of the attachment
		 */
		static attachment declare_dynamic(vk::Format aFormat, subpass_usage_type aUsage);

		/**	Declare multisampled format of an attachment for a dynamic rendering pipeline. This attachment can only be used with pipeline that has dynamic rendering enabled.
	 	 *  It has fewer parameters than regular attachment since some of its values (load/store ops etc...) are set when starting the dynamic render pass
		 *  as opposed to being declared beforehand..
		 *	@param	aImageView			The format of the attachment is copied from the given image view.
		 */
		static attachment declare_dynamic_for(const image_view_t& aImageView, subpass_usage_type aUsage);


		attachment& set_clear_color(std::array<float, 4> aColor) { mColorClearValue = aColor; return *this; }
		attachment& set_depth_clear_value(float aDepthClear) { mDepthClearValue = aDepthClear; return *this; }
		attachment& set_stencil_clear_value(uint32_t aStencilClear) { mStencilClearValue = aStencilClear; return *this; }

		attachment& set_load_operation(attachment_load_config aLoadOp) { mLoadOperation = aLoadOp; return *this; }
		attachment& set_store_operation(attachment_store_config aStoreOp) { mStoreOperation = aStoreOp; return *this; }
		attachment& set_stencil_load_operation(attachment_load_config aLoadOp) { mStencilLoadOperation = aLoadOp; return *this; }
		attachment& set_stencil_store_operation(attachment_store_config aStoreOp) { mStencilStoreOperation = aStoreOp; return *this; }

		/** The color/depth/stencil format of the attachment */
		auto format() const { return mFormat; }

		auto get_first_color_depth_input() const { return mSubpassUsages.first_color_depth_input_usage(); }

		auto get_last_color_depth_input() const { return mSubpassUsages.last_color_depth_input_usage(); }

		auto is_used_as_depth_stencil_attachment() const { return mSubpassUsages.contains_depth_stencil(); }

		auto is_used_as_color_attachment() const { return mSubpassUsages.contains_color(); }

		auto is_used_as_input_attachment() const { return mSubpassUsages.contains_input(); }

		/** True if the sample count is greater than 1 */
		bool is_multisampled() const { return mSampleCount != vk::SampleCountFlagBits::e1; }
		/** The sample count for this attachment. */
		auto sample_count() const { return mSampleCount; }
		/** True if a multisample resolve pass shall be set up. */
		auto is_to_be_resolved() const { return mSubpassUsages.contains_resolve(); }
		/** True if this attachment is declared for dynamic rendering pipelines ie. using one of the dynamic declare functions*/
		bool is_for_dynamic_rendering() const { return mDynamicRenderingAttachment; }

		/** Returns the stencil load operation */
		auto get_stencil_load_op() const { return mStencilLoadOperation.value_or(mLoadOperation); }
		/** Returns the stencil store operation */
		auto get_stencil_store_op() const { return mStencilStoreOperation.value_or(mStoreOperation); }

		auto clear_color() const { return mColorClearValue; }
		auto depth_clear_value() const { return mDepthClearValue; }
		auto stencil_clear_value() const { return mStencilClearValue; }

		vk::Format mFormat;
		vk::SampleCountFlagBits mSampleCount;
		attachment_load_config mLoadOperation;
		attachment_store_config mStoreOperation;
		std::optional<attachment_load_config> mStencilLoadOperation;
		std::optional<attachment_store_config> mStencilStoreOperation;
		subpass_usages mSubpassUsages;
		std::array<float, 4> mColorClearValue;
		float mDepthClearValue;
		uint32_t mStencilClearValue;
		bool mDynamicRenderingAttachment;
	};
}
