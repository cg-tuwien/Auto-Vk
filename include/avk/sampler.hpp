#pragma once
#include <avk/avk.hpp>

namespace avk
{
	/** Sampler which is used to configure how to sample from an image. */
	class sampler_t
	{
		friend class root;
	public:
		sampler_t() = default;
		sampler_t(sampler_t&&) noexcept = default;
		sampler_t(const sampler_t&) = delete;
		sampler_t& operator=(sampler_t&&) noexcept = default;
		sampler_t& operator=(const sampler_t&) = delete;
		~sampler_t() = default;

		const auto& create_info() const { return mCreateInfo; }
		auto& create_info()				{ return mCreateInfo; }
		const auto& handle() const { return mSampler.get(); }
		const auto& descriptor_info() const		{ return mDescriptorInfo; }
		const auto& descriptor_type() const		{ return mDescriptorType; }

	private:
		// Sampler creation configuration
		vk::SamplerCreateInfo mCreateInfo;
		// Sampler handle. It will contain a valid handle only after successful sampler creation.
		vk::UniqueSampler mSampler;
		vk::DescriptorImageInfo mDescriptorInfo;
		vk::DescriptorType mDescriptorType;
	};

	/** Typedef representing any kind of OWNING sampler representations. */
	using sampler = owning_resource<sampler_t>;

	/** Compares two `sampler_t`s for equality.
	 *	They are considered equal if their handles are the same.
	 *	The config structs' data is not evaluated for equality comparison.
	 */
	static bool operator==(const sampler_t& left, const sampler_t& right)
	{
		return left.handle() == right.handle();
	}

	/** Returns `true` if the two `sampler_t`s are not equal. */
	static bool operator!=(const sampler_t& left, const sampler_t& right)
	{
		return !(left == right);
	}
}

namespace std // Inject hash for `ak::sampler_t` into std::
{
	template<> struct hash<avk::sampler_t>
	{
		std::size_t operator()(avk::sampler_t const& o) const noexcept
		{
			std::size_t h = 0;
			avk::hash_combine(h, static_cast<VkSampler>(o.handle()));
			return h;
		}
	};
}
