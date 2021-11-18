#pragma once
#include<compare>
#include <avk/avk.hpp>

namespace avk
{
	/**	Binding information about buffers that provide vertex input to graphics pipelines.
	 *	This refers to both, per-vertex input buffers and per-instance input buffers.
	 */
	struct vertex_input_buffer_binding
	{
		enum struct kind { vertex, instance };

		uint32_t mBinding = 0;
		size_t mStride = 0; // TODO: Not sure if this stride is used properly
		kind mKind;
	};

	inline bool operator ==(const vertex_input_buffer_binding& left, const vertex_input_buffer_binding& right)
	{
		return left.mBinding == right.mBinding && left.mStride == right.mStride && left.mKind == right.mKind;
	}

	inline bool operator <(const vertex_input_buffer_binding& left, const vertex_input_buffer_binding& right)
	{
		return left.mBinding < right.mBinding 
			|| (left.mBinding == right.mBinding && (left.mStride < right.mStride 
												|| (left.mStride == right.mStride && left.mKind == vertex_input_buffer_binding::kind::vertex && right.mKind == vertex_input_buffer_binding::kind::instance)
				));
	}

	inline auto operator <=>(const vertex_input_buffer_binding& left, const vertex_input_buffer_binding& right)
	{
		if (left == right) return std::strong_ordering::equal;
		if (left < right) return std::strong_ordering::less;
		return std::strong_ordering::greater;
	}

	/** Data about one specific input location description to a graphics pipeline.
	 *	Or put differently: Which buffer is used to stream vertex/instance input to which shader location.
	 */
	struct input_binding_to_location_mapping
	{
		vertex_input_buffer_binding mGeneralData;
		buffer_element_member_meta mMemberMetaData;
		uint32_t mLocation;
	};

	/**	Describes the input to a graphics pipeline
	 */
	class input_description
	{
		// The buffers at the binding locations 0, 1, ...
		// Each one of those buffer-metas contains the separate locations as childs.
		using input_buffer_meta_t = std::variant<std::monostate, vertex_buffer_meta, instance_buffer_meta>;

	public:
		/** Create a complete input description record based on multiple `input_binding_location_data` records. */
		static input_description declare(std::initializer_list<input_binding_to_location_mapping> aBindings);

	private:
		// Contains all the data, ordered by the binding ids
		// (and internally ordered by locations)
		std::map<uint32_t, input_buffer_meta_t> mInputBuffers;
	};

	/** A helper struct which is used to create a fully configured input_binding_to_location_mapping struct */
	struct partial_input_binding_to_location_mapping
	{
		/**	Operator -> for syntactic sugar.
		 *	You'll have to invoke either `stream_per_vertex` or `stream_per_instance`,
		 *	and you must invoke `to_location` at the end.
		 */
		partial_input_binding_to_location_mapping* operator->()			{ return this; }

		
		/** Describe a certain vertex input for a graphics pipeline by manually specifying offset, format and stride.
		 *	@param	aOffset		Offset to the first element 
		 *	@param	aFormat		Format of the element
		 *	@param	aStride		Stride between subsequent elements
		 *	
		 *	Usage example:
		 *	```
		 *  from_buffer_binding(13) -> stream_per_vertex(0, vk::Format::eR32G32Sfloat, sizeof(std::array<float,4>)) -> to_location(2)
		 *	```
		 */
		partial_input_binding_to_location_mapping& stream_per_vertex(size_t aOffset, vk::Format aFormat, size_t aStride)
		{
			if (!mGeneralData.has_value()) {
				throw avk::logic_error("You must initiate the creation of vertex input binding with `from_buffer_binding(...)` and call ` -> stream_per_instance(...)` subsequently.");
			}
			
			// mBinding should already have been set in from_buffer_binding()
			mGeneralData.value().mStride = aStride;
			mGeneralData.value().mKind = vertex_input_buffer_binding::kind::vertex;

			if (mMemberMetaData.has_value()) {
				AVK_LOG_WARNING("Member meta data is already set. This is probably because stream_per_instance or stream_per_vertex has been called previously. The existing data will be overwritten.");
			}
			else {
				mMemberMetaData = buffer_element_member_meta{};
			}
			
			// No info about mLocation so far
			mMemberMetaData.value().mOffset = aOffset;
			mMemberMetaData.value().mFormat = aFormat;
			
			return *this;
		}

		/** Describe a certain vertex input for a graphics pipeline.
		 *	Read offset, format, and stride from given meta data and member description.
		 *	@param	aBufferMeta			Meta data from a buffer.
		 *	@param	aMemberDescription	Member description from a buffer's meta data
		 *	
		 *	Usage example:
		 *	```
		 *  from_buffer_binding(13) -> stream_per_vertex(mVertexBuffer.meta<instance_buffer_meta>(), mVertexBuffer.member_description(content_description::position)) -> to_location(2)
		 *	```
		 */
		partial_input_binding_to_location_mapping& stream_per_vertex(const buffer_meta& aBufferMeta, const buffer_element_member_meta& aMemberDescription)
		{
			return stream_per_vertex(aMemberDescription.mOffset, aMemberDescription.mFormat, aBufferMeta.sizeof_one_element());
		}

		/** Describe a certain vertex input for a graphics pipeline.
		 *	Read offset, format, and stride from a buffer's meta data, which is assigned to
		 *	a member of the given type.
		 *	@param	aBuffer				The buffer to read `instance_buffer_meta` from.
		 *	@param	aMember				The content description of a member which MUST be included in the `instance_buffer_meta`'s member descriptions.
		 *	@param	aBufferMetaSkip		If aBuffer contains multiple `instance_buffer_meta`, which one shall be used (skipping aBufferMetaSkip-times)
		 *	
		 *	Usage example:
		 *	```
		 *  from_buffer_binding(13) -> stream_per_vertex(mVertexBuffer, content_description::position) -> to_location(2)
		 *	```
		 */
		partial_input_binding_to_location_mapping& stream_per_vertex(resource_reference<const buffer_t> aBuffer, content_description aMember, size_t aBufferMetaSkip = 0)
		{
			assert(aBuffer->has_meta<vertex_buffer_meta>(aBufferMetaSkip));
			auto& metaData = aBuffer->meta<vertex_buffer_meta>(aBufferMetaSkip);
			auto& member = metaData.member_description(aMember);
			return stream_per_vertex(member.mOffset, member.mFormat, metaData.sizeof_one_element());
		}

		/** Describe a certain vertex input for a graphics pipeline where the buffer
		 *	data is assumed to consist of one member only which is tightly packed (i.e.
		 *	stride == sizeof(M), i.e. one element after the other with no space in between)
		 *	
		 *	Usage example:
		 *	```
		 *  from_buffer_binding(13) -> stream_per_vertex<std::array<double,3>>() -> to_location(2)
		 *	```
		 */
		template <class M>
		partial_input_binding_to_location_mapping& stream_per_vertex()
		{
			return stream_per_vertex(0, format_for<M>(), sizeof(M));
		}

		/** Describe a certain vertex input for a graphics pipeline where the buffer
		 *	data is assumed to consist of one member only which is tightly packed (i.e.
		 *	stride == sizeof(M), i.e. one element after the other with no space in between)
		 *	
		 *	Usage example:
		 *	```
		 *  from_buffer_binding(13) -> stream_per_vertex(mVertexData[0]) -> to_location(2)
		 *	```
		 */
		template <class M>
		partial_input_binding_to_location_mapping& stream_per_vertex(const M&)
		{
			return stream_per_vertex(0, format_for<M>(), sizeof(M));
		}

	#if defined(_MSC_VER) && defined(__cplusplus)
		/** Describe a certain vertex input for a graphics pipeline.
		 *	Let the compiler figure out offset, format, and stride.
		 *	
		 *	Usage example:
		 *	```
		 *  from_buffer_binding(13) -> stream_per_vertex(&Vertex::position) -> to_location(2)
		 *	```
		 */
		template <class T, class M> 
		partial_input_binding_to_location_mapping& stream_per_vertex(M T::* aMember)
		{
			return stream_per_vertex(
				// ReSharper disable CppCStyleCast
				((::size_t)&reinterpret_cast<char const volatile&>((((T*)0)->*aMember))),
				// ReSharper restore CppCStyleCast
				format_for<M>(),
				sizeof(T));
		}
	#endif


		/** Describe a certain instance input for a graphics pipeline by manually specifying offset, format and stride.
		 *	@param	aOffset		Offset to the first element 
		 *	@param	aFormat		Format of the element
		 *	@param	aStride		Stride between subsequent elements
		 *	
		 *	Usage example:
		 *	```
		 *  from_buffer_binding(13) -> stream_per_instance(0, vk::Format::eR32G32Sfloat, sizeof(std::array<float,4>)) -> to_location(2)
		 *	```
		 */
		partial_input_binding_to_location_mapping& stream_per_instance(size_t aOffset, vk::Format aFormat, size_t aStride)
		{
			if (!mGeneralData.has_value()) {
				throw avk::logic_error("You must initiate the creation of vertex input binding with `from_buffer_binding(...)` and call ` -> stream_per_instance(...)` subsequently.");
			}
			
			// mBinding should already have been set in from_buffer_binding()
			mGeneralData.value().mStride = aStride;
			mGeneralData.value().mKind = vertex_input_buffer_binding::kind::instance;

			if (mMemberMetaData.has_value()) {
				AVK_LOG_WARNING("Member meta data is already set. This is probably because stream_per_instance or stream_per_vertex has been called previously. The existing data will be overwritten.");
			}
			else {
				mMemberMetaData = buffer_element_member_meta{};
			}
			
			// No info about mLocation so far
			mMemberMetaData.value().mOffset = aOffset;
			mMemberMetaData.value().mFormat = aFormat;

			return *this;
		}

		/** Describe a certain instance input for a graphics pipeline.
		 *	Read offset, format, and stride from given meta data and member description.
		 *	@param	aBufferMeta			Meta data from a buffer.
		 *	@param	aMemberDescription	Member description from a buffer's meta data
		 *	
		 *	Usage example:
		 *	```
		 *  from_buffer_binding(13) -> stream_per_instance(mVertexBuffer.meta<instance_buffer_meta>(), mVertexBuffer.member_description(content_description::position)) -> to_location(2)
		 *	```
		 */
		partial_input_binding_to_location_mapping& stream_per_instance(const buffer_meta& aBufferMeta, const buffer_element_member_meta& aMemberDescription)
		{
			return stream_per_instance(aMemberDescription.mOffset, aMemberDescription.mFormat, aBufferMeta.sizeof_one_element());
		}

		/** Describe a certain instance input for a graphics pipeline.
		 *	Read offset, format, and stride from a buffer's meta data, which is assigned to
		 *	a member of the given type.
		 *	@param	aBuffer				The buffer to read `instance_buffer_meta` from.
		 *	@param	aMember				The content description of a member which MUST be included in the `instance_buffer_meta`'s member descriptions.
		 *	@param	aBufferMetaSkip		If aBuffer contains multiple `instance_buffer_meta`, which one shall be used (skipping aBufferMetaSkip-times)
		 *	
		 *	Usage example:
		 *	```
		 *  from_buffer_binding(13) -> stream_per_instance(mVertexBuffer, content_description::position) -> to_location(2)
		 *	```
		 */
		partial_input_binding_to_location_mapping& stream_per_instance(resource_reference<const buffer_t> aBuffer, content_description aMember, size_t aBufferMetaSkip = 0)
		{
			assert(aBuffer->has_meta<instance_buffer_meta>(aBufferMetaSkip));
			auto& metaData = aBuffer->meta<instance_buffer_meta>(aBufferMetaSkip);
			auto& member = metaData.member_description(aMember);
			return stream_per_instance(member.mOffset, member.mFormat, metaData.sizeof_one_element());
		}

		/** Describe a certain instance input for a graphics pipeline where the buffer
		 *	data is assumed to consist of one member only which is tightly packed (i.e.
		 *	stride == sizeof(M), i.e. one element after the other with no space in between)
		 *	
		 *	Usage example:
		 *	```
		 *  from_buffer_binding(13) -> stream_per_instance<std::array<double,3>>() -> to_location(2)
		 *	```
		 */
		template <class M>
		partial_input_binding_to_location_mapping& stream_per_instance()
		{
			return stream_per_instance(0, format_for<M>(), sizeof(M));
		}

		/** Describe a certain instance input for a graphics pipeline where the buffer
		 *	data is assumed to consist of one member only which is tightly packed (i.e.
		 *	stride == sizeof(M), i.e. one element after the other with no space in between)
		 *	
		 *	Usage example:
		 *	```
		 *  from_buffer_binding(13) -> stream_per_instance(mVertexData[0]) -> to_location(2)
		 *	```
		 */
		template <class M>
		partial_input_binding_to_location_mapping& stream_per_instance(const M&)
		{
			return stream_per_instance(0, format_for<M>(), sizeof(M));
		}

	#if defined(_MSC_VER) && defined(__cplusplus)
		/** Describe a certain instance input for a graphics pipeline.
		 *	Let the compiler figure out offset, format, and stride.
		 *	
		 *	Usage example:
		 *	```
		 *  from_buffer_binding(13) -> stream_per_instance(&Vertex::position) -> to_location(2)
		 *	```
		 */
		template <class T, class M> 
		partial_input_binding_to_location_mapping& stream_per_instance(M T::* aMember)
		{
			return stream_per_instance(
				// ReSharper disable CppCStyleCast
				((::size_t)&reinterpret_cast<char const volatile&>((((T*)0)->*aMember))),
				// ReSharper restore CppCStyleCast
				format_for<M>(),
				sizeof(T));
		}
	#endif

		/** State the vertex input layout location as used in shaders.
		 *	I.e. the location that corresponds to GLSL's `layout(location = 0) in` declarations.
		 */
		input_binding_to_location_mapping to_location(uint32_t aLocation) const
		{
			if (!mGeneralData.has_value() || !mMemberMetaData.has_value()) {
				throw avk::logic_error("Vertex input binding has not been configured completely. Have you invoked ` -> stream_per_vertex(...)` or ` -> stream_per_instance(...)` before invoking ` -> to_location(...)`?");
			}
			return input_binding_to_location_mapping { mGeneralData.value(), mMemberMetaData.value(), aLocation };
		}

		std::optional<vertex_input_buffer_binding> mGeneralData;
		std::optional<buffer_element_member_meta> mMemberMetaData;
	};

	/**	Use this function as a starting point for vertex input binding descriptions. Always!
	 *	Please note that in order to create a complete vertex input description, you'll HAVE TO:
	 *	- either call `stream_per_vertex` or `stream_per_instance`
	 *	- and, finally, `to_location`!
	 *
	 *	Example usage:
	 *	``` 
	 *	from_buffer_binding(13) -> stream_per_vertex<std::array<double,3>>() -> to_location(2)
	 *	```
	 */
	inline partial_input_binding_to_location_mapping from_buffer_binding(uint32_t aBindingIndex)
	{
		partial_input_binding_to_location_mapping result;
		result.mGeneralData = vertex_input_buffer_binding{};
		result.mGeneralData.value().mBinding = aBindingIndex;
		return result;
	}
}
