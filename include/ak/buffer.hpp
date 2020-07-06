#pragma once
#include <ak/ak.hpp>

namespace ak
{
	class buffer_descriptor
	{
		friend class buffer_t;
		
	public:
		auto descriptor_type() const { return mDescriptorType; }
		const auto& descriptor_info() const { return mDescriptorInfo; }
		
	private:
		vk::DescriptorType mDescriptorType;
		vk::DescriptorBufferInfo mDescriptorInfo;
	};
	
	/** Represents a Vulkan buffer along with its assigned memory, holds the 
	*	native handle and takes care about lifetime management of the native handles.
	*/
	class buffer_t
	{
		friend class root;
		
	public:
		buffer_t() = default;
		buffer_t(buffer_t&&) noexcept = default;
		buffer_t(const buffer_t&) = delete;
		buffer_t& operator=(buffer_t&&) noexcept = default;
		buffer_t& operator=(const buffer_t&) = delete;
		~buffer_t() = default; // Declaration order determines destruction order (inverse!)

		template <typename Meta>
		const Meta& meta_data(size_t aMetaDataIndex = 0) const			{ return reinterpret_cast<Meta&>(mMetaData[aMetaDataIndex]); }
		
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
					.setRange(config().size); // TODO: Support different offsets and ranges
			}
			return mDescriptorInfo;
		}

		auto has_descriptor_type(size_t aOffset = 0) const
		{
			return meta_data<buffer_meta>(aOffset).descriptor_type().has_value();
		}

		auto descriptor_type(size_t aOffset = 0) const
		{
			return meta_data<buffer_meta>(aOffset).descriptor_type().value();
		} //< might throw

		auto as_buffer_from_meta(size_t aMetaDataIndex = 0) const
		{
			buffer_descriptor result;
#ifdef _DEBUG
			if (!mDescriptorInfo.has_value()) {
				AK_LOG_ERROR("Descriptor info is not set => buffer can not be bound.");
			}
#endif
			result.mDescriptorInfo = mDescriptorInfo.value();;
			auto descType = meta_data(aMetaDataIndex).descriptor_type();
#ifdef _DEBUG
			if (!descType.has_value()) {
				AK_LOG_ERROR("Buffer meta data at index #" + std::to_string(aMetaDataIndex) + " does not have a descriptor type => buffer can not be bound.");
			}
#endif
			result.mDescriptorType = descType.value();
			return result;
		}
		
		auto as_uniform_buffer() const
		{
			buffer_descriptor result;
#ifdef _DEBUG
			if (!mDescriptorInfo.has_value()) {
				AK_LOG_ERROR("Descriptor info is not set => buffer can not be bound.");
			}
#endif
			result.mDescriptorInfo = mDescriptorInfo.value();
			result.mDescriptorType = vk::DescriptorType::eUniformBuffer;
			return result;
		}
		
		auto as_uniform_texel_buffer() const
		{
			buffer_descriptor result;
#ifdef _DEBUG
			if (!mDescriptorInfo.has_value()) {
				AK_LOG_ERROR("Descriptor info is not set => buffer can not be bound.");
			}
#endif
			result.mDescriptorInfo = mDescriptorInfo.value();
			result.mDescriptorType = vk::DescriptorType::eUniformTexelBuffer;
			return result;
		}
		
		auto as_storage_buffer() const
		{
			buffer_descriptor result;
#ifdef _DEBUG
			if (!mDescriptorInfo.has_value()) {
				AK_LOG_ERROR("Descriptor info is not set => buffer can not be bound.");
			}
#endif
			result.mDescriptorInfo = mDescriptorInfo.value();
			result.mDescriptorType = vk::DescriptorType::eStorageBuffer;
			return result;
		}
		
		auto as_storage_texel_buffer() const
		{
			buffer_descriptor result;
#ifdef _DEBUG
			if (!mDescriptorInfo.has_value()) {
				AK_LOG_ERROR("Descriptor info is not set => buffer can not be bound.");
			}
#endif
			result.mDescriptorInfo = mDescriptorInfo.value();
			result.mDescriptorType = vk::DescriptorType::eStorageTexelBuffer;
			return result;
		}

		/** Fill buffer with data.
		 */
		std::optional<command_buffer> fill(const void* pData, size_t aMetaDataIndex = 0, sync aSyncHandler = sync::wait_idle());

		std::optional<command_buffer> read(void* aData, size_t aMetaDataIndex = 0, sync aSyncHandler = sync::wait_idle()) const;

		/**
		 * Read back data from a buffer.
		 *
		 * This is a convenience overload to ak::read.
		 *
		 * Example usage:
		 * uint32_t readData = ak::read<uint32_t>(mMySsbo, ak::sync::not_required());
		 * // ^ given that mMySsbo is a host-coherent buffer. If it is not, sync is required.
		 *
		 * @tparam	Ret			Specify the type of data that shall be read from the buffer (this is `uint32_t` in the example above).
		 * @returns				A value of type `Ret` which is returned by value.
		 */
		template <typename Ret>
		Ret read(size_t aMetaDataIndex = 0, sync aSyncHandler = sync::wait_idle())
		{
			auto memProps = memory_properties();
			if (!ak::has_flag(memProps, vk::MemoryPropertyFlagBits::eHostVisible)) {
				throw ak::runtime_error("This ak::read overload can only be used with host-visible buffers. Use ak::void read(const ak::buffer_t<Meta>& _Source, void* _Data, sync aSyncHandler) instead!");
			}
			Ret result;
			read(static_cast<void*>(&result), aMetaDataIndex, std::move(aSyncHandler));
			return result;
		}		
		
	private:
		std::vector<buffer_meta> mMetaData;
		vk::BufferCreateInfo mCreateInfo;
		vk::MemoryPropertyFlags mMemoryPropertyFlags;
		vk::UniqueDeviceMemory mMemory;
		vk::BufferUsageFlags mBufferUsageFlags;
		vk::PhysicalDevice mPhysicalDevice;
		vk::UniqueBuffer mBuffer;
		std::optional<vk::DeviceAddress> mDeviceAddress;
		std::optional<vk::DescriptorBufferInfo> mDescriptorInfo;
	};

	/** Typedef representing any kind of OWNING image view representations. */
	using buffer = owning_resource<buffer_t>;

}
