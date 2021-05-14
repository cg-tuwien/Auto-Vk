#pragma once
#include <avk/avk.hpp>

namespace avk
{
#if VK_HEADER_VERSION >= 135
	class top_level_acceleration_structure_t
	{
		friend class root;
		
	public:
		top_level_acceleration_structure_t() = default;
		top_level_acceleration_structure_t(top_level_acceleration_structure_t&&) noexcept = default;
		top_level_acceleration_structure_t(const top_level_acceleration_structure_t&) = delete;
		top_level_acceleration_structure_t& operator=(top_level_acceleration_structure_t&&) noexcept = default;
		top_level_acceleration_structure_t& operator=(const top_level_acceleration_structure_t&) = delete;
		~top_level_acceleration_structure_t();

#if VK_HEADER_VERSION >= 162
		auto& config() { return mCreateInfo; }
		auto acceleration_structure_handle() { return mAccStructure.mHandle; }
		auto* acceleration_structure_handle_ptr() { return &mAccStructure.mHandle; }
		const auto& config() const { return mCreateInfo; }
		auto acceleration_structure_handle() const { return mAccStructure.mHandle; }
		const auto* acceleration_structure_handle_ptr() const { return &mAccStructure.mHandle; }
#else
		auto& config() { return mCreateInfo; }
		auto& acceleration_structure_handle() { return mAccStructure.mHandle; }
		auto* acceleration_structure_handle_ptr() { return &mAccStructure.mHandle; }
		auto& memory_handle() { return mMemory.get(); }
		auto* memory_handle_ptr() { return &mMemory.get(); }
		const auto& config() const { return mCreateInfo; }
		const auto& acceleration_structure_handle() const { return mAccStructure.mHandle; }
		const auto* acceleration_structure_handle_ptr() const { return &mAccStructure.mHandle; }
		const auto& memory_handle() const { return mMemory.get(); }
		const auto* memory_handle_ptr() const { return &mMemory.get(); }
#endif
		auto device_address() const { return mDeviceAddress; }

#if VK_HEADER_VERSION >= 162
		size_t required_acceleration_structure_size() const { return static_cast<size_t>(mMemoryRequirementsForAccelerationStructure); }
		size_t required_scratch_buffer_build_size() const { return static_cast<size_t>(mMemoryRequirementsForBuildScratchBuffer); }
		size_t required_scratch_buffer_update_size() const { return static_cast<size_t>(mMemoryRequirementsForScratchBufferUpdate); }
#else
		size_t required_acceleration_structure_size() const { return static_cast<size_t>(mMemoryRequirementsForAccelerationStructure.memoryRequirements.size); }
		size_t required_scratch_buffer_build_size() const { return static_cast<size_t>(mMemoryRequirementsForBuildScratchBuffer.memoryRequirements.size); }
		size_t required_scratch_buffer_update_size() const { return static_cast<size_t>(mMemoryRequirementsForScratchBufferUpdate.memoryRequirements.size); }
#endif

		const auto& descriptor_info() const
		{
			mDescriptorInfo = vk::WriteDescriptorSetAccelerationStructureKHR{}
				.setAccelerationStructureCount(1u)
				.setPAccelerationStructures(acceleration_structure_handle_ptr());
			return mDescriptorInfo;
		}

		auto descriptor_type() const			{ return vk::DescriptorType::eAccelerationStructureKHR; } 

		/** Build this top level acceleration structure using vectors of geometry instances or buffers containing geometry instances.
		 *	Supported parameters:
		 *	 - avk::resource_reference<const buffer_t> ........... A buffer containing geometry instances. Optimally, use avk::const_referenced to pass such buffers
		 *	 - const std::vector<geometry_instance>& ............. A vector containing one or multiple geometry_instances. They will be stored into a buffer before processing them further. // TODO: Can we support direct TLAS-builds without that geometry_instance->buffer step?
		 *	 - std::optional<std::reference_wrapper<buffer_t>> ... Provide an external scratch buffer. If none is provided, one will be created internally.                                  // TODO: Make a nicer interface for passing the scratch buffer!
		 *	 - sync .............................................. Provide a sync handler! The default is sync::wait_idle()
		 *
		 *	While it only makes sense to pass one scratch buffer and one sync-handler, an arbitrary number of buffers containing geometry
		 *	instances, and an arbitrary number of vectors containing geometry_instances is supported.
		 */
		template <typename... Args>
		void build(Args&&... aArgs)
		{
			gather_config_and_build_or_update(tlas_action::build, std::forward<Args>(aArgs)...);
		}

		/** Update this top level acceleration structure using vectors of geometry instances or buffers containing geometry instances.
		 *	Supported parameters:
		 *	 - avk::resource_reference<const buffer_t> ........... A buffer containing geometry instances. Optimally, use avk::const_referenced to pass such buffers
		 *	 - const std::vector<geometry_instance>& ............. A vector containing one or multiple geometry_instances. They will be stored into a buffer before processing them further. // TODO: Can we support direct TLAS-builds without that geometry_instance->buffer step?
		 *	 - std::optional<std::reference_wrapper<buffer_t>> ... Provide an external scratch buffer. If none is provided, one will be created internally.                                  // TODO: Make a nicer interface for passing the scratch buffer!
		 *	 - sync .............................................. Provide a sync handler! The default is sync::wait_idle()
		 *
		 *	While it only makes sense to pass one scratch buffer and one sync-handler, an arbitrary number of buffers containing geometry
		 *	instances, and an arbitrary number of vectors containing geometry_instances is supported.
		 */
		template <typename... Args>
		void update(Args&&... aArgs)
		{
			gather_config_and_build_or_update(tlas_action::update, std::forward<Args>(aArgs)...);
		}
		
	private:
		enum struct tlas_action { build, update };
		std::optional<command_buffer> build_or_update(
			tlas_action aBuildAction,
			std::variant<avk::resource_reference<const buffer_t>, std::reference_wrapper<const std::vector<vk::AccelerationStructureInstanceKHR>>, std::reference_wrapper<const std::vector<vk::DeviceOrHostAddressConstKHR>>> aArrayOrArrayOfPointers,
			std::optional<std::reference_wrapper<buffer_t>> aScratchBuffer,
			sync aSyncHandler);

		static void gather_args(std::optional<avk::resource_reference<const buffer_t>>& aBufferRef, std::vector<avk::buffer>& aCreatedBuffers, std::vector<vk::DeviceOrHostAddressConstKHR>& aMixedReferences, std::optional<std::reference_wrapper<buffer_t>>& aScratchBuffer, sync& aSyncHandler)
		{
			// stopper
		}

		static void transfer_buffer_ref_into_mixed_references(std::optional<avk::resource_reference<const buffer_t>>& aBufferRef, std::vector<avk::buffer>& aCreatedBuffers, std::vector<vk::DeviceOrHostAddressConstKHR>& aMixedReferences);

		// Warning: only invoke this method once it can be ensured that aHostInstances does not change in size anymore
		static void tidy_up(std::optional<avk::resource_reference<const buffer_t>>& aBufferRef, std::vector<avk::buffer>& aCreatedBuffers, std::vector<vk::DeviceOrHostAddressConstKHR>& aMixedReferences);

		template <typename... Rest>
		void gather_args(
			std::optional<avk::resource_reference<const buffer_t>>& aBufferRef, std::vector<avk::buffer>& aCreatedBuffers, std::vector<vk::DeviceOrHostAddressConstKHR>& aMixedReferences, std::optional<std::reference_wrapper<buffer_t>>& aScratchBuffer, sync& aSyncHandler,
			sync& aSyncHandlerIn, Rest&&... aRest)
		{
			aSyncHandler = std::move(aSyncHandlerIn);
			gather_args(aBufferRef, aCreatedBuffers, aMixedReferences, aScratchBuffer, aSyncHandler, std::forward<Rest>(aRest)...);
		}

		template <typename... Rest>
		void gather_args(
			std::optional<avk::resource_reference<const buffer_t>>& aBufferRef, std::vector<avk::buffer>& aCreatedBuffers, std::vector<vk::DeviceOrHostAddressConstKHR>& aMixedReferences, std::optional<std::reference_wrapper<buffer_t>>& aScratchBuffer, sync& aSyncHandler,
			sync&& aSyncHandlerIn, Rest&&... aRest)
		{
			aSyncHandler = std::move(aSyncHandlerIn);
			gather_args(aBufferRef, aCreatedBuffers, aMixedReferences, aScratchBuffer, aSyncHandler, std::forward<Rest>(aRest)...);
		}

		template <typename... Rest>
		void gather_args(
			std::optional<avk::resource_reference<const buffer_t>>& aBufferRef, std::vector<avk::buffer>& aCreatedBuffers, std::vector<vk::DeviceOrHostAddressConstKHR>& aMixedReferences, std::optional<std::reference_wrapper<buffer_t>>& aScratchBuffer, sync& aSyncHandler,
			avk::resource_reference<const buffer_t> aDeviceBuffer, Rest&&... aRest)
		{
			while (aBufferRef.has_value()) {
				transfer_buffer_ref_into_mixed_references(aBufferRef, aCreatedBuffers, aMixedReferences);
			}
			aBufferRef = aDeviceBuffer;
			gather_args(aBufferRef, aCreatedBuffers, aMixedReferences, aScratchBuffer, aSyncHandler, std::forward<Rest>(aRest)...);
		}

		template <typename... Rest>
		void gather_args(
			std::optional<avk::resource_reference<const buffer_t>>& aBufferRef, std::vector<avk::buffer>& aCreatedBuffers, std::vector<vk::DeviceOrHostAddressConstKHR>& aMixedReferences, std::optional<std::reference_wrapper<buffer_t>>& aScratchBuffer, sync& aSyncHandler,
			const buffer& aDeviceBuffer, Rest&&... aRest)
		{
			gather_args(aBufferRef, aCreatedBuffers, aMixedReferences, aScratchBuffer, aSyncHandler, avk::const_referenced(aDeviceBuffer), std::forward<Rest>(aRest)...);
		}

		template <typename... Rest>
		void gather_args(
			std::optional<avk::resource_reference<const buffer_t>>& aBufferRef, std::vector<avk::buffer>& aCreatedBuffers, std::vector<vk::DeviceOrHostAddressConstKHR>& aMixedReferences, std::optional<std::reference_wrapper<buffer_t>>& aScratchBuffer, sync& aSyncHandler,
			const std::vector<geometry_instance>& aGeometryInstances, Rest&&... aRest)
		{
			auto geomInstances = convert_for_gpu_usage(aGeometryInstances);

			auto geomInstBuffer = root::create_buffer(
				mPhysicalDevice, mDevice, mAllocator,
				AVK_STAGING_BUFFER_MEMORY_USAGE, {},
				geometry_instance_buffer_meta::create_from_data(geomInstances)
			);
			geomInstBuffer->fill(geomInstances.data(), 0, sync::not_required());

			auto& newBfr = aCreatedBuffers.emplace_back(std::move(geomInstBuffer));

			gather_args(aBufferRef, aCreatedBuffers, aMixedReferences, aScratchBuffer, aSyncHandler, newBfr, std::forward<Rest>(aRest)...);
		}

		template <typename... Rest>
		void gather_args(
			std::optional<avk::resource_reference<const buffer_t>>& aBufferRef, std::vector<avk::buffer>& aCreatedBuffers, std::vector<vk::DeviceOrHostAddressConstKHR>& aMixedReferences, std::optional<std::reference_wrapper<buffer_t>>& aScratchBuffer, sync& aSyncHandler,
			std::vector<geometry_instance>&& aGeometryInstances, Rest&&... aRest)
		{
			gather_args(aBufferRef, aCreatedBuffers, aMixedReferences, aScratchBuffer, aSyncHandler, static_cast<const std::vector<geometry_instance>&>(aGeometryInstances), std::forward<Rest>(aRest)...);
		}

		template <typename... Rest>
		void gather_args(
			std::optional<avk::resource_reference<const buffer_t>>& aBufferRef, std::vector<avk::buffer>& aCreatedBuffers, std::vector<vk::DeviceOrHostAddressConstKHR>& aMixedReferences, std::optional<std::reference_wrapper<buffer_t>>& aScratchBuffer, sync& aSyncHandler,
			std::optional<std::reference_wrapper<buffer_t>>& aScratchBufferIn, Rest&&... aRest)
		{
			aScratchBuffer = aScratchBufferIn;
			gather_args(aBufferRef, aCreatedBuffers, aMixedReferences, aScratchBuffer, aSyncHandler, std::forward<Rest>(aRest)...);
		}

		template <typename... Args>
		std::optional<command_buffer> gather_config_and_build_or_update(tlas_action aBuildAction, Args&&... aArgs)
		{
			std::optional<avk::resource_reference<const avk::buffer_t>> bufferRef = {};
			std::vector<avk::buffer> createdBuffers;
			std::vector<vk::DeviceOrHostAddressConstKHR> mixedReferences;
			std::optional<std::reference_wrapper<avk::buffer_t>> scratchBuffer = {};
			sync syncHandler = sync::wait_idle();

			gather_args(bufferRef, createdBuffers, mixedReferences, scratchBuffer, syncHandler, std::forward<Args>(aArgs)...);
			tidy_up(bufferRef, createdBuffers, mixedReferences);

			assert((bufferRef.has_value() ? 1 : 0) + (mixedReferences.empty() ? 0 : 1) == 1); // Only one of these data structures must be set, otherwise we made an implementation mistake
			auto result = build_or_update(
				aBuildAction, 
				bufferRef.has_value()
					? std::variant<avk::resource_reference<const buffer_t>, std::reference_wrapper<const std::vector<vk::AccelerationStructureInstanceKHR>>, std::reference_wrapper<const std::vector<vk::DeviceOrHostAddressConstKHR>>>{ bufferRef.value() }
					: std::variant<avk::resource_reference<const buffer_t>, std::reference_wrapper<const std::vector<vk::AccelerationStructureInstanceKHR>>, std::reference_wrapper<const std::vector<vk::DeviceOrHostAddressConstKHR>>>{ std::cref(mixedReferences) },
				scratchBuffer, 
				std::move(syncHandler)
			);

			if (result.has_value()) {
				// Handle lifetime:
				result.value()->set_custom_deleter([lBufferRef = std::move(bufferRef), lCreatedBuffers = std::move(createdBuffers), lMixedReferences = std::move(mixedReferences)](){});
			}
			else {
				AVK_LOG_INFO("Sorry for this mDevice::waitIdle call :( It will be gone after command/commands-refactoring");
				mDevice.waitIdle();
			}
			return result;
		}

		buffer_t& get_and_possibly_create_scratch_buffer();

#if VK_HEADER_VERSION >= 162
		vk::DeviceSize mMemoryRequirementsForAccelerationStructure;
		vk::DeviceSize mMemoryRequirementsForBuildScratchBuffer;
		vk::DeviceSize mMemoryRequirementsForScratchBufferUpdate;
		buffer mAccStructureBuffer;
#else
		vk::MemoryRequirements2KHR mMemoryRequirementsForAccelerationStructure;
		vk::MemoryRequirements2KHR mMemoryRequirementsForBuildScratchBuffer;
		vk::MemoryRequirements2KHR mMemoryRequirementsForScratchBufferUpdate;
		vk::MemoryAllocateInfo mMemoryAllocateInfo;
		vk::UniqueDeviceMemory mMemory;
#endif

#if VK_HEADER_VERSION >= 162
		std::vector<vk::AccelerationStructureGeometryKHR> mAccStructureGeometries;
		std::vector<uint32_t> mBuildPrimitiveCounts;
		vk::AccelerationStructureBuildGeometryInfoKHR mBuildGeometryInfo;
#else
		std::vector<vk::AccelerationStructureCreateGeometryTypeInfoKHR> mGeometryInfos;
#endif
		
		vk::AccelerationStructureCreateInfoKHR mCreateInfo;
		vk::BuildAccelerationStructureFlagsKHR mFlags;
		vk::PhysicalDevice mPhysicalDevice;
		vk::Device mDevice;
		AVK_MEM_ALLOCATOR_TYPE mAllocator;
#if VK_HEADER_VERSION >= 162
		avk::handle_wrapper<vk::AccelerationStructureKHR> mAccStructure;
#else
		//vk::ResultValueType<vk::UniqueHandle<vk::AccelerationStructureKHR, vk::DispatchLoaderDynamic>>::type mAccStructure;
		avk::handle_wrapper<vk::AccelerationStructureKHR> mAccStructure;
#endif
		vk::DispatchLoaderDynamic mDynamicDispatch;
		vk::DeviceAddress mDeviceAddress;

		std::optional<buffer> mScratchBuffer;
		
		mutable vk::WriteDescriptorSetAccelerationStructureKHR mDescriptorInfo;
	};

	using top_level_acceleration_structure = avk::owning_resource<top_level_acceleration_structure_t>;
#endif
}
