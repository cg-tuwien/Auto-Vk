#pragma once
#include <avk/avk.hpp>

namespace avk
{
	/** A class which represents a Vulkan renderpass and all its attachments,
	 *	subpasses and subpass dependencies
	 */
	class renderpass_t
	{
		friend class root;
		
		struct subpass_data
		{
			subpass_data() = default;
			subpass_data(subpass_data&&) noexcept = default;
			subpass_data(const subpass_data&) = default;
			subpass_data& operator=(subpass_data&&) noexcept = default;
			subpass_data& operator=(const subpass_data&) = default;
			~subpass_data() = default;
			
			// Ordered list of input attachments
			std::vector<vk::AttachmentReference2KHR> mOrderedInputAttachmentRefs;

			// The ordered list of color attachments (ordered by shader-location).
			std::vector<vk::AttachmentReference2KHR> mOrderedColorAttachmentRefs;

			// The ordered list of depth attachments. Actually, only one or zero are supported.
			std::vector<vk::AttachmentReference2KHR> mOrderedDepthStencilAttachmentRefs;

			// The ordered list of attachments that shall be resolved.
			// The length of this list must be zero or the same length as the color attachments.
			std::vector<vk::AttachmentReference2KHR> mOrderedResolveAttachmentRefs;

			// The list of attachments that are to be preserved
			std::vector<uint32_t> mPreserveAttachments;
		};
		
	public:
		renderpass_t() = default;
		renderpass_t(renderpass_t&&) noexcept = default;
		renderpass_t(const renderpass_t&) = delete;
		renderpass_t& operator=(renderpass_t&&) noexcept = default;
		renderpass_t& operator=(const renderpass_t&) = delete;
		~renderpass_t() = default;

		auto number_of_attachment_descriptions() const { return mAttachmentDescriptions.size(); }
		auto attachment_descriptions() const { return mAttachmentDescriptions; }
		auto clear_values() const { return mClearValues; }

		const auto& subpasses() const { return mSubpasses; }
		const auto& subpass_dependencies() const { return mSubpassDependencies; }

		auto& subpasses() { return mSubpasses; }
		auto& subpass_dependencies() { return mSubpassDependencies; }

		bool is_input_attachment(uint32_t aSubpassId, size_t aAttachmentIndex) const;
		bool is_color_attachment(uint32_t aSubpassId, size_t aAttachmentIndex) const;
		bool is_depth_stencil_attachment(uint32_t aSubpassId, size_t aAttachmentIndex) const;
		bool is_resolve_attachment(uint32_t aSubpassId, size_t aAttachmentIndex) const;
		bool is_preserve_attachment(uint32_t aSubpassId, size_t aAttachmentIndex) const;

		const std::vector<vk::AttachmentReference2KHR>& input_attachments_for_subpass(uint32_t aSubpassId);
		const std::vector<vk::AttachmentReference2KHR>& color_attachments_for_subpass(uint32_t aSubpassId);
		const std::vector<vk::AttachmentReference2KHR>& depth_stencil_attachments_for_subpass(uint32_t aSubpassId);
		const std::vector<vk::AttachmentReference2KHR>& resolve_attachments_for_subpass(uint32_t aSubpassId);
		const std::vector<uint32_t>& preserve_attachments_for_subpass(uint32_t aSubpassId);

		const auto& create_info() const	{ return mCreateInfo; }
		auto& create_info()				{ return mCreateInfo; }
		auto handle() const { return mRenderPass.get(); }

	private:
		// All the attachments to this renderpass
		std::vector<vk::AttachmentDescription2KHR> mAttachmentDescriptions;

		// All the clear values
		std::vector<vk::ClearValue> mClearValues;

		// Subpass data
		std::vector<subpass_data> mSubpassData;

		// Subpass descriptions
		std::vector<vk::SubpassDescription2KHR> mSubpasses;

		// Dependencies between internal and external subpasses
		avk::subpass_dependencies mSubpassDependencies;

		// The native handle
		vk::UniqueHandle<vk::RenderPass, DISPATCH_LOADER_EXT_TYPE> mRenderPass;

		// CreateInfo structure
		vk::RenderPassCreateInfo2KHR mCreateInfo;
	};

	using renderpass = avk::owning_resource<renderpass_t>;
	
}