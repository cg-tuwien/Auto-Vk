#pragma once
#include <avk/avk.hpp>

namespace avk
{
	class image_sampler_t
	{
		friend class root;
	public:
		image_sampler_t() = default;
		image_sampler_t(image_sampler_t&&) noexcept = default;
		image_sampler_t(const image_sampler_t&) = delete;
		image_sampler_t& operator=(image_sampler_t&&) noexcept = default;
		image_sampler_t& operator=(const image_sampler_t&) = delete;
		~image_sampler_t() = default;

		[[nodiscard]] avk::resource_reference<const image_view_t> get_image_view() const	{ return avk::const_referenced(mImageView); }
		[[nodiscard]] avk::resource_reference<image_view_t> get_image_view()				{ return avk::referenced(mImageView); }
		[[nodiscard]] avk::resource_reference<const sampler_t> get_sampler() const			{ return avk::const_referenced(mSampler); }
		[[nodiscard]] avk::resource_reference<sampler_t> get_sampler()						{ return avk::referenced(mSampler); }
		[[nodiscard]] avk::resource_reference<const image_t> get_image() const				{ return avk::const_referenced(mImageView->get_image()); }
		[[nodiscard]] avk::resource_reference<image_t> get_image()							{ return avk::referenced(mImageView->get_image()); }
		auto view_handle() const				{ return mImageView->handle(); }
		auto image_handle() const					{ return mImageView->get_image().handle(); }
		auto sampler_handle() const				{ return mSampler->handle(); }
		auto descriptor_info() const		{ return mDescriptorInfo; }
		auto descriptor_type() const		{ return mDescriptorType; }
		/** Gets the width of the image */
		uint32_t width() const { return mImageView->get_image().width(); }
		/** Gets the height of the image */
		uint32_t height() const { return mImageView->get_image().height(); }
		/** Gets the depth of the image */
		uint32_t depth() const { return mImageView->get_image().depth(); }
		/** Gets the format of the image */
		vk::Format format() const { return mImageView->get_image().format(); }

	private:
		image_view mImageView;
		sampler mSampler;
		vk::DescriptorImageInfo mDescriptorInfo;
		vk::DescriptorType mDescriptorType;
	};

	/** Typedef representing any kind of OWNING image-sampler representations. */
	using image_sampler = owning_resource<image_sampler_t>;

	/** Compares two `image_sampler_t`s for equality.
	 *	They are considered equal if all their handles (image, image-view, sampler) are the same.
	 *	The config structs or the descriptor data is not evaluated for equality comparison.
	 */
	static bool operator==(const image_sampler_t& left, const image_sampler_t& right)
	{
		return left.view_handle() == right.view_handle()
			&& left.image_handle() == right.image_handle()
			&& left.sampler_handle() == right.sampler_handle();
	}

	/** Returns `true` if the two `image_sampler_t`s are not equal. */
	static bool operator!=(const image_sampler_t& left, const image_sampler_t& right)
	{
		return !(left == right);
	}
}

namespace std // Inject hash for `ak::image_sampler_t` into std::
{
	template<> struct hash<avk::image_sampler_t>
	{
		std::size_t operator()(avk::image_sampler_t const& o) const noexcept
		{
			std::size_t h = 0;
			avk::hash_combine(h,
				static_cast<VkImageView>(o.view_handle()),
				static_cast<VkImage>(o.image_handle()),
				static_cast<VkSampler>(o.sampler_handle())
			);
			return h;
		}
	};
}
