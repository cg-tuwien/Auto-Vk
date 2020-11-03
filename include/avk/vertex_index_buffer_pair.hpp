#pragma once
#include <avk/avk.hpp>

namespace avk
{
	/**	A pair references: one to a vertex buffer, and one to an index buffer.
	 */
	class vertex_index_buffer_pair
	{
	public:
		vertex_index_buffer_pair(avk::resource_reference<const buffer_t> bfr1, avk::resource_reference<const buffer_t> bfr2)
			: mVertexBuffer(bfr1) // could be wrong, though
			, mIndexBuffer(bfr2)  // could be wrong, though
		{
			if (bfr2->has_meta<avk::vertex_buffer_meta>()) {
				mVertexBuffer = std::cref(bfr2);
				mIndexBuffer = std::cref(bfr1);
				assert(bfr1->has_meta<avk::index_buffer_meta>());
			}
			else {
				assert(bfr2->has_meta<avk::index_buffer_meta>());
				assert(bfr1->has_meta<avk::vertex_buffer_meta>());
			}
		}

		vertex_index_buffer_pair(vertex_index_buffer_pair&&) noexcept = default;
		vertex_index_buffer_pair(const vertex_index_buffer_pair&) = default;
		vertex_index_buffer_pair& operator=(vertex_index_buffer_pair&&) noexcept = default;
		vertex_index_buffer_pair& operator=(const vertex_index_buffer_pair&) = default;
		~vertex_index_buffer_pair() = default;

		avk::resource_reference<const buffer_t> vertex_buffer() const { return mVertexBuffer; }
		avk::resource_reference<const buffer_t> index_buffer() const { return mIndexBuffer; }

	private:
		avk::resource_reference<const buffer_t> mVertexBuffer;
		avk::resource_reference<const buffer_t> mIndexBuffer;
	};
}
