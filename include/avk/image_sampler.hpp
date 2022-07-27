#pragma once
#include <avk/avk.hpp>

namespace avk
{
	class combined_image_sampler_descriptor_info
	{
		friend class image_sampler_t;
	public:
		const auto& descriptor_info() const { return mDescriptorInfo; }

	private:
		vk::DescriptorImageInfo mDescriptorInfo;
	};

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

		[[nodiscard]] image_view     get_image_view() const	{ return mImageView; }
		[[nodiscard]] sampler        get_sampler()    const { return mSampler; }
		[[nodiscard]] const image_t& get_image()      const	{ return mImageView->get_image(); }
		auto view_handle() const				{ return mImageView->handle(); }
		auto image_handle() const				{ return mImageView->get_image().handle(); }
		auto sampler_handle() const				{ return mSampler->handle(); }
		/** Gets the width of the image */
		uint32_t width() const { return mImageView->get_image().width(); }
		/** Gets the height of the image */
		uint32_t height() const { return mImageView->get_image().height(); }
		/** Gets the depth of the image */
		uint32_t depth() const { return mImageView->get_image().depth(); }
		/** Gets the format of the image */
		vk::Format format() const { return mImageView->get_image().format(); }

		/** Declare that this image and sampler are to be used as "combined image sampler"
		 *	@param	aImageLayout	The layout of the image during its usage as combined image sampler
		 */
		combined_image_sampler_descriptor_info as_combined_image_sampler(avk::layout::image_layout aImageLayout) const
		{
			combined_image_sampler_descriptor_info result;
			result.mDescriptorInfo = vk::DescriptorImageInfo{}
				.setImageView(view_handle())
				.setSampler(sampler_handle())
				.setImageLayout(aImageLayout.mLayout);
			return result;
		}


	private:
		image_view mImageView;
		sampler mSampler;
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
