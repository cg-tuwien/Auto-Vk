#pragma once
#include <avk/avk.hpp>

namespace avk
{
	class query_t;

	/** Represents a Vulkan query pool
	*/
	class query_pool_t
	{
		friend class root;
		
	public:
		query_pool_t() = default;
		query_pool_t(const query_pool_t&) = delete;
		query_pool_t(query_pool_t&&) noexcept = default;
		query_pool_t& operator=(const query_pool_t&) = delete;
		query_pool_t& operator=(query_pool_t&&) noexcept = default;
		~query_pool_t() = default;

		const auto& create_info() const	{ return mCreateInfo; }
		auto& create_info()				{ return mCreateInfo; }
		const auto& handle() const { return mQueryPool.get(); }
		const auto* handle_ptr() const { return &mQueryPool.get(); }

		/** Performs a host reset of some or all queries inside the query pool
		 *	@param	aFirstQueryIndex		The first query to reset
		 *	@param	aNumQueries				The number of queries to reset, starting at aFirstQueryIndex.
		 *									I.e. the query indices [aFirstQueryIndex..aNumQueries) will be reset.
		 *									If not set, all queries from [aFirstQueryIndex..maxIndex] will be reset.
		 */
		void host_reset(uint32_t aFirstQueryIndex = 0u, std::optional<uint32_t> aNumQueries = {});

		/** Performs a reset of some or all queries inside the query pool using a command buffer
		 *	@param	aFirstQueryIndex		The first query to reset
		 *	@param	aNumQueries				The number of queries to reset, starting at aFirstQueryIndex.
		 *									I.e. the query indices [aFirstQueryIndex..aNumQueries) will be reset.
		 *									If not set, all queries from [aFirstQueryIndex..maxIndex] will be reset.
		 */
		std::optional<command_buffer> reset(uint32_t aFirstQueryIndex = 0u, std::optional<uint32_t> aNumQueries = {}, avk::sync aSync = avk::sync::wait_idle());

		/**	Issues a command to write a timestamp after all current commands
		 *	in a queue have reached the given pipeline stage.
		 *
		 *	@param	aQueryIndex		Which query of the pool shall the timestamp be written to
		 *	@param	aTimestampStage	The stage(s) which all previously submitted commands have
		 *							to reach before the timestamp is written.
		 *	@param	aSync			How to submit this instruction
		 * 
		 */
		std::optional<command_buffer> write_timestamp(uint32_t aQueryIndex, pipeline_stage aTimestampStage, sync aSync);

		/**	Issues a command to begin a certain query.
		 *
		 *	@param	aQueryIndex		Which query of the pool shall the result be written to
		 *	@param	aFlags			Additional flags for the instruction
		 *	@param	aSync			How to submit this instruction
		 * 
		 */
		std::optional<command_buffer> begin_query(uint32_t aQueryIndex, vk::QueryControlFlags aFlags, sync aSync);

		/**	Issues a command to end a certain query.
		 *
		 *	@param	aQueryIndex		Which query of the pool shall the result be written to
		 *	@param	aSync			How to submit this instruction
		 * 
		 */
		std::optional<command_buffer> end_query(uint32_t aQueryIndex, sync aSync);

		/**	Get results of multiple queries
		 *
		 *	@tparam T					The data receiving the result.
		 *								In most cases, you'll want to use either uint32_t or uint64_t.
		 *								You can, however, also use arbitrary data types. Please note
		 *								that in this case the FIRST member is filled with the query
		 *								result, always.
		 *	@tparam	N					How many query values shall be retrieved
		 *	@param	aFirstQueryIndex	Query index of the FIRST result to be written to.
		 *								This one and the N-1 subsequent indices are affected.
		 *	@param	aFlags				Additional flags. If T has the size of an 64-bit integer, the
		 *								flag vk::QueryResultFlagBits::e64 will automatically be added.
		 */
		template <typename T, size_t N>
		std::array<T, N> get_results(uint32_t aFirstQueryIndex, vk::QueryResultFlags aFlags)
		{
			std::array<T, N> results;

			if (sizeof(T) == sizeof(uint64_t)) {
				aFlags |= vk::QueryResultFlagBits::e64;
			}

			//assert (create_info().queryType != vk::QueryType::eTimestamp || !has_flag(aFlags, vk::QueryResultFlagBits::ePartial));
			
			mQueryPool.getOwner().getQueryPoolResults(
				mQueryPool.get(), 
				aFirstQueryIndex, static_cast<uint32_t>(N),
				sizeof(results), results.data(), sizeof(T),
				aFlags
			);

			return results;
		}

		/**	Get the result of one query
		 *
		 *	@tparam T					The data receiving the result.
		 *								In most cases, you'll want to use either uint32_t or uint64_t.
		 *								You can, however, also use arbitrary data types. Please note
		 *								that in this case the FIRST member is filled with the query
		 *								result, always.
		 *	@param	aOnlyQueryIndex		Which query of the pool shall the result be written to
		 *	@param	aFlags				Additional flags. If T has the size of an 64-bit integer, the
		 *								flag vk::QueryResultFlagBits::e64 will automatically be added.
		 */
		template <typename T>
		T get_result(uint32_t aOnlyQueryIndex, vk::QueryResultFlags aFlags)
		{
			return get_results<T, 1>(aOnlyQueryIndex, aFlags)[0];
		}

		/**	Copies the values of multiple queries into the given buffer.
		 *	The user must ensure proper lifetime handling of the buffer.
		 *
		 *	@param	aFirstQueryIndex	Query index of the FIRST result to be copied into the buffer.
		 *	@param	aNumQueries			Number queries that shall be copied into the buffer, namely aNumQueries-1 many after the first one.
		 *	@param	aBuffer				The destination buffer where to copy the values into.
		 *								Note: The buffer MUST have an avk::query_results_buffer_meta meta data
		 *								      AND also a member description with content type query_result.
		 *	@param	aBufferMetaSkip		Optional: Skip this many meta data entries of type avk::query_results_buffer_meta (if the buffer has multiple).
		 *	@param	aFlags				Additional flags. Make sure to pass vk::QueryResultFlagBits::e64 if the query
		 *								data shall be stored into a 64-bit integer.
		 *	@param	aSync				How to submit the instructions
		 */
		std::optional<command_buffer> copy_results(uint32_t aFirstQueryIndex, uint32_t aNumQueries, buffer_t& aBuffer, size_t aBufferMetaSkip, vk::QueryResultFlags aFlags, sync aSync);
		
		/**	Copies the value of one single queries into the given buffer.
		 *	The user must ensure proper lifetime handling of the buffer.
		 *
		 *	@param	aOnlyQueryIndex		Query index of the ONLY result to be copied into the buffer.
		 *	@param	aBuffer				The destination buffer where to copy the value into.
		 *								Note: The buffer MUST have an avk::query_results_buffer_meta meta data
		 *								      AND also a member description with content type query_result.
		 *	@param	aBufferMetaSkip		Optional: Skip this many meta data entries of type avk::query_results_buffer_meta (if the buffer has multiple).
		 *	@param	aFlags				Additional flags. Make sure to pass vk::QueryResultFlagBits::e64 if the query
		 *								data shall be stored into a 64-bit integer.
		 *	@param	aSync				How to submit the instructions
		 */
		std::optional<command_buffer> copy_result(uint32_t aOnlyQueryIndex, buffer_t& aBuffer, size_t aBufferMetaSkip, vk::QueryResultFlags aFlags, sync aSync);
		
	private:
		vk::QueryPoolCreateInfo mCreateInfo;
		vk::UniqueQueryPool mQueryPool;
	};

	using query_pool = owning_resource<query_pool_t>;
	
}
