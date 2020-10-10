#pragma once

namespace avk
{
	/**	Helper class that can be used with avk::mem_handle's or avk::vma_handle's
	 *	memory mapping functionality. It is advisable to use this class instead of
	 *	calling their ::map_memory and ::unmap_memory member functions directly,
	 *	because this class provides more safety: It ensures that ::unmap_memory
	 *	is invoked under all circumstances and also that the call to ::unmap_memory
	 *	is not forgotten since it must be ensured that for each ::map_memory call,
	 *	also an ::unmap_memory call is issued.
	 *
	 *	Usage example:
	 *
	 *	// Invoke ::map_memory on mBuffer, express the intent to read from it:
	 *	auto mapped = scoped_mapping{mBuffer, mapping_access::read};
	 *	// Copy 10 byte from the mapped memory (at address 'mapped.get()' to aDataPtr:
	 *	memcpy(aDataPtr, mapped.get(), 10);
	 *	// The destructor of 'mapped' will invoke ::unmap_memory.
	 */
	template <typename T>
	class scoped_mapping
	{
	public:
		/**	Invoke ::map_memory on aMemHandle
		 *	@param	aAccess		In which way are you planning to access aMemHandle?
		 *						This can be a combination of multiple flags.
		 */
		scoped_mapping(const T& aMemHandle, mapping_access aAcces)
			: mMemHandle{ &aMemHandle }
			, mAccess{ aAcces }
			, mMappedMemory{ nullptr }
		{
			mMappedMemory = mMemHandle->map_memory(mAccess);
		}

		/**	Get the memory address of the mapped memory.
		 *	Use this data pointer to write to or read from!
		 */
		void* get() const
		{
			return mMappedMemory;
		}

		/**	The destructor will invoke ::map_memory on the resource.
		 */
		~scoped_mapping()
		{
			mMemHandle->unmap_memory(mAccess);
			mMemHandle = nullptr;
		}

	private:
		const T* mMemHandle;
		mapping_access mAccess;
		void* mMappedMemory;
	};
}
