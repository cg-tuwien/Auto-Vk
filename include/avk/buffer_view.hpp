#pragma once
#include <avk/avk.hpp>

namespace avk
{
	/**	A helper-class representing a descriptor to a given buffer view,
	 *	containing the descriptor type and the descriptor info.
	 *
	 *	If a buffer has multiple meta data, it can not always be automatically
	 *	determined which meta data is the right one to be used to infer the
	 *	descriptor type. This helper class solves this problem.
	 */
	class buffer_view_descriptor
	{
		friend class buffer_view_t;
		
	public:
		auto descriptor_type() const { return mDescriptorType; }
		const auto& descriptor_info() const { return mDescriptorInfo; }
		const auto& view_handle() const { return mBufferViewHandle; }
		
	private:
		vk::DescriptorType mDescriptorType;
		vk::DescriptorBufferInfo mDescriptorInfo;
		vk::BufferView mBufferViewHandle;
	};
	
	/** Class representing a buffer view, which "wraps" a uniform texel buffer or a storage texel buffer */
	class buffer_view_t
	{
		friend class root;
		
	public:
		buffer_view_t() = default;
		buffer_view_t(const buffer_view_t&) = delete;
		buffer_view_t(buffer_view_t&&) noexcept = default;
		buffer_view_t& operator=(const buffer_view_t&) = delete;
		buffer_view_t& operator=(buffer_view_t&&) noexcept = default;
		~buffer_view_t() = default;

		/** Get the config which is used to created this image view with the API. */
		const auto& create_info() const { return mCreateInfo; }
		/** Get the config which is used to created this image view with the API. */
		auto& create_info() { return mCreateInfo; }

		/** Gets the buffer handle which this view has been created for. */
		vk::Buffer buffer_handle() const;
		/** Gets the buffer's config */
		const vk::BufferCreateInfo& buffer_create_info() const;
		
		/** Gets the buffer view's vulkan handle */
		const auto& view_handle() const { return mBufferView.get(); }

		/** Gets the descriptor type from the wrapped buffer, using the buffer's
		 *	meta data at the specified index.
		 */
		vk::DescriptorType descriptor_type(size_t aMetaDataIndex = 0) const;

		/** Gets the descriptor type from the wrapped buffer, using the given meta
		 *	data type Meta and an optional meta data offset.
		 *	@tparam		Meta				Buffer's meta data type
		 *	@param		aMetaDataOffset		Optional offset into buffer's meta data types.
		 *									I.e., e.g., take n-th uniform_buffer_meta meta data entry.
		 */
		template <typename Meta>
		vk::DescriptorType descriptor_type(size_t aMetaDataOffset = 0) const
		{
			if (std::holds_alternative<buffer>(mBuffer)) {
				return std::get<buffer>(mBuffer)->meta<Meta>(aMetaDataOffset).descriptor_type().value();
			}
			throw avk::runtime_error("Which descriptor type?");
		}

		/**	Search for the given meta data of type Meta, and build a
		 *	buffer_view_descriptor instance with descriptor info and
		 *	descriptor type set.
		 */
		template <typename Meta>
		auto get_buffer_view_descriptor() const
		{
			if (std::holds_alternative<buffer>(mBuffer)) {
				buffer_view_descriptor result;
				result.mDescriptorInfo = std::get<buffer>(mBuffer)->descriptor_info();
				result.mDescriptorType = std::get<buffer>(mBuffer)->meta<Meta>().descriptor_type().value();
				result.mBufferViewHandle = view_handle();
				return result;
			}
			throw avk::runtime_error("Which descriptor type?");
		}

		/** Get a buffer_view_descriptor for binding this buffer as a uniform texel buffer view. */
		auto as_uniform_texel_buffer_view() const { return get_buffer_view_descriptor<uniform_texel_buffer_meta>(); }
		/** Get a buffer_view_descriptor for binding this buffer as a storage texel buffer view. */
		auto as_storage_texel_buffer_view() const { return get_buffer_view_descriptor<storage_texel_buffer_meta>(); }

	private:
		
		// Owning XOR non-owning handle to a buffer.
		std::variant<buffer, std::tuple<vk::Buffer, vk::BufferCreateInfo>> mBuffer;
		// Config which is passed to the create call and contains all the parameters for buffer view creation.
		vk::BufferViewCreateInfo mCreateInfo;
		// The image view's handle. This member will contain a valid handle only after successful image view creation.
		vk::UniqueBufferView mBufferView;
	};

	/** Typedef representing any kind of OWNING image view representations. */
	using buffer_view = avk::owning_resource<buffer_view_t>;

	/** Compares two `buffer_view_t`s for equality.
	 *	They are considered equal if all their handles (buffer, buffer-view) are the same.
	 *	The config structs or the descriptor data is not evaluated for equality comparison.
	 */
	static bool operator==(const buffer_view_t& left, const buffer_view_t& right)
	{
		return left.view_handle() == right.view_handle()
			&& left.buffer_handle() == right.buffer_handle();
	}

	/** Returns `true` if the two `buffer_view_t`s are not equal. */
	static bool operator!=(const buffer_view_t& left, const buffer_view_t& right)
	{
		return !(left == right);
	}
}

namespace std // Inject hash for `ak::image_sampler_t` into std::
{
	template<> struct hash<avk::buffer_view_t>
	{
		std::size_t operator()(avk::buffer_view_t const& o) const noexcept
		{
			std::size_t h = 0;
			avk::hash_combine(h,
				static_cast<VkBufferView>(o.view_handle()),
				static_cast<VkBuffer>(o.buffer_handle())
			);
			return h;
		}
	};
}

