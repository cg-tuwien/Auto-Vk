#pragma once

namespace ak
{
	// Forward declare:
	struct queue_submit_proxy;

	enum struct queue_selection_preference
	{
		/** Express preference for a specialized queue that has as few other
		 *	capabilities as possible besides the requested ones.
		 *	Use this for getting a specialized transfer-only queue.
		 *	Use this for getting the classical "async compute" (compute-only!) queue.
		 */
		specialized_queue,

		/**	Express preference for a versatile queue that has as many other
		 *	capabilities (besides the requested ones) as possible.
		 *	Use this for establishing a "one queue for everything" configuration.
		 */
		versatile_queue
	};
	
	/** Represents a device queue, storing the queue itself, 
	*	the queue family's index, and the queue's index.
	*/
	class queue
	{
	public:
		static std::vector<std::tuple<uint32_t, vk::QueueFamilyProperties>> find_queue_families_for_criteria(
			vk::PhysicalDevice aPhysicalDevice,
			vk::QueueFlags aRequiredFlags, 
			vk::QueueFlags aForbiddenFlags, 
			std::optional<vk::SurfaceKHR> aSurface
		);

		static std::vector<std::tuple<uint32_t, vk::QueueFamilyProperties>> find_best_queue_family_for(
			vk::PhysicalDevice aPhysicalDevice,
			vk::QueueFlags aRequiredFlags,
			queue_selection_preference aQueueSelectionPreference,
			std::optional<vk::SurfaceKHR> aSurface
		);

		static uint32_t select_queue_family_index(
			vk::PhysicalDevice aPhysicalDevice,
			vk::QueueFlags aRequiredFlags,
			queue_selection_preference aQueueSelectionPreference,
			std::optional<vk::SurfaceKHR> aSupportForSurface
		);
		
		/** Prepare another queue and for the given queue family index. */
		static queue prepare(
			vk::PhysicalDevice aPhysicalDevice,
			uint32_t aQueueFamilyIndex,
			uint32_t aQueueIndex,
			float aQueuePriority = 0.5f
		);

		/** Create a new queue on the logical device. */
		static queue create(uint32_t aQueueFamilyIndex, uint32_t aQueueIndex);
		/** Create a new queue on the logical device. */
		static void create(queue& aPreparedQueue);

		/** Gets the queue family index of this queue */
		auto family_index() const { return mQueueFamilyIndex; }
		/** Gets queue index (inside the queue family) of this queue. */
		auto queue_index() const { return mQueueIndex; }
		auto priority() const { return mPriority; }
		const auto& handle() const { return mQueue; }
		const auto* handle_ptr() const { return &mQueue; }

		/** TODO */
		semaphore submit_with_semaphore(command_buffer_t& aCommandBuffer);
		
		/** TODO */
		void submit(command_buffer_t& aCommandBuffer);
		
		/** TODO */
		void submit(std::vector<std::reference_wrapper<command_buffer_t>> aCommandBuffers);

		/** TODO */
		fence submit_with_fence(command_buffer_t& aCommandBuffer, std::vector<semaphore> aWaitSemaphores = {});
		
		/** TODO */
		fence submit_with_fence(std::vector<std::reference_wrapper<command_buffer_t>> aCommandBuffers, std::vector<semaphore> aWaitSemaphores = {});

		/** TODO */
		semaphore submit_and_handle_with_semaphore(command_buffer aCommandBuffer, std::vector<semaphore> aWaitSemaphores = {});
		semaphore submit_and_handle_with_semaphore(std::optional<command_buffer> aCommandBuffer, std::vector<semaphore> aWaitSemaphores = {});
		
		/** TODO */
		semaphore submit_and_handle_with_semaphore(std::vector<command_buffer> aCommandBuffers, std::vector<semaphore> aWaitSemaphores = {});
		
	private:
		uint32_t mQueueFamilyIndex;
		uint32_t mQueueIndex;
		float mPriority;
		vk::Queue mQueue;
	};

	static bool operator==(const queue& left, const queue& right)
	{
		const auto same = left.family_index() == right.family_index() && left.queue_index() == right.queue_index();
		assert(!same || left.priority() == right.priority());
		assert(!same || left.handle() == right.handle());
		return same;
	}

	struct queue_submit_proxy
	{
		queue_submit_proxy() = default;
		queue_submit_proxy(queue_submit_proxy&&) = delete;
		queue_submit_proxy(const queue_submit_proxy&) = delete;
		queue_submit_proxy& operator=(queue_submit_proxy&&) = delete;
		queue_submit_proxy& operator=(const queue_submit_proxy&) = delete;

		queue& mQueue;
		vk::SubmitInfo mSubmitInfo;
		std::vector<command_buffer> mCommandBuffers;
		std::vector<semaphore> mWaitSemaphores;
		std::vector<semaphore> mSignalSemaphores;
	};
}
