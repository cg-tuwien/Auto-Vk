#pragma once
#include <avk/avk.hpp>

namespace avk
{
	extern vk::IndexType to_vk_index_type(size_t aSize);
	
	extern vk::Bool32 to_vk_bool(bool value);

	/** Converts a avk::shader_type to the vulkan-specific vk::ShaderStageFlagBits type */
	extern vk::ShaderStageFlagBits to_vk_shader_stage(shader_type aType);

	extern vk::ShaderStageFlags to_vk_shader_stages(shader_type aType);

	extern vk::VertexInputRate to_vk_vertex_input_rate(vertex_input_buffer_binding::kind aValue);
	
	extern vk::PrimitiveTopology to_vk_primitive_topology(cfg::primitive_topology aValue);

	extern vk::PolygonMode to_vk_polygon_mode(cfg::polygon_drawing_mode aValue);

	extern vk::CullModeFlags to_vk_cull_mode(cfg::culling_mode aValue);

	extern vk::FrontFace to_vk_front_face(cfg::winding_order aValue);

	extern vk::CompareOp to_vk_compare_op(cfg::compare_operation aValue);

	extern vk::ColorComponentFlags to_vk_color_components(cfg::color_channel aValue);

	extern vk::BlendFactor to_vk_blend_factor(cfg::blending_factor aValue);

	extern vk::BlendOp to_vk_blend_operation(cfg::color_blending_operation aValue);

	extern vk::LogicOp to_vk_logic_operation(cfg::blending_logic_operation aValue);

	extern vk::AttachmentLoadOp to_vk_load_op(avk::on_load aValue);

	extern vk::AttachmentStoreOp to_vk_store_op(avk::on_store aValue);

	extern vk::PipelineStageFlags to_vk_pipeline_stage_flags(avk::pipeline_stage aValue);
	extern vk::PipelineStageFlags to_vk_pipeline_stage_flags(std::optional<avk::pipeline_stage> aValue);

	extern vk::AccessFlags to_vk_access_flags(avk::memory_access aValue);
	extern vk::AccessFlags to_vk_access_flags(std::optional<avk::memory_access> aValue);
	extern avk::memory_access to_memory_access(avk::read_memory_access aValue);
	extern std::optional<avk::memory_access> to_memory_access(std::optional<avk::read_memory_access> aValue);
	extern avk::memory_access to_memory_access(avk::write_memory_access aValue);
	extern std::optional<avk::memory_access> to_memory_access(std::optional<avk::write_memory_access> aValue);

	extern vk::ImageViewType to_image_view_type(const vk::ImageCreateInfo& info);
	
}
