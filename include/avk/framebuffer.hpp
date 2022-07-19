#pragma once
#include <avk/avk.hpp>

namespace avk
{
	/** Represents a Vulkan framebuffer, holds the native handle and takes
	*	care about their lifetime management.
	*/
	class framebuffer_t
	{
		friend class root;
		
	public:
		framebuffer_t() = default;
		framebuffer_t(framebuffer_t&&) noexcept = default;
		framebuffer_t(const framebuffer_t&) = delete;
		framebuffer_t& operator=(framebuffer_t&&) noexcept = default;
		framebuffer_t& operator=(const framebuffer_t&) = delete;
		~framebuffer_t() = default;

		renderpass get_renderpass() const { return mRenderpass; }
		const auto& image_views() const { return mImageViews; } // TODO: Probably remove this?!
		const image_t& image_at(size_t i) const { return mImageViews[i]->get_image(); }
		image_view image_view_at(size_t i) const { return mImageViews[i]; }
		auto& image_views() { return mImageViews; } // TODO: Probably remove this?!
		const auto& create_info() const	{ return mCreateInfo; }
		auto& create_info()				{ return mCreateInfo; }
		auto handle() const { return mFramebuffer.get(); }

	private:
		renderpass mRenderpass;
		std::vector<image_view> mImageViews;
		vk::FramebufferCreateInfo mCreateInfo;
		vk::UniqueHandle<vk::Framebuffer, DISPATCH_LOADER_CORE_TYPE> mFramebuffer;
	};

	using framebuffer = avk::owning_resource<framebuffer_t>;

}
