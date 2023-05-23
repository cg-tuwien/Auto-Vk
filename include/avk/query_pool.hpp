#pragma once
#include "avk/avk.hpp"

namespace avk
{
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
		avk::command::action_type_command reset(uint32_t aFirstQueryIndex = 0u, std::optional<uint32_t> aNumQueries = {});

		/**	Issues a command to write a timestamp after all current commands
		 *	in a queue have reached the given pipeline stage.
		 *
		 *	@param	aQueryIndex		Which query of the pool shall the timestamp be written to
		 *	@param	aTimestampStage	The stage(s) which all previously submitted commands have
		 *							to reach before the timestamp is written.
		 */
		avk::command::action_type_command write_timestamp(uint32_t aQueryIndex, stage::pipeline_stage_flags_precisely aTimestampStage);

		/**	Issues a command to begin a certain query.
		 *
		 *	@param	aQueryIndex		Which query of the pool shall the result be written to
		 *	@param	aFlags			Additional flags for the instruction
		 */
		avk::command::action_type_command begin_query(uint32_t aQueryIndex = 0u, vk::QueryControlFlags aFlags = {});

		/**	Issues a command to end a certain query.
		 *
		 *	@param	aQueryIndex		Which query of the pool shall the result be written to
		 */
		avk::command::action_type_command end_query(uint32_t aQueryIndex = 0u);

		/**
		 * Calculates the number of values returned per query. 
		 * \param aFlags Flags used for the theoretical query. The flags can influence the number of values returned.
		 * \return Number of values per query from this pool with the given flags.
		 */
		uint32_t num_values_per_query(vk::QueryResultFlags aFlags)
		{
			const auto valueAsSomeKindOfInt = static_cast<vk::QueryPipelineStatisticFlags::MaskType>(mCreateInfo.pipelineStatistics);
			const auto count = avk::bit_count(valueAsSomeKindOfInt);
			const auto numValuesPerQuery = std::max(count, 1u)
				+ avk::has_flag(aFlags, vk::QueryResultFlagBits::eWithAvailability)
				+ avk::has_flag(aFlags, vk::QueryResultFlagBits::eWithStatusKHR);
			return numValuesPerQuery;
		}

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
		template <typename T, uint32_t N>
		std::array<T, N> get_results(uint32_t aFirstQueryIndex, uint32_t aNumQueries, vk::QueryResultFlags aFlags)
		{
			std::array<T, N> results;

			if (sizeof(T) == sizeof(uint64_t)) {
				aFlags |= vk::QueryResultFlagBits::e64;
			}
#ifdef _DEBUG
			const auto numValuesPerQuery = num_values_per_query(aFlags);
			if (numValuesPerQuery * aNumQueries != N) {
				AVK_LOG_ERROR("Wrong array size passed to query_pool::get_results. "
				              "The configuration of aNumQueries[" + std::to_string(aNumQueries) + "] "
					          "and aFlags[" + vk::to_string(aFlags) + "] "
                              "requires a size of " + std::to_string(numValuesPerQuery * aNumQueries) + " "
					          "(which is " + std::to_string(numValuesPerQuery) + " values per query), "
					          "but N[" + std::to_string(N) + "] was passed.");
			}
#endif
			
			auto errorCode = mQueryPool.getOwner().getQueryPoolResults(
				mQueryPool.get(), 
				aFirstQueryIndex, aNumQueries,
				sizeof(results), results.data(), sizeof(T),
				aFlags
			);
			if (vk::Result::eSuccess != errorCode) {
				AVK_LOG_WARNING("getQueryPoolResults returned " + vk::to_string(errorCode));
			}

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
#ifdef _DEBUG
			if (const auto numValuesPerQuery = num_values_per_query(aFlags) > 1) {
				AVK_LOG_ERROR("query_pool::get_result invoked on a pool/with flags which returns more than one value, namely " + std::to_string(numValuesPerQuery) + 
					"In such a case, query_pool::get_result is unsuitable. Use query_pool::get_results instead and pass " + std::to_string(numValuesPerQuery) +
					" for N (for one single query), or N * aNumQueries!");
			}
#endif
			return get_results<T, 1>(aOnlyQueryIndex, aFlags)[0];
		}

		/**	Copies the values of multiple queries into the given buffer.
		 *	The user must ensure proper lifetime handling of the buffer.
		 *
		 *	@param	aFirstQueryIndex	Query index of the FIRST result to be copied into the buffer.
		 *	@param	aNumQueries			Number queries that shall be copied into the buffer, namely aNumQueries-1 many after the first one.
		 *	@param	aBuffer				The destination buffer where to copy the values into.
		 *								Note 1: The buffer MUST have an avk::query_results_buffer_meta meta data
		 *								        AND also a member description with content type query_result.
		 *								Note 2: This buffer parameter does not support lifetime handling, because
		 *								        it would not make any sense to have this buffer fire&forget. 
		 *										Therefore, the user of this function must ensure that the buffer outlives the command execution on the GPU.
		 *	@param	aBufferMetaSkip		Optional: Skip this many meta data entries of type avk::query_results_buffer_meta (if the buffer has multiple).
		 *	@param	aFlags				Additional flags. Make sure to pass vk::QueryResultFlagBits::e64 if the query
		 *								data shall be stored into a 64-bit integer.
		 */
		avk::command::action_type_command copy_results(uint32_t aFirstQueryIndex, uint32_t aNumQueries, const buffer_t& aBuffer, size_t aBufferMetaSkip, vk::QueryResultFlags aFlags);
		
		/**	Copies the value of one single queries into the given buffer.
		 *	The user must ensure proper lifetime handling of the buffer.
		 *
		 *	@param	aOnlyQueryIndex		Query index of the ONLY result to be copied into the buffer.
		 *	@param	aBuffer				The destination buffer where to copy the values into.
		 *								Note 1: The buffer MUST have an avk::query_results_buffer_meta meta data
		 *								        AND also a member description with content type query_result.
		 *								Note 2: This buffer parameter does not support lifetime handling, because
		 *								        it would not make any sense to have this buffer fire&forget.
		 *										Therefore, the user of this function must ensure that the buffer outlives the command execution on the GPU.
		 *	@param	aBufferMetaSkip		Optional: Skip this many meta data entries of type avk::query_results_buffer_meta (if the buffer has multiple).
		 *	@param	aFlags				Additional flags. Make sure to pass vk::QueryResultFlagBits::e64 if the query
		 *								data shall be stored into a 64-bit integer.
		 */
		avk::command::action_type_command copy_result(uint32_t aOnlyQueryIndex, const buffer_t& aBuffer, size_t aBufferMetaSkip, vk::QueryResultFlags aFlags);
		
	private:
		// Create info used for creating this pool
		vk::QueryPoolCreateInfo mCreateInfo;
		// Unique handle to this pool
		vk::UniqueHandle<vk::QueryPool, DISPATCH_LOADER_CORE_TYPE> mQueryPool;
	};

	using query_pool = owning_resource<query_pool_t>;
	
}
