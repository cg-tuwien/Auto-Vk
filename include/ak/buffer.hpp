#pragma once

namespace ak
{
	/** Represents a Vulkan buffer along with its assigned memory, holds the 
	*	native handle and takes care about lifetime management of the native handles.
	*/
	template <typename Meta>
	class buffer_t
	{
		friend class root;
	public:
		buffer_t() = default;
		~buffer_t() = default; // Declaration order determines destruction order (inverse!)

		// Copy/move constructors for buffer_t<Meta>:		
		buffer_t(buffer_t&&) noexcept = default;
		buffer_t(const buffer_t&) = delete;
		buffer_t& operator=(buffer_t&&) noexcept = default;
		buffer_t& operator=(const buffer_t&) = delete;
		
		const auto& meta_data() const			{ return reinterpret_cast<Meta&>(mMetaData); }
		auto size() const						{ return mMetaData.total_size(); }
		auto& config() const					{ return mCreateInfo; }
		const auto& memory_properties() const	{ return mMemoryPropertyFlags; }
		const auto& memory_handle() const		{ return mMemory.get(); }
		const auto* memory_handle_ptr() const	{ return &mMemory.get(); }
		const auto& buffer_usage_flags() const	{ return mBufferUsageFlags; }
		const auto& buffer_handle() const		{ return mBuffer.get(); }
		const auto* buffer_handle_ptr() const	{ return &mBuffer.get(); }
		const auto has_device_address() const { return mDeviceAddress.has_value(); }
		const auto device_address() const { return mDeviceAddress.value(); }

		const auto& descriptor_info()
		{
			if (!mDescriptorInfo.has_value()) {
				mDescriptorInfo = vk::DescriptorBufferInfo()
					.setBuffer(buffer_handle())
					.setOffset(0)
					.setRange(size());
			}
			return mDescriptorInfo;
		}

		auto has_descriptor_type()
		{
			if (!mDescriptorType.has_value()) {
				mMetaData.set_descriptor_type(mDescriptorType);
			}
			return mDescriptorType.has_value();
		}

		auto descriptor_type()
		{
			if (!mDescriptorType.has_value()) {
				mMetaData.set_descriptor_type(mDescriptorType);
			}
			return mDescriptorType.value();
		} //< might throw

	private:
		buffer_meta mMetaData;
		vk::BufferCreateInfo mCreateInfo;
		vk::MemoryPropertyFlags mMemoryPropertyFlags;
		vk::UniqueDeviceMemory mMemory;
		vk::BufferUsageFlags mBufferUsageFlags;
		vk::UniqueBuffer mBuffer;
		std::optional<vk::DescriptorBufferInfo> mDescriptorInfo;
		std::optional<vk::DescriptorType> mDescriptorType;
		std::optional<vk::DeviceAddress> mDeviceAddress;
	};

	// SET DATA
	template <typename Meta>
	std::optional<command_buffer> fill(
		const ak::buffer_t<Meta>& target,
		const void* pData, 
		sync aSyncHandler = sync::wait_idle())
	{
		auto bufferSize = static_cast<vk::DeviceSize>(target.size());

		auto memProps = target.memory_properties();

		// #1: Is our memory on the CPU-SIDE? 
		if (ak::has_flag(memProps, vk::MemoryPropertyFlagBits::eHostVisible)) {
			void* mapped = ak::context().logical_device().mapMemory(target.memory_handle(), 0, bufferSize);
			memcpy(mapped, pData, bufferSize);
			// Coherent memory is done; non-coherent memory not yet
			if (!ak::has_flag(memProps, vk::MemoryPropertyFlagBits::eHostCoherent)) {
				// Setup the range 
				auto range = vk::MappedMemoryRange()
					.setMemory(target.memory_handle())
					.setOffset(0)
					.setSize(bufferSize);
				// Flush the range
				ak::context().logical_device().flushMappedMemoryRanges(1, &range);
			}
			ak::context().logical_device().unmapMemory(target.memory_handle());
			// TODO: Handle has_flag(memProps, vk::MemoryPropertyFlagBits::eHostCached) case

			// No need to sync, so.. don't sync!
			return {}; // TODO: This should be okay, is it?
		}

		// #2: Otherwise, it must be on the GPU-SIDE!
		else {
			aSyncHandler.set_queue_hint(ak::context().transfer_queue());
			
			assert(ak::has_flag(memProps, vk::MemoryPropertyFlagBits::eDeviceLocal));

			// We have to create a (somewhat temporary) staging buffer and transfer it to the GPU
			// "somewhat temporary" means that it can not be deleted in this function, but only
			//						after the transfer operation has completed => handle via sync
			auto stagingBuffer = create_and_fill(
				generic_buffer_meta::create_from_size(target.size()),
				ak::memory_usage::host_coherent, 
				pData, 
				sync::not_required(), //< It's host coherent memory => no sync will be required
				vk::BufferUsageFlagBits::eTransferSrc);

			auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
			// Sync before:
			aSyncHandler.establish_barrier_before_the_operation(pipeline_stage::transfer, read_memory_access{memory_access::transfer_read_access});

			// Operation:
			auto copyRegion = vk::BufferCopy{}
				.setSrcOffset(0u)
				.setDstOffset(0u)
				.setSize(bufferSize);
			commandBuffer.handle().copyBuffer(stagingBuffer->buffer_handle(), target.buffer_handle(), { copyRegion });

			// Sync after:
			aSyncHandler.establish_barrier_after_the_operation(pipeline_stage::transfer, write_memory_access{memory_access::transfer_write_access});

			// Take care of the lifetime handling of the stagingBuffer, it might still be in use:
			commandBuffer.set_custom_deleter([
				lOwnedStagingBuffer{ std::move(stagingBuffer) }
			]() { /* Nothing to do here, the buffers' destructors will do the cleanup, the lambda is just storing it. */ });
			
			// Finish him:
			return aSyncHandler.submit_and_sync();			
		}
	}

	// For convenience:
	inline std::optional<command_buffer> fill(const generic_buffer_t& target, const void* pData,		sync aSyncHandler = sync::wait_idle())	{ return fill<generic_buffer_meta>(target, pData,			std::move(aSyncHandler)); }
	inline std::optional<command_buffer> fill(const uniform_buffer_t& target, const void* pData,		sync aSyncHandler = sync::wait_idle())	{ return fill<uniform_buffer_meta>(target, pData,			std::move(aSyncHandler)); }
	inline std::optional<command_buffer> fill(const uniform_texel_buffer_t& target, const void* pData,	sync aSyncHandler = sync::wait_idle())	{ return fill<uniform_texel_buffer_meta>(target, pData,		std::move(aSyncHandler)); }
	inline std::optional<command_buffer> fill(const storage_buffer_t& target, const void* pData,		sync aSyncHandler = sync::wait_idle())	{ return fill<storage_buffer_meta>(target, pData,			std::move(aSyncHandler)); }
	inline std::optional<command_buffer> fill(const storage_texel_buffer_t& target, const void* pData,	sync aSyncHandler = sync::wait_idle())	{ return fill<storage_texel_buffer_meta>(target, pData,		std::move(aSyncHandler)); }
	inline std::optional<command_buffer> fill(const vertex_buffer_t& target, const void* pData,			sync aSyncHandler = sync::wait_idle())	{ return fill<vertex_buffer_meta>(target, pData,			std::move(aSyncHandler)); }
	inline std::optional<command_buffer> fill(const index_buffer_t& target, const void* pData,			sync aSyncHandler = sync::wait_idle())	{ return fill<index_buffer_meta>(target, pData,				std::move(aSyncHandler)); }
	inline std::optional<command_buffer> fill(const instance_buffer_t& target, const void* pData,		sync aSyncHandler = sync::wait_idle())	{ return fill<instance_buffer_meta>(target, pData,			std::move(aSyncHandler)); }
	
	template <typename Meta>
	std::optional<command_buffer> read(
		const ak::buffer_t<Meta>& _Source,
		void* _Data,
		sync aSyncHandler
	)
	{
		auto bufferSize = static_cast<vk::DeviceSize>(_Source.size());
		auto memProps = _Source.memory_properties();

		// #1: Is our memory accessible on the CPU-SIDE? 
		if (ak::has_flag(memProps, vk::MemoryPropertyFlagBits::eHostVisible)) {
			
			const void* mapped = ak::context().logical_device().mapMemory(_Source.memory_handle(), 0, bufferSize);
			if (!ak::has_flag(memProps, vk::MemoryPropertyFlagBits::eHostCoherent)) {
				// Setup the range 
				auto range = vk::MappedMemoryRange()
					.setMemory(_Source.memory_handle())
					.setOffset(0)
					.setSize(bufferSize);
				// Flush the range
				ak::context().logical_device().invalidateMappedMemoryRanges(1, &range); // TODO: Test this! (should be okay, but double-check against spec.: https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/vkInvalidateMappedMemoryRanges.html)
			}
			memcpy(_Data, mapped, bufferSize);
			ak::context().logical_device().unmapMemory(_Source.memory_handle());
			// TODO: Handle has_flag(memProps, vk::MemoryPropertyFlagBits::eHostCached) case
			return {};
		}

		// #2: Otherwise, it must be on the GPU-SIDE!
		else {
			assert(ak::has_flag(memProps, vk::MemoryPropertyFlagBits::eDeviceLocal));

			aSyncHandler.set_queue_hint(ak::context().transfer_queue());

			// We have to create a (somewhat temporary) staging buffer and transfer it to the GPU
			// "somewhat temporary" means that it can not be deleted in this function, but only
			//						after the transfer operation has completed => handle via ak::sync!
			auto stagingBuffer = create(
				generic_buffer_meta::create_from_size(_Source.size()),
				ak::memory_usage::host_coherent);
			// TODO: Creating a staging buffer in every read()-call is probably not optimal. => Think about alternative ways!

			// TODO: What about queue ownership?! If not the device_queue_selection_strategy::prefer_everything_on_single_queue strategy is being applied, it could very well be that this fails.
			auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
			// Sync before:
			aSyncHandler.establish_barrier_before_the_operation(pipeline_stage::transfer, read_memory_access{memory_access::transfer_read_access});

			// Operation:
			auto copyRegion = vk::BufferCopy{}
				.setSrcOffset(0u)
				.setDstOffset(0u)
				.setSize(bufferSize);
			commandBuffer.handle().copyBuffer(_Source.buffer_handle(), stagingBuffer->buffer_handle(), { copyRegion });

			// Sync after:
			aSyncHandler.establish_barrier_after_the_operation(pipeline_stage::transfer, write_memory_access{memory_access::transfer_write_access});

			// Take care of the stagingBuffer's lifetime handling and also of reading the data for this branch:
			commandBuffer.set_custom_deleter([ 
				lOwnedStagingBuffer{ std::move(stagingBuffer) }, 
				_Data
			]() {
				read(lOwnedStagingBuffer, _Data, sync::not_required()); // TODO: not sure about that sync
			});

			// Finish him:
			return aSyncHandler.submit_and_sync();
		}
	}

	inline void read(const generic_buffer_t& target, void* pData,		sync aSyncHandler)	{ read<generic_buffer_meta>(target, pData,			std::move(aSyncHandler)); }
	inline void read(const uniform_buffer_t& target, void* pData,		sync aSyncHandler)	{ read<uniform_buffer_meta>(target, pData,			std::move(aSyncHandler)); }
	inline void read(const uniform_texel_buffer_t& target, void* pData,	sync aSyncHandler)	{ read<uniform_texel_buffer_meta>(target, pData,	std::move(aSyncHandler)); }
	inline void read(const storage_buffer_t& target, void* pData,		sync aSyncHandler)	{ read<storage_buffer_meta>(target, pData,			std::move(aSyncHandler)); }
	inline void read(const storage_texel_buffer_t& target, void* pData,	sync aSyncHandler)	{ read<storage_texel_buffer_meta>(target, pData,	std::move(aSyncHandler)); }
	inline void read(const vertex_buffer_t& target, void* pData,		sync aSyncHandler)	{ read<vertex_buffer_meta>(target, pData,			std::move(aSyncHandler)); }
	inline void read(const index_buffer_t& target, void* pData,			sync aSyncHandler)	{ read<index_buffer_meta>(target, pData,			std::move(aSyncHandler)); }
	inline void read(const instance_buffer_t& target, void* pData,		sync aSyncHandler)	{ read<instance_buffer_meta>(target, pData,			std::move(aSyncHandler)); }

	/**
	 * Read back data from a buffer.
	 *
	 * This is a convenience overload to ak::read.
	 *
	 * Example usage:
	 * uint32_t readData = ak::read<uint32_t>(mMySsbo, ak::sync::not_required());
	 * // ^ given that mMySsbo is a host-coherent buffer. If it is not, sync is required.
	 *
	 * @tparam	Meta		Refers to a certain type of buffer (e.g. `uniform_buffer_meta`, `storage_buffer_meta`, etc.)
	 *						Can usually be deduced automatically.
	 * @tparam	Ret			Specify the type of data that shall be read from the buffer (this is `uint32_t` in the example above).
	 * @returns				A value of type `Ret` which is returned by value.
	 */
	template <typename Meta, typename Ret>
	Ret read(
		const ak::buffer_t<Meta>& _Source,
		sync aSyncHandler
	)
	{
		auto memProps = _Source.memory_properties();
		if (!ak::has_flag(memProps, vk::MemoryPropertyFlagBits::eHostVisible)) {
			throw ak::runtime_error("This ak::read overload can only be used with host-visible buffers. Use ak::void read(const ak::buffer_t<Meta>& _Source, void* _Data, sync aSyncHandler) instead!");
		}
		Ret result;
		read(_Source, static_cast<void*>(&result), std::move(aSyncHandler));
		return result;
	}

	template <typename Ret> Ret read(const generic_buffer_t& target,		sync aSyncHandler)	{ return read<generic_buffer_meta, Ret>(target,		  std::move(aSyncHandler)); }
	template <typename Ret> Ret read(const uniform_buffer_t& target,		sync aSyncHandler)	{ return read<uniform_buffer_meta, Ret>(target,		  std::move(aSyncHandler)); }
	template <typename Ret> Ret read(const uniform_texel_buffer_t& target,	sync aSyncHandler)	{ return read<uniform_texel_buffer_meta, Ret>(target, std::move(aSyncHandler)); }
	template <typename Ret> Ret read(const storage_buffer_t& target,		sync aSyncHandler)	{ return read<storage_buffer_meta, Ret>(target,		  std::move(aSyncHandler)); }
	template <typename Ret> Ret read(const storage_texel_buffer_t& target,	sync aSyncHandler)	{ return read<storage_texel_buffer_meta, Ret>(target, std::move(aSyncHandler)); }
	template <typename Ret> Ret read(const vertex_buffer_t& target,			sync aSyncHandler)	{ return read<vertex_buffer_meta, Ret>(target,		  std::move(aSyncHandler)); }
	template <typename Ret> Ret read(const index_buffer_t& target,			sync aSyncHandler)	{ return read<index_buffer_meta, Ret>(target,		  std::move(aSyncHandler)); }
	template <typename Ret> Ret read(const instance_buffer_t& target,		sync aSyncHandler)	{ return read<instance_buffer_meta, Ret>(target,	  std::move(aSyncHandler)); }

}
