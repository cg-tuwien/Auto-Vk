#pragma once
#include <avk/avk.hpp>

namespace avk
{
	class command_buffer_t;
	using command_buffer = avk::owning_resource<command_buffer_t>;
	class old_sync;
	
	/**	A helper-class representing a descriptor to a given buffer,
	 *	containing the descriptor type and the descriptor info.
	 *
	 *	A buffer can have mutliple usage types, when bound via
	 *	descriptor, only one specific usage type matters.
	 *	This helper-class helps representing such a usage.
	 */
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

		struct get_buffer_meta
		{
			get_buffer_meta(const buffer_meta*& aOut) : mOut{ &aOut } {}
			const buffer_meta** mOut = nullptr;
			void operator()(const buffer_meta& data)					const	{ *mOut = &data; }
			void operator()(const generic_buffer_meta& data)			const	{ *mOut = &data; }
			void operator()(const uniform_buffer_meta& data)			const	{ *mOut = &data; }
			void operator()(const uniform_texel_buffer_meta& data)		const	{ *mOut = &data; }
			void operator()(const storage_buffer_meta& data)			const	{ *mOut = &data; }
			void operator()(const storage_texel_buffer_meta& data)		const	{ *mOut = &data; }
			void operator()(const vertex_buffer_meta& data)				const	{ *mOut = &data; }
			void operator()(const index_buffer_meta& data)				const	{ *mOut = &data; }
			void operator()(const instance_buffer_meta& data)			const	{ *mOut = &data; }
			void operator()(const query_results_buffer_meta& data)		const	{ *mOut = &data; }
			void operator()(const indirect_buffer_meta& data)			const	{ *mOut = &data; }
#if VK_HEADER_VERSION >= 135
			void operator()(const aabb_buffer_meta& data)				const	{ *mOut = &data; }
			void operator()(const geometry_instance_buffer_meta& data)	const	{ *mOut = &data; }
#endif
		};
		
	public:
		buffer_t() = default;
		buffer_t(buffer_t&&) noexcept = default;
		buffer_t(const buffer_t&) = delete;
		buffer_t& operator=(buffer_t&&) noexcept = default;
		buffer_t& operator=(const buffer_t&) = delete;
		~buffer_t() = default; // Declaration order determines destruction order (inverse!)

		/** Returns the number of meta data entries assigned to this buffer */
		auto meta_count() const { return mMetaData.size(); }
		
		/**	Gets the meta data at the specified index, cast to type Meta.
		 *	This method does not throw, except the index is out of bounds.
		 *	There must be at least one meta data entry in the buffer.
		 *	I.e. index 0 should always work.
		 *	@param	aMetaDataIndex		Index into the meta data, must be
		 *								between 0 and meta_count()-1
		 */
		template <typename Meta>
		const Meta& meta_at_index(size_t aMetaDataIndex = 0) const
		{
			const buffer_meta* metaPtr = nullptr;
			std::visit(get_buffer_meta(metaPtr), mMetaData[aMetaDataIndex]);
			return *reinterpret_cast<const Meta*>(metaPtr);
		}
		
		const auto& create_info() const	{ return mCreateInfo; }
		auto& create_info()				{ return mCreateInfo; }
		auto handle() const			{ return mBuffer.resource(); }

		/** Returns a reference to this buffer's memory handle.
		 *	Attention: It returns a reference! => Use with caution,
		 *	           and consider using map_memory instead if that's what you're after!
		 */
		const auto& memory_handle() const { return mBuffer; }

		/**	Invokes ::map_memory on the memory_handle, i.e. IF the buffer is in host-visible
		 *	memory, its data pointer can be used to read or write to/from the mapped memory.
		 *	A scoped_mapping is returned which will automatically unmap the memory upon destruction.
		 *	Use its .get() method to get the data pointer, but do not unmap manually!
		 */
		scoped_mapping<AVK_MEM_BUFFER_HANDLE> map_memory(mapping_access aAcces) const { return {mBuffer, aAcces}; }
		
		auto usage_flags() const	{ return mBufferUsageFlags; }
		auto memory_properties() const          { return mBuffer.memory_properties(); }
		const auto has_device_address() const { return mDeviceAddress.has_value(); }
		const auto device_address() const { return mDeviceAddress.value(); }

		/**	Returns a reference to the descriptor info. If no descriptor info exists yet,
		 *	one is created, that includes the buffer handle, offset is set to 0, and
		 *	the size to total_size.
		 */
		const auto& descriptor_info() const
		{
			if (!mDescriptorInfo.has_value()) {
				mDescriptorInfo = vk::DescriptorBufferInfo()
					.setBuffer(handle())
					.setOffset(0)
					.setRange(create_info().size); // TODO: Support different offsets and ranges
			}
			return mDescriptorInfo.value();
		}

		/**	Tests whether this buffer contains the given meta data.
		 *	@return		true if it does contain a meta data of type Meta, false otherwise.
		 */
		template <typename Meta>
		bool has_meta(size_t aSkip = 0) const
		{
			for (const auto& m : mMetaData) {
				if (std::holds_alternative<Meta>(m)) {
					if (aSkip-- > 0) { continue; }
					return true;
				}
			}
			return false;
		}

		/**	Searches for the given Meta type in meta data and and if contained, 
		 *	returns its index. If not contained, -1 is returned.
		 *	@param aSkip	Number of times, Meta shall be skipped. 
		 *	@return			Index of a meta data element of type Meta, or -1
		 */
		template <typename Meta>
		int index_of_meta(size_t aSkip = 0) const
		{
			for (int i = 0; i < static_cast<int>(mMetaData.size()); ++i) {
				const auto& m = mMetaData[i];
				if (std::holds_alternative<Meta>(m)) {
					if (aSkip-- > 0) { continue; }
					return i;
				}
			}
			return -1;
		}
		
		/**	Searches for the given Meta type in meta and if contained, returns
		 *	a const reference to it. Throws if not contained.
		 *	@param aSkip	Number of times, Meta shall be skipped.
		 *	@return			Reference to meta data of type Meta found.
		 */
		template <typename Meta>
		const Meta& meta(size_t aSkip = 0) const
		{
			for (const auto& m : mMetaData) {
				if (std::holds_alternative<Meta>(m)) {
					if (aSkip-- > 0) { continue; }
					return std::get<Meta>(m);
				}
			}
			throw avk::runtime_error(std::string("Meta data of type '") + typeid(Meta).name() + "' not found");
		}

		/**	Searches for the given Meta type in meta and if contained, returns
		 *	a mutable reference to it. Throws if not contained.
		 *	@param aSkip	Number of times, Meta shall be skipped.
		 *	@return			Reference to meta data of type Meta found.
		 */
		template <typename Meta>
		Meta& meta(size_t aSkip = 0)
		{
			for (auto& m : mMetaData) {
				if (std::holds_alternative<Meta>(m)) {
					if (aSkip-- > 0) { continue; }
					return std::get<Meta>(m);
				}
			}
			throw avk::runtime_error(std::string("Meta data of type '") + typeid(Meta).name() + "' not found");
		}

		/**	Search for the given meta data of type Meta, and build a
		 *	buffer_descriptor instance with descriptor info and
		 *	descriptor type set.
		 */
		template <typename Meta>
		auto get_buffer_descriptor() const
		{
			buffer_descriptor result;
			result.mDescriptorInfo = descriptor_info();
			result.mDescriptorType = meta<Meta>().descriptor_type().value();
			return result;			
		}

		/** Get a buffer_descriptor for binding this buffer as a uniform buffer. */
		auto as_uniform_buffer() const { return get_buffer_descriptor<uniform_buffer_meta>(); }
		/** Get a buffer_descriptor for binding this buffer as a uniform buffer. */
		auto as_storage_buffer() const { return get_buffer_descriptor<storage_buffer_meta>(); }

		/** Fill buffer with data.
		 *  The buffer's size is determined from its metadata.
		 *	Please note: The returned command will not contain any sort of lifetime handling measure for the given buffer.
		 *               Instead, the caller must ensure that the buffer outlives the lifetime of the returned command.
		 *  @param aDataPtr			Pointer to the data to copy to the buffer. MUST point to at least enough data to fill the buffer entirely.
		 *  @param aMetaDataIndex	Index of the buffer metadata to use (for determining the buffer size)
		 */
		command::action_type_command fill(const void* aDataPtr, size_t aMetaDataIndex) const;

		// TODO: Maybe the following overload could be re-enabled after command/commands refactoring?!
		///** Fill buffer with data according to the meta data of the given type Meta.
		// *  The buffer's size is determined from its metadata
		// *  @param aDataPtr			Pointer to the data to copy to the buffer. MUST point to at least enough data to fill the buffer entirely.
		// *  @param aMetaDataSkip	How often a meta data of type Meta shall be skipped. I.e. values != 0 only make sense if there ar multiple meta data entries of type Meta.
		// */
		//template <typename Meta>
		//std::optional<command_buffer> fill(const void* aDataPtr, size_t aMetaDataSkip, old_sync aSyncHandler)
		//{
		//	assert(has_meta<Meta>(aMetaDataSkip));
		//	return fill(aDataPtr, index_of_meta<Meta>(aMetaDataSkip), std::move(aSyncHandler));
		//}

		/** Fill buffer partially with data.
		*
		*  @param aDataPtr			Pointer to the data to copy to the buffer
		*  @param aMetaDataIndex	Index of the buffer metadata to use (for size validation only)
		*  @param aOffsetInBytes	Offset from the start of the buffer (data will be copied to bufferstart + aOffset)
		*  @param aDataSizeInBytes	Number of bytes to copy
		*/
		command::action_type_command fill(const void* aDataPtr, size_t aMetaDataIndex, size_t aOffsetInBytes, size_t aDataSizeInBytes) const;

		/** Read data from buffer back to the CPU-side, into some given memory.
		 *	@param	aDataPtr		Target memory where to write read-back data into
		 *	@param	aMetaDataIndex	Index of the meta data index which is used for the buffer's read-back data (size and stuff)
		 */	
		avk::command::action_type_command read_into(void* aDataPtr, size_t aMetaDataIndex) const;

		/**
		 * Read back data from a buffer.
		 *
		 * This is a convenience overload to avk::read.
		 *
		 * Example usage:
		 * uint32_t readData = avk::read<uint32_t>(mMySsbo, avk::old_sync::not_required());
		 * // ^ given that mMySsbo is a host-coherent buffer. If it is not, sync is required.
		 *
		 * @tparam	Ret			Specify the type of data that shall be read from the buffer (this is `uint32_t` in the example above).
		 * @returns				A value of type `Ret` which is returned by value.
		 */
		template<typename Ret>
		[[nodiscard]] Ret read(size_t aMetaDataIndex) {
			auto memProps = memory_properties();
			Ret result;
			read_into(static_cast<void*>(&result), aMetaDataIndex);
			return result;
		}

		[[nodiscard]] const auto* root_ptr() const { return mRoot; }

	private:
#if VK_HEADER_VERSION >= 135
		std::vector<std::variant<buffer_meta, generic_buffer_meta, uniform_buffer_meta, uniform_texel_buffer_meta, storage_buffer_meta, storage_texel_buffer_meta, vertex_buffer_meta, index_buffer_meta, instance_buffer_meta, aabb_buffer_meta, geometry_instance_buffer_meta, query_results_buffer_meta, indirect_buffer_meta>> mMetaData;
#else
		std::vector<std::variant<buffer_meta, generic_buffer_meta, uniform_buffer_meta, uniform_texel_buffer_meta, storage_buffer_meta, storage_texel_buffer_meta, vertex_buffer_meta, index_buffer_meta, instance_buffer_meta, query_results_buffer_meta, indirect_buffer_meta>> mMetaData;
#endif
		vk::BufferCreateInfo mCreateInfo;
		vk::BufferUsageFlags mBufferUsageFlags;
		AVK_MEM_BUFFER_HANDLE mBuffer;
		const root* mRoot;
		std::optional<vk::DeviceAddress> mDeviceAddress;

		mutable std::optional<vk::DescriptorBufferInfo> mDescriptorInfo;
	};

	/** Typedef representing any kind of OWNING buffer representation. */
	using buffer = owning_resource<buffer_t>;

}
