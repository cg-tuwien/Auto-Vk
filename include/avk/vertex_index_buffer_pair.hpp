#pragma once
#include "avk/avk.hpp"

namespace avk
{
	/**	A pair references: one to a vertex buffer, and one to an index buffer.
	 */
	class vertex_index_buffer_pair
	{
	public:
		vertex_index_buffer_pair(avk::resource_argument<buffer_t> bfr1, avk::resource_argument<buffer_t> bfr2)
			: mVertexBuffer(std::move(bfr1)) // could be wrong, though
			, mIndexBuffer(std::move(bfr2))  // could be wrong, though
		{
			if (!mVertexBuffer->has_meta<avk::vertex_buffer_meta>() || !mIndexBuffer->has_meta<avk::index_buffer_meta>()) {
				std::swap(mVertexBuffer, mIndexBuffer);
			}

			if (!mVertexBuffer->has_meta<avk::vertex_buffer_meta>() || !mIndexBuffer->has_meta<avk::index_buffer_meta>()) {
				throw avk::runtime_error("One of the two buffers passed to the constructor of avk::vertex_index_buffer_pair must have avk::vertex_buffer_meta, and the other one must have avk::index_buffer_meta. These requirements are not fulfilled.");
			}

#ifdef _DEBUG
			const int vertexMetaCount = (mVertexBuffer->has_meta<avk::vertex_buffer_meta>() ? 1 : 0) + (mIndexBuffer->has_meta<avk::vertex_buffer_meta>() ? 1 : 0);
			const int indexMetaCount  = (mVertexBuffer->has_meta<avk::index_buffer_meta>()  ? 1 : 0) + (mIndexBuffer->has_meta<avk::index_buffer_meta>()  ? 1 : 0);
			if (1 != vertexMetaCount && 1 != indexMetaCount) {
				AVK_LOG_WARNING("In the constructor of avk::vertex_index_buffer_pair: " + std::to_string(vertexMetaCount) + " buffers have avk::vertex_buffer_meta, and " + std::to_string(indexMetaCount) + " buffers have avk::index_buffer_meta. That looks very suspicions. Configuration might be messed up.");
			}
#endif
		}

		vertex_index_buffer_pair(vertex_index_buffer_pair&&) noexcept = default;
		vertex_index_buffer_pair(const vertex_index_buffer_pair&) = default;
		vertex_index_buffer_pair& operator=(vertex_index_buffer_pair&&) noexcept = default;
		vertex_index_buffer_pair& operator=(const vertex_index_buffer_pair&) = default;
		~vertex_index_buffer_pair() = default;

		/**	Returns the reference to the vertex buffer member
		 *	This might look a bit strange, but it's okay since this class is intended to be used as a temporary class for parameter passing.
		 *	Feel free to move from the returned reference if you do not need it at other places.
		 */
		avk::resource_argument<buffer_t>& vertex_buffer() { return mVertexBuffer; }

		/**	Returns the reference to the index buffer member
		 *	This might look a bit strange, but it's okay since this class is intended to be used as a temporary class for parameter passing.
		 *	Feel free to move from the returned reference if you do not need it at other places.
		 */
		avk::resource_argument<buffer_t>& index_buffer() { return mIndexBuffer; }

	private:
		avk::resource_argument<buffer_t> mVertexBuffer;
		avk::resource_argument<buffer_t> mIndexBuffer;
	};
}
