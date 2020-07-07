#pragma once
#include <ak/ak.hpp>

namespace ak
{
	/**	A pair references: one to a vertex buffer, and one to an index buffer.
	 */
	class vertex_index_buffer_pair
	{
	public:
		vertex_index_buffer_pair(const buffer_t& bfr1, const buffer_t& bfr2)
			: mVertexBuffer(std::cref(bfr1)) // could be wrong, though
			, mIndexBuffer(std::cref(bfr2))  // could be wrong, though
		{
			if (bfr2.has_meta<ak::vertex_buffer_meta>()) {
				mVertexBuffer = std::cref(bfr2);
				mIndexBuffer = std::cref(bfr1);
				assert(bfr1.has_meta<ak::index_buffer_meta>());
			}
			else {
				assert(bfr1.has_meta<ak::index_buffer_meta>());
				assert(bfr2.has_meta<ak::vertex_buffer_meta>());
			}
		}

		vertex_index_buffer_pair(vertex_index_buffer_pair&&) noexcept = default;
		vertex_index_buffer_pair(const vertex_index_buffer_pair&) = delete;
		vertex_index_buffer_pair& operator=(vertex_index_buffer_pair&&) noexcept = default;
		framebuffer_t& operator=(const vertex_index_buffer_pair&) = delete;
		~vertex_index_buffer_pair() = default;

		const buffer_t& vertex_buffer() const { return mVertexBuffer; }
		const buffer_t& index_buffer() const { return mIndexBuffer; }

	private:
		std::reference_wrapper<const buffer_t> mVertexBuffer;
		std::reference_wrapper<const buffer_t> mIndexBuffer;
	};
}
