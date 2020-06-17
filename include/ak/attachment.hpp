#pragma once

namespace ak
{
	/** Describes an attachment to a framebuffer or a renderpass.
	 *	It can describe color attachments as well as depth/stencil attachments
	 *	and holds some additional config parameters for these attachments.
	 */
	struct attachment
	{
		static attachment declare(std::tuple<vk::Format, vk::SampleCountFlagBits> aFormatAndSamples, on_load aLoadOp, usage_desc aUsageInSubpasses, on_store aStoreOp);
		static attachment declare(vk::Format aFormat, on_load aLoadOp, usage_desc aUsageInSubpasses, on_store aStoreOp);
		static attachment declare_for(const image_view_t& aImageView, on_load aLoadOp, usage_desc aUsageInSubpasses, on_store aStoreOp);

		attachment& set_clear_color(std::array<float, 4> aColor)    { mColorClearValue = aColor; return *this; }
		attachment& set_depth_clear_value(float aDepthClear)        { mDepthClearValue = aDepthClear; return *this; }
		attachment& set_stencil_clear_value(uint32_t aStencilClear) { mStencilClearValue = aStencilClear; return *this; }
		
		attachment& set_load_operation(on_load aLoadOp)             { mLoadOperation = aLoadOp; return *this; }
		attachment& set_store_operation(on_store aStoreOp)          { mStoreOperation = aStoreOp; return *this; }
		attachment& set_stencil_load_operation(on_load aLoadOp)     { mStencilLoadOperation = aLoadOp; return *this; }
		attachment& set_stencil_store_operation(on_store aStoreOp)  { mStencilStoreOperation = aStoreOp; return *this; }

		attachment& set_image_usage_hint(ak::image_usage aImageUsageBeforeAndAfter)
		{
			mImageUsageHintBefore = aImageUsageBeforeAndAfter;
			mImageUsageHintAfter = aImageUsageBeforeAndAfter;
			return *this;
		}
		
		attachment& set_image_usage_hints(ak::image_usage aImageUsageBefore, ak::image_usage aImageUsageAfter)
		{
			mImageUsageHintBefore = aImageUsageBefore;
			mImageUsageHintAfter = aImageUsageAfter;
			return *this;
		}
		
		/** The color/depth/stencil format of the attachment */
		auto format() const { return mFormat; }
		
		auto shall_be_presentable() const { return on_store::store_in_presentable_format == mStoreOperation; }

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

		/** Returns the stencil load operation */
		auto get_stencil_load_op() const { return mStencilLoadOperation.value_or(mLoadOperation); }
		/** Returns the stencil store operation */
		auto get_stencil_store_op() const { return mStencilStoreOperation.value_or(mStoreOperation); }

		auto clear_color() const { return mColorClearValue; }
		auto depth_clear_value() const { return mDepthClearValue; }
		auto stencil_clear_value() const { return mStencilClearValue; }

		vk::Format mFormat;
		vk::SampleCountFlagBits mSampleCount;
		on_load mLoadOperation;
		on_store mStoreOperation;
		std::optional<on_load> mStencilLoadOperation;
		std::optional<on_store> mStencilStoreOperation;
		usage_desc mSubpassUsages;
		std::array<float, 4> mColorClearValue;
		float mDepthClearValue;
		uint32_t mStencilClearValue;
		std::optional<image_usage> mImageUsageHintBefore;
		std::optional<image_usage> mImageUsageHintAfter;
	};
}
