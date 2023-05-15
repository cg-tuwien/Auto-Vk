#pragma once
#include "avk/avk.hpp"

#include "avk.hpp"

namespace avk 
{
	class bottom_level_acceleration_structure_t;
	//class buffer_t;
	class buffer_view_t;
	//class command_buffer_t;
	class command_pool_t;
	class compute_pipeline_t;
	class fence_t;
	class framebuffer_t;
	class graphics_pipeline_t;
	class image_t;
	class image_sampler_t;
	class image_view_t;
	class query_pool_t;
	class ray_tracing_pipeline_t;
	class renderpass_t;
	class sampler_t;
	class semaphore_t;
	class top_level_acceleration_structure_t;

	namespace command
	{
		struct state_type_command;
		struct action_type_command;
	}
	class sync_type_command;
	using recorded_commands_t = std::variant<command::state_type_command, command::action_type_command, sync::sync_type_command>;

	using bottom_level_acceleration_structure = avk::owning_resource<bottom_level_acceleration_structure_t>;
	//using buffer = avk::owning_resource<buffer_t>;
	using buffer_view = avk::owning_resource<buffer_view_t>;
	//using command_buffer = avk::owning_resource<command_buffer_t>;
	using command_pool = avk::owning_resource<command_pool_t>;
	using compute_pipeline = avk::owning_resource<compute_pipeline_t>;
	using fence = avk::owning_resource<fence_t>;
	using framebuffer = avk::owning_resource<framebuffer_t>;
	using graphics_pipeline = avk::owning_resource<graphics_pipeline_t>;
	using image = avk::owning_resource<image_t>;
	using image_sampler = avk::owning_resource<image_sampler_t>;
	using image_view = avk::owning_resource<image_view_t>;
	using query_pool = avk::owning_resource<query_pool_t>;
	using ray_tracing_pipeline = avk::owning_resource<ray_tracing_pipeline_t>;
	using renderpass = avk::owning_resource<renderpass_t>;
	using sampler = avk::owning_resource<sampler_t>;
	using semaphore = avk::owning_resource<semaphore_t>;
	using top_level_acceleration_structure = avk::owning_resource<top_level_acceleration_structure_t>;

	//class renderpass_t;
	//class command_pool_t;
	//class image_t;
	//class graphics_pipeline_t;
	//class compute_pipeline_t;
	//class ray_tracing_pipeline_t;
	//class set_of_descriptor_set_layouts;
	//class framebuffer_t;

	struct binding_data;

	using any_owning_resource_t = std::variant<
		bottom_level_acceleration_structure,
		buffer,
		buffer_view,
		command_buffer,
		command_pool,
		compute_pipeline,
		fence,
		framebuffer,
		graphics_pipeline,
		image,
		image_sampler,
		image_view,
		query_pool,
		ray_tracing_pipeline,
		renderpass,
		sampler,
		semaphore,
		top_level_acceleration_structure
	>;
	
	enum struct command_buffer_state
	{
		none,
		recording,
		finished_recording,
		submitted
	};

	struct using_raygen_group_at_index
	{
		using_raygen_group_at_index(uint32_t aGroupIndex) : mGroupIndex{ aGroupIndex } {}
		uint32_t mGroupIndex;
	};

	struct using_miss_group_at_index
	{
		using_miss_group_at_index(uint32_t aGroupIndex) : mGroupIndex{ aGroupIndex } {}
		uint32_t mGroupIndex;
	};

	struct using_hit_group_at_index
	{
		using_hit_group_at_index(uint32_t aGroupIndex) : mGroupIndex{ aGroupIndex } {}
		uint32_t mGroupIndex;
	};

	struct using_callable_group_at_index
	{
		using_callable_group_at_index(uint32_t aGroupIndex) : mGroupIndex{ aGroupIndex } {}
		uint32_t mGroupIndex;
	};

	/** A command buffer which has been created for a certain queue family */
	class command_buffer_t
	{
		friend class root;
		friend class queue;
		friend class command_pool_t;
		
	public:
		command_buffer_t() = default;
		command_buffer_t(command_buffer_t&&) noexcept = default;
		command_buffer_t(const command_buffer_t&) = delete;
		command_buffer_t& operator=(command_buffer_t&&) noexcept = default;
		command_buffer_t& operator=(const command_buffer_t&) = delete;
		~command_buffer_t();

		/** Set a custom deleter function.
		 *	This is often used for resource cleanup, e.g. a buffer which can be deleted when this command buffer is destroyed.
		 */
		template <typename F>
		command_buffer_t& set_custom_deleter(F&& aDeleter) noexcept
		{
			if (mCustomDeleter.has_value()) {
				// There is already a custom deleter! Make sure that this stays alive as well.
				mCustomDeleter = [
					existingDeleter = std::move(mCustomDeleter.value()),
					additionalDeleter = std::forward<F>(aDeleter)
				]() {
					// Invoke in inverse order of addition:
					additionalDeleter();
					existingDeleter();
				};
			}
			else {
				mCustomDeleter = std::forward<F>(aDeleter);
			}
			return *this;
		}

		// TODO: comment
		command_buffer_t& handle_lifetime_of(any_owning_resource_t aResource);

		/** Set a post execution handler function.
		 *	This is (among possible other use cases) used for keeping the C++-side of things in sync with the GPU-side,
		 *	e.g., to update image layout transitions after command buffers with renderpasses have been submitted.
		 */
		template <typename F>
		command_buffer_t& set_post_execution_handler(F&& aHandler) noexcept
		{
			if (mPostExecutionHandler.has_value()) {
				// There is already a custom deleter! Make sure that this stays alive as well.
				mPostExecutionHandler = [
					existingHandler = std::move(mPostExecutionHandler.value()),
					additionalHandler = std::forward<F>(aHandler)
				]() {
					// Invoke IN addition order:
					existingHandler();
					additionalHandler();
				};
			}
			else {
				mPostExecutionHandler = std::forward<F>(aHandler);
			}
			return *this;
		}

		void invoke_post_execution_handler() const;

		void begin_recording();
		void end_recording();

		/**	Record a given state-type command directly into the given command buffer.
		 *	I.e., when calling this method, the actions of the given command are
		 *	immediately executed, recording the operations into this command buffer.
		 */
		void record(const avk::command::state_type_command& aToBeRecorded);

		/**	Record a given action-type command directly into the given command buffer.
		 *	I.e., when calling this method, the actions of the given command are
		 *	immediately executed, recording the operations into this command buffer.
		 */
		void record(const avk::command::action_type_command& aToBeRecorded);

		/**	Record a given synchronization-type command directly into the given command buffer.
		 *	I.e., when calling this method, the actions of the given command are
		 *	immediately executed, recording the operations into this command buffer.
		 */
		void record(const avk::sync::sync_type_command& aToBeRecorded);

		/**	Record a list of commands directly into the given command buffer.
		 *	I.e., when calling this method, the actions of the given commands are
		 *	immediately executed, recording the operations into this command buffer.
		 */
		void record(std::vector<avk::recorded_commands_t> aRecordedCommandsAndSyncInstructions);

		/** Prepare a command buffer for re-recording.
		 *   This essentially calls (and removes) any custom deleters, and removes any post-execution-handlers.
		 *   Call this method before re-recording an existing command buffer.
		 */
		void prepare_for_reuse();

		/** Calls prepare_for_reuse() and then vkResetCommandBuffer
		 */
		void reset();


		auto& begin_info() const { return mBeginInfo; }
		const vk::CommandBuffer& handle() const { return mCommandBuffer.get(); }
		const vk::CommandBuffer* handle_ptr() const { return &mCommandBuffer.get(); }
		auto state() const { return mState; }

		void bind_descriptors(vk::PipelineBindPoint aBindingPoint, vk::PipelineLayout aLayoutHandle, std::vector<descriptor_set> aDescriptorSets);

		void save_subpass_contents_state(vk::SubpassContents x) { mSubpassContentsState = x; }
		
		[[nodiscard]] const auto* root_ptr() const { return mRoot; }

	private:
		const root* mRoot;
		std::shared_ptr<vk::UniqueHandle<vk::CommandPool, DISPATCH_LOADER_CORE_TYPE>> mCommandPool;

		command_buffer_state mState;
		vk::CommandBufferBeginInfo mBeginInfo;
		vk::UniqueHandle<vk::CommandBuffer, DISPATCH_LOADER_CORE_TYPE> mCommandBuffer;
		vk::SubpassContents mSubpassContentsState;
		
		std::optional<avk::unique_function<void()>> mPostExecutionHandler;

		/** A custom deleter function called upon destruction of this command buffer */
		std::optional<avk::unique_function<void()>> mCustomDeleter;
		
		std::vector<any_owning_resource_t> mLifetimeHandledResources;
	};

	// Typedef for a variable representing an owner of a command_buffer
	using command_buffer = avk::owning_resource<command_buffer_t>;
	
}
