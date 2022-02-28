#pragma once
#include <avk/avk.hpp>

namespace avk
{
	/** Represents an image and its associated memory
	 */
	class image_t
	{
		friend class root;

	public:
		image_t() = default;
		image_t(image_t&&) noexcept = default;
		image_t(const image_t& aOther);
		image_t& operator=(image_t&&) noexcept = default;
		image_t& operator=(const image_t&) = delete;
		~image_t() = default;

		/** Get the config which is used to created this image with the API. */
		const auto& create_info() const	{ return mCreateInfo; }
		/** Get the config which is used to created this image with the API. */
		auto& create_info()				{ return mCreateInfo; }
		/** Gets the image handle. */
		vk::Image handle() const
		{
			assert(!std::holds_alternative<std::monostate>(mImage));
			return std::holds_alternative<AVK_MEM_IMAGE_HANDLE>(mImage)
					? std::get<AVK_MEM_IMAGE_HANDLE>(mImage).resource()
					: std::get<vk::Image>(mImage);
		}
		/** Gets the memory properties, IF this instance represents an allocated image, not a wrapped one. */
		auto memory_properties() const
		{
			assert(!std::holds_alternative<std::monostate>(mImage));
			if (!std::holds_alternative<AVK_MEM_IMAGE_HANDLE>(mImage)) {
				throw avk::runtime_error("Can not determine memory property flags of a non-allocated (but wrapped) image.");
			}
			return std::get<AVK_MEM_IMAGE_HANDLE>(mImage).memory_properties();
		}

		/** Gets the width of the image */
		uint32_t width() const { return create_info().extent.width; }
		/** Gets the height of the image */
		uint32_t height() const { return create_info().extent.height; }
		/** Gets the depth of the image */
		uint32_t depth() const { return create_info().extent.depth; }
		/** Gets the format of the image */
		vk::Format format() const { return create_info().format; }

		/**	Sets a new target layout for this image, simply overwriting any previous value.
		 *	Attention: Only use if you know what you are doing!
		 */
		void set_target_layout(vk::ImageLayout aNewTargetLayout)
		{
			mTargetLayout = aNewTargetLayout;
		}

		/** Gets this image's target layout as specified during image creation. */
		auto target_layout() const { return mTargetLayout; }

		/** Sets the current image layout.
		 *	Attention: Only use if you know what you are doing!
		 */
		void set_current_layout(vk::ImageLayout aNewLayout) { mCurrentLayout = aNewLayout; }
		
		/** Gets the current image layout */
		auto current_layout() const { return mCurrentLayout; }

		/** Gets the usage config flags as specified during image creation. */
		auto usage_config() const { return mImageUsage; }

		/** Gets a struct defining the subresource range, encompassing all subresources associated with this image,
		 *	i.e. all layers, all mip-levels
		 */
		vk::ImageSubresourceRange entire_subresource_range() const;

		auto aspect_flags() const { return mAspectFlags; }

		/** Transition the image into the given layout, or (if no layout specified) into its target layout (i.e. into `target_layout()`).
		 *	Attention: IF the image is already in the requested (or target) layout, no command will be executed and `aSyncHandler` will NOT be invoked!
		 */
		std::optional<command_buffer> transition_to_layout(std::optional<vk::ImageLayout> aTargetLayout = {}, sync aSyncHandler = sync::wait_idle());

		/**	Generate all the coarser MIP levels from the current level 0.
		 *	Attention: IF the image has no MIP levels, `aSyncHandler` will NOT be invoked!
		 */
		std::optional<command_buffer> generate_mip_maps(sync aSyncHandler = sync::wait_idle());

		[[nodiscard]] const auto* root_ptr() const { return mRoot; }

	private:
		const root* mRoot;
		// The image create info which contains all the parameters for image creation
		vk::ImageCreateInfo mCreateInfo;
		// The image handle. This member will contain a valid handle only after successful image creation.
		std::variant<std::monostate, AVK_MEM_IMAGE_HANDLE, vk::Image> mImage;
		// The image's target layout
		vk::ImageLayout mTargetLayout;
		// The current image layout
		vk::ImageLayout mCurrentLayout;
		// The image_usage flags specified during creation
		image_usage mImageUsage;
		// Image aspect flags (set during creation)
		vk::ImageAspectFlags mAspectFlags;
	};

	/** Typedef representing any kind of OWNING image representations. */
	using image	= owning_resource<image_t>;

	/** Compares two `image_t`s for equality.
	 *	They are considered equal if all their handle is the same.
	 *	The config structs' data is not evaluated for equality comparison.
	 */
	static bool operator==(const image_t& left, const image_t& right)
	{
		return left.handle() == right.handle();
	}

	/** Returns `true` if the two `image_t`s are not equal. */
	static bool operator!=(const image_t& left, const image_t& right)
	{
		return !(left == right);
	}
}

namespace std // Inject hash for `ak::image_sampler_t` into std::
{
	template<> struct hash<avk::image_t>
	{
		std::size_t operator()(avk::image_t const& o) const noexcept
		{
			const VkImage vki = static_cast<const VkImage>(o.handle());
			std::size_t h = std::hash<VkImage>{}(vki);
			return h;
		}
	};
}
