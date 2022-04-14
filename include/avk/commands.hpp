#pragma once
#include <avk/avk.hpp>

#include "buffer.hpp"
#include "image.hpp"

namespace avk
{
	class framebuffer_t;
	class graphics_pipeline_t;
	class semaphore_t;
	class fence_t;
	class renderpass_t;
	
	namespace sync
	{
		struct sync_hint final
		{
			/**	Which stages of the affected command must wait on whatever comes before?
			 *	I.e., this member would mean the DESTINATION stage of a barrier that comes before the affected command.
			 */
			std::optional<vk::PipelineStageFlags2KHR> mStageHintBefore = {};

			/**	Which access of the affected command must wait on whatever comes before?
			 *	I.e., this member would mean the DESTINATION access of a barrier that comes before the affected command.
			 */
			std::optional<vk::AccessFlags2KHR> mAccessHintBefore = {};

			/**	Which stages of the affected command must be waited-on from whatever comes after?
			 *	I.e., this member would mean the SOURCE stage of a barrier that comes after the affected command.
			 */
			std::optional<vk::PipelineStageFlags2KHR> mStageHintAfter = {};

			/**	Which access of the affected command must be waited-on from whatever comes after?
			 *	I.e., this member would mean the SOURCE access of a barrier that comes after the affected command.
			 */
			std::optional<vk::AccessFlags2KHR> mAccessHintAfter = {};

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
			vk::Buffer mBuffer; 
			vk::DeviceSize mOffset;
			vk::DeviceSize mSize;
		};

		struct image_sync_info
		{
			vk::Image mImage;
			vk::ImageSubresourceRange mSubresourceRange;
			std::optional<avk::layout::image_layout_transition> mLayoutTransition;
		};

		class barrier_data final
		{
		public:
			// Constructs a global execution barrier:
			barrier_data(avk::stage::execution_dependency aStages)
				: mStages{ aStages }, mAccesses{}, mQueueFamilyOwnershipTransfer{}, mSpecificData{} {}

			// Constructs a global memory barrier:
			barrier_data(avk::stage::execution_dependency aStages, avk::access::memory_dependency aAccesses)
				: mStages{ aStages }, mAccesses{ aAccesses }, mQueueFamilyOwnershipTransfer{}, mSpecificData{} {}

			// Constructs an image memory barrier:
			barrier_data(avk::stage::execution_dependency aStages, avk::access::memory_dependency aAccesses, avk::resource_reference<const avk::image_t> aImage, vk::ImageSubresourceRange aSubresourceRange)
				: mStages{ aStages }, mAccesses{ aAccesses }, mQueueFamilyOwnershipTransfer{}
				, mSpecificData{ image_sync_info{ aImage->handle(), aSubresourceRange, std::optional<avk::layout::image_layout_transition>{} } } {}

			// Constructs a buffer memory barrier:
			barrier_data(avk::stage::execution_dependency aStages, avk::access::memory_dependency aAccesses, avk::resource_reference<const avk::buffer_t> aBuffer, vk::DeviceSize aOffset, vk::DeviceSize aSize)
				: mStages{ aStages }, mAccesses{ aAccesses }, mQueueFamilyOwnershipTransfer{}
				, mSpecificData{ buffer_sync_info{ aBuffer->handle(), aOffset, aSize } } {}

			// Adds memory access, potentially turning a execution barrier into a memory barrier.
			barrier_data& with_memory_access(avk::access::memory_dependency aMemoryAccess)
			{
				mAccesses = aMemoryAccess;
				return *this;
			}

			// Adds an image layout transition to this barrier_data:
			barrier_data& with_layout_transition(avk::layout::image_layout_transition aLayoutTransition)
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
			[[nodiscard]] decltype(avk::access::memory_dependency::mSrc) src_access() const {
				return mAccesses.has_value() ? mAccesses->mSrc : decltype(avk::access::memory_dependency::mSrc){};
			}
			[[nodiscard]] decltype(avk::access::memory_dependency::mDst) dst_access() const {
				return mAccesses.has_value() ? mAccesses->mDst : decltype(avk::access::memory_dependency::mDst){};
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
			avk::stage::execution_dependency mStages;
			std::optional<avk::access::memory_dependency> mAccesses;
			std::optional<queue_family_info> mQueueFamilyOwnershipTransfer;
			std::variant<std::monostate, buffer_sync_info, image_sync_info> mSpecificData;
		};

		inline static barrier_data global_execution_barrier(avk::stage::execution_dependency aStages)
		{
			return barrier_data{ aStages };
		}

		inline static barrier_data global_memory_barrier(avk::stage::execution_dependency aStages, avk::access::memory_dependency aAccesses = avk::access::none >> avk::access::none)
		{
			return barrier_data{ aStages, aAccesses };
		}

		inline static barrier_data image_memory_barrier(avk::resource_reference<const avk::image_t> aImage, avk::stage::execution_dependency aStages, avk::access::memory_dependency aAccesses = avk::access::none >> avk::access::none)
		{
			return barrier_data{ aStages, aAccesses, aImage, aImage->entire_subresource_range() };
		}

		inline static barrier_data buffer_memory_barrier(avk::resource_reference<const avk::buffer_t> aBuffer, avk::stage::execution_dependency aStages, avk::access::memory_dependency aAccesses = avk::access::none >> avk::access::none)
		{
			return barrier_data{ aStages, aAccesses, aBuffer, 0, VK_WHOLE_SIZE };
		}
	}

	// Define recorded* type:
	using recorded_commands_and_sync_instructions_t = std::variant<command::state_type_command, command::action_type_command, sync::barrier_data>;
	
	namespace command
	{
		struct state_type_command final
		{
			//state_type_command(const state_type_command&) = delete;
			//state_type_command(state_type_command&&) noexcept = default;
			//state_type_command& operator=(const state_type_command&) = delete;
			//state_type_command& operator=(state_type_command&&) noexcept = default;
			//~state_type_command() = default;

			using rec_fun = std::function<void(avk::resource_reference<avk::command_buffer_t>)>;

			rec_fun mFun;
		};

		template <typename P, typename T>
		inline static state_type_command push_constants(const P& aPipeline, const T& aPushConstants, vk::ShaderStageFlags aStageFlags = vk::ShaderStageFlagBits::eAll)
		{
			return state_type_command{
				[
					lLayoutHandle = aPipeline->layout_handle(),
					lPushConstants = aPushConstants,
					lStageFlags = aStageFlags
				] (avk::resource_reference<avk::command_buffer_t> cb) {
					cb->handle().pushConstants(lLayoutHandle, lStageFlags, 0, sizeof(lPushConstants), &lPushConstants);
				}
			};
		};

		struct action_type_command final
		{
			//action_type_command(const action_type_command&) = delete;
			//action_type_command(action_type_command&&) noexcept = default;
			//action_type_command& operator=(const action_type_command&) = delete;
			//action_type_command& operator=(action_type_command&&) noexcept = default;
			//~action_type_command() = default;

			using rec_fun = std::function<void(avk::resource_reference<avk::command_buffer_t>)>;

			avk::sync::sync_hint mSyncHint = {};
			rec_fun mBeginFun = {};
			std::vector<recorded_commands_and_sync_instructions_t> mNestedCommandsAndSyncInstructions;
			rec_fun mEndFun = {};

			action_type_command& handle_lifetime_of(any_owning_resource_t aResource)
			{
				mLifetimeHandledResources.push_back(std::move(aResource));
				return *this;
			}
			
			std::vector<any_owning_resource_t> mLifetimeHandledResources;
		};

		/**	A utility function that creates an action_type_command which consists solely of custom commmands.
		 *	@param	aCommandRecordingCallback	Must be convertible to action_type_command::rec_fun, which is
		 *	                                    a function that takes one parameter of type
		 *										avk::resource_reference<avk::command_buffer_t>, and returns void.
		 *
		 *	@example                            custom_command([](avk::resource_reference<avk::command_buffer_t> cb) {
		 *	                                        cb->draw_vertices(1, 1, 0, 0);
		 *                                      }
		 */
		template <typename F>
		inline static avk::command::action_type_command custom_commands(F aCommandRecordingCallback)
		{
			return avk::command::action_type_command{
				avk::sync::sync_hint {}, // No sync hints by default
				std::move(aCommandRecordingCallback)
			};
		}
		
		extern action_type_command render_pass(
			avk::resource_reference<const avk::renderpass_t> aRenderpass,
			avk::resource_reference<avk::framebuffer_t> aFramebuffer,
			std::vector<recorded_commands_and_sync_instructions_t> aNestedCommandsAndSyncInstructions = {},
			vk::Offset2D aRenderAreaOffset = { 0, 0 }, 
			std::optional<vk::Extent2D> aRenderAreaExtent = {}, 
			bool aSubpassesInline = true
		);

		extern state_type_command bind(avk::resource_reference<const graphics_pipeline_t> aPipeline);

		extern state_type_command bind_descriptors(avk::resource_reference<const graphics_pipeline_t> aPipeline, std::vector<descriptor_set> aDescriptorSets);

		extern action_type_command draw(uint32_t aVertexCount, uint32_t aInstanceCount, uint32_t aFirstVertex, uint32_t aFirstInstance);

		template <typename... Rest>
		void bind_vertex_buffer(vk::Buffer* aHandlePtr, vk::DeviceSize* aOffsetPtr)
		{
			// stop this reCURSEion!
		}

		template <typename... Rest>
		void bind_vertex_buffer(vk::Buffer* aHandlePtr, vk::DeviceSize* aOffsetPtr, resource_reference<const buffer_t> aVertexBuffer, Rest... aRest)
		{
			*aHandlePtr = aVertexBuffer->handle();
			*aOffsetPtr = 0;
			bind_vertex_buffer(aHandlePtr + 1, aOffsetPtr + 1, aRest...);
		}

		template <typename... Rest>
		void bind_vertex_buffer(vk::Buffer* aHandlePtr, vk::DeviceSize* aOffsetPtr, std::tuple<resource_reference<const buffer_t>, size_t> aVertexBufferAndOffset, Rest... aRest)
		{
			*aHandlePtr = std::get<resource_reference<const buffer_t>>(aVertexBufferAndOffset)->handle();
			*aOffsetPtr = static_cast<vk::DeviceSize>(std::get<size_t>(aVertexBufferAndOffset));
			bind_vertex_buffer(aHandlePtr + 1, aOffsetPtr + 1, aRest...);
		}
		
		/**	Perform an indexed draw call with vertex buffer bindings starting at BUFFER-BINDING #0 top to the number of total vertex buffers passed -1.
		 *	"BUFFER-BINDING" means that it corresponds to the binding specified in `input_binding_location_data::from_buffer_at_binding`.
		 *	There can be no gaps between buffer bindings.
		 *	@param	aIndexBuffer		Reference to an index buffer
		 *	@param	aNumberOfInstances	Number of instances to draw
		 *	@param	aFirstIndex			Offset to the first index
		 *	@param	aVertexOffset		Offset to the first vertex
		 *	@param	aFirstInstance		The ID of the first instance
		 *	@param	aVertexBuffers		Multiple const-references to buffers, or tuples of const-references to buffers + offsets.
		 *								First case:   Pass resource_reference<const buffer_t> types!
		 *								Second case:  Pass tuples of type std::tuple<resource_reference<const buffer_t>, size_t>!
		 *								Note:         You MUST manually convert to avk::resource_reference via avk::const_referenced!
		 */
		template <typename... Bfrs>
		action_type_command draw_indexed(avk::resource_reference<const buffer_t> aIndexBuffer, uint32_t aNumberOfInstances, uint32_t aFirstIndex, uint32_t aVertexOffset, uint32_t aFirstInstance, Bfrs... aVertexBuffers)
		{
			constexpr size_t N = sizeof...(aVertexBuffers);
			std::array<vk::Buffer, N> handles;
			std::array<vk::DeviceSize, N> offsets;
			bind_vertex_buffer(&handles[0], &offsets[0], aVertexBuffers...);

			const auto& indexMeta = aIndexBuffer->template meta<avk::index_buffer_meta>();
			vk::IndexType indexType;
			switch (indexMeta.sizeof_one_element()) {
				case sizeof(uint16_t) : indexType = vk::IndexType::eUint16; break;
				case sizeof(uint32_t) : indexType = vk::IndexType::eUint32; break;
				default: AVK_LOG_ERROR("The given size[" + std::to_string(indexMeta.sizeof_one_element()) + "] does not correspond to a valid vk::IndexType"); break;
			}

			return action_type_command{
				avk::sync::sync_hint {
					vk::PipelineStageFlagBits2KHR::eAllGraphics,
					vk::AccessFlagBits2KHR::eInputAttachmentRead | vk::AccessFlagBits2KHR::eColorAttachmentRead | vk::AccessFlagBits2KHR::eColorAttachmentWrite | vk::AccessFlagBits2KHR::eDepthStencilAttachmentRead | vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite,
					vk::PipelineStageFlagBits2KHR::eAllGraphics,
					vk::AccessFlagBits2KHR::eColorAttachmentWrite | vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite
				},
				[
					lBindingCount = static_cast<uint32_t>(N),
					handles, offsets, indexType,
					lNumElemments = static_cast<uint32_t>(indexMeta.num_elements()),
					lIndexBufferHandle = aIndexBuffer->handle(),
					aNumberOfInstances, aFirstIndex, aVertexOffset, aFirstInstance
				](avk::resource_reference<avk::command_buffer_t> cb) {
					cb->handle().bindVertexBuffers(
						0u, // TODO: Should the first binding really always be 0?
						lBindingCount, handles.data(), offsets.data()
					);
					cb->handle().bindIndexBuffer(lIndexBufferHandle, 0u, indexType);
					cb->handle().drawIndexed(lNumElemments, aNumberOfInstances, aFirstIndex, aVertexOffset, aFirstInstance);
				}
			};
		}

		/**	Perform an indexed draw call with vertex buffer bindings starting at BUFFER-BINDING #0 top to the number of total vertex buffers passed -1.
		 *	"BUFFER-BINDING" means that it corresponds to the binding specified in `input_binding_location_data::from_buffer_at_binding`.
		 *	There can be no gaps between buffer bindings.
		 *	Number of instances is set to 1.
		 *	The first index is set to 0.
		 *	The vertex offset is set to 0.
		 *	The ID of the first instance is set to 0.
		 *	@param	aIndexBuffer		Reference to an index buffer
		 *	@param	aVertexBuffers		References to one or multiple vertex buffers, i.e. you MUST manually convert
		 *								to avk::resource_reference, either via avk::referenced or via avk::const_referenced
		 */
		template <typename... Bfrs>
		action_type_command draw_indexed(avk::resource_reference<const buffer_t> aIndexBuffer, Bfrs... aVertexBuffers)
		{
			return draw_indexed(std::move(aIndexBuffer), 1u, 0u, 0u, 0u, std::move(aVertexBuffers) ...);
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

	struct semaphore_wait_info_owning
	{
		avk::resource_ownership<avk::semaphore_t> mWaitSemaphore;
		avk::stage::pipeline_stage_flags mDstStage;
	};

	inline semaphore_wait_info_owning operator>> (avk::resource_ownership<avk::semaphore_t> a, avk::stage::pipeline_stage_flags b)
	{
		return semaphore_wait_info_owning{ std::move(a), b };
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

	struct semaphore_signal_info_owning
	{
		avk::stage::pipeline_stage_flags mSrcStage;
		avk::resource_ownership<avk::semaphore_t> mSignalSemaphore;
	};

	inline semaphore_signal_info_owning operator>> (avk::stage::pipeline_stage_flags a, avk::resource_ownership<avk::semaphore_t> b)
	{
		return semaphore_signal_info_owning{ a, std::move(b) };
	}

	class recorded_command_buffer;

	// Something that submits stuff to a queue.
	// The submission itself either happens in submit() or in this class' destructor, if submit()/go()/do_it() has never been invoked before.
	class submission_data final
	{
	public:
		submission_data(const root* aRoot, avk::resource_reference<avk::command_buffer_t> aCommandBuffer, const queue* aQueue, const avk::recorded_command_buffer* aDangerousRecordedCommandBufferPointer = nullptr)
			: mRoot{ aRoot }
			, mCommandBufferToSubmit{ std::move(aCommandBuffer) }
			, mQueueToSubmitTo{ aQueue }
			, mSubmissionCount{ 0u }
			, mDangerousRecordedCommandBufferPointer{ aDangerousRecordedCommandBufferPointer }
		{}
		submission_data(const root* aRoot, avk::resource_reference<avk::command_buffer_t> aCommandBuffer, semaphore_wait_info aSemaphoreWaitInfo, const avk::recorded_command_buffer* aDangerousRecordedCommandBufferPointer = nullptr)
			: mRoot{ aRoot }
			, mCommandBufferToSubmit{ std::move(aCommandBuffer) }
			, mQueueToSubmitTo{ nullptr }
			, mSemaphoreWaits{ std::move(aSemaphoreWaitInfo) }
			, mSubmissionCount{ 0u }
			, mDangerousRecordedCommandBufferPointer{ aDangerousRecordedCommandBufferPointer }
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

		const auto* recorded_command_buffer_ptr() const { return mDangerousRecordedCommandBufferPointer; }

	private:
		const root* mRoot = nullptr;
		avk::resource_reference<avk::command_buffer_t> mCommandBufferToSubmit;
		const queue* mQueueToSubmitTo;
		std::vector<semaphore_wait_info> mSemaphoreWaits;
		std::vector<semaphore_signal_info> mSemaphoreSignals;
		std::optional<avk::resource_reference<avk::fence_t>> mFence;
		uint32_t mSubmissionCount;
		const avk::recorded_command_buffer* mDangerousRecordedCommandBufferPointer;
	};

	class recorded_commands;

	// This class turns a std::vector<recorded_commands_and_sync_instructions_t> into an actual command buffer
	class recorded_command_buffer final
	{
	public:
		// The constructor performs all the parsing, therefore, there's no std::vector<recorded_commands_and_sync_instructions_t> member.
		recorded_command_buffer(const root* aRoot, const std::vector<recorded_commands_and_sync_instructions_t>& aRecordedCommandsAndSyncInstructions, avk::resource_reference<avk::command_buffer_t> aCommandBuffer, const avk::recorded_commands* aDangerousRecordedCommandsPointer = nullptr);
		
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

		const auto* recorded_commands_ptr() const { return mDangerousRecordedComandsPointer; }

	private:
		const root* mRoot;
		avk::resource_reference<avk::command_buffer_t> mCommandBufferToRecordInto;
		const avk::recorded_commands* mDangerousRecordedComandsPointer;
	};

	// This class gathers recorded commands which can then be recorded into a command buffer via into_command_buffer()
	// (or they can also be stored somewhere for later recording into a command buffer).
	// But this class is intended to be used as temporary object.
	class recorded_commands final
	{
	public:
		recorded_commands(const root* aRoot, std::vector<recorded_commands_and_sync_instructions_t> aRecordedCommandsAndSyncInstructions);
		recorded_commands(const recorded_commands&) = delete;
		recorded_commands(recorded_commands&&) noexcept = default;
		recorded_commands& operator=(const recorded_commands&) = delete;
		recorded_commands& operator=(recorded_commands&&) noexcept = default;
		~recorded_commands() = default;
		
		recorded_commands& move_into(std::vector<recorded_commands_and_sync_instructions_t>& aTarget);
		recorded_commands& prepend_by(std::vector<recorded_commands_and_sync_instructions_t>& aCommands);
		recorded_commands& append_by(std::vector<recorded_commands_and_sync_instructions_t>& aCommands);

		recorded_commands& handle_lifetime_of(any_owning_resource_t aResource);

		std::vector<recorded_commands_and_sync_instructions_t> and_store();
		recorded_command_buffer into_command_buffer(avk::resource_reference<avk::command_buffer_t> aCommandBuffer);

		const auto& recorded_commands_and_sync_instructions() const { return mRecordedCommandsAndSyncInstructions; }

	private:
		const root* mRoot;
		std::vector<recorded_commands_and_sync_instructions_t> mRecordedCommandsAndSyncInstructions;
		std::vector<any_owning_resource_t> mLifetimeHandledResources;
	};

}
