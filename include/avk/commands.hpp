#pragma once
#include <avk/avk.hpp>

namespace avk
{
	class command_buffer_t;
	using command_buffer = avk::owning_resource<command_buffer_t>;
	
	class commands
	{
		using rec_cmd = std::function<void(avk::command_buffer_t&)>;
		using sync_data = std::tuple<avk::pipeline_stage, avk::memory_access>;
		
	public:
		/** You're not supposed to store an instance of commands anywhere. */
		commands() = delete;
		commands(commands&&) noexcept = default;
		commands(const commands&) = delete;
		commands& operator=(commands&&) noexcept = default;
		commands& operator=(const commands&) = delete;
		~commands() = default;

		template <typename F>
		commands(avk::pipeline_stage aPreSyncDst, avk::memory_access aPreAccessDst, F&& aCommands, avk::pipeline_stage aPostSyncSrc, avk::memory_access aPostAccessSrc)
			: mRecordCommands{ std::forward(aCommands) }
			, mPreSync{ aPreSyncDst, aPreAccessDst }
			, mPostSync{ aPostSyncSrc, aPostAccessSrc }
		{ }
		
		void record_into(command_buffer_t& aCommandBuffer)
		{
			mRecordCommands(aCommandBuffer);
		}
		
		rec_cmd mRecordCommands;
		sync_data mPreSync;
		sync_data mPostSync;
	};
}
