#pragma once
#include <avk/avk.hpp>

namespace avk
{
	enum struct pipeline_stage : uint32_t
	{
		top_of_pipe = 0x00000001,
		draw_indirect = 0x00000002,
		vertex_input = 0x00000004,
		vertex_shader = 0x00000008,
		tessellation_control_shader = 0x00000010,
		tessellation_evaluation_shader = 0x00000020,
		geometry_shader = 0x00000040,
		fragment_shader = 0x00000080,
		early_fragment_tests = 0x00000100,
		late_fragment_tests = 0x00000200,
		color_attachment_output = 0x00000400,
		compute_shader = 0x00000800,
		transfer = 0x00001000,
		bottom_of_pipe = 0x00002000,
		host = 0x00004000,
		all_graphics = 0x00008000,
		all_commands = 0x00010000,
		transform_feedback = 0x00020000,
		conditional_rendering = 0x00040000,
		command_preprocess = 0x00080000,
		shading_rate_image = 0x00100000,
		ray_tracing_shaders = 0x00200000,
		acceleration_structure_build = 0x00400000,
		task_shader = 0x00800000,
		mesh_shader = 0x01000000,
		fragment_density_process = 0x02000000,
	};

	inline pipeline_stage operator| (pipeline_stage a, pipeline_stage b)
	{
		typedef std::underlying_type<pipeline_stage>::type EnumType;
		return static_cast<pipeline_stage>(static_cast<EnumType>(a) | static_cast<EnumType>(b));
	}

	inline pipeline_stage operator& (pipeline_stage a, pipeline_stage b)
	{
		typedef std::underlying_type<pipeline_stage>::type EnumType;
		return static_cast<pipeline_stage>(static_cast<EnumType>(a) & static_cast<EnumType>(b));
	}

	inline pipeline_stage& operator |= (pipeline_stage& a, pipeline_stage b)
	{
		return a = a | b;
	}

	inline pipeline_stage& operator &= (pipeline_stage& a, pipeline_stage b)
	{
		return a = a & b;
	}

	inline pipeline_stage exclude(pipeline_stage original, pipeline_stage toExclude)
	{
		typedef std::underlying_type<pipeline_stage>::type EnumType;
		return static_cast<pipeline_stage>(static_cast<EnumType>(original) & (~static_cast<EnumType>(toExclude)));
	}

	inline bool is_included(const pipeline_stage toTest, const pipeline_stage includee)
	{
		return (toTest & includee) == includee;
	}


	class queue;

	struct queue_ownership
	{
		queue_ownership& from(queue* aSrcQueue) { mSrcQueue = aSrcQueue; return *this; }
		queue_ownership& to(queue* aDstQueue) { mDstQueue = aDstQueue; return *this; }

		queue_ownership* operator-> () { return this; }

		std::optional<queue*> mSrcQueue;
		std::optional<queue*> mDstQueue;
	};

	class buffer_t;

	struct pipeline_barrier_buffer_data
	{
		pipeline_barrier_buffer_data(buffer_t& aBufferRef)
			: mBufferRef(&aBufferRef)
		{
		}

		pipeline_barrier_buffer_data* operator-> () { return this; }

		pipeline_barrier_buffer_data& operator>> (const queue_ownership& aQueueOwnershipTransfer)
		{
			mSrcQueue = aQueueOwnershipTransfer.mSrcQueue;
			mDstQueue = aQueueOwnershipTransfer.mDstQueue;
			return *this;
		}

		buffer_t* mBufferRef; // TODO: should be owning resource
		std::optional<queue*> mSrcQueue;
		std::optional<queue*> mDstQueue;
	};

	using for_buffer = pipeline_barrier_buffer_data;

	class command_buffer_t;

	class pipeline_barrier_data
	{
	public:
		pipeline_barrier_data& top_of_pipe() { mDstStage = pipeline_stage::top_of_pipe; return *this; }
		pipeline_barrier_data& draw_indirect() { mDstStage = pipeline_stage::draw_indirect; return *this; }
		pipeline_barrier_data& vertex_input() { mDstStage = pipeline_stage::vertex_input; return *this; }
		pipeline_barrier_data& vertex_shader() { mDstStage = pipeline_stage::vertex_shader; return *this; }
		pipeline_barrier_data& tessellation_control_shader() { mDstStage = pipeline_stage::tessellation_control_shader; return *this; }
		pipeline_barrier_data& tessellation_evaluation_shader() { mDstStage = pipeline_stage::tessellation_evaluation_shader; return *this; }
		pipeline_barrier_data& geometry_shader() { mDstStage = pipeline_stage::geometry_shader; return *this; }
		pipeline_barrier_data& fragment_shader() { mDstStage = pipeline_stage::fragment_shader; return *this; }
		pipeline_barrier_data& early_fragment_tests() { mDstStage = pipeline_stage::early_fragment_tests; return *this; }
		pipeline_barrier_data& late_fragment_tests() { mDstStage = pipeline_stage::late_fragment_tests; return *this; }
		pipeline_barrier_data& color_attachment_output() { mDstStage = pipeline_stage::color_attachment_output; return *this; }
		pipeline_barrier_data& compute_shader() { mDstStage = pipeline_stage::compute_shader; return *this; }
		pipeline_barrier_data& transfer() { mDstStage = pipeline_stage::transfer; return *this; }
		pipeline_barrier_data& bottom_of_pipe() { mDstStage = pipeline_stage::bottom_of_pipe; return *this; }
		pipeline_barrier_data& host() { mDstStage = pipeline_stage::host; return *this; }
		pipeline_barrier_data& all_graphics() { mDstStage = pipeline_stage::all_graphics; return *this; }
		pipeline_barrier_data& all_commands() { mDstStage = pipeline_stage::all_commands; return *this; }
		pipeline_barrier_data& transform_feedback() { mDstStage = pipeline_stage::transform_feedback; return *this; }
		pipeline_barrier_data& conditional_rendering() { mDstStage = pipeline_stage::conditional_rendering; return *this; }
		pipeline_barrier_data& command_preprocess() { mDstStage = pipeline_stage::command_preprocess; return *this; }
		pipeline_barrier_data& shading_rate_image() { mDstStage = pipeline_stage::shading_rate_image; return *this; }
		pipeline_barrier_data& ray_tracing_shaders() { mDstStage = pipeline_stage::ray_tracing_shaders; return *this; }
		pipeline_barrier_data& acceleration_structure_build() { mDstStage = pipeline_stage::acceleration_structure_build; return *this; }
		pipeline_barrier_data& task_shader() { mDstStage = pipeline_stage::task_shader; return *this; }
		pipeline_barrier_data& mesh_shader() { mDstStage = pipeline_stage::mesh_shader; return *this; }
		pipeline_barrier_data& fragment_density_process() { mDstStage = pipeline_stage::fragment_density_process; return *this; }

		pipeline_barrier_data* operator-> () { return this; }
		
		pipeline_barrier_data& operator> (pipeline_barrier_buffer_data aBufferBarrierData) { mBufferMemoryBarriers.emplace_back(std::move(aBufferBarrierData)); return *this; }

		void make_barrier(command_buffer_t& aIntoCommandBuffer) const;

	protected:
		std::optional<pipeline_stage> mSrcStage;
		std::optional<pipeline_stage> mDstStage;
		std::vector<pipeline_barrier_buffer_data> mBufferMemoryBarriers;
	};

	class top_of_pipe : public pipeline_barrier_data { public: top_of_pipe() { mSrcStage = pipeline_stage::top_of_pipe; } };
	class draw_indirect : public pipeline_barrier_data { public: draw_indirect() { mSrcStage = pipeline_stage::draw_indirect; } };
	class vertex_input : public pipeline_barrier_data { public: vertex_input() { mSrcStage = pipeline_stage::vertex_input; } };
	class vertex_shader : public pipeline_barrier_data { public: vertex_shader() { mSrcStage = pipeline_stage::vertex_shader; } };
	class tessellation_control_shader : public pipeline_barrier_data { public: tessellation_control_shader() { mSrcStage = pipeline_stage::tessellation_control_shader; } };
	class tessellation_evaluation_shader : public pipeline_barrier_data { public: tessellation_evaluation_shader() { mSrcStage = pipeline_stage::tessellation_evaluation_shader; } };
	class geometry_shader : public pipeline_barrier_data { public: geometry_shader() { mSrcStage = pipeline_stage::geometry_shader; } };
	class fragment_shader : public pipeline_barrier_data { public: fragment_shader() { mSrcStage = pipeline_stage::fragment_shader; } };
	class early_fragment_tests : public pipeline_barrier_data { public: early_fragment_tests() { mSrcStage = pipeline_stage::early_fragment_tests; } };
	class late_fragment_tests : public pipeline_barrier_data { public: late_fragment_tests() { mSrcStage = pipeline_stage::late_fragment_tests; } };
	class color_attachment_output : public pipeline_barrier_data { public: color_attachment_output() { mSrcStage = pipeline_stage::color_attachment_output; } };
	class compute_shader : public pipeline_barrier_data { public: compute_shader() { mSrcStage = pipeline_stage::compute_shader; } };
	class transfer : public pipeline_barrier_data { public: transfer() { mSrcStage = pipeline_stage::transfer; } };
	class bottom_of_pipe : public pipeline_barrier_data { public: bottom_of_pipe() { mSrcStage = pipeline_stage::bottom_of_pipe; } };
	class host : public pipeline_barrier_data { public: host() { mSrcStage = pipeline_stage::host; } };
	class all_graphics : public pipeline_barrier_data { public: all_graphics() { mSrcStage = pipeline_stage::all_graphics; } };
	class all_commands : public pipeline_barrier_data { public: all_commands() { mSrcStage = pipeline_stage::all_commands; } };
	class transform_feedback : public pipeline_barrier_data { public: transform_feedback() { mSrcStage = pipeline_stage::transform_feedback; } };
	class conditional_rendering : public pipeline_barrier_data { public: conditional_rendering() { mSrcStage = pipeline_stage::conditional_rendering; } };
	class command_preprocess : public pipeline_barrier_data { public: command_preprocess() { mSrcStage = pipeline_stage::command_preprocess; } };
	class shading_rate_image : public pipeline_barrier_data { public: shading_rate_image() { mSrcStage = pipeline_stage::shading_rate_image; } };
	class ray_tracing_shaders : public pipeline_barrier_data { public: ray_tracing_shaders() { mSrcStage = pipeline_stage::ray_tracing_shaders; } };
	class acceleration_structure_build : public pipeline_barrier_data { public: acceleration_structure_build() { mSrcStage = pipeline_stage::acceleration_structure_build; } };
	class task_shader : public pipeline_barrier_data { public: task_shader() { mSrcStage = pipeline_stage::task_shader; } };
	class mesh_shader : public pipeline_barrier_data { public: mesh_shader() { mSrcStage = pipeline_stage::mesh_shader; } };
	class fragment_density_process : public pipeline_barrier_data { public: fragment_density_process() { mSrcStage = pipeline_stage::fragment_density_process; } };

}
