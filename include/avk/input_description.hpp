#pragma once
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
		partial_input_binding_to_location_mapping* operator->()			{ return this; }

		
		/** Describe an input location for a pipeline's vertex input.
		 *	The binding point is set to 0 in this case (opposed to `cgb::vertex_input_binding` where you have to specify it),
		 *	but you'll have to set it to some other value if you are going to use multiple buffers. 
		 *	Suggested usage/example: `cgb::vertex_input_location(0, &Vertex::position).from_buffer_at_binding(0);`
		 *	The binding point represents a specific buffer which provides the data for the location specified.
		 */
		partial_input_binding_to_location_mapping& stream_per_vertex(size_t aOffset, vk::Format aFormat, size_t aStride)
		{
			// mBinding should already have been set in from_buffer_binding()
			mGeneralData.mStride = aStride;
			mGeneralData.mKind = vertex_input_buffer_binding::kind::vertex;

			// No info about mLocation so far
			mMemberMetaData.mOffset = aOffset;
			mMemberMetaData.mFormat = aFormat;

			return *this;
		}

		// TODO: Figure out how to use this best
		template <class M>
		partial_input_binding_to_location_mapping& stream_per_vertex()
		{
			return stream_per_vertex(0, format_for<M>(), sizeof(M));
		}

		// TODO: Figure out how to use this best
		template <class M>
		partial_input_binding_to_location_mapping& stream_per_vertex(const M&)
		{
			return stream_per_vertex(0, format_for<M>(), sizeof(M));
		}

	#if defined(_MSC_VER) && defined(__cplusplus)
		/** Describe an input location for a pipeline's vertex input.
		 *	The binding point is set to 0 in this case (opposed to `cgb::vertex_input_binding` where you have to specify it),
		 *	but you'll have to set it to some other value if you are going to use multiple buffers. 
		 *	Suggested usage/example: `cgb::vertex_input_location(0, &Vertex::position).from_buffer_at_binding(0);`
		 *	The binding point represents a specific buffer which provides the data for the location specified.
		 *	
		 *	Usage example:
		 *	```
		 *  {
		 *		vertex_input_location(0, &Vertex::pos);
		 *		vertex_input_location(1, &Vertex::color);
		 *		vertex_input_location(2, &Vertex::texCoord);
		 *  }
		 *	```
		 *
		 *	Or if all the data comes from different buffers:
		 *	```
		 *  {
		 *		vertex_input_location(0, &Vertex::pos).from_buffer_at_binding(0);
		 *		vertex_input_location(1, &Vertex::color).from_buffer_at_binding(1);
		 *		vertex_input_location(2, &Vertex::texCoord).from_buffer_at_binding(2);
		 *  }
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


		/** Describe an input location for a pipeline's instance input.
		*	The binding point is set to 0 in this case (opposed to `cgb::vertex_input_binding` where you have to specify it),
		 *	but you'll have to set it to some other value if you are going to use multiple buffers. 
		 *	Suggested usage/example: `cgb::vertex_input_location(0, &Vertex::position).from_buffer_at_binding(0);`
		 *	The binding point represents a specific buffer which provides the data for the location specified.
		*/
		partial_input_binding_to_location_mapping& stream_per_instance(size_t aOffset, vk::Format aFormat, size_t aStride)
		{
			// mBinding should already have been set in from_buffer_binding()
			mGeneralData.mStride = aStride;
			mGeneralData.mKind = vertex_input_buffer_binding::kind::instance;

			// No info about mLocation so far
			mMemberMetaData.mOffset = aOffset;
			mMemberMetaData.mFormat = aFormat;

			return *this;
		}

		// TODO: Figure out how to use this best
		template <class M>
		partial_input_binding_to_location_mapping& stream_per_instance()
		{
			return stream_per_instance(0, format_for<M>(), sizeof(M));
		}

		// TODO: Figure out how to use this best
		template <class M>
		partial_input_binding_to_location_mapping& stream_per_instance(const M&)
		{
			return stream_per_instance(0, format_for<M>(), sizeof(M));
		}

	#if defined(_MSC_VER) && defined(__cplusplus)

		/** Describe an input location for a pipeline's instance input.
		*	Also, assign the input location to a specific binding point (first parameter `pBinding`).
		*	The binding point represents a specific buffer which provides the data for the location specified.
		*	
		*	Usage example:
		*	```
		*  {
		*		instance_input_location(0, &SomeInstanceData::color);
		*  }
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
		
		input_binding_to_location_mapping to_location(uint32_t aLocation) const
		{
			return input_binding_to_location_mapping { mGeneralData, mMemberMetaData, aLocation };
		}

		uint32_t mLocation = 0u;
		vertex_input_buffer_binding mGeneralData;
		buffer_element_member_meta mMemberMetaData;
	};

	inline partial_input_binding_to_location_mapping from_buffer_binding(uint32_t aBindingIndex)
	{
		partial_input_binding_to_location_mapping result;
		result.mGeneralData.mBinding = aBindingIndex;
		return result;
	}
}
