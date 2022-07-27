#pragma once
#include <avk/avk.hpp>

namespace avk
{
	/** A synchronization object which allows GPU->CPU synchronization */
	class fence_t
	{
		friend class root;
		
	public:
		fence_t() = default;
		fence_t(const fence_t&) = delete;
		fence_t(fence_t&&) noexcept = default;
		fence_t& operator=(const fence_t&) = delete;
		fence_t& operator=(fence_t&&) noexcept = default;
		~fence_t();
		
		template <typename F>
		fence_t& set_custom_deleter(F&& aDeleter) noexcept
		{
			if (mCustomDeleter) {
				// There is already a custom deleter! Make sure that this stays alive as well.
				mCustomDeleter = [
					existingDeleter = std::move(mCustomDeleter),
					additionalDeleter = std::forward<F>(aDeleter)
				]() {};
			}
			else {
				mCustomDeleter = std::forward<F>(aDeleter);
			}
			return *this;
		}

		fence_t& handle_lifetime_of(any_owning_resource_t aResource);

		const auto& create_info() const { return mCreateInfo; }
		auto& create_info()				{ return mCreateInfo; }
		const auto& handle() const { return mFence.get(); }
		const auto* handle_ptr() const { return &mFence.get(); }

		void wait_until_signalled(std::optional<uint64_t> aTimeout = {}) const;
		void reset();

	private:
		vk::FenceCreateInfo mCreateInfo;
		vk::UniqueHandle<vk::Fence, DISPATCH_LOADER_CORE_TYPE> mFence;

		// --- Some advanced features of a fence object ---

		/** A custom deleter function called upon destruction of this semaphore */
		std::optional<avk::unique_function<void()>> mCustomDeleter;

		std::vector<any_owning_resource_t> mLifetimeHandledResources;
	};

	using fence = avk::owning_resource<fence_t>;
}
