#pragma once
#include <avk/avk.hpp>

namespace avk
{
	/**	The old_sync class is a fundamental part of the framework and is used wherever synchronization is or can be needed.
	 *	It allows a caller to inject a specific synchronization strategy into a particular method/function.
	 */
	class old_sync
	{
	public:
		static command_pool sPoolToAllocCommandBuffersFrom;
		static queue* sQueueToUse;
		
		struct presets
		{
			static void default_handler_before_operation(command_buffer_t& aCommandBuffer, pipeline_stage aDestinationStage, std::optional<read_memory_access> aDestinationAccess);
			
			static void default_handler_after_operation(command_buffer_t& aCommandBuffer, pipeline_stage aSourceStage, std::optional<write_memory_access> aSourceAccess);
			
			struct image_copy
			{
				/** Destination image is assumed to be ready to be read. A rather coarse barrier is established for the source image, waiting for all commands and any memory access. */
				static avk::unique_function<void(command_buffer_t&, pipeline_stage, std::optional<read_memory_access>)> wait_for_previous_operations(avk::image_t& aSourceImage, avk::image_t& aDestinationImage);

				/** Set up rather coarse barrier for subsequent operations and transfer the destination image into color attachment optimal format */
				static avk::unique_function<void(command_buffer_t&, pipeline_stage, std::optional<write_memory_access>)> let_subsequent_operations_wait(avk::image_t& aSourceImage, avk::image_t& aDestinationImage);

				/** Set up lightweight old_sync for the image is to be sent to present right afterwards. Destination image's layout is transferred in to presentable format. */
				static avk::unique_function<void(command_buffer_t&, pipeline_stage, std::optional<write_memory_access>)> directly_into_present(avk::image_t& aSourceImage, avk::image_t& aDestinationImage);
			};
		};
		
		enum struct sync_type { not_required, by_return, by_existing_command_buffer, via_wait_idle, via_wait_idle_deliberately, via_semaphore, via_barrier };
		enum struct commandbuffer_request { not_specified, single_use, reusable };
		using steal_before_handler_t = void(*)(command_buffer_t&, pipeline_stage, std::optional<read_memory_access>);
		using steal_after_handler_t = void(*)(command_buffer_t&, pipeline_stage, std::optional<write_memory_access>);
		static void steal_before_handler_on_demand(command_buffer_t&, pipeline_stage, std::optional<read_memory_access>) {}
		static void steal_after_handler_on_demand(command_buffer_t&, pipeline_stage, std::optional<write_memory_access>) {}
		static void steal_before_handler_immediately(command_buffer_t&, pipeline_stage, std::optional<read_memory_access>) {}
		static void steal_after_handler_immediately(command_buffer_t&, pipeline_stage, std::optional<write_memory_access>) {}
		static bool is_about_to_steal_before_handler_on_demand(const avk::unique_function<void(command_buffer_t&, pipeline_stage, std::optional<read_memory_access>)>& aToTest) {
			const auto trgPtr = aToTest.target<steal_before_handler_t>();
			return nullptr == trgPtr ? false : *trgPtr == steal_before_handler_on_demand ? true : false;
		}
		static bool is_about_to_steal_after_handler_on_demand(const avk::unique_function<void(command_buffer_t&, pipeline_stage, std::optional<write_memory_access>)>& aToTest) {
			const auto trgPtr = aToTest.target<steal_after_handler_t>();
			return nullptr == trgPtr ? false : *trgPtr == steal_after_handler_on_demand ? true : false;
		}
		static bool is_about_to_steal_before_handler_immediately(const avk::unique_function<void(command_buffer_t&, pipeline_stage, std::optional<read_memory_access>)>& aToTest) {
			const auto trgPtr = aToTest.target<steal_before_handler_t>();
			return nullptr == trgPtr ? false : *trgPtr == steal_before_handler_immediately ? true : false;
		}
		static bool is_about_to_steal_after_handler_immediately(const avk::unique_function<void(command_buffer_t&, pipeline_stage, std::optional<write_memory_access>)>& aToTest) {
			const auto trgPtr = aToTest.target<steal_after_handler_t>();
			return nullptr == trgPtr ? false : *trgPtr == steal_after_handler_immediately ? true : false;
		}
		
		old_sync() = default;
		old_sync(old_sync&&) noexcept;
		old_sync(const old_sync&) = delete;
		old_sync& operator=(old_sync&&) noexcept;
		old_sync& operator=(const old_sync&) = delete;
		~old_sync();
		
#pragma region static creation functions
		/**	Indicate that no old_sync is required. If you are wrong, there will be an exception.
		 */
		static old_sync not_required();
	
		/**	Establish very coarse (and inefficient) synchronization by waiting for the queue to become idle before continuing.
		 */
		static old_sync wait_idle(bool aDontWarn = false);

		/**	Establish semaphore-based synchronization with a custom semaphore lifetime handler.
		 *	@tparam F							void(semaphore)
		 *	@param	aSignalledAfterOperation	A function to handle the lifetime of a created semaphore. 
		 *	@param	aWaitBeforeOperation		A vector of other semaphores to be waited on before executing the command.
		 *
		 *	Example usage:
		 *	avk::old_sync::with_semaphore([](avk::semaphore s) { avk::context().main_window()->set_extra_semaphore_dependency(std::move(s)); }
		 */
		template <typename F>
		static old_sync with_semaphore(F&& aSignalledAfterOperation, std::vector<semaphore> aWaitBeforeOperation = {})
		{
			old_sync result;
			result.mSemaphoreLifetimeHandler = std::forward<F>(aSignalledAfterOperation);
			result.mWaitBeforeSemaphores = std::move(aWaitBeforeOperation);
			return result;
		}

		/**	Establish barrier-based synchronization and return the resulting command buffer from the operation.
		 *	Note: Not all operations support this type of synchronization. You can notice them by a method
		 *	      signature that does NOT return `std::optional<command_buffer>`.
		 *	
		 *	@param	aCommandBufferLifetimeHandler		A function to handle the lifetime of a command buffer.
		 *	
		 *	@param	aEstablishBarrierBeforeOperation	Function signature: void(avk::command_buffer_t&, avk::pipeline_stage, std::optional<avk::read_memory_access>)
		 *												Callback which gets called at the beginning of the operation, in order to sync with whatever comes before.
		 *												This handler is generally considered to be optional an hence, set to {} by default --- i.e. not used.
		 *												
		 *	@param	aEstablishBarrierAfterOperation		Function signature: void(avk::command_buffer_t&, avk::pipeline_stage, std::optional<avk::write_memory_access>)
		 *												Callback which gets called at the end of the operation, in order to sync with whatever comes after.
		 *												This handler is generally considered to be neccessary and hence, set to a default handler by default.
		 */
		static old_sync with_barriers_by_return(
			avk::unique_function<void(command_buffer_t&, pipeline_stage /* destination stage */, std::optional<read_memory_access> /* destination access */)> aEstablishBarrierBeforeOperation = {},
			avk::unique_function<void(command_buffer_t&, pipeline_stage /* source stage */,	  std::optional<write_memory_access> /* source access */)> aEstablishBarrierAfterOperation = presets::default_handler_after_operation)
		{
			old_sync result;
			result.mSpecialSync = sync_type::by_return;
			result.mEstablishBarrierAfterOperationCallback = std::move(aEstablishBarrierAfterOperation);
			result.mEstablishBarrierBeforeOperationCallback = std::move(aEstablishBarrierBeforeOperation);
			return result;
		}

		/**	Establish barrier-based synchronization and record all the commands into an existing command buffer (instead of creating a new one).
		 *	Note: Not all operations support this type of synchronization. You can notice them by a method
		 *	      signature that does NOT return `std::optional<command_buffer>`.
		 *	
		 *	@param	aExistingCommandBuffer				An already existing command buffer, which the commands will be recorded into.
		 *												The command buffer must already be in recording state, i.e. after `begin_recording()` has been invoked on it.
		 *	
		 *	@param	aEstablishBarrierBeforeOperation	Function signature: void(avk::command_buffer_t&, avk::pipeline_stage, std::optional<avk::read_memory_access>)
		 *												Callback which gets called at the beginning of the operation, in order to sync with whatever comes before.
		 *												This handler is generally considered to be optional an hence, set to {} by default --- i.e. not used.
		 *												
		 *	@param	aEstablishBarrierAfterOperation		Function signature: void(avk::command_buffer_t&, avk::pipeline_stage, std::optional<avk::write_memory_access>)
		 *												Callback which gets called at the end of the operation, in order to sync with whatever comes after.
		 *												This handler is generally considered to be neccessary and hence, set to a default handler by default.
		 */
		static old_sync with_barriers_into_existing_command_buffer(
			command_buffer_t& aExistingCommandBuffer,
			avk::unique_function<void(command_buffer_t&, pipeline_stage /* destination stage */, std::optional<read_memory_access> /* destination access */)> aEstablishBarrierBeforeOperation = {},
			avk::unique_function<void(command_buffer_t&, pipeline_stage /* source stage */,	  std::optional<write_memory_access> /* source access */)> aEstablishBarrierAfterOperation = presets::default_handler_after_operation)
		{
			old_sync result;
			result.mSpecialSync = sync_type::by_existing_command_buffer;
			result.mCommandBufferRefOrLifetimeHandler = std::ref(aExistingCommandBuffer);
			result.mEstablishBarrierAfterOperationCallback = std::move(aEstablishBarrierAfterOperation);
			result.mEstablishBarrierBeforeOperationCallback = std::move(aEstablishBarrierBeforeOperation);
			return result;
		}
		
		/**	Establish barrier-based synchronization with a custom command buffer lifetime handler.
		 *	@tparam F									void(command_buffer)
		 *	@param	aCommandBufferLifetimeHandler		A function to handle the lifetime of a command buffer.
		 *												`window::handle_command_buffer_lifetime` might be suitable for your use case.
		 *	
		 *	@param	aEstablishBarrierBeforeOperation	Function signature: void(avk::command_buffer_t&, avk::pipeline_stage, std::optional<avk::read_memory_access>)
		 *												Callback which gets called at the beginning of the operation, in order to sync with whatever comes before.
		 *												This handler is generally considered to be optional an hence, set to {} by default --- i.e. not used.
		 *												
		 *	@param	aEstablishBarrierAfterOperation		Function signature: void(avk::command_buffer_t&, avk::pipeline_stage, std::optional<avk::write_memory_access>)
		 *												Callback which gets called at the end of the operation, in order to sync with whatever comes after.
		 *												This handler is generally considered to be neccessary and hence, set to a default handler by default.
		 */
		template <typename F>
		static old_sync with_barriers(
			F&& aCommandBufferLifetimeHandler,
			avk::unique_function<void(command_buffer_t&, pipeline_stage /* destination stage */, std::optional<read_memory_access> /* destination access */)> aEstablishBarrierBeforeOperation = {},
			avk::unique_function<void(command_buffer_t&, pipeline_stage /* source stage */,	  std::optional<write_memory_access> /* source access */)> aEstablishBarrierAfterOperation = presets::default_handler_after_operation)
		{
			old_sync result;
			result.mCommandBufferRefOrLifetimeHandler = std::forward<F>(aCommandBufferLifetimeHandler); // <-- Set the lifetime handler, not the command buffer reference.
			result.mEstablishBarrierAfterOperationCallback = std::move(aEstablishBarrierAfterOperation);
			result.mEstablishBarrierBeforeOperationCallback = std::move(aEstablishBarrierBeforeOperation);
			return result;
		}

		/**	Establish barrier-based synchronization for a command which is subordinate to a
		 *	"master"-old_sync handler. The master handler is usually provided by the user and this
		 *	method is used to create old_sync objects which go along with the master old_sync, i.e.,
		 *	lifetime of subordinate operations' command buffers are handled along with the
		 *	master handler.
		 *
		 *	@param	aMasterSync		Master sync handler which is being modified by this method
		 *							in order to also handle lifetime of subordinate command buffers.
		 */
		static old_sync auxiliary_with_barriers(
			old_sync& aMasterSync,
			avk::unique_function<void(command_buffer_t&, pipeline_stage /* destination stage */, std::optional<read_memory_access> /* destination access */)> aEstablishBarrierBeforeOperation,
			avk::unique_function<void(command_buffer_t&, pipeline_stage /* source stage */, std::optional<write_memory_access> /* source access */)> aEstablishBarrierAfterOperation
		);
#pragma endregion

#pragma region commandbuffer-related settings
		old_sync& create_reusable_commandbuffer();
		old_sync& create_single_use_commandbuffer();
#pragma endregion

#pragma region ownership-related settings
		/**	Set the queue where the command is to be submitted to AND also where the old_sync will happen.
		 */
		old_sync& on_queue(std::reference_wrapper<queue> aQueue);
#pragma endregion 

#pragma region getters 
		/** Determine the fundamental old_sync approach configured in this `old_sync`. */
		sync_type get_sync_type() const;
		
		/** Queue which the command and old_sync will be submitted to. */
		std::reference_wrapper<queue> queue_to_use() const;

		/** Get the command buffer reference stored internally or create a single-use command buffer and store it within the old_sync object */
		command_buffer_t& get_or_create_command_buffer();
#pragma endregion 

#pragma region essential functions which establish the actual sync. Used by the framework internally.
		void set_queue_hint(std::reference_wrapper<queue> aQueueRecommendation);
		
		void establish_barrier_before_the_operation(pipeline_stage aDestinationPipelineStages, std::optional<read_memory_access> aDestinationMemoryStages);
		void establish_barrier_after_the_operation(pipeline_stage aSourcePipelineStages, std::optional<write_memory_access> aSourceMemoryStages);

		/**	Submit the command buffer and engage old_sync!
		 *	This method is intended not to be used by framework-consuming code, but by the framework-internals.
		 *	Whichever synchronization strategy has been configured for this `ak::old_sync`, it will be executed here
		 *	(i.e. waiting idle, establishing a barrier, or creating a semaphore).
		 *
		 *	@param	aCommandBuffer				Hand over ownership of a command buffer in a "fire and forget"-manner from this method call on.
		 *										The command buffer will be submitted to a queue (whichever queue is configured in this `ak::sync`)
		 */
		std::optional<command_buffer> submit_and_sync();
#pragma endregion
		
	private:
		std::optional<sync_type> mSpecialSync;
		commandbuffer_request mCommandbufferRequest;
		avk::unique_function<void(semaphore)> mSemaphoreLifetimeHandler;
		std::vector<semaphore> mWaitBeforeSemaphores;
		std::variant<std::monostate, avk::unique_function<void(command_buffer)>, std::reference_wrapper<command_buffer_t>> mCommandBufferRefOrLifetimeHandler;
		std::optional<command_buffer> mCommandBuffer;
		avk::unique_function<void(command_buffer_t&, pipeline_stage /* destination stage */, std::optional<read_memory_access> /* destination access */)> mEstablishBarrierBeforeOperationCallback;
		avk::unique_function<void(command_buffer_t&, pipeline_stage /* source stage */,	  std::optional<write_memory_access> /* source access */)>	  mEstablishBarrierAfterOperationCallback;
		std::optional<std::reference_wrapper<queue>> mQueueToUse;
		std::optional<std::reference_wrapper<queue>> mQueueRecommendation;
	};
}
