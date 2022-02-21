#pragma once
#include <avk/avk.hpp>

#include "buffer.hpp"
#include "image.hpp"

namespace avk
{
	namespace syncxxx
	{
		struct sync_hint final
		{
			/**	Which stages of the affected command must wait on whatever comes before?
			 *	I.e., this member would mean the DESTINATION stage of a barrier that comes before the affected command.
			 */
			std::optional<vk::PipelineStageFlags2KHR> mStageHintBefore;

			/**	Which access of the affected command must wait on whatever comes before?
			 *	I.e., this member would mean the DESTINATION access of a barrier that comes before the affected command.
			 */
			std::optional<vk::AccessFlags2KHR> mAccessHintBefore;

			/**	Which stages of the affected command must be waited-on from whatever comes after?
			 *	I.e., this member would mean the SOURCE stage of a barrier that comes after the affected command.
			 */
			std::optional<vk::PipelineStageFlags2KHR> mStageHintAfter;

			/**	Which access of the affected command must be waited-on from whatever comes after?
			 *	I.e., this member would mean the SOURCE access of a barrier that comes after the affected command.
			 */
			std::optional<vk::AccessFlags2KHR> mAccessHintAfter;

			// TODO: I think it would be pretty hard to incorporate the following two in a meaningful manner:
			//       (If possible, we'd need to track their respective vk::Image and vk::Buffer handles.)
			//std::optional<vk::ImageSubresourceRange> mImageSubresourceRangeAffected;
			//std::optional<std::tuple<vk::DeviceSize, vk::DeviceSize>> mBufferOffsetSizeAffected;
		};

		struct queue_family_info
		{
			uint32_t mSrcQueueFamilyIndex;
			uint32_t mDstQueueFamilyIndex;
		};

		struct buffer_sync_info
		{
			// TODO: Better use the vk-handle here?
			// TODO: What about making it persistant? Turn resource_reference into resource_ownership somehow?
			avk::resource_reference<const avk::buffer_t> mBuffer; 
			vk::DeviceSize mOffset;
			vk::DeviceSize mSize;
		};

		struct image_sync_info
		{
			// TODO: Better use the vk-handle here?
			// TODO: What about making it persistant? Turn resource_reference into resource_ownership somehow?
			avk::resource_reference<const avk::image_t> mImage;
			vk::ImageSubresourceRange mSubresourceRange;
			std::optional<avk::image_layout::image_layout2> mLayoutTransition;
		};

		class barrier_data final
		{
		public:
			// Constructs a global execution barrier:
			barrier_data(avk::stage::pipeline_stage2 aStages)
				: mStages{ aStages }, mAccesses{}, mQueueFamilyOwnershipTransfer{}, mSpecificData{} {}

			// Constructs a global memory barrier:
			barrier_data(avk::stage::pipeline_stage2 aStages, avk::access::memory_access2 aAccesses)
				: mStages{ aStages }, mAccesses{ aAccesses }, mQueueFamilyOwnershipTransfer{}, mSpecificData{} {}

			// Constructs an image memory barrier:
			barrier_data(avk::stage::pipeline_stage2 aStages, avk::access::memory_access2 aAccesses, avk::resource_reference<const avk::image_t> aImage, vk::ImageSubresourceRange aSubresourceRange)
				: mStages{ aStages }, mAccesses{ aAccesses }, mQueueFamilyOwnershipTransfer{}
				, mSpecificData{ image_sync_info{ aImage, aSubresourceRange, std::optional<avk::image_layout::image_layout2>{} } } {}

			// Constructs a buffer memory barrier:
			barrier_data(avk::stage::pipeline_stage2 aStages, avk::access::memory_access2 aAccesses, avk::resource_reference<const avk::buffer_t> aBuffer, vk::DeviceSize aOffset, vk::DeviceSize aSize)
				: mStages{ aStages }, mAccesses{ aAccesses }, mQueueFamilyOwnershipTransfer{}
				, mSpecificData{ buffer_sync_info{ aBuffer, aOffset, aSize } } {}

			// Adds memory access, potentially turning a execution barrier into a memory barrier.
			barrier_data& with_memory_access(avk::access::memory_access2 aMemoryAccess)
			{
				mAccesses = aMemoryAccess;
				return *this;
			}

			// Adds an image layout transition to this barrier_data:
			barrier_data& with_layout_transition(avk::image_layout::image_layout2 aLayoutTransition)
			{
				if (!std::holds_alternative<image_sync_info>(mSpecificData)) {
					throw avk::runtime_error("with_layout_transition has been called for a barrier_data which does not represent an image memory barrier.");
				}
				std::get<image_sync_info>(mSpecificData).mLayoutTransition = aLayoutTransition;
				return *this;
			}

			// Adds a subresource range to this barrier_data:
			barrier_data& for_subresource_range(vk::ImageSubresourceRange aSubresourceRange)
			{
				if (!std::holds_alternative<image_sync_info>(mSpecificData)) {
					throw avk::runtime_error("for_subresource_range has been called for a barrier_data which does not represent an image memory barrier.");
				}
				std::get<image_sync_info>(mSpecificData).mSubresourceRange = aSubresourceRange;
				return *this;
			}

			// Adds buffer offset and size to this barrier_data:
			barrier_data& for_offset_and_size(vk::DeviceSize aOffset, vk::DeviceSize aSize)
			{
				if (!std::holds_alternative<buffer_sync_info>(mSpecificData)) {
					throw avk::runtime_error("for_offset_and_size has been called for a barrier_data which does not represent a buffer memory barrier.");
				}
				std::get<buffer_sync_info>(mSpecificData).mOffset = aOffset;
				std::get<buffer_sync_info>(mSpecificData).mSize   = aSize;
				return *this;
			}

			// Adds an queue family ownership transfer to this barrier_data:
			barrier_data& with_queue_family_ownership_transfer(uint32_t aSrcQueueFamilyIndex, uint32_t aDstQueueFamilyIndex)
			{
				if (!std::holds_alternative<image_sync_info>(mSpecificData) && !std::holds_alternative<buffer_sync_info>(mSpecificData)) {
					throw avk::runtime_error("with_queue_family_ownership_transfer has been called for a barrier_data which does not represent an image memory barrier nor a buffer memory barrier.");
				}
				mQueueFamilyOwnershipTransfer = queue_family_info{ aSrcQueueFamilyIndex, aDstQueueFamilyIndex };
				return *this;
			}

			[[nodiscard]] bool is_global_execution_barrier() const {
				return !mAccesses.has_value() && !mQueueFamilyOwnershipTransfer.has_value() && std::holds_alternative<std::monostate>(mSpecificData);
			}

			[[nodiscard]] bool is_global_memory_barrier() const {
				return mAccesses.has_value() && !mQueueFamilyOwnershipTransfer.has_value() && std::holds_alternative<std::monostate>(mSpecificData);
			}

			[[nodiscard]] bool is_image_memory_barrier() const {
				return std::holds_alternative<image_sync_info>(mSpecificData);
			}

			[[nodiscard]] bool is_buffer_memory_barrier() const {
				return std::holds_alternative<buffer_sync_info>(mSpecificData);
			}

			[[nodiscard]] bool is_ill_formed() const {
				return !(is_global_execution_barrier() || is_global_memory_barrier() || is_image_memory_barrier() || is_buffer_memory_barrier());
			}

			[[nodiscard]] auto src_stage() const { return mStages.mSrc; }
			[[nodiscard]] auto dst_stage() const { return mStages.mDst; }
			[[nodiscard]] decltype(avk::access::memory_access2::mSrc) src_access() const {
				return mAccesses.has_value() ? decltype(avk::access::memory_access2::mSrc){} : mAccesses->mSrc;
			}
			[[nodiscard]] decltype(avk::access::memory_access2::mDst) dst_access() const {
				return mAccesses.has_value() ? decltype(avk::access::memory_access2::mDst){} : mAccesses->mDst;
			}
			[[nodiscard]] auto queue_family_ownership_transfer() const { return mQueueFamilyOwnershipTransfer; }
			[[nodiscard]] auto buffer_memory_barrier_data() const {
				assert(is_buffer_memory_barrier());
				return std::get<buffer_sync_info>(mSpecificData);
			}
			[[nodiscard]] auto image_memory_barrier_data() const {
				assert(is_image_memory_barrier());
				return std::get<image_sync_info>(mSpecificData);
			}
		private:
			avk::stage::pipeline_stage2 mStages;
			std::optional<avk::access::memory_access2> mAccesses;
			std::optional<queue_family_info> mQueueFamilyOwnershipTransfer;
			std::variant<std::monostate, buffer_sync_info, image_sync_info> mSpecificData;
		};

		inline static barrier_data global_execution_barrier(avk::stage::pipeline_stage2 aStages)
		{
			return barrier_data{ aStages };
		}

		inline static barrier_data global_memory_barrier(avk::stage::pipeline_stage2 aStages, avk::access::memory_access2 aAccesses)
		{
			return barrier_data{ aStages, aAccesses };
		}

		inline static barrier_data image_memory_barrier(avk::resource_reference<const avk::image_t> aImage, avk::stage::pipeline_stage2 aStages, avk::access::memory_access2 aAccesses)
		{
			return barrier_data{ aStages, aAccesses, aImage, aImage->entire_subresource_range() };
		}

		inline static barrier_data buffer_memory_barrier(avk::resource_reference<const avk::buffer_t> aBuffer, avk::stage::pipeline_stage2 aStages, avk::access::memory_access2 aAccesses)
		{
			return barrier_data{ aStages, aAccesses, aBuffer, 0, VK_WHOLE_SIZE };
		}
	}


	// Predefine command types:
	namespace command
	{
		struct state_type_command;
		struct action_type_command;
	}

	// Define recorded* type:
	using recorded_commands_and_sync_instructions_t = std::variant<command::state_type_command, command::action_type_command, syncxxx::barrier_data>;
	
	namespace command
	{
		struct state_type_command final
		{
			using rec_fun = std::function<void(avk::resource_reference<avk::command_buffer_t>)>;

			rec_fun mFun;
		};

		template <typename P, typename T>
		inline static state_type_command push_constants(const P& aPipeline, const T& aPushConstants, vk::ShaderStageFlags aStageFlags = vk::ShaderStageFlagBits::eAll)
		{
			return state_type_command{
				[
					bLayoutHandle = aPipeline->layout_handle(),
					bPushConstants = aPushConstants,
					bStageFlags = aStageFlags
				] (avk::resource_reference<avk::command_buffer_t> cb) {
					cb->handle().pushConstants(bLayoutHandle, bStageFlags, 0, sizeof(bPushConstants), &bPushConstants);
				}
			};
		};

		struct action_type_command final
		{
			using rec_fun = std::function<void(avk::resource_reference<avk::command_buffer_t>)>;

			avk::syncxxx::sync_hint mSyncHint;
			rec_fun mBeginFun;
			std::vector<recorded_commands_and_sync_instructions_t> mNestedCommandsAndSyncInstructions;
			rec_fun mEndFun;

		};
		
		inline static action_type_command render_pass(
			avk::resource_reference<const avk::renderpass_t> aRenderpass, 
			avk::resource_reference<avk::framebuffer_t> aFramebuffer,
			std::vector<recorded_commands_and_sync_instructions_t> aNestedCommandsAndSyncInstructions)
		{
			return action_type_command {
				// Define a sync hint that corresponds to the implicit subpass dependencies (see specification chapter 8.1)
				avk::syncxxx::sync_hint {
					vk::PipelineStageFlagBits2KHR::eAllCommands, // eAllGraphics does not include new stages or ext-stages. Therefore, eAllCommands!
					vk::AccessFlagBits2KHR::eInputAttachmentRead | vk::AccessFlagBits2KHR::eColorAttachmentRead | vk::AccessFlagBits2KHR::eColorAttachmentWrite | vk::AccessFlagBits2KHR::eDepthStencilAttachmentRead | vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite,
					vk::PipelineStageFlagBits2KHR::eAllCommands, // Same comment as above regarding eAllCommands vs. eAllGraphics
					vk::AccessFlagBits2KHR::eColorAttachmentWrite | vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite
				},
				[aRenderpass, aFramebuffer](avk::resource_reference<avk::command_buffer_t> cb){
					cb->begin_render_pass_for_framebuffer(aRenderpass, aFramebuffer);
				},
				std::move(aNestedCommandsAndSyncInstructions),
				[](avk::resource_reference<avk::command_buffer_t> cb) {
					cb->end_render_pass();
				}
			};
		}

		inline static state_type_command bind(avk::resource_reference<const graphics_pipeline_t> aPipeline)
		{
			return state_type_command {
				[aPipeline](avk::resource_reference<avk::command_buffer_t> cb) {
					cb->handle().bindPipeline(vk::PipelineBindPoint::eGraphics, aPipeline->handle());
				}
			};
		}

		inline static action_type_command draw(uint32_t aVertexCount, uint32_t aInstanceCount, uint32_t aFirstVertex, uint32_t aFirstInstance)
		{
			return action_type_command {
				avk::syncxxx::sync_hint {
					vk::PipelineStageFlagBits2KHR::eAllGraphics,
					vk::AccessFlagBits2KHR::eInputAttachmentRead | vk::AccessFlagBits2KHR::eColorAttachmentRead | vk::AccessFlagBits2KHR::eColorAttachmentWrite | vk::AccessFlagBits2KHR::eDepthStencilAttachmentRead | vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite,
					vk::PipelineStageFlagBits2KHR::eAllGraphics,
					vk::AccessFlagBits2KHR::eColorAttachmentWrite | vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite
				},
				[aVertexCount, aInstanceCount, aFirstVertex, aFirstInstance](avk::resource_reference<avk::command_buffer_t> cb) {
					cb->handle().draw(aVertexCount, aInstanceCount, aFirstVertex, aFirstInstance);
				}
			};
		}

	}



	struct semaphore_wait_info
	{
		avk::resource_reference<const avk::semaphore_t> mWaitSemaphore;
		avk::stage::pipeline_stage_flags mDstStage;
	};

	inline semaphore_wait_info operator>> (avk::resource_reference<const avk::semaphore_t> a, avk::stage::pipeline_stage_flags b)
	{
		return semaphore_wait_info{ a, b };
	}



	struct semaphore_signal_info
	{
		avk::stage::pipeline_stage_flags mSrcStage;
		avk::resource_reference<const avk::semaphore_t> mSignalSemaphore;
	};

	inline semaphore_signal_info operator>> (avk::stage::pipeline_stage_flags a, avk::resource_reference<const avk::semaphore_t> b)
	{
		return semaphore_signal_info{ a, b };
	}

	// Something that submits stuff to a queue.
	// The submission itself either happens in submit() or in this class' destructor, if submit()/go()/do_it() has never been invoked before.
	class submission_data final
	{
	public:
		submission_data(const root* aRoot, avk::resource_reference<avk::command_buffer_t> aCommandBuffer, const queue* aQueue)
			: mRoot{ aRoot }
			, mCommandBufferToSubmit{ std::move(aCommandBuffer) }
			, mQueueToSubmitTo{ aQueue }
			, mSubmissionCount{ 0u }
		{}
		submission_data(const root* aRoot, avk::resource_reference<avk::command_buffer_t> aCommandBuffer, semaphore_wait_info aSemaphoreWaitInfo)
			: mRoot{ aRoot }
			, mCommandBufferToSubmit{ std::move(aCommandBuffer) }
			, mQueueToSubmitTo{ nullptr }
			, mSemaphoreWaits{ std::move(aSemaphoreWaitInfo) }
			, mSubmissionCount{ 0u }
		{}
		submission_data(const submission_data&) = delete;
		submission_data(submission_data&&) noexcept;
		submission_data& operator=(const submission_data&) = delete;
		submission_data& operator=(submission_data&&) noexcept;
		~submission_data() noexcept(false);

		submission_data&& store_for_now() noexcept;

		submission_data& submit_to(const queue* aQueue);
		submission_data& waiting_for(avk::semaphore_wait_info aWaitInfo);
		submission_data& signaling_upon_completion(semaphore_signal_info aSignalInfo);
		submission_data& signaling_upon_completion(avk::resource_reference<avk::fence_t> aFence);

		bool is_sane() const { return nullptr != mRoot; }

		void submit();
		void go() { submit(); }
		void do_it() { submit(); }

	private:
		const root* mRoot = nullptr;
		avk::resource_reference<avk::command_buffer_t> mCommandBufferToSubmit;
		const queue* mQueueToSubmitTo;
		std::vector<semaphore_wait_info> mSemaphoreWaits;
		std::vector<semaphore_signal_info> mSemaphoreSignals;
		std::optional<avk::resource_reference<avk::fence_t>> mFence;
		uint32_t mSubmissionCount;
	};

	// This class turns a std::vector<recorded_commands_and_sync_instructions_t> into an actual command buffer
	class recorded_command_buffer final
	{
	public:
		// The constructor performs all the parsing, therefore, there's no std::vector<recorded_commands_and_sync_instructions_t> member.
		recorded_command_buffer(const root* aRoot, const std::vector<recorded_commands_and_sync_instructions_t>& aRecordedCommandsAndSyncInstructions, avk::resource_reference<avk::command_buffer_t> aCommandBuffer);
		
		recorded_command_buffer(const recorded_command_buffer&) = default;
		recorded_command_buffer(recorded_command_buffer&&) noexcept = default;
		recorded_command_buffer& operator=(const recorded_command_buffer&) = default;
		recorded_command_buffer& operator=(recorded_command_buffer&&) noexcept = default;
		~recorded_command_buffer() = default;
		
		template <typename T>
		recorded_command_buffer& handling_lifetime_of(T&& aResource)
		{
			mCommandBufferToRecordInto->handle_lifetime_of(any_owning_resource_t{ std::move(aResource) });
			return *this;
		}

		submission_data then_waiting_for(avk::semaphore_wait_info aWaitInfo);
		submission_data then_submit_to(const queue* aQueue);

	private:
		const root* mRoot;
		avk::resource_reference<avk::command_buffer_t> mCommandBufferToRecordInto;
	};

	// This class gathers recorded commands which can then be recorded into a command buffer via into_command_buffer()
	// (or they can also be stored somewhere for later recording into a command buffer).
	// But this class is intended to be used as temporary object.
	class recorded_commands final
	{
	public:
		recorded_commands(const root* aRoot, std::vector<recorded_commands_and_sync_instructions_t> aRecordedCommandsAndSyncInstructions)
			: mRoot{ aRoot }
			, mRecordedCommandsAndSyncInstructions{ std::move(aRecordedCommandsAndSyncInstructions) }
		{}
		recorded_commands(const recorded_commands&) = default;
		recorded_commands(recorded_commands&&) noexcept = default;
		recorded_commands& operator=(const recorded_commands&) = default;
		recorded_commands& operator=(recorded_commands&&) noexcept = default;
		~recorded_commands() = default;
		
		recorded_commands& move_into(std::vector<recorded_commands_and_sync_instructions_t>& aTarget);
		recorded_commands& prepend_by(std::vector<recorded_commands_and_sync_instructions_t>& aCommands);
		recorded_commands& append_by(std::vector<recorded_commands_and_sync_instructions_t>& aCommands);

		std::vector<recorded_commands_and_sync_instructions_t> and_store();
		recorded_command_buffer into_command_buffer(avk::resource_reference<avk::command_buffer_t> aCommandBuffer);
		
	private:
		const root* mRoot;
		std::vector<recorded_commands_and_sync_instructions_t> mRecordedCommandsAndSyncInstructions;
	};

}
