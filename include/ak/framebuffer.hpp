#pragma once

namespace ak
{
	/** Represents a Vulkan framebuffer, holds the native handle and takes
	*	care about their lifetime management.
	*/
	class framebuffer_t
	{
	public:
		framebuffer_t() = default;
		framebuffer_t(framebuffer_t&&) noexcept = default;
		framebuffer_t(const framebuffer_t&) = delete;
		framebuffer_t& operator=(framebuffer_t&&) noexcept = default;
		framebuffer_t& operator=(const framebuffer_t&) = delete;
		~framebuffer_t() = default;

		static ak::owning_resource<framebuffer_t> create(renderpass aRenderpass, std::vector<ak::image_view> aImageViews, uint32_t aWidth, uint32_t aHeight, std::function<void(framebuffer_t&)> aAlterConfigBeforeCreation = {});
		static ak::owning_resource<framebuffer_t> create(std::vector<ak::attachment> aAttachments, std::vector<ak::image_view> aImageViews, uint32_t aWidth, uint32_t aHeight, std::function<void(framebuffer_t&)> aAlterConfigBeforeCreation = {});
		static ak::owning_resource<framebuffer_t> create(renderpass aRenderpass, std::vector<ak::image_view> aImageViews, std::function<void(framebuffer_t&)> aAlterConfigBeforeCreation = {});
		static ak::owning_resource<framebuffer_t> create(std::vector<ak::attachment> aAttachments, std::vector<ak::image_view> aImageViews, std::function<void(framebuffer_t&)> aAlterConfigBeforeCreation = {});

		template <typename ...ImViews>
		static ak::owning_resource<framebuffer_t> create(std::vector<ak::attachment> aAttachments, ImViews... aImViews)
		{
			std::vector<image_view> imageViews;
			(imageViews.push_back(std::move(aImViews)), ...);
			return create(std::move(aAttachments), std::move(imageViews));
		}

		template <typename ...ImViews>
		static ak::owning_resource<framebuffer_t> create(ak::renderpass aRenderpass, ImViews... aImViews)
		{
			std::vector<image_view> imageViews;
			(imageViews.push_back(std::move(aImViews)), ...);
			return create(std::move(aRenderpass), std::move(imageViews));
		}

		const auto& get_renderpass() const { return mRenderpass; }
		const auto& image_views() const { return mImageViews; }
		const auto& image_view_at(size_t i) const { return mImageViews[i]; }
		auto& get_renderpass() { return mRenderpass; }
		auto& image_views() { return mImageViews; }
		auto& image_view_at(size_t i) { return mImageViews[i]; }
		const auto& create_info() const { return mCreateInfo; }
		const auto& handle() const { return mFramebuffer.get(); }

		/**	Initializes the attachments by transferring their image layouts away from uninitialized into something useful.
		 *	You don't have to do this, but it could be very helpful in some situations, where you are going to use the
		 *	images not only for rendering into, but also maybe for displaying them in the UI.
		 */
		std::optional<command_buffer> initialize_attachments(ak::sync aSync = ak::sync::wait_idle());
		
	private:
		// Helper methods for the create methods that take attachments and image views
		static void check_and_config_attachments_based_on_views(std::vector<ak::attachment>& aAttachments, std::vector<ak::image_view>& aImageViews);
		
		renderpass mRenderpass;
		std::vector<ak::image_view> mImageViews;
		vk::FramebufferCreateInfo mCreateInfo;
		vk::UniqueFramebuffer mFramebuffer;
	};

	using framebuffer = ak::owning_resource<framebuffer_t>;

}
