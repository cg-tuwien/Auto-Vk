#pragma once

namespace ak 
{
	extern VkAccelerationStructureInstanceKHR convert_for_gpu_usage(const geometry_instance& aGeomInst);
	extern std::vector<VkAccelerationStructureInstanceKHR> convert_for_gpu_usage(const std::vector<geometry_instance>& aGeomInstances);	
}
