#pragma once
#include <avk/avk.hpp>

namespace avk
{
	class image_view_as_sampled_image
	{
		friend class image_view_t;
	public:
		const auto& descriptor_info() const { return mDescriptorInfo; }

	private:
		vk::DescriptorImageInfo mDescriptorInfo;
	};

	class image_view_as_input_attachment
	{
		friend class image_view_t;
	public:
		const auto& descriptor_info() const		{ return mDescriptorInfo; }
		
	private:
		vk::DescriptorImageInfo mDescriptorInfo;
	};

	class image_view_as_storage_image
	{
		friend class image_view_t;
	public:
		const auto& descriptor_info() const { return mDescriptorInfo; }

	private:
		vk::DescriptorImageInfo mDescriptorInfo;
	};
	
	/** Class representing an image view. */
	class image_view_t
	{
		friend class root;
		
		struct helper_t
		{
			helper_t(image_t aImage) : mImage{std::move(aImage)} {}
			helper_t(helper_t&&) noexcept = default;
			helper_t(const helper_t&) noexcept = default;
			helper_t& operator=(helper_t&&) noexcept = default;
			helper_t& operator=(const helper_t&) noexcept = delete;
			~helper_t() = default;
			
			image_t mImage;
		};
		
	public:
		image_view_t() = default;
		image_view_t(image_view_t&&) noexcept = default;
		image_view_t(const image_view_t&) = delete;
		image_view_t& operator=(image_view_t&&) noexcept = default;
		image_view_t& operator=(const image_view_t&) = delete;
		~image_view_t() = default;

		/** Get the config which is used to created this image view with the API. */
		const auto& create_info() const { return mCreateInfo; }
		/** Get the config which is used to created this image view with the API. */
		auto& create_info() { return mCreateInfo; }

		/** Gets the associated image or throws if no `ak::image` is associated. */
		const image_t& get_image() const
		{
			assert(!std::holds_alternative<std::monostate>(mImage));
			return std::holds_alternative<helper_t>(mImage) ? std::get<helper_t>(mImage).mImage : std::get<image>(mImage).get();
		}

		/** Gets the associated image or throws if no `ak::image` is associated. */
		image_t& get_image() 
		{
			assert(!std::holds_alternative<std::monostate>(mImage));
			return std::holds_alternative<helper_t>(mImage) ? std::get<helper_t>(mImage).mImage : std::get<image>(mImage).get();
		}
		
		/** Gets the image view's vulkan handle */
		const auto& handle() const { return mImageView.get(); }

		/** Declare that this image is intended to be used as sampled image.
		 *	@param	aImageLayout	The layout of the image during its usage as sampled image
		 */
		image_view_as_sampled_image as_sampled_image(avk::image_layout::image_layout aImageLayout) const
		{
			image_view_as_sampled_image result;
			result.mDescriptorInfo = vk::DescriptorImageInfo{}
				.setImageView(handle())
				.setImageLayout(aImageLayout.mLayout);
			return result;
		}
		
		/** Declare that this image is intended to be used as input attachment.
		 *	@param	aImageLayout	The layout of the image during its usage as sampled image
		 */
		image_view_as_input_attachment as_input_attachment(avk::image_layout::image_layout aImageLayout) const
		{
			image_view_as_input_attachment result;
			result.mDescriptorInfo = vk::DescriptorImageInfo{}
				.setImageView(handle())
				.setImageLayout(aImageLayout.mLayout);
			return result;
		}
		
		/** Declare that this image is intended to be used as storage image.
		 *	@param	aImageLayout	The layout of the image during its usage as sampled image
		 */
		image_view_as_storage_image as_storage_image(avk::image_layout::image_layout aImageLayout) const
		{
			image_view_as_storage_image result;
			result.mDescriptorInfo = vk::DescriptorImageInfo{}
				.setImageView(handle())
				.setImageLayout(aImageLayout.mLayout);
			return result;
		}

	private:
		// The "wrapped" image:
		std::variant<std::monostate, helper_t, image> mImage;
		// Config which is passed to the create call and contains all the parameters for image view creation.
		vk::ImageViewCreateInfo mCreateInfo;
		vk::ImageViewUsageCreateInfo mUsageInfo;
		// The image view's handle. This member will contain a valid handle only after successful image view creation.
		vk::UniqueHandle<vk::ImageView, DISPATCH_LOADER_CORE_TYPE> mImageView;
	};

	/** Typedef representing any kind of OWNING image view representations. */
	using image_view = owning_resource<image_view_t>;

	/** Compares two `image_view_t`s for equality.
	 *	They are considered equal if all their handles (image, image-view) are the same.
	 *	The config structs or the descriptor data is not evaluated for equality comparison.
	 */
	static bool operator==(const image_view_t& left, const image_view_t& right)
	{
		return left.handle() == right.handle()
			&& left.get_image().handle() == right.get_image().handle();
	}

	/** Returns `true` if the two `image_view_t`s are not equal. */
	static bool operator!=(const image_view_t& left, const image_view_t& right)
	{
		return !(left == right);
	}
}

namespace std // Inject hash for `ak::image_sampler_t` into std::
{
	template<> struct hash<avk::image_view_t>
	{
		std::size_t operator()(avk::image_view_t const& o) const noexcept
		{
			std::size_t h = 0;
			avk::hash_combine(h,
				static_cast<VkImageView>(o.handle()),
				static_cast<VkImage>(o.get_image().handle())
			);
			return h;
		}
	};
}
