#pragma once
#include "avk/avk.hpp"

namespace avk
{
	// Forward declaration:
	class queue;

	/** A synchronization object which allows GPU->GPU synchronization */
	class semaphore_t
	{
		friend class root;
		
	public:
		semaphore_t();
		semaphore_t(semaphore_t&&) noexcept = default;
		semaphore_t(const semaphore_t&) = delete;
		semaphore_t& operator=(semaphore_t&&) noexcept = default;
		semaphore_t& operator=(const semaphore_t&) = delete;
		~semaphore_t();

		/** Set a custom deleter function.
		 *	This is often used for resource cleanup, e.g. a buffer which can be deleted when this semaphore is destroyed.
		 */
		template <typename F>
		semaphore_t& set_custom_deleter(F&& aDeleter) noexcept
		{
			if (mCustomDeleter.has_value()) {
				// There is already a custom deleter! Make sure that this stays alive as well.
				mCustomDeleter = [
					existingDeleter = std::move(mCustomDeleter.value()),
					additionalDeleter = std::forward<F>(aDeleter)
				]() {
					additionalDeleter();
					existingDeleter();
				};
			}
			else {
				mCustomDeleter = std::forward<F>(aDeleter);
			}
			return *this;
		}

		semaphore_t& handle_lifetime_of(any_owning_resource_t aResource);

		const auto& create_info() const	{ return mCreateInfo; }
		auto& create_info()				{ return mCreateInfo; }
		const auto& handle() const { return mSemaphore.get(); }
		const auto* handle_addr() const { return &mSemaphore.get(); }

		// timeline semaphore specific functions

		// returns the current value of the timeline semaphore
		const uint64_t query_current_value() const;
		// sets the timeline semaphore to the specified value
		void signal(uint64_t aNewValue) const;
		/** \brief Waits on host until the timiline semaphore reaches the given value or the timeout(in nanoseconds) happens.
		* \return Value of type vk::Result containing information about whether the wait operation succeeded, or the timeout has been triggered.
		*/
		vk::Result wait_until_signalled(uint64_t aRequiredValue, std::optional<uint64_t> aTimeout = {}) const;
		/** \brief Waits on host until the condition specified with the parameters is met.
		* \param aSemaphores Vector of timeline semaphores that should be waited on. All semaphores are required to be owned by the same logical device.
		* \param aTimestamps Vector of payload values to wait on. Is required to have the same size as aSemaphores. The n-th value in aTimestamps corresponds to the n-th entry in aSemaphores.
		* \param aWaitOnAll (optional) If true, waits until ALL semaphores have reached their target timestamps. If false, waits until ANY semaphore has reached its target timestamp.
		* \param aTimeout (optional) Defines a timeout (in nanoseconds) after which the function returns regardless of the semaphore state.
		* \return Value of type vk::Result containing information about whether the wait operation succeeded, or the timeout has been triggered.
		*/
		static vk::Result wait_until_signalled(const std::vector<const semaphore_t*> &aSemaphores, const std::vector<uint64_t> &aTimestamps, bool aWaitOnAll = true, std::optional<uint64_t> aTimeout = {});
		/** \brief Waits on host until the condition specified with the parameters is met.
		* \param aDevice The logical device owning all referenced timeline semaphores.
		* \param aInfo Struct containing all relevant information about the wait operation.
		* \param aTimeout (optional) Defines a timeout (in nanoseconds) after which the function returns regardless of the semaphore state.
		* \return Value of type vk::Result containing information about whether the wait operation succeeded, or the timeout has been triggered.
		*/
		static vk::Result wait_until_signalled(const vk::Device &aDevice, const vk::SemaphoreWaitInfo &aInfo, std::optional<uint64_t> aTimeout = {});

	private:
		// The semaphore config struct:
		vk::SemaphoreCreateInfo mCreateInfo;
		// The semaphore handle:
		vk::UniqueHandle<vk::Semaphore, DISPATCH_LOADER_CORE_TYPE> mSemaphore;

		/** A custom deleter function called upon destruction of this semaphore */
		std::optional<avk::unique_function<void()>> mCustomDeleter;

		std::vector<any_owning_resource_t> mLifetimeHandledResources;
	};

	// Typedef for a variable representing an owner of a semaphore
	using semaphore = avk::owning_resource<semaphore_t>;
}
