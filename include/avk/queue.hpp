#pragma once
#include <avk/avk.hpp>

#include "avk.hpp"

namespace avk
{
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
		queue() = default;
		queue(const queue&) = delete;
		queue(queue&&) noexcept = default;
		queue& operator=(const queue&) = delete;
		queue& operator=(queue&&) noexcept = default;
		~queue() = default;
		
		static std::vector<std::tuple<uint32_t, vk::QueueFamilyProperties>> find_queue_families_for_criteria(
			vk::PhysicalDevice aPhysicalDevice,
			vk::QueueFlags aRequiredFlags, 
			vk::QueueFlags aForbiddenFlags, 
			std::optional<vk::SurfaceKHR> aSurface = {}
		);

		static std::vector<std::tuple<uint32_t, vk::QueueFamilyProperties>> find_best_queue_family_for(
			vk::PhysicalDevice aPhysicalDevice,
			vk::QueueFlags aRequiredFlags,
			queue_selection_preference aQueueSelectionPreference,
			std::optional<vk::SurfaceKHR> aSurface = {}
		);

		static uint32_t select_queue_family_index(
			vk::PhysicalDevice aPhysicalDevice,
			vk::QueueFlags aRequiredFlags,
			queue_selection_preference aQueueSelectionPreference,
			std::optional<vk::SurfaceKHR> aSupportForSurface = {}
		);
		
		/** Prepare another queue and for the given queue family index. */
		static queue prepare(
			avk::root* aRoot,
			uint32_t aQueueFamilyIndex,
			uint32_t aQueueIndex,
			float aQueuePriority = 0.5f
		);

		/**	Input: Iterators [begin end) to avk::queue elements.
		 */
		template <typename It>
		static std::tuple<
			std::vector<vk::DeviceQueueCreateInfo>,
			std::vector<std::vector<float>>
		> get_queue_config_for_DeviceCreateInfo(It begin, It end)
		{
			std::vector<vk::DeviceQueueCreateInfo> createInfos;
			std::vector<std::vector<float>> priorities;
			std::vector<std::vector<bool>> fixed;

			It it = begin;
			while (it != end) {
				auto pos = std::lower_bound(std::begin(createInfos), std::end(createInfos), it->family_index(), [](const vk::DeviceQueueCreateInfo& left, uint32_t famIdx) { 
					return left.queueFamilyIndex < famIdx;
				});
				const auto targetIndex = std::distance(std::begin(createInfos), pos);

				// Maybe add queues to an already existing family:
				if (pos != std::end(createInfos) && pos->queueFamilyIndex == it->family_index()) {
					assert (priorities[targetIndex].size() > 0);
					for (uint32_t i = 0; i <= it->queue_index(); ++i) {
						if (priorities[targetIndex].size() <= i) {
							priorities[targetIndex].push_back(it->priority());
						}
						else {
							if (i == it->queue_index()) {
								if (fixed[targetIndex][i]) {
									throw avk::runtime_error("Invalid queue configuration: queueFamily[" + std::to_string(it->family_index()) + "] and queueIndex[" + std::to_string(it->queue_index()) + "] are set multiple times");
								}
								priorities[targetIndex][i] = it->priority();
								fixed     [targetIndex][i] = true;
							}
						}
					}
				}
				// ...or add a new queue family (and one queue):
				else {
					createInfos.insert(pos, vk::DeviceQueueCreateInfo{ vk::DeviceQueueCreateFlags{}, it->family_index(), it->queue_index() + 1u, nullptr });
					priorities .insert(std::begin(priorities) + targetIndex, std::vector<float> {});
					fixed      .insert(std::begin(fixed)      + targetIndex, std::vector<bool>  {});
					for (uint32_t i = 0; i <= it->queue_index(); ++i) {
						priorities[targetIndex].push_back(it->priority());
						fixed     [targetIndex].push_back(i == it->queue_index());
					}
				}
				++it;
			}

			// No guarantee that this is maintained afterwards! Better call this code afterwards again (must be done by the user)!
			for (auto i = 0; i < createInfos.size(); ++i) {
				createInfos[i].setPQueuePriorities(priorities[i].data());
			}

			return { std::move(createInfos), std::move(priorities) };
		}

		/** Assign the queue handle from the given, already created(!) logical device. 
		 *	This assumes that the logical device has been created with the proper queue create configuration,
		 *	so that the properties this queue has been configured with (family-index and queue-index) are available.
		 */
		void assign_handle();

		/** Gets the queue family index of this queue */
		auto family_index() const { return mQueueFamilyIndex; }
		/** Gets queue index (inside the queue family) of this queue. */
		auto queue_index() const { return mQueueIndex; }
		auto priority() const { return mPriority; }
		const auto& handle() const { return mQueue; }
		const auto* handle_ptr() const { return &mQueue; }

		avk::submission_data submit(avk::command_buffer_t& aCommandBuffer) const;

		bool is_prepared() const;
		
	private:
		const root* mRoot = nullptr;
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

}
