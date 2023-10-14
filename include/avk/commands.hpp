#pragma once
#include "avk/avk.hpp"

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
			/**	In which stages and accesses does the associated command starts to perform its work, s.t.
			 *	previous commands can synchronize with it?
			 *	I.e., this member would mean the DESTINATION stage/access of a barrier that comes before the associated command.
			 */
			std::optional<stage_and_access_precisely> mDstForPreviousCmds = {};

			/**	In which stages and accesses does the associated command end to perform its work, s.t.
			 *	subsequent commands can synchronize with it?
			 *	I.e., this member would mean the SOURCE stage of a barrier that comes after the associated command.
			 */
			std::optional<stage_and_access_precisely> mSrcForSubsequentCmds = {};

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

		class sync_type_command final
		{
		public:
			// Constructs a global execution barrier:
			sync_type_command(avk::stage::execution_dependency aStages)
				: mStages{ aStages }, mAccesses{}, mQueueFamilyOwnershipTransfer{}, mSpecificData{} {}

			// Constructs a global memory barrier:
			sync_type_command(avk::stage::execution_dependency aStages, avk::access::memory_dependency aAccesses)
				: mStages{ aStages }, mAccesses{ aAccesses }, mQueueFamilyOwnershipTransfer{}, mSpecificData{} {}

			// Constructs an image memory barrier:
			sync_type_command(avk::stage::execution_dependency aStages, avk::access::memory_dependency aAccesses, const avk::image_t& aImage, vk::ImageSubresourceRange aSubresourceRange)
				: mStages{ aStages }, mAccesses{ aAccesses }, mQueueFamilyOwnershipTransfer{}
				, mSpecificData{ image_sync_info{ aImage.handle(), aSubresourceRange, std::optional<avk::layout::image_layout_transition>{} } } 
			{}

			// Constructs a buffer memory barrier:
			sync_type_command(avk::stage::execution_dependency aStages, avk::access::memory_dependency aAccesses, const avk::buffer_t& aBuffer, vk::DeviceSize aOffset, vk::DeviceSize aSize)
				: mStages{ aStages }, mAccesses{ aAccesses }, mQueueFamilyOwnershipTransfer{}
				, mSpecificData{ buffer_sync_info{ aBuffer.handle(), aOffset, aSize } } 
			{}

			// Adds memory access, potentially turning a execution barrier into a memory barrier.
			sync_type_command& with_memory_access(avk::access::memory_dependency aMemoryAccess)
			{
				mAccesses = aMemoryAccess;
				return *this;
			}

			// Adds an image layout transition to this sync_type_command:
			sync_type_command& with_layout_transition(avk::layout::image_layout_transition aLayoutTransition)
			{
				if (!std::holds_alternative<image_sync_info>(mSpecificData)) {
					throw avk::runtime_error("with_layout_transition has been called for a sync_type_command which does not represent an image memory barrier.");
				}
				std::get<image_sync_info>(mSpecificData).mLayoutTransition = aLayoutTransition;
				return *this;
			}

			// Adds a subresource range to this sync_type_command:
			sync_type_command& for_subresource_range(vk::ImageSubresourceRange aSubresourceRange)
			{
				if (!std::holds_alternative<image_sync_info>(mSpecificData)) {
					throw avk::runtime_error("for_subresource_range has been called for a sync_type_command which does not represent an image memory barrier.");
				}
				std::get<image_sync_info>(mSpecificData).mSubresourceRange = aSubresourceRange;
				return *this;
			}

			// Adds buffer offset and size to this sync_type_command:
			sync_type_command& for_offset_and_size(vk::DeviceSize aOffset, vk::DeviceSize aSize)
			{
				if (!std::holds_alternative<buffer_sync_info>(mSpecificData)) {
					throw avk::runtime_error("for_offset_and_size has been called for a sync_type_command which does not represent a buffer memory barrier.");
				}
				std::get<buffer_sync_info>(mSpecificData).mOffset = aOffset;
				std::get<buffer_sync_info>(mSpecificData).mSize   = aSize;
				return *this;
			}

			// Adds an queue family ownership transfer to this sync_type_command:
			sync_type_command& with_queue_family_ownership_transfer(uint32_t aSrcQueueFamilyIndex, uint32_t aDstQueueFamilyIndex)
			{
				if (!std::holds_alternative<image_sync_info>(mSpecificData) && !std::holds_alternative<buffer_sync_info>(mSpecificData)) {
					throw avk::runtime_error("with_queue_family_ownership_transfer has been called for a sync_type_command which does not represent an image memory barrier nor a buffer memory barrier.");
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

		/**	Create a global execution barrier which only has execution dependencies, but no memory dependencies (i.e., both access scopes set to eNone).
		 *	The best way to create the execution_dependency parameter is to use operator>> to combine source and destination stages as follows:
		 *	Example:    avk::stage::copy >> avk::stage::fragment_shader
		 *	            ^ This creates an execution_dependency with source stage eCopy and destination stage eFragmentShader
		 *
		 *	@param	aStages		Source and destination stages of this global execution barrier.
		 *						Create it by using operator>> with two avk::stage::pipeline_stage_flags operands!
		 *
		 *	@return	An avk::sync::sync_type_command instance which contains all the relevant data for recording a memory barrier into a command buffer
		 */
		inline static sync_type_command global_execution_barrier(avk::stage::execution_dependency aStages)
		{
			return sync_type_command{ aStages };
		}

		/**	Create a global memory barrier which has execution and memory barriers.
		 *	The best way to create the aStages and aAccesses parameters is to use operator>> to combine source and destination stages, or source and destination access masks as follows:
		 *	Example:    avk::stage::copy >> avk::stage::fragment_shader,   avk::access::transfer_write >> avk::access::shader_read
		 *	            ^ This creates an execution_dependency with source stage eCopy and destination stage eFragmentShader,
		 *	              and a memory_dependency with source access eTransferWrite and destination access eShaderRead.
		 *
		 *	@param	aStages		Source and destination stages of this global memory barrier.
		 *						Create it by using operator>> with two avk::stage::pipeline_stage_flags operands!
		 *	@param	aAccesses	Source and destination access flags of this global memory barrier.
		 *						Create it by using operator>> with two avk::access::memory_access_flags operands!
		 *
		 *	@return	An avk::sync::sync_type_command instance which contains all the relevant data for recording a memory barrier into a command buffer
		 */
		inline static sync_type_command global_memory_barrier(avk::stage::execution_dependency aStages, avk::access::memory_dependency aAccesses = avk::access::none >> avk::access::none)
		{
			return sync_type_command{ aStages, aAccesses };
		}

		/**	Syntactic-sugary alternative to sync::global_memory_barrier, where stages and accesses can be passed as follows:
		 *	Example:    avk::stage::copy + avk::access::transfer_write >> avk::stage::fragment_shader + avk::access::shader_read
		 *	            ^ This creates an execution_dependency with source stage eCopy and source access aTransferWrite,
		 *				  and destination stage eFragmentShader with destination access eShaderRead.
		 *
		 *	@param	aDependency	Source and destination stages and memory accesses of this global memory barrier.
		 *						Create it by using operator+ with avk::stage::pipeline_stage_flags and avk::access::memory_access_flags 
		 *						to create source and destination data, then combine source and destination with operator>>.
		 *
		 *	@return	An avk::sync::sync_type_command instance which contains all the relevant data for recording a memory barrier into a command buffer
		 */
		inline static sync_type_command global_memory_barrier(avk::stage_and_access_dependency aDependency)
		{
			return global_memory_barrier(aDependency.mSrc.mStage >> aDependency.mDst.mStage, aDependency.mSrc.mAccess >> aDependency.mDst.mAccess);
		}

		/**	Create an image memory barrier which has execution and memory barriers.
		 *	The best way to create the aStages and aAccesses parameters is to use operator>> to combine source and destination stages, or source and destination access masks as follows:
		 *	Example:    avk::stage::copy >> avk::stage::fragment_shader,   avk::access::transfer_write >> avk::access::shader_read
		 *	            ^ This creates an execution_dependency with source stage eCopy and destination stage eFragmentShader,
		 *	              and a memory_dependency with source access eTransferWrite and destination access eShaderRead.
		 *
		 *	@param	aImage		The image this image memory barrier refers to.
		 *	@param	aStages		Source and destination stages of this image memory barrier.
		 *						Create it by using operator>> with two avk::stage::pipeline_stage_flags operands!
		 *	@param	aAccesses	Source and destination access flags of this image memory barrier.
		 *						Create it by using operator>> with two avk::access::memory_access_flags operands!
		 *
		 *	@return	An avk::sync::sync_type_command instance which contains all the relevant data for recording a memory barrier into a command buffer
		 */
		inline static sync_type_command image_memory_barrier(const avk::image_t& aImage, avk::stage::execution_dependency aStages, avk::access::memory_dependency aAccesses = avk::access::none >> avk::access::none)
		{
			return sync_type_command{ aStages, aAccesses, aImage, aImage.entire_subresource_range() };
		}

		/**	Syntactic-sugary alternative to sync::image_memory_barrier, where stages and accesses can be passed as follows:
		 *	Example:    avk::stage::copy + avk::access::transfer_write >> avk::stage::fragment_shader + avk::access::shader_read
		 *	            ^ This creates an execution_dependency with source stage eCopy and source access aTransferWrite,
		 *				  and destination stage eFragmentShader with destination access eShaderRead.
		 *
		 *	@param	aImage		The image this image memory barrier refers to.
		 *	@param	aDependency	Source and destination stages and memory accesses of this image memory barrier.
		 *						Create it by using operator+ with avk::stage::pipeline_stage_flags and avk::access::memory_access_flags
		 *						to create source and destination data, then combine source and destination with operator>>.
		 *
		 *	@return	An avk::sync::sync_type_command instance which contains all the relevant data for recording a memory barrier into a command buffer
		 */
		inline static sync_type_command image_memory_barrier(const avk::image_t& aImage, avk::stage_and_access_dependency aDependency)
		{
			return image_memory_barrier(aImage, aDependency.mSrc.mStage >> aDependency.mDst.mStage, aDependency.mSrc.mAccess >> aDependency.mDst.mAccess);
		}

		/**	Create a buffer memory barrier which has execution and memory barriers.
		 *	The best way to create the aStages and aAccesses parameters is to use operator>> to combine source and destination stages, or source and destination access masks as follows:
		 *	Example:    avk::stage::copy >> avk::stage::fragment_shader,   avk::access::transfer_write >> avk::access::shader_read
		 *	            ^ This creates an execution_dependency with source stage eCopy and destination stage eFragmentShader,
		 *	              and a memory_dependency with source access eTransferWrite and destination access eShaderRead.
		 *
		 *	@param	aBuffer		The buffer this buffer memory barrier refers to.
		 *	@param	aStages		Source and destination stages of this buffer memory barrier.
		 *						Create it by using operator>> with two avk::stage::pipeline_stage_flags operands!
		 *	@param	aAccesses	Source and destination access flags of this buffer memory barrier.
		 *						Create it by using operator>> with two avk::access::memory_access_flags operands!
		 *
		 *	@return	An avk::sync::sync_type_command instance which contains all the relevant data for recording a memory barrier into a command buffer
		 */
		inline static sync_type_command buffer_memory_barrier(const avk::buffer_t& aBuffer, avk::stage::execution_dependency aStages, avk::access::memory_dependency aAccesses = avk::access::none >> avk::access::none)
		{
			return sync_type_command{ aStages, aAccesses, aBuffer, 0, VK_WHOLE_SIZE };
		}

		/**	Syntactic-sugary alternative to sync::buffer_memory_barrier, where stages and accesses can be passed as follows:
		 *	Example:    avk::stage::copy + avk::access::transfer_write >> avk::stage::fragment_shader + avk::access::shader_read
		 *	            ^ This creates an execution_dependency with source stage eCopy and source access aTransferWrite,
		 *				  and destination stage eFragmentShader with destination access eShaderRead.
		 *
		 *	@param	aBuffer		The buffer this buffer memory barrier refers to.
		 *	@param	aDependency	Source and destination stages and memory accesses of this buffer memory barrier.
		 *						Create it by using operator+ with avk::stage::pipeline_stage_flags and avk::access::memory_access_flags
		 *						to create source and destination data, then combine source and destination with operator>>.
		 *
		 *	@return	An avk::sync::sync_type_command instance which contains all the relevant data for recording a memory barrier into a command buffer
		 */
		inline static sync_type_command buffer_memory_barrier(const avk::buffer_t& aBuffer, avk::stage_and_access_dependency aDependency)
		{
			return buffer_memory_barrier(aBuffer, aDependency.mSrc.mStage >> aDependency.mDst.mStage, aDependency.mSrc.mAccess >> aDependency.mDst.mAccess);
		}
	}

	// Define recorded* type:
	using recorded_commands_t = std::variant<command::state_type_command, command::action_type_command, sync::sync_type_command>;
	
	namespace command
	{
		struct state_type_command final
		{
			//state_type_command(const state_type_command&) = delete;
			//state_type_command(state_type_command&&) noexcept = default;
			//state_type_command& operator=(const state_type_command&) = delete;
			//state_type_command& operator=(state_type_command&&) noexcept = default;
			//~state_type_command() = default;

			using rec_fun = std::function<void(avk::command_buffer_t&)>;

			rec_fun mFun;
		};

		template <typename PL, typename D>
		inline static state_type_command push_constants(const PL& aPipelineLayoutTuple, D aData, std::optional<shader_type> aShaderStages = {})
		{
			auto dataSize = static_cast<uint32_t>(sizeof(aData));
			std::optional<vk::ShaderStageFlags> stageFlags;
			if (aShaderStages.has_value()) {
				stageFlags = to_vk_shader_stages(aShaderStages.value());
			}
			if (!stageFlags.has_value()) {
				auto pcRanges = std::get<const std::vector<vk::PushConstantRange>*>(aPipelineLayoutTuple);
				for (auto& r : *pcRanges) {
					if (r.size == dataSize) {
						stageFlags = r.stageFlags;
						break;
					}
					// TODO: How to deal with push constants of same size and multiple vk::PushConstantRanges??
				}
				if (!stageFlags.has_value()) {
					AVK_LOG_WARNING("No vk::PushConstantRange entry found that matches the dataSize[" + std::to_string(dataSize) + "]");
				}
			}

			return state_type_command{
				[
					lLayoutHandle = std::get<const vk::PipelineLayout>(aPipelineLayoutTuple),
					lStageFlags = stageFlags.value_or(vk::ShaderStageFlagBits::eAll),
					lDataSize = dataSize,
					aData
				] (avk::command_buffer_t& cb) {
					cb.handle().pushConstants(
						lLayoutHandle,
						lStageFlags,
						0, // TODO: How to deal with offset?
						lDataSize,
						&aData
					);
				}
			};
		};

		template <typename PL, typename D>
		inline static state_type_command push_constants(const PL& aPipelineLayoutTuple, const D* aDataPtr, std::optional<shader_type> aShaderStages = {})
		{
			auto dataSize = static_cast<uint32_t>(sizeof(*aDataPtr));
			std::optional<vk::ShaderStageFlags> stageFlags;
			if (aShaderStages.has_value()) {
				stageFlags = to_vk_shader_stages(aShaderStages.value());
			}
			if (!stageFlags.has_value()) {
				auto pcRanges = std::get<const std::vector<vk::PushConstantRange>*>(aPipelineLayoutTuple);
				for (auto& r : *pcRanges) {
					if (r.size == dataSize) {
						stageFlags = r.stageFlags;
						break;
					}
					// TODO: How to deal with push constants of same size and multiple vk::PushConstantRanges??
				}
				if (!stageFlags.has_value()) {
					AVK_LOG_WARNING("No vk::PushConstantRange entry found that matches the dataSize[" + std::to_string(dataSize) + "]");
				}
			}

			return state_type_command{
				[
					lLayoutHandle = std::get<const vk::PipelineLayout>(aPipelineLayoutTuple),
					lStageFlags = stageFlags.value_or(vk::ShaderStageFlagBits::eAll),
					lDataSize = dataSize,
					aDataPtr
				] (avk::command_buffer_t& cb) {
					cb.handle().pushConstants(
						lLayoutHandle,
						lStageFlags,
						0, // TODO: How to deal with offset?
						lDataSize,
						aDataPtr);
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

			using rec_fun = std::function<void(avk::command_buffer_t&)>;

			avk::sync::sync_hint mSyncHint = {};
			std::vector<std::tuple<std::variant<vk::Image, vk::Buffer>, avk::sync::sync_hint>> mResourceSpecificSyncHints;
			rec_fun mBeginFun = {};
			std::vector<recorded_commands_t> mNestedCommandsAndSyncInstructions;
			rec_fun mEndFun = {};

			/**	Takes care of the lifetime of the given resource.
			 *	It will be deleted when this action_type goes out of scope. TODO: Or will it?
			 */
			action_type_command& handle_lifetime_of(any_owning_resource_t aResource)
			{
				mLifetimeHandledResources.push_back(std::move(aResource));
				return *this;
			}

			/**	IF there are entries in mResourceSpecificSyncHints, overwrited whatever values are set in
			 *	the mSyncHint member by accumulating all the sync hints from the mResourceSpecificSyncHints.
			 */
			void infer_sync_hint_from_resource_sync_hints()
			{
				if (mResourceSpecificSyncHints.empty()) {
					return;
				}

				vk::PipelineStageFlags2KHR dstStageForPrevCmds  = vk::PipelineStageFlagBits2KHR::eNone;
				vk::AccessFlags2KHR        dstAccessForPrevCmds = vk::AccessFlagBits2KHR::eNone;
				vk::PipelineStageFlags2KHR srcStageForSubsCmds  = vk::PipelineStageFlagBits2KHR::eNone;
				vk::AccessFlags2KHR        srcAccessForSubsCmds = vk::AccessFlagBits2KHR::eNone;

				for (const auto& [res, resSyncHint] : mResourceSpecificSyncHints) {
					if (resSyncHint.mDstForPreviousCmds.has_value()) {
						dstStageForPrevCmds  |= resSyncHint.mDstForPreviousCmds.value().mStage;
						dstAccessForPrevCmds |= resSyncHint.mDstForPreviousCmds.value().mAccess;
					}
					if (resSyncHint.mSrcForSubsequentCmds.has_value()) {
						srcStageForSubsCmds  |= resSyncHint.mSrcForSubsequentCmds.value().mStage;
						srcAccessForSubsCmds |= resSyncHint.mSrcForSubsequentCmds.value().mAccess;
					}
				}
				
				mSyncHint.mDstForPreviousCmds.emplace();
				mSyncHint.mDstForPreviousCmds->mStage  = dstStageForPrevCmds;
				mSyncHint.mDstForPreviousCmds->mAccess = dstAccessForPrevCmds;

				mSyncHint.mSrcForSubsequentCmds.emplace();
				mSyncHint.mSrcForSubsequentCmds->mStage  = srcStageForSubsCmds;
				mSyncHint.mSrcForSubsequentCmds->mAccess = srcAccessForSubsCmds;
			}

			/**	Overwrites the mSyncHint member with the mSyncHint of the first nested action command's "before" sync hint,
			 *	and the "after" sync hint with the last nested action command's "after" sync hint.
			 *	TODO: implement a better logic for this!
			 *	TODO: Maybe also take sync_type_commands into account? <--- Particularly image layout transitions?!
			 */
			void infer_sync_hint_from_nested_commands()
			{
				if (mNestedCommandsAndSyncInstructions.empty()) {
					return;
				}

				vk::PipelineStageFlags2KHR dstStageForPrevCmds  = vk::PipelineStageFlagBits2KHR::eNone;
				vk::AccessFlags2KHR        dstAccessForPrevCmds = vk::AccessFlagBits2KHR::eNone;
				vk::PipelineStageFlags2KHR srcStageForSubsCmds  = vk::PipelineStageFlagBits2KHR::eNone;
				vk::AccessFlags2KHR        srcAccessForSubsCmds = vk::AccessFlagBits2KHR::eNone;

				for (auto it = mNestedCommandsAndSyncInstructions.begin(); it != mNestedCommandsAndSyncInstructions.end(); ++it) {
					if (std::holds_alternative<command::action_type_command>(*it)) {
						if (std::get<command::action_type_command>(*it).mSyncHint.mDstForPreviousCmds.has_value()) {
							dstStageForPrevCmds  |= std::get<command::action_type_command>(*it).mSyncHint.mDstForPreviousCmds.value().mStage;
							dstAccessForPrevCmds |= std::get<command::action_type_command>(*it).mSyncHint.mDstForPreviousCmds.value().mAccess;
						}
					}
				}

				for (auto it = mNestedCommandsAndSyncInstructions.rbegin(); it != mNestedCommandsAndSyncInstructions.rend(); ++it) {
					if (std::holds_alternative<command::action_type_command>(*it)) {
						if (std::get<command::action_type_command>(*it).mSyncHint.mSrcForSubsequentCmds.has_value()) {
							srcStageForSubsCmds  |= std::get<command::action_type_command>(*it).mSyncHint.mSrcForSubsequentCmds.value().mStage;
							srcAccessForSubsCmds |= std::get<command::action_type_command>(*it).mSyncHint.mSrcForSubsequentCmds.value().mAccess;
						}
					}
				}

				mSyncHint.mDstForPreviousCmds   = { dstStageForPrevCmds, dstAccessForPrevCmds };
				mSyncHint.mSrcForSubsequentCmds = { srcStageForSubsCmds, srcAccessForSubsCmds };
			}

			std::vector<any_owning_resource_t> mLifetimeHandledResources;
		};

		/**	A utility function that creates an action_type_command which consists solely of custom commmands.
		 *	@param	aCommandRecordingCallback	Must be convertible to action_type_command::rec_fun, which is
		 *	                                    a function that takes one parameter of type
		 *										avk::command_buffer_t&, and returns void.
		 *
		 *	@example                            custom_command([](avk::command_buffer_t& cb) {
		 *	                                        cb->draw_vertices(1, 1, 0, 0);
		 *                                      }
		 */
		template <typename F>
		inline static avk::command::action_type_command custom_commands(F aCommandRecordingCallback)
		{
			return avk::command::action_type_command{
				{}, {}, // No sync hints by default
				std::move(aCommandRecordingCallback)
			};
		}

		/**	Begins a render pass for a given framebuffer
		 *	@param	aRenderpass			Renderpass which shall begin (auto lifetime handling not supported by this command)
		 *	@param	aFramebuffer		Framebuffer to use with the renderpass (auto lifetime handling not supported by this command)
		 *	@param	aRenderAreaOffset	Render area offset (default is (0,0), i.e., no offset)
		 *	@param	aRenderAreaExtent	Render area extent (default is full extent)
		 *	@param	aSubpassesInline	Whether or not subpasses are inline (default is true)
		 */
		extern action_type_command begin_render_pass_for_framebuffer(const renderpass_t& aRenderpass, const framebuffer_t& aFramebuffer, vk::Offset2D aRenderAreaOffset = { 0, 0 }, std::optional<vk::Extent2D> aRenderAreaExtent = {}, bool aSubpassesInline = true);

		/**	Ends a render pass
		 */
		extern action_type_command end_render_pass();

		/**	Begins and ends a render pass for a given framebuffer, and supports some nested commands to be recorded in between
		 *	@param	aRenderpass			Renderpass which shall begin (auto lifetime handling not supported by this command)
		 *	@param	aFramebuffer		Framebuffer to use with the renderpass (auto lifetime handling not supported by this command)
		 *	@param	aNestedCommands		Nested commands to be recorded between begin and end
		 *	@param	aRenderAreaOffset	Render area offset (default is (0,0), i.e., no offset)
		 *	@param	aRenderAreaExtent	Render area extent (default is full extent)
		 *	@param	aSubpassesInline	Whether or not subpasses are inline (default is true)
		 */
		extern action_type_command render_pass(
			const renderpass_t& aRenderpass,
			const framebuffer_t& aFramebuffer,
			std::vector<recorded_commands_t> aNestedCommands = {},
			vk::Offset2D aRenderAreaOffset = { 0, 0 }, 
			std::optional<vk::Extent2D> aRenderAreaExtent = {}, 
			bool aSubpassesInline = true
		);

		/** Advances to the next subpass within a render pass.
		 */
		extern action_type_command next_subpass(bool aSubpassesInline = true);

		/** Binds a graphics pipeline.
		 *	@param	aPipeline	The graphics pipeline to bind
		 */
		extern state_type_command bind_pipeline(const graphics_pipeline_t& aPipeline);

		/** Binds a compute pipeline.
		 *	@param	aPipeline	The graphics pipeline to bind
		 */
		extern state_type_command bind_pipeline(const compute_pipeline_t& aPipeline);

#if VK_HEADER_VERSION >= 135
		/** Binds a ray tracing pipeline.
		 *	@param	aPipeline	The graphics pipeline to bind
		 */
		extern state_type_command bind_pipeline(const ray_tracing_pipeline_t& aPipeline);
#endif 

		/** Binds a graphics pipeline.
		 *	@param	aPipelineLayout		The layout of the pipeline to bind descriptors to
		 *	@param	aDescriptorSets		The descriptor sets to be bound
		 */
		extern state_type_command bind_descriptors(std::tuple<const graphics_pipeline_t*, const vk::PipelineLayout, const std::vector<vk::PushConstantRange>*> aPipelineLayout, std::vector<descriptor_set> aDescriptorSets);

		/** Binds a graphics pipeline.
		 *	@param	aPipelineLayout		The layout of the pipeline to bind descriptors to
		 *	@param	aDescriptorSets		The descriptor sets to be bound
		 */
		extern state_type_command bind_descriptors(std::tuple<const compute_pipeline_t*, const vk::PipelineLayout, const std::vector<vk::PushConstantRange>*> aPipelineLayout, std::vector<descriptor_set> aDescriptorSets);

#if VK_HEADER_VERSION >= 135
		/** Binds a graphics pipeline.
		 *	@param	aPipelineLayout		The layout of the pipeline to bind descriptors to
		 *	@param	aDescriptorSets		The descriptor sets to be bound
		 */
		extern state_type_command bind_descriptors(std::tuple<const ray_tracing_pipeline_t*, const vk::PipelineLayout, const std::vector<vk::PushConstantRange>*> aPipelineLayout, std::vector<descriptor_set> aDescriptorSets);
#endif

		extern action_type_command draw(uint32_t aVertexCount, uint32_t aInstanceCount, uint32_t aFirstVertex, uint32_t aFirstInstance);

		template <typename... Rest>
		void bind_vertex_buffer(vk::Buffer* aHandlePtr, vk::DeviceSize* aOffsetPtr)
		{
			// stop this reCURSEion!
		}

	    template <typename... Rest>
		void bind_vertex_buffer(vk::Buffer* aHandlePtr, vk::DeviceSize* aOffsetPtr, const std::tuple<const buffer_t&, size_t>& aVertexBufferAndOffset, const Rest&... aRest);

		template <typename... Rest>
		void bind_vertex_buffer(vk::Buffer* aHandlePtr, vk::DeviceSize* aOffsetPtr, const buffer_t& aVertexBuffer, const Rest&... aRest)
		{
			*aHandlePtr = aVertexBuffer.handle();
			*aOffsetPtr = 0;
			bind_vertex_buffer(aHandlePtr + 1, aOffsetPtr + 1, aRest...);
		}

		template <typename... Rest>
		void bind_vertex_buffer(vk::Buffer* aHandlePtr, vk::DeviceSize* aOffsetPtr, const std::tuple<const buffer_t&, size_t>& aVertexBufferAndOffset, const Rest&... aRest)
		{
			*aHandlePtr = std::get<const buffer_t&>(aVertexBufferAndOffset).handle();
			*aOffsetPtr = static_cast<vk::DeviceSize>(std::get<size_t>(aVertexBufferAndOffset));
			bind_vertex_buffer(aHandlePtr + 1, aOffsetPtr + 1, aRest...);
		}

		/**	Draw vertices with vertex buffer bindings starting at BUFFER-BINDING #0 top to the number of total buffers passed -1.
		 *	"BUFFER-BINDING" means that it corresponds to the binding specified in `input_binding_location_data::from_buffer_at_binding`.
		 *	There can be no gaps between buffer bindings.
		 *  @param  aNumberOfVertices   Number of vertices to draw
		 *	@param	aNumberOfInstances	Number of instances to draw
		 *	@param	aFirstVertex		Offset to the first vertex
		 *	@param	aFirstInstance		The ID of the first instance
		 *	@param	aFurtherBuffers		Multiple const-references to buffers, or tuples of const-references to buffers + offsets.
		 *								First case:   Pass const buffer_t& types!
		 *								Second case:  Pass tuples of type std::tuple<const buffer_t&, size_t>!
		 *											  Hint:    std::forward_as_tuple might be useful to get that reference into a std::tuple.
		 *											  Example: avk::buffer myVertexBuffer;
		 *											           auto myTuple = std::forward_as_tuple(myVertexBuffer.get(), size_t{0});
		 */
		template <typename... Bfrs>
		action_type_command draw_vertices(uint32_t aNumberOfVertices, uint32_t aNumberOfInstances, uint32_t aFirstVertex, uint32_t aFirstInstance, const Bfrs&... aFurtherBuffers)
		{
			constexpr size_t N = sizeof...(aFurtherBuffers);

			if constexpr (N == 0) {
				return action_type_command{
					avk::sync::sync_hint {
						{{ // DESTINATION dependencies for previous commands:
							vk::PipelineStageFlagBits2KHR::eAllGraphics,
							vk::AccessFlagBits2KHR::eInputAttachmentRead | vk::AccessFlagBits2KHR::eColorAttachmentRead | vk::AccessFlagBits2KHR::eColorAttachmentWrite | vk::AccessFlagBits2KHR::eDepthStencilAttachmentRead | vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite
						}},
						{{ // SOURCE dependencies for subsequent commands:
							vk::PipelineStageFlagBits2KHR::eAllGraphics,
							vk::AccessFlagBits2KHR::eColorAttachmentWrite | vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite
						}}
					},
					{}, // no resource-specific sync hints
					[
						aNumberOfVertices, aNumberOfInstances, aFirstVertex, aFirstInstance
					](avk::command_buffer_t& cb) {
						cb.handle().draw(aNumberOfVertices, aNumberOfInstances, aFirstVertex, aFirstInstance);
					}
				};
 			}
 			else {
				std::array<vk::Buffer, N> handles;
				std::array<vk::DeviceSize, N> offsets;
				bind_vertex_buffer(&handles[0], &offsets[0], aFurtherBuffers...);

				return action_type_command{
					avk::sync::sync_hint {
						{{ // DESTINATION dependencies for previous commands:
							vk::PipelineStageFlagBits2KHR::eAllGraphics,
							vk::AccessFlagBits2KHR::eInputAttachmentRead | vk::AccessFlagBits2KHR::eColorAttachmentRead | vk::AccessFlagBits2KHR::eColorAttachmentWrite | vk::AccessFlagBits2KHR::eDepthStencilAttachmentRead | vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite
						}},
						{{ // SOURCE dependencies for subsequent commands:
							vk::PipelineStageFlagBits2KHR::eAllGraphics,
							vk::AccessFlagBits2KHR::eColorAttachmentWrite | vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite
						}}
					},
					{}, // no resource-specific sync hints
					[
						lBindingCount = static_cast<uint32_t>(N),
						handles, offsets, 
						aNumberOfVertices, aNumberOfInstances, aFirstVertex, aFirstInstance
				    ](avk::command_buffer_t& cb) {
						cb.handle().bindVertexBuffers(
							0u, // TODO: Should the first binding really always be 0?
							static_cast<uint32_t>(N), handles.data(), offsets.data()
						);
						cb.handle().draw(aNumberOfVertices, aNumberOfInstances, aFirstVertex, aFirstInstance);
					}
				};
			}
		}

		/**	Draw vertices with vertex buffer bindings starting at BUFFER-BINDING #0 top to the number of total buffers passed -1.
		 *	"BUFFER-BINDING" means that it corresponds to the binding specified in `input_binding_location_data::from_buffer_at_binding`.
		 *	There can be no gaps between buffer bindings.
		 *   Number of vertices is automatically determined from the vertex buffer
		 *	@param	aVertexBuffer		There must be at least one vertex buffer, the meta data of which will be used
		 *								to get the number of vertices to draw.
		 *	@param	aNumberOfInstances	Number of instances to draw
		 *	@param	aFirstVertex		Offset to the first vertex
		 *	@param	aFirstInstance		The ID of the first instance
		 *	@param	aFurtherBuffers		Multiple const-references to buffers, or tuples of const-references to buffers + offsets.
		 *								First case:   Pass const buffer_t& types!
		 *								Second case:  Pass tuples of type std::tuple<const buffer_t&, size_t>!
		 *											  Hint:    std::forward_as_tuple might be useful to get that reference into a std::tuple.
		 *											  Example: avk::buffer myVertexBuffer;
		 *											           auto myTuple = std::forward_as_tuple(myVertexBuffer.get(), size_t{0});
		 */
		template <typename... Bfrs>
		action_type_command draw_vertices(uint32_t aNumberOfInstances, uint32_t aFirstVertex, uint32_t aFirstInstance, const buffer_t& aVertexBuffer, const Bfrs&... aFurtherBuffers)
		{
			const auto& vertexMeta = aVertexBuffer.template meta<avk::vertex_buffer_meta>();
			return draw_vertices(static_cast<uint32_t>(vertexMeta.num_elements()), aNumberOfInstances, aFirstVertex, aFirstInstance, aVertexBuffer, aFurtherBuffers...);
		}
		
		/**	Draw vertices with vertex buffer bindings starting at BUFFER-BINDING #0 top to the number of total buffers passed -1.
		 *	"BUFFER-BINDING" means that it corresponds to the binding specified in `input_binding_location_data::from_buffer_at_binding`.
		 *	There can be no gaps between buffer bindings.
		 *  Number of vertices is automatically determined from the vertex buffer
		 *	Number of instances is set to 1.
		 *	Offset to the first vertex is set to 0.
		 *	The ID of the first instance is set to 0.
		 *	@param	aVertexBuffer		There must be at least one vertex buffer, the meta data of which will be used
		 *								to get the number of vertices to draw.
		 *	@param	aFurtherBuffers		Multiple const-references to buffers, or tuples of const-references to buffers + offsets.
		 *								First case:   Pass const buffer_t& types!
		 *								Second case:  Pass tuples of type std::tuple<const buffer_t&, size_t>!
		 *											  Hint:    std::forward_as_tuple might be useful to get that reference into a std::tuple.
		 *											  Example: avk::buffer myVertexBuffer;
		 *											           auto myTuple = std::forward_as_tuple(myVertexBuffer.get(), size_t{0});
		 */
		template <typename... Bfrs>
		action_type_command draw_vertices(const buffer_t& aVertexBuffer, const Bfrs&... aFurtherBuffers)
		{
			return draw_vertices(1u, 0u, 0u, aVertexBuffer, aFurtherBuffers...);
		}

		/**	Perform an indexed draw call with vertex buffer bindings starting at BUFFER-BINDING #0 top to the number of total vertex buffers passed -1.
		 *	"BUFFER-BINDING" means that it corresponds to the binding specified in `input_binding_location_data::from_buffer_at_binding`.
		 *	There can be no gaps between buffer bindings.
		 *	@param	aIndexBufferAndOffsetAndNumElements	Tuple of a const-reference and an offset and a number of elements to draw
		 *												Pass a tuple of type std::tuple<const buffer_t&, size_t, uint32_t>!
		 *												  Hint:    std::forward_as_tuple might be useful to get that reference into a std::tuple.
		 *												  Example: avk::buffer myIndexBuffer;
		 *												           auto myTuple = std::forward_as_tuple(myIndexBuffer.get(), size_t{0}, uint32_t{1});
		 *	@param	aNumberOfInstances		Number of instances to draw
		 *	@param	aFirstIndex				Offset to the first index
		 *	@param	aVertexOffset			Offset to the first vertex
		 *	@param	aFirstInstance			The ID of the first instance
		 *	@param	aVertexBuffers			Multiple const-references to buffers, or tuples of const-references to buffers + offsets.
		 *									First case:   Pass const buffer_t& types!
		 *									Second case:  Pass tuples of type std::tuple<const buffer_t&, size_t>!
		 *												  Hint:    std::forward_as_tuple might be useful to get that reference into a std::tuple.
		 *												  Example: avk::buffer myVertexBuffer;
		 *												           auto myTuple = std::forward_as_tuple(myVertexBuffer.get(), size_t{0});
		 */
		template <typename... Bfrs>
		action_type_command draw_indexed(const std::tuple<const buffer_t&, size_t, uint32_t>& aIndexBufferAndOffsetAndNumElements, uint32_t aNumberOfInstances, uint32_t aFirstIndex, uint32_t aVertexOffset, uint32_t aFirstInstance, const Bfrs&... aVertexBuffers)
		{
			constexpr size_t N = sizeof...(aVertexBuffers);
			std::array<vk::Buffer, N> handles;
			std::array<vk::DeviceSize, N> offsets;
			bind_vertex_buffer(&handles[0], &offsets[0], aVertexBuffers...);

			const auto& indexMeta = std::get<const buffer_t&>(aIndexBufferAndOffsetAndNumElements).template meta<avk::index_buffer_meta>();
			vk::IndexType indexType;
			switch (indexMeta.sizeof_one_element()) {
				case sizeof(uint16_t) : indexType = vk::IndexType::eUint16; break;
				case sizeof(uint32_t) : indexType = vk::IndexType::eUint32; break;
				default: AVK_LOG_ERROR("The given size[" + std::to_string(indexMeta.sizeof_one_element()) + "] does not correspond to a valid vk::IndexType"); break;
			}

			return action_type_command{
				avk::sync::sync_hint {
					{{ // DESTINATION dependencies for previous commands:
						vk::PipelineStageFlagBits2KHR::eAllGraphics,
						vk::AccessFlagBits2KHR::eInputAttachmentRead | vk::AccessFlagBits2KHR::eColorAttachmentRead | vk::AccessFlagBits2KHR::eColorAttachmentWrite | vk::AccessFlagBits2KHR::eDepthStencilAttachmentRead | vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite
					}},
					{{ // SOURCE dependencies for subsequent commands:
						vk::PipelineStageFlagBits2KHR::eAllGraphics,
						vk::AccessFlagBits2KHR::eColorAttachmentWrite | vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite
					}}
				},
				{}, // no resource-specific sync hints
				[
					indexBufferOffset = std::get<size_t>(aIndexBufferAndOffsetAndNumElements),
					lBindingCount = static_cast<uint32_t>(N),
					handles, offsets, indexType,
					lNumElemments = std::get<uint32_t>(aIndexBufferAndOffsetAndNumElements),
					lIndexBufferHandle = std::get<const buffer_t&>(aIndexBufferAndOffsetAndNumElements).handle(),
					aNumberOfInstances, aFirstIndex, aVertexOffset, aFirstInstance
				](avk::command_buffer_t& cb) {
					cb.handle().bindVertexBuffers(
						0u, // TODO: Should the first binding really always be 0?
						lBindingCount, handles.data(), offsets.data()
					);
					cb.handle().bindIndexBuffer(lIndexBufferHandle, indexBufferOffset, indexType);
					cb.handle().drawIndexed(lNumElemments, aNumberOfInstances, aFirstIndex, aVertexOffset, aFirstInstance);
				}
			};
		}

		/**	Perform an indexed draw call with vertex buffer bindings starting at BUFFER-BINDING #0 top to the number of total vertex buffers passed -1.
		 *	"BUFFER-BINDING" means that it corresponds to the binding specified in `input_binding_location_data::from_buffer_at_binding`.
		 *	Offset into the index buffer is 0.
		 *	There can be no gaps between buffer bindings.
		 *	@param	aIndexBuffer		Reference to an index buffer
		 *	@param	aNumberOfInstances	Number of instances to draw
		 *	@param	aFirstIndex			Offset to the first index
		 *	@param	aVertexOffset		Offset to the first vertex
		 *	@param	aFirstInstance		The ID of the first instance
		 *	@param	aVertexBuffers		Multiple const-references to buffers, or tuples of const-references to buffers + offsets.
		 *								First case:   Pass const buffer_t& types!
		 *								Second case:  Pass tuples of type std::tuple<const buffer_t&, size_t>!
		 *											  Hint:    std::forward_as_tuple might be useful to get that reference into a std::tuple.
		 *											  Example: avk::buffer myVertexBuffer;
		 *											           auto myTuple = std::forward_as_tuple(myVertexBuffer.get(), size_t{0});
		 */
		template <typename... Bfrs>
		action_type_command draw_indexed(const buffer_t& aIndexBuffer, uint32_t aNumberOfInstances, uint32_t aFirstIndex, uint32_t aVertexOffset, uint32_t aFirstInstance, const Bfrs&... aVertexBuffers)
		{
			const auto& indexMeta = aIndexBuffer.meta<avk::index_buffer_meta>();
            return draw_indexed(std::forward_as_tuple(aIndexBuffer, size_t{0}, indexMeta.num_elements()), aNumberOfInstances, aFirstIndex, aVertexOffset, aFirstInstance, aVertexBuffers...);
		}

		/**	Perform an indexed draw call with vertex buffer bindings starting at BUFFER-BINDING #0 top to the number of total vertex buffers passed -1.
		 *	"BUFFER-BINDING" means that it corresponds to the binding specified in `input_binding_location_data::from_buffer_at_binding`.
		 *	There can be no gaps between buffer bindings.
		 *	Number of instances is set to 1.
		 *	The first index is set to 0.
		 *	The vertex offset is set to 0.
		 *	The ID of the first instance is set to 0.
		 *	@param	aIndexBufferAndOffsetAndNumElements	Tuple of a const-reference and an offset and a number of elements to draw
		 *												Pass a tuple of type std::tuple<const buffer_t&, size_t, uint32_t>!
		 *												  Hint:    std::forward_as_tuple might be useful to get that reference into a std::tuple.
		 *												  Example: avk::buffer myIndexBuffer;
		 *												           auto myTuple = std::forward_as_tuple(myIndexBuffer.get(), size_t{0}, uint32_t{1});
		 *	@param	aVertexBuffers			Multiple const-references to buffers, or tuples of const-references to buffers + offsets.
		 *									First case:   Pass const buffer_t& types!
		 *									Second case:  Pass tuples of type std::tuple<const buffer_t&, size_t>!
		 *												  Hint:    std::forward_as_tuple might be useful to get that reference into a std::tuple.
		 *												  Example: avk::buffer myVertexBuffer;
		 *												           auto myTuple = std::forward_as_tuple(myVertexBuffer.get(), size_t{0});
		 */
		template <typename... Bfrs>
		action_type_command draw_indexed(const std::tuple<const buffer_t&, size_t, uint32_t>& aIndexBufferAndOffsetAndNumElements, const Bfrs&... aVertexBuffers)
		{
			return draw_indexed(aIndexBufferAndOffsetAndNumElements, 1u, 0u, 0u, 0u, aVertexBuffers...);
		}

		/**	Perform an indexed draw call with vertex buffer bindings starting at BUFFER-BINDING #0 top to the number of total vertex buffers passed -1.
		 *	"BUFFER-BINDING" means that it corresponds to the binding specified in `input_binding_location_data::from_buffer_at_binding`.
		 *	There can be no gaps between buffer bindings.
		 *	Number of instances is set to 1.
		 *	The first index is set to 0.
		 *	The vertex offset is set to 0.
		 *	The ID of the first instance is set to 0.
		 *	@param	aIndexBuffer		Reference to an index buffer
		 *	@param	aVertexBuffers		Multiple const-references to buffers, or tuples of const-references to buffers + offsets.
		 *								First case:   Pass const buffer_t& types!
		 *								Second case:  Pass tuples of type std::tuple<const buffer_t&, size_t>!
		 *											  Hint:    std::forward_as_tuple might be useful to get that reference into a std::tuple.
		 *											  Example: avk::buffer myVertexBuffer;
		 *											           auto myTuple = std::forward_as_tuple(myVertexBuffer.get(), size_t{0});
		 */
		template <typename... Bfrs>
		action_type_command draw_indexed(const buffer_t& aIndexBuffer, const Bfrs&... aVertexBuffers)
		{
			return draw_indexed(aIndexBuffer, 1u, 0u, 0u, 0u, aVertexBuffers...);
		}

		/**	Perform an indexed indirect draw call with vertex buffer bindings starting at BUFFER-BINDING #0 top to the number of total vertex buffers passed -1.
		 *	"BUFFER-BINDING" means that it corresponds to the binding specified in `input_binding_location_data::from_buffer_at_binding`.
		 *	There can be no gaps between buffer bindings.
		 *	@param	aParametersBuffer	Reference to an draw parameters buffer, containing a list of vk::DrawIndexedIndirectCommand structures
		 *	@param	aIndexBuffer		Reference to an index buffer
		 *	@param	aNumberOfDraws		Number of draws to execute
		 *	@param	aParametersOffset	Byte offset into the parameters buffer where the actual draw parameters begin
		 *	@param	aParametersStride	Byte stride between successive sets of draw parameters in the parameters buffer
		 *	@param	aVertexBuffers		Multiple const-references to buffers, or tuples of const-references to buffers + offsets.
		 *								First case:   Pass const buffer_t& types!
		 *								Second case:  Pass tuples of type std::tuple<const buffer_t&, size_t>!
		 *											  Hint:    std::forward_as_tuple might be useful to get that reference into a std::tuple.
		 *											  Example: avk::buffer myVertexBuffer;
		 *											           auto myTuple = std::forward_as_tuple(myVertexBuffer.get(), size_t{0});
		 *
		 *  NOTE: Make sure the _exact_ types are used for aParametersOffset (vk::DeviceSize) and aParametersStride (uint32_t) to avoid compile errors.
		 */
		template <typename... Bfrs>
		action_type_command draw_indexed_indirect(const buffer_t& aParametersBuffer, const buffer_t& aIndexBuffer, uint32_t aNumberOfDraws, vk::DeviceSize aParametersOffset, uint32_t aParametersStride, const Bfrs&... aVertexBuffers)
		{
			constexpr size_t N = sizeof...(aVertexBuffers);
			std::array<vk::Buffer, N> handles;
			std::array<vk::DeviceSize, N> offsets;
			bind_vertex_buffer(&handles[0], &offsets[0], aVertexBuffers...);

			const auto& indexMeta = aIndexBuffer.template meta<avk::index_buffer_meta>();
			vk::IndexType indexType;
			switch (indexMeta.sizeof_one_element()) {
				case sizeof(uint16_t): indexType = vk::IndexType::eUint16; break;
				case sizeof(uint32_t): indexType = vk::IndexType::eUint32; break;
				default: AVK_LOG_ERROR("The given size[" + std::to_string(indexMeta.sizeof_one_element()) + "] does not correspond to a valid vk::IndexType"); break;
			}

			return action_type_command{
				avk::sync::sync_hint {
					{{ // DESTINATION dependencies for previous commands:
						vk::PipelineStageFlagBits2KHR::eAllGraphics,
						vk::AccessFlagBits2KHR::eInputAttachmentRead | vk::AccessFlagBits2KHR::eColorAttachmentRead | vk::AccessFlagBits2KHR::eColorAttachmentWrite | vk::AccessFlagBits2KHR::eDepthStencilAttachmentRead | vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite
					}},
					{{ // SOURCE dependencies for subsequent commands:
						vk::PipelineStageFlagBits2KHR::eAllGraphics,
						vk::AccessFlagBits2KHR::eColorAttachmentWrite | vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite
					}}
				},
				{}, // no resource-specific sync hints
				[
					lBindingCount = static_cast<uint32_t>(N),
					handles, offsets, indexType,
					lParametersBufferHandle = aParametersBuffer.handle(),
					lIndexBufferHandle = aIndexBuffer.handle(),
					aNumberOfDraws, aParametersOffset, aParametersStride
				](avk::command_buffer_t& cb) {
					cb.handle().bindVertexBuffers(
						0u, // TODO: Should the first binding really always be 0?
						lBindingCount, handles.data(), offsets.data()
					);
					cb.handle().bindIndexBuffer(lIndexBufferHandle, 0u, indexType);
					cb.handle().drawIndexedIndirect(lParametersBufferHandle, aParametersOffset, aNumberOfDraws, aParametersStride);
				}
			};
		}

		/**	Perform an indexed indirect draw call with vertex buffer bindings starting at BUFFER-BINDING #0 top to the number of total vertex buffers passed -1.
		 *	"BUFFER-BINDING" means that it corresponds to the binding specified in `input_binding_location_data::from_buffer_at_binding`.
		 *	There can be no gaps between buffer bindings.
		 *	The parameter offset is set to 0.
		 *	The parameter stride is set to sizeof(vk::DrawIndexedIndirectCommand).
		 *	@param	aParametersBuffer	Reference to an draw parameters buffer, containing a list of vk::DrawIndexedIndirectCommand structures
		 *	@param	aIndexBuffer		Reference to an index buffer
		 *	@param	aNumberOfDraws		Number of draws to execute
		 *	@param	aVertexBuffers		Multiple const-references to buffers, or tuples of const-references to buffers + offsets.
		 *								First case:   Pass const buffer_t& types!
		 *								Second case:  Pass tuples of type std::tuple<const buffer_t&, size_t>!
		 *											  Hint:    std::forward_as_tuple might be useful to get that reference into a std::tuple.
		 *											  Example: avk::buffer myVertexBuffer;
		 *											           auto myTuple = std::forward_as_tuple(myVertexBuffer.get(), size_t{0});
		 */
		template <typename... Bfrs>
		action_type_command draw_indexed_indirect(const buffer_t& aParametersBuffer, const buffer_t& aIndexBuffer, uint32_t aNumberOfDraws, const Bfrs&... aVertexBuffers)
		{
			return draw_indexed_indirect(aParametersBuffer, aIndexBuffer, aNumberOfDraws, vk::DeviceSize{ 0 }, static_cast<uint32_t>(sizeof(vk::DrawIndexedIndirectCommand)), aVertexBuffers...);
		}

		/**	Perform an indexed indirect count draw call with vertex buffer bindings starting at BUFFER-BINDING #0 top to the number of total vertex buffers passed -1.
		 *	"BUFFER-BINDING" means that it corresponds to the binding specified in `input_binding_location_data::from_buffer_at_binding`.
		 *	There can be no gaps between buffer bindings.
		 *	@param	aParametersBuffer	Reference to an draw parameters buffer, containing a list of vk::DrawIndexedIndirectCommand structures
		 *	@param	aIndexBuffer		Reference to an index buffer
		 *	@param	aMaxNumberOfDraws	Maximum number of draws to execute (the actual number of draws is the minimum of aMaxNumberOfDraws and the value stored in the draw count buffer)
		 *	@param	aParametersOffset	Byte offset into the parameters buffer where the actual draw parameters begin
		 *	@param	aParametersStride	Byte stride between successive sets of draw parameters in the parameters buffer
		 *  @param  aDrawCountBuffer    Reference to a draw count buffer, containing the number of draws to execute in one uint32_t
		 *	@param	aDrawCountOffset	Byte offset into the draw count buffer where the actual draw count is located
		 *	@param	aVertexBuffers		Multiple const-references to buffers, or tuples of const-references to buffers + offsets.
		 *								First case:   Pass const buffer_t& types!
		 *								Second case:  Pass tuples of type std::tuple<const buffer_t&, size_t>!
		 *											  Hint:    std::forward_as_tuple might be useful to get that reference into a std::tuple.
		 *											  Example: avk::buffer myVertexBuffer;
		 *											           auto myTuple = std::forward_as_tuple(myVertexBuffer.get(), size_t{0});
		 *
		 *   See vkCmdDrawIndexedIndirectCount in the Vulkan specification for more details.
		 */
		template <typename... Bfrs>
		action_type_command draw_indexed_indirect_count(const buffer_t& aParametersBuffer, const buffer_t& aIndexBuffer, uint32_t aMaxNumberOfDraws, vk::DeviceSize aParametersOffset, uint32_t aParametersStride, const buffer_t& aDrawCountBuffer, vk::DeviceSize aDrawCountOffset, const Bfrs&... aVertexBuffers)
		{
			constexpr size_t N = sizeof...(aVertexBuffers);
			std::array<vk::Buffer, N> handles;
			std::array<vk::DeviceSize, N> offsets;
			bind_vertex_buffer(&handles[0], &offsets[0], aVertexBuffers...);

			const auto& indexMeta = aIndexBuffer.template meta<avk::index_buffer_meta>();
			vk::IndexType indexType;
			switch (indexMeta.sizeof_one_element()) {
				case sizeof(uint16_t) : indexType = vk::IndexType::eUint16; break;
					case sizeof(uint32_t) : indexType = vk::IndexType::eUint32; break;
					default: AVK_LOG_ERROR("The given size[" + std::to_string(indexMeta.sizeof_one_element()) + "] does not correspond to a valid vk::IndexType"); break;
			}

			return action_type_command{
				avk::sync::sync_hint {
					{{ // DESTINATION dependencies for previous commands:
						vk::PipelineStageFlagBits2KHR::eAllGraphics,
						vk::AccessFlagBits2KHR::eInputAttachmentRead | vk::AccessFlagBits2KHR::eColorAttachmentRead | vk::AccessFlagBits2KHR::eColorAttachmentWrite | vk::AccessFlagBits2KHR::eDepthStencilAttachmentRead | vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite
					}},
					{{ // SOURCE dependencies for subsequent commands:
						vk::PipelineStageFlagBits2KHR::eAllGraphics,
						vk::AccessFlagBits2KHR::eColorAttachmentWrite | vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite
					}}
				},
				{}, // no resource-specific sync hints
				[
					lBindingCount = static_cast<uint32_t>(N),
					handles, offsets, indexType,
					lIndexBufferHandle = aIndexBuffer.handle(),
					lParametersBufferHandle = aParametersBuffer.handle(),
					lDrawCountBufferHandle = aDrawCountBuffer.handle(),
					aParametersOffset, aDrawCountOffset, aMaxNumberOfDraws, aParametersStride
				](avk::command_buffer_t& cb) {
					cb.handle().bindVertexBuffers(
						0u, // TODO: Should the first binding really always be 0?
						lBindingCount, handles.data(), offsets.data()
					);
					cb.handle().bindIndexBuffer(lIndexBufferHandle, 0u, indexType);
					cb.handle().drawIndexedIndirectCount(lParametersBufferHandle, aParametersOffset, lDrawCountBufferHandle, aDrawCountOffset, aMaxNumberOfDraws, aParametersStride);
				}
			};
		}

		/**	Perform an indexed indirect count draw call with vertex buffer bindings starting at BUFFER-BINDING #0 top to the number of total vertex buffers passed -1.
		 *	"BUFFER-BINDING" means that it corresponds to the binding specified in `input_binding_location_data::from_buffer_at_binding`.
		 *	There can be no gaps between buffer bindings.
		 *	The parameter offset is set to 0.
		 *	The parameter stride is set to sizeof(vk::DrawIndexedIndirectCommand).
		 *   The draw count offset is set to 0.
		 *	@param	aParametersBuffer	Reference to an draw parameters buffer, containing a list of vk::DrawIndexedIndirectCommand structures
		 *	@param	aIndexBuffer		Reference to an index buffer
		 *	@param	aMaxNumberOfDraws	Maximum number of draws to execute (the actual number of draws is the minimum of aMaxNumberOfDraws and the value stored in the draw count buffer)
		 *  @param  aDrawCountBuffer    Reference to a draw count buffer, containing the number of draws to execute in one uint32_t
		 *	@param	aVertexBuffers		Multiple const-references to buffers, or tuples of const-references to buffers + offsets.
		 *								First case:   Pass const buffer_t& types!
		 *								Second case:  Pass tuples of type std::tuple<const buffer_t&, size_t>!
		 *											  Hint:    std::forward_as_tuple might be useful to get that reference into a std::tuple.
		 *											  Example: avk::buffer myVertexBuffer;
		 *											           auto myTuple = std::forward_as_tuple(myVertexBuffer.get(), size_t{0});
		 *
		 *   See vkCmdDrawIndexedIndirectCount in the Vulkan specification for more details.
		 */
		template <typename... Bfrs>
		action_type_command draw_indexed_indirect_count(const buffer_t& aParametersBuffer, const buffer_t& aIndexBuffer, uint32_t aMaxNumberOfDraws, const buffer_t& aDrawCountBuffer, const Bfrs&... aVertexBuffers)
		{
			return draw_indexed_indirect_count(aParametersBuffer, aIndexBuffer, aMaxNumberOfDraws, vk::DeviceSize{ 0 }, static_cast<uint32_t>(sizeof(vk::DrawIndexedIndirectCommand)), aDrawCountBuffer, vk::DeviceSize{ 0 }, aVertexBuffers...);
		}

        /**
         * @brief Perform a dispatch call, i.e., invoke a compute shader
         * @param aGroupCountX The number of local workgroups to dispatch in the X dimension.
         * @param aGroupCountY The number of local workgroups to dispatch in the Y dimension.
         * @param aGroupCountZ The number of local workgroups to dispatch in the Z dimension.
         * @return An action_type_command instance which you must submit to a queue.
         */
        extern action_type_command dispatch(uint32_t aGroupCountX, uint32_t aGroupCountY, uint32_t aGroupCountZ);

        /**
         * @brief Perform an indirect dispatch call, i.e., invoke a compute shader with dispatch parameters read from a buffer
         * @param aCountBuffer			Buffer containing the draw count in form of a VkDispatchIndirectCommand record. See Vulkan specification: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDispatchIndirectCommand.html
         * @param aCountBufferOffset	Byte offset into countBuffer to the beginning of such a VkDispatchIndirectCommand record.
         * @return An action_type_command instance which you must submit to a queue.
         */
        extern action_type_command dispatch_indirect(avk::buffer aCountBuffer, uint32_t aCountBufferOffset);

		/**
		 * @brief Invoke a graphics mesh pipeline-type draw call using an API function provided by VK_NV_mesh_shader.
		 * @param aTaskCount The number of local workgroups to dispatch in the X dimension. Y and Z dimension are implicitly set to one.
		 * @param aFirstTask The X component of the first workgroup ID
		 * @return An action_type_command instance which you must submit to a queue.
		 */
		extern action_type_command draw_mesh_tasks_nv(uint32_t aTaskCount, uint32_t aFirstTask);

#if VK_HEADER_VERSION >= 239
        /**
         * @brief Invoke a graphics mesh pipeline-type draw call using an API function provided by VK_EXT_mesh_shader.
         * @param aGroupCountX The number of local workgroups to dispatch in the X dimension.
         * @param aGroupCountY The number of local workgroups to dispatch in the Y dimension.
         * @param aGroupCountZ The number of local workgroups to dispatch in the Z dimension.
         * @return An action_type_command instance which you must submit to a queue.
         */
        extern action_type_command draw_mesh_tasks_ext(uint32_t aGroupCountX, uint32_t aGroupCountY, uint32_t aGroupCountZ);

        /**
         * @brief Invoke a graphics mesh pipeline-type draw call using an API function provided by VK_EXT_mesh_shader.
         * @param aDrawBuffer			buffer containing draw parameters.
         * @param aDrawBufferOffset		byte offset into buffer where parameters begin.
         * @param aDrawCount			number of draws to execute, and can be zero.
         * @param aStride				byte stride between successive sets of draw parameters.
         * @return An action_type_command instance which you must submit to a queue.
         */
        extern action_type_command draw_mesh_tasks_indirect_ext(avk::buffer aDrawBuffer, vk::DeviceSize aDrawBufferOffset, uint32_t aDrawCount, uint32_t aStride);

        /**
         * @brief Invoke a graphics mesh pipeline-type draw call using an API function provided by VK_EXT_mesh_shader.
         * @param aDrawBuffer			buffer containing draw parameters.
         * @param aDrawBufferOffset		byte offset into buffer where parameters begin.
         * @param aCountBuffer			buffer containing the draw count.
         * @param aCountBufferOffset	byte offset into countBuffer where the draw count begins.
         * @param aMaxDrawCount			number of draws to execute, and can be zero.
         * @param aStride				byte stride between successive sets of draw parameters.
         * @return An action_type_command instance which you must submit to a queue.
         */
        extern action_type_command draw_mesh_tasks_indirect_count_ext(avk::buffer aDrawBuffer, vk::DeviceSize aDrawBufferOffset, avk::buffer aCountBuffer, uint32_t aCountBufferOffset, uint32_t aMaxDrawCount, uint32_t aStride);
#endif

#if VK_HEADER_VERSION >= 135
		/**	Issue a trace rays call.
		 *	@param	aRaygenDimensions			Dimensions of the trace rays call. This can be the extent of a window's backbuffer
		 *	@param	aShaderBindingTableRef		Reference to the shader binding table (SBT) to be used for this trace rays call.
		 *	@param	aRaygenSbtRef				Offset, stride, and size about which SBT entries to use for the ray generation shaders.
		 *										The `.buffer` member must be set to `aShaderBindingTableRef.mSbtBufferHandle`.
		 *	@param	aRaymissSbtRef				Offset, stride, and size about which SBT entries to use for the miss shaders.
		 *										The `.buffer` member must be set to `aShaderBindingTableRef.mSbtBufferHandle`.
		 *	@param	aRayhitSbtRef				Offset, stride, and size about which SBT entries to use for the (triangle|procedural) hit groups.
		 *										The `.buffer` member must be set to `aShaderBindingTableRef.mSbtBufferHandle`.
		 *	@param	aCallableSbtRef				Offset, stride, and size about which SBT entries to use for the callable shaders.
		 *										The `.buffer` member must be set to `aShaderBindingTableRef.mSbtBufferHandle`.
		 *
		 *	Hint: You can display information about the shader binding table and its groups via `ray_tracing_pipeline_t::print_shader_binding_table_groups`
		 */
		action_type_command trace_rays(
			vk::Extent3D aRaygenDimensions,
			const shader_binding_table_ref& aShaderBindingTableRef,
#if VK_HEADER_VERSION >= 162
			const vk::StridedDeviceAddressRegionKHR& aRaygenSbtRef = vk::StridedDeviceAddressRegionKHR{ 0, 0, 0 },
			const vk::StridedDeviceAddressRegionKHR& aRaymissSbtRef = vk::StridedDeviceAddressRegionKHR{ 0, 0, 0 },
			const vk::StridedDeviceAddressRegionKHR& aRayhitSbtRef = vk::StridedDeviceAddressRegionKHR{ 0, 0, 0 },
			const vk::StridedDeviceAddressRegionKHR& aCallableSbtRef = vk::StridedDeviceAddressRegionKHR{ 0, 0, 0 }

#else
			const vk::StridedBufferRegionKHR & aRaygenSbtRef = vk::StridedBufferRegionKHR{ nullptr, 0, 0, 0 },
			const vk::StridedBufferRegionKHR & aRaymissSbtRef = vk::StridedBufferRegionKHR{ nullptr, 0, 0, 0 },
			const vk::StridedBufferRegionKHR & aRayhitSbtRef = vk::StridedBufferRegionKHR{ nullptr, 0, 0, 0 },
			const vk::StridedBufferRegionKHR & aCallableSbtRef = vk::StridedBufferRegionKHR{ nullptr, 0, 0, 0 }
#endif
		);

#if VK_HEADER_VERSION >= 162
#define STRIDED_REGION_PARAMS vk::StridedDeviceAddressRegionKHR& aRaygen, vk::StridedDeviceAddressRegionKHR& aRaymiss, vk::StridedDeviceAddressRegionKHR& aRayhit, vk::StridedDeviceAddressRegionKHR& aCallable
#else
#define STRIDED_REGION_PARAMS vk::StridedBufferRegionKHR& aRaygen, vk::StridedBufferRegionKHR& aRaymiss, vk::StridedBufferRegionKHR& aRayhit, vk::StridedBufferRegionKHR& aCallable
#endif

		// End of recursive variadic template handling
		static void add_config(const shader_binding_table_ref& aShaderBindingTableRef, STRIDED_REGION_PARAMS) { /* We're done here. */ }

		// Looks like we need to forward-declare these two:
		template <typename... Ts>
		static void add_config(const shader_binding_table_ref& aShaderBindingTableRef, STRIDED_REGION_PARAMS, const using_hit_group_at_index& aHitGroupAtIndex, const Ts&... args);
		template <typename... Ts>
		static void add_config(const shader_binding_table_ref& aShaderBindingTableRef, STRIDED_REGION_PARAMS, const using_miss_group_at_index& aMissGroupAtIndex, const Ts&... args);

		// Add a specific config setting to the trace rays call
		template <typename... Ts>
		static void add_config(const shader_binding_table_ref& aShaderBindingTableRef, STRIDED_REGION_PARAMS, const using_raygen_group_at_index& aRaygenGroupAtIndex, const Ts&... args)
		{
			aRaygen
#if VK_HEADER_VERSION >= 162
				.setDeviceAddress(aShaderBindingTableRef.mSbtBufferDeviceAddress + aShaderBindingTableRef.mSbtGroupsInfo.get().mRaygenGroupsInfo[aRaygenGroupAtIndex.mGroupIndex].mByteOffset)
#else
				.setBuffer(aShaderBindingTableRef.mSbtBufferHandle)
				.setOffset(aShaderBindingTableRef.mSbtGroupsInfo.get().mRaygenGroupsInfo[aRaygenGroupAtIndex.mGroupIndex].mByteOffset)
#endif
				.setStride(aShaderBindingTableRef.mSbtEntrySize)
				.setSize(aShaderBindingTableRef.mSbtGroupsInfo.get().mRaygenGroupsInfo[aRaygenGroupAtIndex.mGroupIndex].mNumEntries * aShaderBindingTableRef.mSbtEntrySize);
			add_config(aShaderBindingTableRef, aRaygen, aRaymiss, aRayhit, aCallable, args...);
		}

		// Add a specific config setting to the trace rays call
		template <typename... Ts>
		static void add_config(const shader_binding_table_ref& aShaderBindingTableRef, STRIDED_REGION_PARAMS, const using_miss_group_at_index& aMissGroupAtIndex, const Ts&... args)
		{
			aRaymiss
#if VK_HEADER_VERSION >= 162
				.setDeviceAddress(aShaderBindingTableRef.mSbtBufferDeviceAddress + aShaderBindingTableRef.mSbtGroupsInfo.get().mMissGroupsInfo[aMissGroupAtIndex.mGroupIndex].mByteOffset)
#else
				.setBuffer(aShaderBindingTableRef.mSbtBufferHandle)
				.setOffset(aShaderBindingTableRef.mSbtGroupsInfo.get().mMissGroupsInfo[aMissGroupAtIndex.mGroupIndex].mByteOffset)
#endif
				.setStride(aShaderBindingTableRef.mSbtEntrySize)
				.setSize(aShaderBindingTableRef.mSbtGroupsInfo.get().mMissGroupsInfo[aMissGroupAtIndex.mGroupIndex].mNumEntries * aShaderBindingTableRef.mSbtEntrySize);
			add_config(aShaderBindingTableRef, aRaygen, aRaymiss, aRayhit, aCallable, args...);
		}

		// Add a specific config setting to the trace rays call
		template <typename... Ts>
		static void add_config(const shader_binding_table_ref& aShaderBindingTableRef, STRIDED_REGION_PARAMS, const using_hit_group_at_index& aHitGroupAtIndex, const Ts&... args)
		{
			aRayhit
#if VK_HEADER_VERSION >= 162
				.setDeviceAddress(aShaderBindingTableRef.mSbtBufferDeviceAddress + aShaderBindingTableRef.mSbtGroupsInfo.get().mHitGroupsInfo[aHitGroupAtIndex.mGroupIndex].mByteOffset)
#else
				.setBuffer(aShaderBindingTableRef.mSbtBufferHandle)
				.setOffset(aShaderBindingTableRef.mSbtGroupsInfo.get().mHitGroupsInfo[aHitGroupAtIndex.mGroupIndex].mByteOffset)
#endif
				.setStride(aShaderBindingTableRef.mSbtEntrySize)
				.setSize(aShaderBindingTableRef.mSbtGroupsInfo.get().mHitGroupsInfo[aHitGroupAtIndex.mGroupIndex].mNumEntries * aShaderBindingTableRef.mSbtEntrySize);
			add_config(aShaderBindingTableRef, aRaygen, aRaymiss, aRayhit, aCallable, args...);
		}

		// Add a specific config setting to the trace rays call
		template <typename... Ts>
		static void add_config(const shader_binding_table_ref& aShaderBindingTableRef, STRIDED_REGION_PARAMS, const using_callable_group_at_index& aCallableGroupAtIndex, const Ts&... args)
		{
			aCallable
#if VK_HEADER_VERSION >= 162
				.setDeviceAddress(aShaderBindingTableRef.mSbtBufferDeviceAddress + aShaderBindingTableRef.mSbtGroupsInfo.get().mCallableGroupsInfo[aCallableGroupAtIndex.mGroupIndex].mByteOffset)
#else
				.setBuffer(aShaderBindingTableRef.mSbtBufferHandle)
				.setOffset(aShaderBindingTableRef.mSbtGroupsInfo.get().mCallableGroupsInfo[aCallableGroupAtIndex.mGroupIndex].mByteOffset)
#endif
				.setStride(aShaderBindingTableRef.mSbtEntrySize)
				.setSize(aShaderBindingTableRef.mSbtGroupsInfo.get().mCallableGroupsInfo[aCallableGroupAtIndex.mGroupIndex].mNumEntries * aShaderBindingTableRef.mSbtEntrySize);
			add_config(aShaderBindingTableRef, aRaygen, aRaymiss, aRayhit, aCallable, args...);
		}

		/**	Issue a trace rays call.
		 *
		 *	First two parameters are mandatory explicitly:
		 *	@param	aRaygenDimensions			Dimensions of the trace rays call. This can be the extent of a window's backbuffer
		 *	@param	aShaderBindingTableRef		Reference to the shader binding table to be used for this trace rays call.
		 *
		 *	Further parameters are mandatory implicitly - i.e. there must be at least some information given about which
		 *	shader binding table entries to be used from the given shader binding table during this trace rays call.
		 *	@param args							Possible values:
		 *											- using_raygen_group_at_index
		 *											- using_miss_group_at_index
		 *											- using_hit_group_at_index
		 *											- using_callable_group_at_index
		 *
		 *	Hint: You can display information about the shader binding table and its groups via `ray_tracing_pipeline_t::print_shader_binding_table_groups`
		 */
		template <typename... Ts>
		action_type_command trace_rays(vk::Extent3D aRaygenDimensions, const shader_binding_table_ref& aShaderBindingTableRef, const Ts&... args)
		{
			// 1. GATHER CONFIG
#if VK_HEADER_VERSION >= 162
			auto raygen = vk::StridedDeviceAddressRegionKHR{ 0, 0, 0 };
			auto raymiss = vk::StridedDeviceAddressRegionKHR{ 0, 0, 0 };
			auto rayhit = vk::StridedDeviceAddressRegionKHR{ 0, 0, 0 };
			auto callable = vk::StridedDeviceAddressRegionKHR{ 0, 0, 0 };
#else
			auto raygen = vk::StridedBufferRegionKHR{ nullptr, 0, 0, 0 };
			auto raymiss = vk::StridedBufferRegionKHR{ nullptr, 0, 0, 0 };
			auto rayhit = vk::StridedBufferRegionKHR{ nullptr, 0, 0, 0 };
			auto callable = vk::StridedBufferRegionKHR{ nullptr, 0, 0, 0 };
#endif
			add_config(aShaderBindingTableRef, raygen, raymiss, rayhit, callable, args...);

			// 2. TRACE. RAYS.
			return trace_rays(aRaygenDimensions, aShaderBindingTableRef, raygen, raymiss, rayhit, callable);
		}
#endif
	}

	/** Helper struct to specify a semaphore and a signal value for it.
	 *  This is used for timeline semaphores.
	 */
	struct semaphore_value_info
	{
		avk::resource_argument<avk::semaphore_t> mSemaphore;
		uint64_t mValue;
	};
	inline semaphore_value_info operator,(avk::resource_argument<avk::semaphore_t> aSemaphore, uint64_t aValue) {
		return semaphore_value_info{std::move(aSemaphore), aValue};
	}

	struct semaphore_wait_info
	{
		avk::resource_argument<avk::semaphore_t> mWaitSemaphore;
		avk::stage::pipeline_stage_flags mDstStage;
		uint64_t mValue;

		semaphore_wait_info& at_value(uint64_t aValue) {
			mValue = aValue;
			return *this;
		}
	};

	inline semaphore_wait_info operator>> (avk::resource_argument<avk::semaphore_t> aSemaphore, avk::stage::pipeline_stage_flags aStageFlags)
	{
		return semaphore_wait_info{ std::move(aSemaphore), aStageFlags, 0 };
	}

	inline semaphore_wait_info operator>> (semaphore_value_info aSemaphoreValueInfo, avk::stage::pipeline_stage_flags aStageFlags)
	{
		return semaphore_wait_info{ std::move(aSemaphoreValueInfo.mSemaphore), aStageFlags, aSemaphoreValueInfo.mValue };
	}

	struct semaphore_signal_info
	{
		avk::stage::pipeline_stage_flags mSrcStage;
		avk::resource_argument<avk::semaphore_t> mSignalSemaphore;
		uint64_t mValue;
	};

	inline semaphore_signal_info operator>> (avk::stage::pipeline_stage_flags aStageFlags, avk::resource_argument<avk::semaphore_t> aSemaphore)
	{
		return semaphore_signal_info{ aStageFlags, std::move(aSemaphore), 0 };
	}

	inline semaphore_signal_info operator>> (avk::stage::pipeline_stage_flags aStageFlags, semaphore_value_info aSemaphoreValueInfo)
	{
		return semaphore_signal_info{ aStageFlags, std::move(aSemaphoreValueInfo.mSemaphore), aSemaphoreValueInfo.mValue };
	}

	class recorded_command_buffer;

	// Something that submits stuff to a queue.
	// The submission itself either happens in submit() or in this class' destructor, if submit()/go()/do_it() has never been invoked before.
	class submission_data final
	{
	public:
		submission_data(const root* aRoot, avk::resource_argument<avk::command_buffer_t> aCommandBuffer, const queue& aQueue, const avk::recorded_command_buffer* aDangerousRecordedCommandBufferPointer = nullptr)
			: mRoot{ aRoot }
			, mCommandBufferToSubmit{ std::move(aCommandBuffer) }
			, mQueueToSubmitTo{ &aQueue }
			, mSubmissionCount{ 0u }
			, mDangerousRecordedCommandBufferPointer{ aDangerousRecordedCommandBufferPointer }
		{}
		submission_data(const root* aRoot, avk::resource_argument<avk::command_buffer_t> aCommandBuffer, semaphore_wait_info aSemaphoreWaitInfo, const avk::recorded_command_buffer* aDangerousRecordedCommandBufferPointer = nullptr)
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

		submission_data& submit_to(const queue& aQueue);
		submission_data& waiting_for(avk::semaphore_wait_info aWaitInfo);
		submission_data& signaling_upon_completion(semaphore_signal_info aSignalInfo);
		submission_data& signaling_upon_completion(avk::resource_argument<avk::fence_t> aFence);

		bool is_sane() const { return nullptr != mRoot; }

		void submit();
		void go() { submit(); }
		void do_it() { submit(); }

		const auto* recorded_command_buffer_ptr() const { return mDangerousRecordedCommandBufferPointer; }

	private:
		const root* mRoot = nullptr;
		avk::resource_argument<avk::command_buffer_t> mCommandBufferToSubmit;
		const queue* mQueueToSubmitTo;
		std::vector<semaphore_wait_info> mSemaphoreWaits;
		std::vector<semaphore_signal_info> mSemaphoreSignals;
		std::optional<avk::resource_argument<avk::fence_t>> mFence;
		uint32_t mSubmissionCount;
		const avk::recorded_command_buffer* mDangerousRecordedCommandBufferPointer;
	};

	class recorded_commands;

	// This class turns a std::vector<recorded_commands_t> into an actual command buffer
	class recorded_command_buffer final
	{
	public:
		// The constructor performs all the parsing, therefore, there's no std::vector<recorded_commands_t> member.
		recorded_command_buffer(const root* aRoot, const std::vector<recorded_commands_t>& aRecordedCommandsAndSyncInstructions, avk::resource_argument<avk::command_buffer_t> aCommandBuffer, const avk::recorded_commands* aDangerousRecordedCommandsPointer = nullptr, bool aBeginEnd = true);
		
		recorded_command_buffer(const recorded_command_buffer&) = default;
		recorded_command_buffer(recorded_command_buffer&&) noexcept = default;
		recorded_command_buffer& operator=(const recorded_command_buffer&) = default;
		recorded_command_buffer& operator=(recorded_command_buffer&&) noexcept = default;
		~recorded_command_buffer() = default;
		
		template <typename T>
		recorded_command_buffer& handling_lifetime_of(T&& aResource)
		{
			mCommandBufferToRecordInto.get().handle_lifetime_of(any_owning_resource_t{ std::move(aResource) });
			return *this;
		}

		submission_data then_waiting_for(avk::semaphore_wait_info aWaitInfo);
		submission_data then_submit_to(const queue& aQueue);

		const auto* recorded_commands_ptr() const { return mDangerousRecordedComandsPointer; }

	private:
		const root* mRoot;
		avk::resource_argument<avk::command_buffer_t> mCommandBufferToRecordInto;
		const avk::recorded_commands* mDangerousRecordedComandsPointer;
	};

	// This class gathers recorded commands which can then be recorded into a command buffer via into_command_buffer()
	// (or they can also be stored somewhere for later recording into a command buffer).
	// But this class is intended to be used as temporary object.
	class recorded_commands final
	{
	public:
		recorded_commands(const root* aRoot, std::vector<recorded_commands_t> aRecordedCommandsAndSyncInstructions);
		recorded_commands(const recorded_commands&) = delete;
		recorded_commands(recorded_commands&&) noexcept = default;
		recorded_commands& operator=(const recorded_commands&) = delete;
		recorded_commands& operator=(recorded_commands&&) noexcept = default;
		~recorded_commands() = default;
		
		recorded_commands& move_into(std::vector<recorded_commands_t>& aTarget);
		recorded_commands& prepend_by(std::vector<recorded_commands_t>& aCommands);
		recorded_commands& append_by(std::vector<recorded_commands_t>& aCommands);

		recorded_commands& handle_lifetime_of(any_owning_resource_t aResource);

		std::vector<recorded_commands_t> and_store();
		recorded_command_buffer into_command_buffer(avk::resource_argument<avk::command_buffer_t> aCommandBuffer, bool aBeginEnd = true);

		const auto& recorded_commands_and_sync_instructions() const { return mRecordedCommandsAndSyncInstructions; }

	private:
		const root* mRoot;
		std::vector<recorded_commands_t> mRecordedCommandsAndSyncInstructions;
		std::vector<any_owning_resource_t> mLifetimeHandledResources;
	};


	// Some convenience functions:

	namespace command
	{
		// End of recursive variadic template handling
		inline static void add_commands(std::vector<avk::recorded_commands_t>&) { /* We're done here. */ }

		// Add a specific pipeline setting to the pipeline config
		template <typename... Ts>
		inline static void add_commands(std::vector<avk::recorded_commands_t>& aGatheredCommands, avk::command::state_type_command& aOneMoreCommand, Ts&... aRest)
		{
			aGatheredCommands.push_back(std::move(aOneMoreCommand));
			add_commands(aGatheredCommands, aRest...);
		}

		// Add a specific pipeline setting to the pipeline config
		template <typename... Ts>
		inline static void add_commands(std::vector<avk::recorded_commands_t>& aGatheredCommands, avk::command::action_type_command& aOneMoreCommand, Ts&... aRest)
		{
			aGatheredCommands.push_back(std::move(aOneMoreCommand));
			add_commands(aGatheredCommands, aRest...);
		}

		// Add a specific pipeline setting to the pipeline config
		template <typename... Ts>
		inline static void add_commands(std::vector<avk::recorded_commands_t>& aGatheredCommands, avk::sync::sync_type_command& aOneMoreCommand, Ts&... aRest)
		{
			aGatheredCommands.push_back(std::move(aOneMoreCommand));
			add_commands(aGatheredCommands, aRest...);
		}

		// Add a specific pipeline setting to the pipeline config
		template <typename... Ts>
		inline static void add_commands(std::vector<avk::recorded_commands_t>& aGatheredCommands, avk::recorded_commands_t& aOneMoreCommand, Ts&... aRest)
		{
			aGatheredCommands.push_back(std::move(aOneMoreCommand));
			add_commands(aGatheredCommands, aRest...);
		}

		// Add a specific pipeline setting to the pipeline config
		template <typename... Ts>
		inline static void add_commands(std::vector<avk::recorded_commands_t>& aGatheredCommands, std::vector<avk::recorded_commands_t>& aManyMoreCommands, Ts&... aRest)
		{
			// Use std::insert with std::make_move_iterator as suggested here: https://stackoverflow.com/questions/15004517/moving-elements-from-stdvector-to-another-one
			aGatheredCommands.insert(
				std::end(aGatheredCommands), 
				std::make_move_iterator(std::begin(aManyMoreCommands)), std::make_move_iterator(std::end(aManyMoreCommands))
			);
			add_commands(aGatheredCommands, aRest...);
		}

		/**	Convenience function for gathering recorded commands
		 *
		 *	It supports the following types:
		 *   - avk::recorded_commands_and_sync_instructions_t
		 *   - std::vector<avk::recorded_commands_t>
		 *
		 */
		template <typename... Ts>
		inline static std::vector<avk::recorded_commands_t> gather(Ts... args)
		{
			std::vector<avk::recorded_commands_t> result;
			add_commands(result, args...);
			return result;
		}

		/**	Convenience function for gathering recorded commands---one avk::command for each entry of a given collection.
		 *	@aCollection	Collection to iterate over
		 *	@aGenerator		Callback function which must return exactly one command (i.e. must be assignable to avk::recorded_commands_t).
		 *					@example std::vector<int> numbers = {1, 2, 3};
		 *							 avk::command::one_for_each(numbers, [](const int& aNumber) {  return ... };
		 *	@return A vector of recorded commands
		 */
		template <typename T, typename F>
		inline static std::vector<avk::recorded_commands_t> one_for_each(const T& aCollection, F aGenerator)
		{
			std::vector<avk::recorded_commands_t> result;
			for (const auto& element : aCollection) {
				result.push_back(aGenerator(element));
			}
			return result;
		}

		/**	Convenience function for gathering recorded commands---a collection of commands for each entry of a given collection.
		 *	@aCollection	Collection to iterate over
		 *	@aGenerator		Callback function which must return a vector of commands (i.e. must be assignable to std::vector<avk::recorded_commands_t>).
		 *					@example std::vector<int> numbers = {1, 2, 3};
		 *							 avk::command::one_for_each(numbers, [](const int& aNumber) { return avk::command::gather( ... ); };
		 *	@return A vector of recorded commands
		 */
		template <typename T, typename F>
		inline static std::vector<avk::recorded_commands_t> many_for_each(const T& aCollection, F aGenerator)
		{
			std::vector<avk::recorded_commands_t> result;
			for (const auto& element : aCollection) {
				auto commands = aGenerator(element);
				for (auto& command : commands) {
					result.push_back(std::move(command));
				}
			}
			return result;
		}

		/**	Convenience function for gathering recorded commands, namely a total number of aN.
		 *	@aN	Collection to iterate over
		 *	@aGenerator		Callback function which must return exactly one command (i.e. must be assignable to avk::recorded_commands_t).
		 *					@example avk::command::one_n_times(5, [](int aIndex) { return ... };
		 *	@return A vector of recorded commands
		 */
		template <typename I, typename F>
		inline static std::vector<avk::recorded_commands_t> one_n_times(I aN, F aGenerator)
		{
			std::vector<avk::recorded_commands_t> result;
			for (I i = 0; i < aN; ++i) {
				result.push_back(aGenerator(i));
			}
			return result;
		}

		/**	Convenience function for gathering recorded commands, namely a total number of aN.
		 *	@aN	Collection to iterate over
		 *	@aGenerator		Callback function which must return exactly one command (i.e. must be assignable to avk::recorded_commands_t).
		 *					@example avk::command::many_n_times(5, [](int aIndex) { return avk::command::gather( ... ); };
		 *	@return A vector of recorded commands
		 */
		template <typename I, typename F>
		inline static std::vector<avk::recorded_commands_t> many_n_times(I aN, F aGenerator)
		{
			std::vector<avk::recorded_commands_t> result;
			for (I i = 0; i < aN; ++i) {
				auto commands = aGenerator(i);
				for (auto& command : commands) {
					result.push_back(std::move(command));
				}
			}
			return result;
		}

		// TODO: Comment
		template <typename C, typename F1>
		inline static avk::recorded_commands_t conditional(C aCondition, F1 aPositive)
		{
			if (aCondition()) {
				return aPositive();
			}
			return avk::recorded_commands_t{ avk::command::state_type_command{} };
		}

		// TODO: Comment
		template <typename C, typename F1, typename F2>
		inline static avk::recorded_commands_t conditional(C aCondition, F1 aPositive, F2 aNegative)
		{
			if (aCondition()) {
				return aPositive();
			}
			else {
				return aNegative();
			}
		}

		// TODO: Comment
		// TODO: Should check for is_vector instead of has_iterators
		template <typename C, typename F1> requires avk::has_iterators<std::invoke_result_t<F1>>
		inline static std::vector<avk::recorded_commands_t> conditional(C aCondition, F1 aPositive)
		{
			if (aCondition()) {
				return aPositive();
			}
			return std::vector<avk::recorded_commands_t>{};
		}

		// TODO: Comment
		// TODO: Should check for is_vector instead of has_iterators
		template <typename C, typename F1, typename F2> requires avk::has_iterators<std::invoke_result_t<F1>> && avk::has_iterators<std::invoke_result_t<F2>>
		inline static std::vector<avk::recorded_commands_t> conditional(C aCondition, F1 aPositive, F2 aNegative)
		{
			if (aCondition()) {
				return aPositive();
			}
			else {
				return aNegative();
			}
		}
	}
}
