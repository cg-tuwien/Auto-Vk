 #pragma once

namespace ak
{
	// Forward declare templated ak::buffer and set up some type defs.
	// The definition of the buffer will always be inside the context-specific implementation files.
	template <typename Meta>
	class buffer_t;

	using generic_buffer_t = buffer_t<generic_buffer_meta>;
	using uniform_buffer_t = buffer_t<uniform_buffer_meta>;
	using uniform_texel_buffer_t = buffer_t<uniform_texel_buffer_meta>;
	using storage_buffer_t = buffer_t<storage_buffer_meta>;
	using storage_texel_buffer_t = buffer_t<storage_texel_buffer_meta>;
	using vertex_buffer_t = buffer_t<vertex_buffer_meta>;
	using index_buffer_t = buffer_t<index_buffer_meta>;
	using instance_buffer_t = buffer_t<instance_buffer_meta>;

	using generic_buffer		= owning_resource<generic_buffer_t>;
	using uniform_buffer		= owning_resource<uniform_buffer_t>;
	using uniform_texel_buffer	= owning_resource<uniform_texel_buffer_t>;
	using storage_buffer		= owning_resource<storage_buffer_t>;
	using storage_texel_buffer	= owning_resource<storage_texel_buffer_t>;
	using vertex_buffer			= owning_resource<vertex_buffer_t>;
	using index_buffer			= owning_resource<index_buffer_t>;
	using instance_buffer		= owning_resource<instance_buffer_t>;


}
