#pragma once
#include <avk/avk.hpp>

namespace avk
{
#if VK_HEADER_VERSION >= 135
	struct acceleration_structure_size_requirements
	{
		static acceleration_structure_size_requirements from_buffers(vertex_index_buffer_pair aPair);

		static acceleration_structure_size_requirements from_aabbs(uint32_t aNumAabbs)
		{
			return acceleration_structure_size_requirements {
				vk::GeometryTypeKHR::eAabbs,
				aNumAabbs,
				0, 0u, {}
			};
		}
		
		template <typename T>
		static acceleration_structure_size_requirements from_aabbs(const T& aCollection) // TODO: This probably needs some refactoring
		{
			return acceleration_structure_size_requirements {
				vk::GeometryTypeKHR::eAabbs,
				static_cast<uint32_t>(aCollection.size()),
				0, 0u, {}
			};
		}

		vk::GeometryTypeKHR mGeometryType;
		uint32_t mNumPrimitives;
		size_t mIndexTypeSize;		
		uint32_t mNumVertices;
		vk::Format mVertexFormat;
	};	
#endif
}
