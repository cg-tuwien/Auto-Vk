#pragma once
#include <avk/avk.hpp>

namespace avk 
{
	class renderpass_t;
	class command_pool_t;
	class image_t;
	class graphics_pipeline_t;
	class compute_pipeline_t;
	class ray_tracing_pipeline_t;
	class set_of_descriptor_set_layouts;
	class framebuffer_t;
	struct binding_data;

	enum struct command_buffer_state
	{
		none,
		recording,
		finished_recording,
		submitted
	};

	struct using_raygen_group_at_index
	{
		using_raygen_group_at_index(uint32_t aGroupIndex) : mGroupIndex{ aGroupIndex } {}
		uint32_t mGroupIndex;
	};

	struct using_miss_group_at_index
	{
		using_miss_group_at_index(uint32_t aGroupIndex) : mGroupIndex{ aGroupIndex } {}
		uint32_t mGroupIndex;
	};

	struct using_hit_group_at_index
	{
		using_hit_group_at_index(uint32_t aGroupIndex) : mGroupIndex{ aGroupIndex } {}
		uint32_t mGroupIndex;
	};

	struct using_callable_group_at_index
	{
		using_callable_group_at_index(uint32_t aGroupIndex) : mGroupIndex{ aGroupIndex } {}
		uint32_t mGroupIndex;
	};

	/** A command buffer which has been created for a certain queue family */
	class command_buffer_t
	{
		friend class root;
		friend class queue;
		friend class command_pool_t;
		
	public:
		command_buffer_t() = default;
		command_buffer_t(command_buffer_t&&) noexcept = default;
		command_buffer_t(const command_buffer_t&) = delete;
		command_buffer_t& operator=(command_buffer_t&&) noexcept = default;
		command_buffer_t& operator=(const command_buffer_t&) = delete;
		~command_buffer_t();

		/** Set a custom deleter function.
		 *	This is often used for resource cleanup, e.g. a buffer which can be deleted when this command buffer is destroyed.
		 */
		template <typename F>
		command_buffer_t& set_custom_deleter(F&& aDeleter) noexcept
		{
			if (mCustomDeleter.has_value()) {
				// There is already a custom deleter! Make sure that this stays alive as well.
				mCustomDeleter = [
					existingDeleter = std::move(mCustomDeleter.value()),
					additionalDeleter = std::forward<F>(aDeleter)
				]() {
					// Invoke in inverse order of addition:
					additionalDeleter();
					existingDeleter();
				};
			}
			else {
				mCustomDeleter = std::forward<F>(aDeleter);
			}
			return *this;
		}

		/** Set a post execution handler function.
		 *	This is (among possible other use cases) used for keeping the C++-side of things in sync with the GPU-side,
		 *	e.g., to update image layout transitions after command buffers with renderpasses have been submitted.
		 */
		template <typename F>
		command_buffer_t& set_post_execution_handler(F&& aHandler) noexcept
		{
			if (mPostExecutionHandler.has_value()) {
				// There is already a custom deleter! Make sure that this stays alive as well.
				mPostExecutionHandler = [
					existingHandler = std::move(mPostExecutionHandler.value()),
					additionalHandler = std::forward<F>(aHandler)
				]() {
					// Invoke IN addition order:
					existingHandler();
					additionalHandler();
				};
			}
			else {
				mPostExecutionHandler = std::forward<F>(aHandler);
			}
			return *this;
		}

		void invoke_post_execution_handler() const;

		void begin_recording();
		void end_recording();
		void begin_render_pass_for_framebuffer(const renderpass_t& aRenderpass, framebuffer_t& aFramebuffer, vk::Offset2D aRenderAreaOffset = {0, 0}, std::optional<vk::Extent2D> aRenderAreaExtent = {}, bool aSubpassesInline = true);
		void next_subpass();
		void establish_execution_barrier(pipeline_stage aSrcStage, pipeline_stage aDstStage);
		void establish_global_memory_barrier(pipeline_stage aSrcStage, pipeline_stage aDstStage, std::optional<memory_access> aSrcAccessToBeMadeAvailable, std::optional<memory_access> aDstAccessToBeMadeVisible);
		void establish_global_memory_barrier_rw(pipeline_stage aSrcStage, pipeline_stage aDstStage, std::optional<write_memory_access> aSrcAccessToBeMadeAvailable, std::optional<read_memory_access> aDstAccessToBeMadeVisible);
		void establish_image_memory_barrier(image_t& aImage, pipeline_stage aSrcStage, pipeline_stage aDstStage, std::optional<memory_access> aSrcAccessToBeMadeAvailable, std::optional<memory_access> aDstAccessToBeMadeVisible);
		void establish_image_memory_barrier_rw(image_t& aImage, pipeline_stage aSrcStage, pipeline_stage aDstStage, std::optional<write_memory_access> aSrcAccessToBeMadeAvailable, std::optional<read_memory_access> aDstAccessToBeMadeVisible);
		void copy_image(const image_t& aSource, const vk::Image& aDestination);
		void end_render_pass();

		/**	Draw vertices with vertex buffer bindings starting at BUFFER-BINDING #0 top to the number of total buffers passed -1.
		 *	"BUFFER-BINDING" means that it corresponds to the binding specified in `input_binding_location_data::from_buffer_at_binding`.
		 *	There can be no gaps between buffer bindings.
		 *	@param	aVertexBuffer		There must be at least one vertex buffer, the meta data of which will be used
		 *								to get the number of vertices to draw.
		 *	@param	aNumberOfInstances	Number of instances to draw
		 *	@param	aFirstVertex		Offset to the first vertex
		 *	@param	aFirstInstance		The ID of the first instance
		 *	@param	aFurtherBuffers		And optionally, there can be further vertex buffers.
		 */
		template <typename... Bfrs>
		void draw_vertices(uint32_t aNumberOfInstances, uint32_t aFirstVertex, uint32_t aFirstInstance, const buffer_t& aVertexBuffer, const Bfrs&... aFurtherBuffers)
		{
			handle().bindVertexBuffers(0u, { aVertexBuffer.buffer_handle(), aFurtherBuffers.buffer_handle() ... }, { vk::DeviceSize{0}, ((void)aFurtherBuffers, vk::DeviceSize{0}) ... });
			//																									Make use of the discarding behavior of the comma operator ^, see: https://stackoverflow.com/a/61098748/387023
			const auto& vertexMeta = aVertexBuffer.template meta<avk::vertex_buffer_meta>();
			handle().draw(vertexMeta.num_elements(), aNumberOfInstances, aFirstVertex, aFirstInstance);                      
		}

		/**	Draw vertices with vertex buffer bindings starting at BUFFER-BINDING #0 top to the number of total buffers passed -1.
		 *	"BUFFER-BINDING" means that it corresponds to the binding specified in `input_binding_location_data::from_buffer_at_binding`.
		 *	There can be no gaps between buffer bindings.
		 *	Number of instances is set to 1.
		 *	Offset to the first vertex is set to 0.
		 *	The ID of the first instance is set to 0.
		 *	@param	aVertexBuffer		There must be at least one vertex buffer, the meta data of which will be used
		 *								to get the number of vertices to draw.
		 *	@param	aFurtherBuffers		And optionally, there can be further vertex buffers.
		 */
		template <typename... Bfrs>
		void draw_vertices(const buffer_t& aVertexBuffer, const Bfrs&... aFurtherBuffers)
		{
			draw_vertices(1u, 0u, 0u, aVertexBuffer, aFurtherBuffers...);
		}

		/**	Perform an indexed draw call with vertex buffer bindings starting at BUFFER-BINDING #0 top to the number of total vertex buffers passed -1.
		 *	"BUFFER-BINDING" means that it corresponds to the binding specified in `input_binding_location_data::from_buffer_at_binding`.
		 *	There can be no gaps between buffer bindings.
		 *	@param	aIndexBuffer		Reference to an index buffer
		 *	@param	aNumberOfInstances	Number of instances to draw
		 *	@param	aFirstIndex			Offset to the first index
		 *	@param	aVertexOffset		Offset to the first vertex
		 *	@param	aFirstInstance		The ID of the first instance
		 *	@param	aVertexBuffers		References to one or multiple vertex buffers
		 */
		template <typename... Bfrs>
		void draw_indexed(const buffer_t& aIndexBuffer, uint32_t aNumberOfInstances, uint32_t aFirstIndex, uint32_t aVertexOffset, uint32_t aFirstInstance, const Bfrs&... aVertexBuffers)
		{
			handle().bindVertexBuffers(0u, { aVertexBuffers.buffer_handle() ... }, { ((void)aVertexBuffers, vk::DeviceSize{0}) ... }); // TODO: Support offsets?!
			//						            Make use of the discarding behavior of the comma operator ^ see: https://stackoverflow.com/a/61098748/387023

			const auto& indexMeta = aIndexBuffer.template meta<avk::index_buffer_meta>();
			vk::IndexType indexType;
			switch (indexMeta.sizeof_one_element()) {
				case sizeof(uint16_t): indexType = vk::IndexType::eUint16; break;
				case sizeof(uint32_t): indexType = vk::IndexType::eUint32; break;
				default: AVK_LOG_ERROR("The given size[" + std::to_string(indexMeta.sizeof_one_element()) + "] does not correspond to a valid vk::IndexType"); break;
			}
			
			handle().bindIndexBuffer(aIndexBuffer.buffer_handle(), 0u, indexType);
			handle().drawIndexed(indexMeta.num_elements(), aNumberOfInstances, aFirstIndex, aVertexOffset, aFirstInstance);
		}
		
		/**	Perform an indexed draw call with vertex buffer bindings starting at BUFFER-BINDING #0 top to the number of total vertex buffers passed -1.
		 *	"BUFFER-BINDING" means that it corresponds to the binding specified in `input_binding_location_data::from_buffer_at_binding`.
		 *	There can be no gaps between buffer bindings.
		 *	Number of instances is set to 1.
		 *	The first index is set to 0.
		 *	The vertex offset is set to 0.
		 *	The ID of the first instance is set to 0.
		 *	@param	aIndexBuffer		Reference to an index buffer
		 *	@param	aVertexBuffers		References to one or multiple vertex buffers
		 */
		template <typename... Bfrs>
		void draw_indexed(const buffer_t& aIndexBuffer, const Bfrs&... aVertexBuffers)
		{
			draw_indexed(aIndexBuffer, 1u, 0u, 0u, 0u, aVertexBuffers ...);
		}
		
		auto& begin_info() const { return mBeginInfo; }
		auto& handle() const { return mCommandBuffer.get(); }
		auto* handle_ptr() const { return &mCommandBuffer.get(); }
		auto state() const { return mState; }

		// Template specializations are implemented in the respective pipeline's header files
		template <typename T> // Expected to be just the pipeline's type
		void bind_pipeline(const T& aPipeline)
		{
			assert(false);
			throw avk::logic_error("No suitable bind_pipeline overload found for the given argument.");
		}

		void bind_descriptors(vk::PipelineBindPoint aBindingPoint, vk::PipelineLayout aLayoutHandle, std::vector<descriptor_set> aDescriptorSets);

		// Template specializations are implemented in the respective pipeline's header files
		template <typename T> 
		void bind_descriptors(T aPipelineLayoutTuple, std::vector<descriptor_set> aDescriptorSets)
		{
			// TODO: In the current state, we're relying on COMPATIBLE layouts. Think about reusing the pipeline's allocated and internally stored layouts!
			assert(false);
			throw avk::logic_error("No suitable bind_descriptors overload found for the given pipeline/layout.");
		}

		// Template specializations are implemented in the respective pipeline's header files
		template <typename T, typename D> 
		void push_constants(T aPipelineLayoutTuple, const D& aData)
		{
			auto pcRanges = std::get<const std::vector<vk::PushConstantRange>*>(aPipelineLayoutTuple);
			auto layoutHandle = std::get<const vk::PipelineLayout>(aPipelineLayoutTuple);
			auto dataSize = static_cast<uint32_t>(sizeof(aData));
			for (auto& r : *pcRanges) {
				if (r.size == dataSize) {
					handle().pushConstants(
						layoutHandle, 
						r.stageFlags, 
						0, // TODO: How to deal with offset?
						dataSize,
						&aData);
					return;
				}
				// TODO: How to deal with push constants of same size and multiple vk::PushConstantRanges??
			}
			AVK_LOG_WARNING("No vk::PushConstantRange entry found that matches the dataSize[" + std::to_string(dataSize) + "]");
		}

#if VK_HEADER_VERSION >= 135
		/**	Issue a trace rays call.
		 *	@param	aRaygenDimensions			Dimensions of the trace rays call. This can be the extent of a window's backbuffer
		 *	@param	aShaderBindingTableRef		Reference to the shader binding table (SBT) to be used for this trace rays call.
		 *	@param	aDynamicDispatch			vk::DispatchLoaderDynamic to be used for the trace rays call.
		 *	@param	aRaygenSbtRef				Offset, stride, and size about which SBT entries to use for the ray generation shaders.
		 *										The `.buffer` member must be set to `aShaderBindingTableRef.mSbtBufferHandle`.
		 *	@param	aRaymissSbtRef				Offset, stride, and size about which SBT entries to use for the miss shaders.
		 *										The `.buffer` member must be set to `aShaderBindingTableRef.mSbtBufferHandle`.
		 *	@param	aRayhitSbtRef				Offset, stride, and size about which SBT entries to use for the (triangle|procedural) hit groups.
		 *										The `.buffer` member must be set to `aShaderBindingTableRef.mSbtBufferHandle`.
		 *	@param	aCallableSbtRef				Offset, stride, and size about which SBT entries to use for the callable shaders.
		 *										The `.buffer` member must be set to `aShaderBindingTableRef.mSbtBufferHandle`.
		 *
		 *	Hint: You can display information about the shader binding table and its groups via `ray_tracing_pipeline_t::print_shader_binding_table_groups`
		 */
		void trace_rays(
			vk::Extent3D aRaygenDimensions, 
			const shader_binding_table_ref& aShaderBindingTableRef, 
			vk::DispatchLoaderDynamic aDynamicDispatch, 
			const vk::StridedBufferRegionKHR& aRaygenSbtRef = vk::StridedBufferRegionKHR{nullptr, 0, 0, 0},
			const vk::StridedBufferRegionKHR& aRaymissSbtRef = vk::StridedBufferRegionKHR{nullptr, 0, 0, 0},
			const vk::StridedBufferRegionKHR& aRayhitSbtRef = vk::StridedBufferRegionKHR{nullptr, 0, 0, 0},
			const vk::StridedBufferRegionKHR& aCallableSbtRef = vk::StridedBufferRegionKHR{nullptr, 0, 0, 0}
		);

		// End of recursive variadic template handling
		static void add_config(const shader_binding_table_ref& aShaderBindingTableRef, vk::StridedBufferRegionKHR& aRaygen, vk::StridedBufferRegionKHR& aRaymiss, vk::StridedBufferRegionKHR& aRayhit, vk::StridedBufferRegionKHR& aCallable) { /* We're done here. */ }

		// Add a specific config setting to the trace rays call
		template <typename... Ts>
		static void add_config(const shader_binding_table_ref& aShaderBindingTableRef, vk::StridedBufferRegionKHR& aRaygen, vk::StridedBufferRegionKHR& aRaymiss, vk::StridedBufferRegionKHR& aRayhit, vk::StridedBufferRegionKHR& aCallable, const using_raygen_group_at_index& aRaygenGroupAtIndex, const Ts&... args)
		{
			aRaygen.setBuffer(aShaderBindingTableRef.mSbtBufferHandle)
				   .setOffset(aShaderBindingTableRef.mSbtGroupsInfo.get().mRaygenGroupsInfo[aRaygenGroupAtIndex.mGroupIndex].mByteOffset)
				   .setStride(aShaderBindingTableRef.mSbtEntrySize)
				   .setSize(aShaderBindingTableRef.mSbtGroupsInfo.get().mRaygenGroupsInfo[aRaygenGroupAtIndex.mGroupIndex].mNumEntries * aShaderBindingTableRef.mSbtEntrySize);
			add_config(aShaderBindingTableRef, aRaygen, aRaymiss, aRayhit, aCallable, args...);
		}

		// Add a specific config setting to the trace rays call
		template <typename... Ts>
		static void add_config(const shader_binding_table_ref& aShaderBindingTableRef, vk::StridedBufferRegionKHR& aRaygen, vk::StridedBufferRegionKHR& aRaymiss, vk::StridedBufferRegionKHR& aRayhit, vk::StridedBufferRegionKHR& aCallable, const using_miss_group_at_index& aMissGroupAtIndex, const Ts&... args)
		{
			aRaymiss.setBuffer(aShaderBindingTableRef.mSbtBufferHandle)
				   .setOffset(aShaderBindingTableRef.mSbtGroupsInfo.get().mMissGroupsInfo[aMissGroupAtIndex.mGroupIndex].mByteOffset)
				   .setStride(aShaderBindingTableRef.mSbtEntrySize)
				   .setSize(aShaderBindingTableRef.mSbtGroupsInfo.get().mMissGroupsInfo[aMissGroupAtIndex.mGroupIndex].mNumEntries * aShaderBindingTableRef.mSbtEntrySize);
			add_config(aShaderBindingTableRef, aRaygen, aRaymiss, aRayhit, aCallable, args...);
		}

		// Add a specific config setting to the trace rays call
		template <typename... Ts>
		static void add_config(const shader_binding_table_ref& aShaderBindingTableRef, vk::StridedBufferRegionKHR& aRaygen, vk::StridedBufferRegionKHR& aRaymiss, vk::StridedBufferRegionKHR& aRayhit, vk::StridedBufferRegionKHR& aCallable, const using_hit_group_at_index& aHitGroupAtIndex, const Ts&... args)
		{
			aRayhit.setBuffer(aShaderBindingTableRef.mSbtBufferHandle)
				   .setOffset(aShaderBindingTableRef.mSbtGroupsInfo.get().mHitGroupsInfo[aHitGroupAtIndex.mGroupIndex].mByteOffset)
				   .setStride(aShaderBindingTableRef.mSbtEntrySize)
				   .setSize(aShaderBindingTableRef.mSbtGroupsInfo.get().mHitGroupsInfo[aHitGroupAtIndex.mGroupIndex].mNumEntries * aShaderBindingTableRef.mSbtEntrySize);
			add_config(aShaderBindingTableRef, aRaygen, aRaymiss, aRayhit, aCallable, args...);
		}

		// Add a specific config setting to the trace rays call
		template <typename... Ts>
		static void add_config(const shader_binding_table_ref& aShaderBindingTableRef, vk::StridedBufferRegionKHR& aRaygen, vk::StridedBufferRegionKHR& aRaymiss, vk::StridedBufferRegionKHR& aRayhit, vk::StridedBufferRegionKHR& aCallable, const using_callable_group_at_index& aCallableGroupAtIndex, const Ts&... args)
		{
			aCallable.setBuffer(aShaderBindingTableRef.mSbtBufferHandle)
				   .setOffset(aShaderBindingTableRef.mSbtGroupsInfo.get().mCallableGroupsInfo[aCallableGroupAtIndex.mGroupIndex].mByteOffset)
				   .setStride(aShaderBindingTableRef.mSbtEntrySize)
				   .setSize(aShaderBindingTableRef.mSbtGroupsInfo.get().mCallableGroupsInfo[aCallableGroupAtIndex.mGroupIndex].mNumEntries * aShaderBindingTableRef.mSbtEntrySize);
			add_config(aShaderBindingTableRef, aRaygen, aRaymiss, aRayhit, aCallable, args...);
		}

		/**	Issue a trace rays call.
		 *
		 *	First two parameters are mandatory explicitely:
		 *	@param	aRaygenDimensions			Dimensions of the trace rays call. This can be the extent of a window's backbuffer
		 *	@param	aShaderBindingTableRef		Reference to the shader binding table to be used for this trace rays call.
		 *
		 *	Further parameters are mandatory implicitely - i.e. there must be at least some information given about which
		 *	shader binding table entries to be used from the given shader binding table during this trace rays call.
		 *	@param args							Possible values:
		 *											- using_raygen_group_at_index
		 *											- using_miss_group_at_index
		 *											- using_hit_group_at_index
		 *											- using_callable_group_at_index
		 *
		 *	Hint: You can display information about the shader binding table and its groups via `ray_tracing_pipeline_t::print_shader_binding_table_groups`
		 */
		template <typename... Ts>
		void trace_rays(vk::Extent3D aRaygenDimensions, const shader_binding_table_ref& aShaderBindingTableRef, const Ts&... args)
		{
			// 1. GATHER CONFIG
			auto raygen  = vk::StridedBufferRegionKHR{nullptr, 0, 0, 0};
			auto raymiss = vk::StridedBufferRegionKHR{nullptr, 0, 0, 0};
			auto rayhit  = vk::StridedBufferRegionKHR{nullptr, 0, 0, 0};
			auto callable= vk::StridedBufferRegionKHR{nullptr, 0, 0, 0};
			add_config(aShaderBindingTableRef, raygen, raymiss, rayhit, callable, args...);

			// 2. TRACE. RAYS.
			return trace_rays(aRaygenDimensions, aShaderBindingTableRef, aShaderBindingTableRef.mDynamicDispatch, raygen, raymiss, rayhit, callable);
		}
#endif
		
	private:
		command_buffer_state mState;
		vk::CommandBufferBeginInfo mBeginInfo;
		vk::UniqueCommandBuffer mCommandBuffer;
		vk::SubpassContents mSubpassContentsState;
		
		/** A custom deleter function called upon destruction of this command buffer */
		std::optional<avk::unique_function<void()>> mCustomDeleter;

		std::optional<avk::unique_function<void()>> mPostExecutionHandler;

		std::shared_ptr<vk::UniqueCommandPool> mCommandPool;
	};

	// Typedef for a variable representing an owner of a command_buffer
	using command_buffer = avk::owning_resource<command_buffer_t>;
	
}
