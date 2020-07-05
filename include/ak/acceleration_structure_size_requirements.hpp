#pragma once
#include <ak/ak.hpp>

namespace ak
{
	struct acceleration_structure_size_requirements
	{
		static acceleration_structure_size_requirements from_buffers(const ak::vertex_buffer_t& aVertexBuffer, const ak::index_buffer_t& aIndexBuffer);
		static acceleration_structure_size_requirements from_buffers(const ak::index_buffer_t& aIndexBuffer, const ak::vertex_buffer_t& aVertexBuffer);

		template <typename T>
		static std::vector<acceleration_structure_size_requirements> from_buffers(const T& aCollection)
		{
			std::vector<acceleration_structure_size_requirements> result;
			for (const auto& element : aCollection) {
				result.push_back(from_buffers(std::get<vertex_buffer>(element), std::get<index_buffer>(element)));
			}

			return result;
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
}
