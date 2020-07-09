#pragma once
#include <ak/ak.hpp>

namespace ak
{
	struct acceleration_structure_size_requirements
	{
		static acceleration_structure_size_requirements from_buffers(vertex_index_buffer_pair aPair);

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
}
