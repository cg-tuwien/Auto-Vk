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

		/**	Set a queue where this fence is designated to be submitted to.
		 *	This is only used for keeping the information of the queue. 
		 *	There is no impact on any internal functionality whether or not a designated queue has been set.
		 */
		fence_t& set_designated_queue(queue& _Queue);

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

		const auto& create_info() const { return mCreateInfo; }
		auto& create_info()				{ return mCreateInfo; }
		const auto& handle() const { return mFence.get(); }
		const auto* handle_ptr() const { return &mFence.get(); }
		auto has_designated_queue() const { return nullptr != mQueue; }
		auto* designated_queue() const { return mQueue; }

		void wait_until_signalled(std::optional<uint64_t> aTimeout = {}) const;
		void reset();

	private:
		vk::FenceCreateInfo mCreateInfo;
		vk::UniqueHandle<vk::Fence, DISPATCH_LOADER_CORE_TYPE> mFence;
		queue* mQueue;

		// --- Some advanced features of a fence object ---

		/** A custom deleter function called upon destruction of this semaphore */
		std::optional<avk::unique_function<void()>> mCustomDeleter;
	};

	using fence = avk::owning_resource<fence_t>;
}
