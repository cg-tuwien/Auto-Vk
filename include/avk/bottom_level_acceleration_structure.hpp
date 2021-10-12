#pragma once
#include <avk/avk.hpp>

namespace avk
{
#if VK_HEADER_VERSION >= 135
	class bottom_level_acceleration_structure_t
	{
		friend class root;
	public:
		bottom_level_acceleration_structure_t() = default;
		bottom_level_acceleration_structure_t(bottom_level_acceleration_structure_t&&) noexcept = default;
		bottom_level_acceleration_structure_t(const bottom_level_acceleration_structure_t&) = delete;
		bottom_level_acceleration_structure_t& operator=(bottom_level_acceleration_structure_t&&) noexcept = default;
		bottom_level_acceleration_structure_t& operator=(const bottom_level_acceleration_structure_t&) = delete;
		~bottom_level_acceleration_structure_t();

#if VK_HEADER_VERSION >= 162
		const auto& create_info() const	{ return mCreateInfo; }
		auto& create_info()				{ return mCreateInfo; }
		auto acceleration_structure_handle() { return mAccStructure.mHandle; }
		auto* acceleration_structure_handle_ptr() { return &mAccStructure.mHandle; }
		auto acceleration_structure_handle() const { return mAccStructure.mHandle; }
		const auto* acceleration_structure_handle_ptr() const { return &mAccStructure.mHandle; }
#else
		const auto& create_info() const { return mCreateInfo; }
		auto& create_info()				{ return mCreateInfo; }
		auto& acceleration_structure_handle() { return mAccStructure.mHandle; }
		auto* acceleration_structure_handle_ptr() { return &mAccStructure.mHandle; }
		auto& memory_handle() { return mMemory.get(); }
		auto* memory_handle_ptr() { return &mMemory.get(); }
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

		/** Build this bottom level acceleration structure using one pair or multiple pairs of buffers.
		 *
		 *	@param	aGeometries		Vector of pairs of buffers where one buffer must be an index buffer and the other must be a vertex buffer.
		 *							I.e. they must have the appropriate meta_data set: index_buffer_meta and vertex_buffer_meta, respectively.
		 *	@param	aScratchBuffer	Optional reference to a buffer to be used as scratch buffer. It must have the buffer usage flags
		 *							vk::BufferUsageFlagBits::eRayTracingKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR set.
		 *							If no scratch buffer is supplied, one will be created internally.
		 *	@param	aSyncHandler	Sync handler which is to be deprecated
		 */
		std::optional<command_buffer> build(const std::vector<vertex_index_buffer_pair>& aGeometries, std::optional<std::reference_wrapper<buffer_t>> aScratchBuffer = {}, sync aSyncHandler = sync::wait_idle());

		/** Update this bottom level acceleration structure using one pair or multiple pairs of buffers.
		 *
		 *	@param	aGeometries		Vector of pairs of buffers where one buffer must be an index buffer and the other must be a vertex buffer.
		 *							I.e. they must have the appropriate meta_data set: index_buffer_meta and vertex_buffer_meta, respectively.
		 *	@param	aScratchBuffer	Optional reference to a buffer to be used as scratch buffer. It must have the buffer usage flags
		 *							vk::BufferUsageFlagBits::eRayTracingKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR set.
		 *							If no scratch buffer is supplied, one will be created internally.
		 *	@param	aSyncHandler	Sync handler which is to be deprecated
		 */
		std::optional<command_buffer> update(const std::vector<vertex_index_buffer_pair>& aGeometries, std::optional<std::reference_wrapper<buffer_t>> aScratchBuffer = {}, sync aSyncHandler = sync::wait_idle());
		
		/** Build this bottom level acceleration structure using a vector of axis-aligned bounding boxes.
		 *
		 *	@param	aGeometries		Vector of axis aligned bounding boxes that will be used as bottom-level acceleration structure geometry primitives.
		 *	@param	aScratchBuffer	Optional reference to a buffer to be used as scratch buffer. It must have the buffer usage flags
		 *							vk::BufferUsageFlagBits::eRayTracingKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR set.
		 *							If no scratch buffer is supplied, one will be created internally.
		 *	@param	aSyncHandler	Sync handler which is to be deprecated
		 */
		std::optional<command_buffer> build(const std::vector<VkAabbPositionsKHR>& aGeometries, std::optional<std::reference_wrapper<buffer_t>> aScratchBuffer = {}, sync aSyncHandler = sync::wait_idle());
		
		/** Update this bottom level acceleration structure using a vector of axis-aligned bounding boxes.
		 *
		 *	@param	aGeometries		Vector of axis aligned bounding boxes that will be used as bottom-level acceleration structure geometry primitives.
		 *	@param	aScratchBuffer	Optional reference to a buffer to be used as scratch buffer. It must have the buffer usage flags
		 *							vk::BufferUsageFlagBits::eRayTracingKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR set.
		 *							If no scratch buffer is supplied, one will be created internally.
		 *	@param	aSyncHandler	Sync handler which is to be deprecated
		 */
		std::optional<command_buffer> update(const std::vector<VkAabbPositionsKHR>& aGeometries, std::optional<std::reference_wrapper<buffer_t>> aScratchBuffer = {}, sync aSyncHandler = sync::wait_idle());

		/** Build this bottom level acceleration structure using a buffer containing axis-aligned bounding boxes.
		 *
		 *	@param	aGeometriesBuffer	Buffer containing containing one or multiple axis-aligned bounding boxes. The buffer must have the appropriate
		 *								meta data set, which is aabb_buffer_meta.
		 *	@param	aScratchBuffer		Optional reference to a buffer to be used as scratch buffer. It must have the buffer usage flags
		 *								vk::BufferUsageFlagBits::eRayTracingKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR set.
		 *								If no scratch buffer is supplied, one will be created internally.
		 *	@param	aSyncHandler		Sync handler which is to be deprecated
		 */
		std::optional<command_buffer> build(const buffer& aGeometriesBuffer, std::optional<std::reference_wrapper<buffer_t>> aScratchBuffer = {}, sync aSyncHandler = sync::wait_idle());

		/** Update this bottom level acceleration structure using a buffer containing axis-aligned bounding boxes.
		 *
		 *	@param	aGeometriesBuffer	Buffer containing one or multiple axis-aligned bounding boxes. The buffer must have the appropriate
		 *								meta data set, which is aabb_buffer_meta.
		 *	@param	aScratchBuffer		Optional reference to a buffer to be used as scratch buffer. It must have the buffer usage flags
		 *								vk::BufferUsageFlagBits::eRayTracingKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR set.
		 *								If no scratch buffer is supplied, one will be created internally.
		 *	@param	aSyncHandler		Sync handler which is to be deprecated
		 */
		std::optional<command_buffer> update(const buffer& aGeometriesBuffer, std::optional<std::reference_wrapper<buffer_t>> aScratchBuffer = {}, sync aSyncHandler = sync::wait_idle());
		
	private:
		enum struct blas_action { build, update };
		std::optional<command_buffer> build_or_update(const std::vector<vertex_index_buffer_pair>& aGeometries, std::optional<std::reference_wrapper<buffer_t>> aScratchBuffer, sync aSyncHandler, blas_action aBuildAction);
		std::optional<command_buffer> build_or_update(const std::vector<VkAabbPositionsKHR>& aGeometries, std::optional<std::reference_wrapper<buffer_t>> aScratchBuffer, sync aSyncHandler, blas_action aBuildAction);
		std::optional<command_buffer> build_or_update(const buffer& aGeometriesBuffer, std::optional<std::reference_wrapper<buffer_t>> aScratchBuffer, sync aSyncHandler, blas_action aBuildAction);
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
		//std::vector<vk::GeometryKHR> mGeometries;
		vk::BuildAccelerationStructureFlagsKHR mFlags;
		vk::AccelerationStructureCreateInfoKHR mCreateInfo;
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
	};

	using bottom_level_acceleration_structure = avk::owning_resource<bottom_level_acceleration_structure_t>;
#endif
}
