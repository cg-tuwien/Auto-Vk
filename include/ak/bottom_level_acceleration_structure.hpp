#pragma once
#include <ak/ak.hpp>

namespace ak
{
	class bottom_level_acceleration_structure_t
	{
		friend class root;
	public:
		bottom_level_acceleration_structure_t() = default;
		bottom_level_acceleration_structure_t(bottom_level_acceleration_structure_t&&) noexcept = default;
		bottom_level_acceleration_structure_t(const bottom_level_acceleration_structure_t&) = delete;
		bottom_level_acceleration_structure_t& operator=(bottom_level_acceleration_structure_t&&) noexcept = default;
		bottom_level_acceleration_structure_t& operator=(const bottom_level_acceleration_structure_t&) = delete;
		~bottom_level_acceleration_structure_t() = default;

		const auto& config() const { return mCreateInfo; }
		const auto& acceleration_structure_handle() const { return mAccStructure.get(); }
		const auto* acceleration_structure_handle_ptr() const { return &mAccStructure.get(); }
		const auto& memory_handle() const { return mMemory.get(); }
		const auto* memory_handle_ptr() const { return &mMemory.get(); }
		auto device_address() const { return mDeviceAddress; }

		size_t required_acceleration_structure_size() const { return static_cast<size_t>(mMemoryRequirementsForAccelerationStructure.memoryRequirements.size); }
		size_t required_scratch_buffer_build_size() const { return static_cast<size_t>(mMemoryRequirementsForBuildScratchBuffer.memoryRequirements.size); }
		size_t required_scratch_buffer_update_size() const { return static_cast<size_t>(mMemoryRequirementsForScratchBufferUpdate.memoryRequirements.size); }

		std::optional<command_buffer> build(const std::vector<vertex_index_buffer_pair>& aGeometries, std::optional<std::reference_wrapper<buffer_t>> aScratchBuffer = {}, sync aSyncHandler = sync::wait_idle());
		std::optional<command_buffer> update(const std::vector<vertex_index_buffer_pair>& aGeometries, std::optional<std::reference_wrapper<buffer_t>> aScratchBuffer = {}, sync aSyncHandler = sync::wait_idle());
		std::optional<command_buffer> build(const std::vector<ak::aabb>& aGeometries, std::optional<std::reference_wrapper<buffer_t>> aScratchBuffer = {}, sync aSyncHandler = sync::wait_idle());
		std::optional<command_buffer> update(const std::vector<ak::aabb>& aGeometries, std::optional<std::reference_wrapper<buffer_t>> aScratchBuffer = {}, sync aSyncHandler = sync::wait_idle());
		
	private:
		enum struct blas_action { build, update };
		std::optional<command_buffer> build_or_update(const std::vector<vertex_index_buffer_pair>& aGeometries, std::optional<std::reference_wrapper<buffer_t>> aScratchBuffer, sync aSyncHandler, blas_action aBuildAction);
		std::optional<command_buffer> build_or_update(const std::vector<ak::aabb>& aGeometries, std::optional<std::reference_wrapper<buffer_t>> aScratchBuffer, sync aSyncHandler, blas_action aBuildAction);
		buffer_t& get_and_possibly_create_scratch_buffer();
		
		vk::MemoryRequirements2KHR mMemoryRequirementsForAccelerationStructure;
		vk::MemoryRequirements2KHR mMemoryRequirementsForBuildScratchBuffer;
		vk::MemoryRequirements2KHR mMemoryRequirementsForScratchBufferUpdate;
		vk::MemoryAllocateInfo mMemoryAllocateInfo;
		vk::UniqueDeviceMemory mMemory;

		std::vector<vk::AccelerationStructureCreateGeometryTypeInfoKHR> mGeometryInfos;
		//std::vector<vk::GeometryKHR> mGeometries;
		vk::AccelerationStructureCreateInfoKHR mCreateInfo;
		vk::PhysicalDevice mPhysicalDevice;
		vk::ResultValueType<vk::UniqueHandle<vk::AccelerationStructureKHR, vk::DispatchLoaderDynamic>>::type mAccStructure;
		vk::DispatchLoaderDynamic mDynamicDispatch;
		vk::DeviceAddress mDeviceAddress;

		std::optional<buffer> mScratchBuffer;
	};

	using bottom_level_acceleration_structure = ak::owning_resource<bottom_level_acceleration_structure_t>;
}
