#pragma once
#include <avk/avk.hpp>

namespace avk
{
	/** Can be used to describe what kind of data a buffer member represents. */
	enum struct content_description
	{
		unspecified,
		index,
		position,
		normal,
		tangent,
		bitangent,
		color,
		texture_coordinate,
		bone_weight,
		bone_index,
		aabb,
		geometry_instance,
		user_defined_01,
		user_defined_02,
		user_defined_03,
		user_defined_04,
		user_defined_05,
		user_defined_06,
		user_defined_07,
		user_defined_08,
		user_defined_09,
		user_defined_10,
		user_defined_11,
		user_defined_12,
		user_defined_13,
		user_defined_14,
		user_defined_15,
		user_defined_16,
		user_defined_17,
		user_defined_18,
		user_defined_19,
		user_defined_20,
		user_defined_21,
		user_defined_22,
		user_defined_23,
		user_defined_24,
		user_defined_25,
		user_defined_26,
		user_defined_27,
		user_defined_28,
		user_defined_29,
		user_defined_30,
		user_defined_31,
		user_defined_32,
		user_defined_33,
		user_defined_34,
		user_defined_35,
		user_defined_36,
		user_defined_37,
		user_defined_38,
		user_defined_39,
		user_defined_40,
		user_defined_41,
		user_defined_42,
		user_defined_43,
		user_defined_44,
		user_defined_45,
		user_defined_46,
		user_defined_47,
		user_defined_48,
		user_defined_49,
		user_defined_50,
		user_defined_51,
		user_defined_52,
		user_defined_53,
		user_defined_54,
		user_defined_55,
		user_defined_56,
		user_defined_57,
		user_defined_58,
		user_defined_59,
		user_defined_60,
		user_defined_61,
		user_defined_62,
		user_defined_63,
		user_defined_64,
		user_defined_65,
		user_defined_66,
		user_defined_67,
		user_defined_68,
		user_defined_69,
		user_defined_70,
		user_defined_71,
		user_defined_72,
		user_defined_73,
		user_defined_74,
		user_defined_75,
		user_defined_76,
		user_defined_77,
		user_defined_78,
		user_defined_79,
		user_defined_80,
		user_defined_81,
		user_defined_82,
		user_defined_83,
		user_defined_84,
		user_defined_85,
		user_defined_86,
		user_defined_87,
		user_defined_88,
		user_defined_89,
		user_defined_90,
		user_defined_91,
		user_defined_92,
		user_defined_93,
		user_defined_94,
		user_defined_95,
		user_defined_96,
		user_defined_97,
		user_defined_98,
		user_defined_99,
	};

	extern std::string to_string(content_description aValue);

	/** Meta data for a buffer element's member.
	 *	This is used to describe
	 */
	struct buffer_element_member_meta
	{
		size_t mOffset = 0;
		vk::Format mFormat;
		content_description mContent = content_description::unspecified;
	};

	/** Base class for buffer meta data */
	class buffer_meta
	{
	public:
		buffer_meta() = default;
		buffer_meta(const buffer_meta&) = default;
		buffer_meta(buffer_meta&&) noexcept = default;
		buffer_meta& operator=(const buffer_meta&) = default;
		buffer_meta& operator=(buffer_meta&&) noexcept = default;
		virtual ~buffer_meta() = default;
		
		/** The size of one element in the buffer. */
		size_t sizeof_one_element() const { return mSizeOfOneElement; }

		/** The total number of elements in the buffer. */
		size_t num_elements() const { return mNumElements; }

		/** Total size of the data represented by the buffer. */
		size_t total_size() const { return sizeof_one_element() * num_elements(); }

		/** Returns a reference to a collection of all member descriptions */
		const auto& member_descriptions() const { return mOrderedMemberDescriptions; }

		/** Finds the member descritpion of the FIRST member of the given content_description type. */
		const buffer_element_member_meta* find_member_description(content_description aMemberToBeFound) const
		{
			auto it = std::find_if(
				std::begin(member_descriptions()), std::end(member_descriptions()), 
				[aMemberToBeFound](const buffer_element_member_meta& md) {
					return md.mContent == aMemberToBeFound;
				});
			if (std::end(member_descriptions()) == it) {
				return nullptr;
			}
			return &(*it);
		}

		/** Gets the descritpion of the FIRST member of the given content_description type or throws. */
		const buffer_element_member_meta& member_description(content_description aMemberToBeFound) const
		{
			const auto* found = find_member_description(aMemberToBeFound);
			if (nullptr == found) {
				throw avk::runtime_error("No member of the given content type '" + to_string(aMemberToBeFound) + "' present in member descriptions.");
			}
			return *found;
		}

		/** Gets the descriptor type that is suitable for binding a buffer of this kind to shaders. */
		virtual std::optional<vk::DescriptorType> descriptor_type() const { return {}; }

		/** Gets buffer usage flags for this kind of buffer. */
		virtual vk::BufferUsageFlags buffer_usage_flags() const { return vk::BufferUsageFlags{}; }

	protected:
		// The size of one record/element
		size_t mSizeOfOneElement = 0;

		// The total number of records/elements
		size_t mNumElements = 0;

		// Descriptions of buffer record/element members, ordered by their offsets
		std::vector<buffer_element_member_meta> mOrderedMemberDescriptions;	
	};
	
	/** This struct contains information for a generic buffer.
	*/
	class generic_buffer_meta : public buffer_meta
	{
	public:		
		/** Create meta info from the total size of the represented data. */
		static generic_buffer_meta create_from_size(size_t aSize) 
		{ 
			generic_buffer_meta result;
			result.mSizeOfOneElement = aSize;
			result.mNumElements = 1;
			return result; 
		}

		/** Create meta info from a STL-container like data structure or a single struct.
		 *	Container types must provide a `size()` method and the index operator.
		 */
		template <typename T>
		static std::enable_if_t<!std::is_pointer_v<T>, generic_buffer_meta> create_from_data(const T& aData)
		{
			generic_buffer_meta result;
			result.mSizeOfOneElement = sizeof(first_or_only_element(aData));
			result.mNumElements = how_many_elements(aData);
			return result; 
		}
	};

	/** This struct contains information for a uniform buffer.
	*/
	class uniform_buffer_meta : public buffer_meta
	{
	public:
		/** Gets the descriptor type that is suitable for binding a buffer of this kind to shaders. */
		std::optional<vk::DescriptorType> descriptor_type() const override { return vk::DescriptorType::eUniformBuffer; }

		/** Gets buffer usage flags for this kind of buffer. */
		vk::BufferUsageFlags buffer_usage_flags() const override { return vk::BufferUsageFlagBits::eUniformBuffer; }
		
		/** Create meta info from the total size of the represented data. */
		static uniform_buffer_meta create_from_size(size_t aSize) 
		{ 
			uniform_buffer_meta result; 
			result.mSizeOfOneElement = aSize;
			result.mNumElements = 1; 
			return result; 
		}

		/** Create meta info from a STL-container like data structure or a single struct.
		*	Container types must provide a `size()` method and the index operator.
		*/
		template <typename T>
		static std::enable_if_t<!std::is_pointer_v<T>, uniform_buffer_meta> create_from_data(const T& aData)
		{
			uniform_buffer_meta result; 
			result.mSizeOfOneElement = sizeof(first_or_only_element(aData));
			result.mNumElements = how_many_elements(aData);
			return result; 
		}
	};

	/** This struct contains information for a uniform texel buffer.
	*/
	class uniform_texel_buffer_meta : public buffer_meta
	{
	public:
		/** Gets the descriptor type that is suitable for binding a buffer of this kind to shaders. */
		std::optional<vk::DescriptorType> descriptor_type() const override { return vk::DescriptorType::eUniformTexelBuffer; }

		/** Gets buffer usage flags for this kind of buffer. */
		vk::BufferUsageFlags buffer_usage_flags() const override { return vk::BufferUsageFlagBits::eUniformTexelBuffer; }
		
		/** Create meta info from the size of one element and the number of elements. 
		 */
		static uniform_texel_buffer_meta create_from_element_size(size_t aSizeElement, size_t aNumElemaents) 
		{ 
			uniform_texel_buffer_meta result; 
			result.mSizeOfOneElement = aSizeElement;
			result.mNumElements = aNumElemaents;
			return result; 
		}

		/** Create meta info from the total size of all elements. */
		static uniform_texel_buffer_meta create_from_total_size(size_t aTotalSize, size_t aNumElements) 
		{ 
			uniform_texel_buffer_meta result; 
			result.mSizeOfOneElement = aTotalSize / aNumElements;
			result.mNumElements = aNumElements;
			return result; 
		}

		/** Create meta info from a STL-container like data structure or a single struct.
		*	Container types must provide a `size()` method and the index operator.
		*/
		template <typename T>
		static std::enable_if_t<!std::is_pointer_v<T>, uniform_texel_buffer_meta> create_from_data(const T& aData)
		{
			uniform_texel_buffer_meta result; 
			result.mSizeOfOneElement = sizeof(first_or_only_element(aData)); 
			result.mNumElements = how_many_elements(aData);
			return result; 
		}

		/** Describe one member. That is, at which offset it is located, which format it has, and which type of content it represents */
		uniform_texel_buffer_meta& describe_member(size_t aOffset, vk::Format aFormat, content_description aContent = content_description::unspecified)
		{
#if defined(_DEBUG)
			if (std::find_if(std::begin(mOrderedMemberDescriptions), std::end(mOrderedMemberDescriptions), [offs = aOffset](const buffer_element_member_meta& e) { return e.mOffset == offs; }) != mOrderedMemberDescriptions.end()) {
				AVK_LOG_WARNING("There is already a member described at offset " + std::to_string(aOffset));
			}
#endif
			// insert already in the right place
			const buffer_element_member_meta newElement{ aOffset, aFormat, aContent };
			const auto it = std::lower_bound(std::begin(mOrderedMemberDescriptions), std::end(mOrderedMemberDescriptions), newElement,
			                                 [](const buffer_element_member_meta& first, const buffer_element_member_meta& second) -> bool { 
				                                 return first.mOffset < second.mOffset;
			                                 });
			mOrderedMemberDescriptions.insert(it, newElement);
			return *this;
		}

		/** If the texel buffer is not a data structure which contains interleaved data, 
		 *	this method can be used to describe its only data member. It is assumed that
		 *	the only data member has the same size as `mSizeOfOneElement`.
		 *	
		 *	Usage example:
		 *	```
		 *	std::vector<vec3> normals;
		 *	uniform_texel_buffer_meta meta = uniform_texel_buffer_meta::create_from_data(normals)
		 *								.describe_only_member(normals[0], content_description::normal);
		 *	```
		 */
		template <typename M>
		uniform_texel_buffer_meta& describe_only_member(const M& aMember, content_description aContent = content_description::unspecified)
		{
			// Do NOT assert(sizeof(_Member) == mSizeOfOneElement), since for uniform_texel_buffers, this could be okay! 
			// => it means that the buffer view views the data in a different format, that can also combine multiple consecutive elements 
			return describe_member(0, format_for<M>(), aContent);
		}

		/** Describe the texel buffer's format in the most generic way. This method also enables
		 *	to set a format which combines several consecutive elements, but can generally be used
		 *	to just set an arbitrary format.
		 *	
		 *	Usage example:
		 *	```
		 *	std::vector<uint32_t> indices;
		 *	uniform_texel_buffer_meta meta = uniform_texel_buffer_meta::create_from_data(indices)
		 *										.set_format<glm::uvec3>(content_description::index);
		 *	```
		 */
		template <typename T>
		uniform_texel_buffer_meta& set_format(content_description aContent = content_description::unspecified)
		{
			// Do NOT assert(sizeof(_Member) == mSizeOfOneElement), since for uniform_texel_buffers, this could be okay! 
			// => it means that the buffer view views the data in a different format, that can also combine multiple consecutive elements 
			return describe_member(0, format_for<T>(), aContent);
		}

#if defined(_MSC_VER) && defined(__cplusplus)
		/** Describe a member and let the compiler figure out offset and format.
		 *	
		 *	Usage example:
		 *	```
		 *	struct MyData { int mFirst; float mSecond; };
		 *	std::vector<MyData> myData;
		 *	uniform_texel_buffer_meta meta = uniform_texel_buffer_meta::create_from_data(myData)
		 *								.describe_member(&MyData::mFirst,  0, content_description::position)
		 *								.describe_member(&MyData::mSecond, 1, content_description::texture_coordinate);
		 *	```
		 */
		template <class T, class M> 
		uniform_texel_buffer_meta& describe_member(M T::* aMember, content_description aContent = content_description::unspecified)
		{
			return describe_member(
				((::size_t)&reinterpret_cast<char const volatile&>((((T*)0)->*aMember))),
				format_for<M>(),
				aContent);
		}
#endif

	};

	/** This struct contains information for a storage buffer.
	*/
	class storage_buffer_meta : public buffer_meta
	{
	public:
		/** Gets the descriptor type that is suitable for binding a buffer of this kind to shaders. */
		std::optional<vk::DescriptorType> descriptor_type() const override { return vk::DescriptorType::eStorageBuffer; }

		/** Gets buffer usage flags for this kind of buffer. */
		vk::BufferUsageFlags buffer_usage_flags() const override { return vk::BufferUsageFlagBits::eStorageBuffer; }
		
		/** Create meta info from the total size of the represented data. */
		static storage_buffer_meta create_from_size(size_t aSize) 
		{ 
			storage_buffer_meta result; 
			result.mSizeOfOneElement = aSize;
			result.mNumElements = 1; 
			return result; 
		}

		/** Create meta info from a STL-container like data structure or a single struct.
		*	Container types must provide a `size()` method and the index operator.
		*/
		template <typename T>
		static std::enable_if_t<!std::is_pointer_v<T>, storage_buffer_meta> create_from_data(const T& aData)
		{
			storage_buffer_meta result; 
			result.mSizeOfOneElement = sizeof(first_or_only_element(aData));
			result.mNumElements = how_many_elements(aData);
			return result; 
		}
	};

	/** This struct contains information for a storage texel buffer.
	*/
	class storage_texel_buffer_meta : public buffer_meta
	{
	public:
		/** Gets the descriptor type that is suitable for binding a buffer of this kind to shaders. */
		std::optional<vk::DescriptorType> descriptor_type() const override { return vk::DescriptorType::eStorageTexelBuffer; }

		/** Gets buffer usage flags for this kind of buffer. */
		vk::BufferUsageFlags buffer_usage_flags() const override { return vk::BufferUsageFlagBits::eStorageTexelBuffer; }
		
		/** Create meta info from the size of one element and the number of elements. 
		 */
		static storage_texel_buffer_meta create_from_element_size(size_t aSizeElement, size_t aNumElements) 
		{ 
			storage_texel_buffer_meta result; 
			result.mSizeOfOneElement = aSizeElement;
			result.mNumElements = aNumElements;
			return result; 
		}

		/** Create meta info from the total size of all elements. */
		static storage_texel_buffer_meta create_from_total_size(size_t aTotalSize, size_t aNumElements) 
		{ 
			storage_texel_buffer_meta result; 
			result.mSizeOfOneElement = aTotalSize / aNumElements;
			result.mNumElements = aNumElements;
			return result; 
		}

		/** Create meta info from a STL-container like data structure or a single struct.
		*	Container types must provide a `size()` method and the index operator.
		*/
		template <typename T>
		static std::enable_if_t<!std::is_pointer_v<T>, storage_texel_buffer_meta> create_from_data(const T& aData)
		{
			storage_texel_buffer_meta result; 
			result.mSizeOfOneElement = sizeof(first_or_only_element(aData)); 
			result.mNumElements = how_many_elements(aData);
			return result; 
		}

		/** Describe which part of an element's member gets mapped to which shader locaton. */
		storage_texel_buffer_meta& describe_member(size_t aOffset, vk::Format aFormat, content_description aContent = content_description::unspecified)
		{
#if defined(_DEBUG)
			if (std::find_if(std::begin(mOrderedMemberDescriptions), std::end(mOrderedMemberDescriptions), [offs = aOffset](const buffer_element_member_meta& e) { return e.mOffset == offs; }) != mOrderedMemberDescriptions.end()) {
				AVK_LOG_WARNING("There is already a member described at offset " + std::to_string(aOffset));
			}
#endif
			// insert already in the right place
			buffer_element_member_meta newElement{ aOffset, aFormat, aContent };
			auto it = std::lower_bound(std::begin(mOrderedMemberDescriptions), std::end(mOrderedMemberDescriptions), newElement,
				[](const buffer_element_member_meta& first, const buffer_element_member_meta& second) -> bool { 
					return first.mOffset < second.mOffset;
				});
			mOrderedMemberDescriptions.insert(it, newElement);
			return *this;
		}

		/** If the texel buffer is not a data structure which contains interleaved data, 
		 *	this method can be used to describe its only data member. It is assumed that
		 *	the only data member has the same size as `mSizeOfOneElement`.
		 *	
		 *	Usage example:
		 *	```
		 *	std::vector<vec3> normals;
		 *	storage_texel_buffer_meta meta = storage_texel_buffer_meta::create_from_data(normals)
		 *								.describe_only_member(normals[0],  2, content_description::normal);
		 *	```
		 */
		template <typename M>
		storage_texel_buffer_meta& describe_only_member(const M& aMember, content_description aContent = content_description::unspecified)
		{
			assert(sizeof(aMember) == mSizeOfOneElement);
			return describe_member(0, format_for<M>(), aContent);
		}

#if defined(_MSC_VER) && defined(__cplusplus)
		/** Describe a member and let the compiler figure out offset and format.
		 *	
		 *	Usage example:
		 *	```
		 *	struct MyData { int mFirst; float mSecond; };
		 *	std::vector<MyData> myData;
		 *	storage_texel_buffer_meta meta = storage_texel_buffer_meta::create_from_data(myData)
		 *								.describe_member(&MyData::mFirst,  0, content_description::position)
		 *								.describe_member(&MyData::mSecond, 1, content_description::texture_coordinate);
		 *	```
		 */
		template <class T, class M> 
		storage_texel_buffer_meta& describe_member(M T::* aMember, content_description aContent = content_description::unspecified)
		{
			return describe_member(
				((::size_t)&reinterpret_cast<char const volatile&>((((T*)0)->*aMember))),
				format_for<M>(),
				aContent);
		}
#endif
	};

	/**	This struct contains information for a buffer which is intended to be used as 
	*	vertex buffer, i.e. vertex attributes provided to a shader.
	*/
	class vertex_buffer_meta : public buffer_meta
	{
	public:
		/** Gets buffer usage flags for this kind of buffer. */
		vk::BufferUsageFlags buffer_usage_flags() const override { return vk::BufferUsageFlagBits::eVertexBuffer; }
		
		/** Create meta info from the size of one element and the number of elements. 
		 *	It is legal to omit the `pNumElements` parameter for only creating an input description
		 *	(e.g. for describing the pipeline layout)
		 */
		static vertex_buffer_meta create_from_element_size(size_t aSizeElement, size_t aNumElements = 0) 
		{ 
			vertex_buffer_meta result; 
			result.mSizeOfOneElement = aSizeElement;
			result.mNumElements = aNumElements;
			return result; 
		}

		/** Create meta info from the total size of all elements. */
		static vertex_buffer_meta create_from_total_size(size_t aTotalSize, size_t aNumElements) 
		{ 
			vertex_buffer_meta result; 
			result.mSizeOfOneElement = aTotalSize / aNumElements;
			result.mNumElements = aNumElements;
			return result; 
		}

		/** Create meta info from a STL-container like data structure or a single struct.
		*	Container types must provide a `size()` method and the index operator.
		*/
		template <typename T>
		static std::enable_if_t<!std::is_pointer_v<T>, vertex_buffer_meta> create_from_data(const T& aData)
		{
			vertex_buffer_meta result; 
			result.mSizeOfOneElement = sizeof(first_or_only_element(aData)); 
			result.mNumElements = how_many_elements(aData);
			return result; 
		}

		/** Describe which part of an element's member gets mapped to which shader locaton. */
		vertex_buffer_meta& describe_member(size_t aOffset, vk::Format aFormat, content_description aContent = content_description::unspecified)
		{
#if defined(_DEBUG)
			if (std::find_if(std::begin(mOrderedMemberDescriptions), std::end(mOrderedMemberDescriptions), [offs = aOffset](const buffer_element_member_meta& e) { return e.mOffset == offs; }) != mOrderedMemberDescriptions.end()) {
				AVK_LOG_WARNING("There is already a member described at offset " + std::to_string(aOffset));
			}
#endif
			// insert already in the right place
			buffer_element_member_meta newElement{ aOffset, aFormat, aContent };
			auto it = std::lower_bound(std::begin(mOrderedMemberDescriptions), std::end(mOrderedMemberDescriptions), newElement,
				[](const buffer_element_member_meta& first, const buffer_element_member_meta& second) -> bool { 
					return first.mOffset < second.mOffset;
				});
			mOrderedMemberDescriptions.insert(it, newElement);
			return *this;
		}

		/** If the vertex buffer is not a data structure which contains interleaved data, 
		 *	this method can be used to describe its only data member. It is assumed that
		 *	the only data member has the same size as `mSizeOfOneElement`.
		 *	
		 *	Usage example:
		 *	```
		 *	std::vector<vec3> normals;
		 *	vertex_buffer_meta meta = vertex_buffer_meta::create_from_data(normals)
		 *								.describe_only_member(normals[0],  2, content_description::normal);
		 *	```
		 */
		template <typename M>
		vertex_buffer_meta& describe_only_member(const M& aMember, content_description aContent = content_description::unspecified)
		{
			assert(sizeof(aMember) == mSizeOfOneElement);
			return describe_member(0, format_for<M>(), aContent);
		}

#if defined(_MSC_VER) && defined(__cplusplus)
		/** Describe which part of an element's member gets mapped to which shader locaton,
		 *	and let the compiler figure out offset and format.
		 *	
		 *	Usage example:
		 *	```
		 *	struct MyData { int mFirst; float mSecond; };
		 *	std::vector<MyData> myData;
		 *	vertex_buffer_meta meta = vertex_buffer_meta::create_from_data(myData)
		 *								.describe_member(&MyData::mFirst,  0, content_description::position)
		 *								.describe_member(&MyData::mSecond, 1, content_description::texture_coordinate);
		 *	```
		 */
		template <class T, class M> 
		vertex_buffer_meta& describe_member(M T::* aMember, content_description aContent = content_description::unspecified)
		{
			return describe_member(
				((::size_t)&reinterpret_cast<char const volatile&>((((T*)0)->*aMember))),
				format_for<M>(),
				aContent);
		}
#endif
	};

	/**	This struct contains information for a buffer which is intended to be used as 
	*	index buffer.
	*/
	class index_buffer_meta : public buffer_meta
	{
	public:
		/** Gets buffer usage flags for this kind of buffer. */
		vk::BufferUsageFlags buffer_usage_flags() const override { return vk::BufferUsageFlagBits::eIndexBuffer; }
		
		/** Create meta info from the size of one index and the number of elements.  */
		static index_buffer_meta create_from_element_size(size_t aSizeElement, size_t aNumElements = 0) 
		{ 
			index_buffer_meta result; 
			result.mSizeOfOneElement = aSizeElement;
			result.mNumElements = aNumElements;
			return result; 
		}

		/** Create meta info from the total size of all elements. */
		static index_buffer_meta create_from_total_size(size_t aTotalSize, size_t aNumElements) 
		{ 
			index_buffer_meta result; 
			result.mSizeOfOneElement = aTotalSize / aNumElements;
			result.mNumElements = aNumElements;
			return result; 
		}

		/** Create meta info from a STL-container like data structure or a single struct.
		*	Container types must provide a `size()` method and the index operator.
		*/
		template <typename T>
		static std::enable_if_t<!std::is_pointer_v<T>, index_buffer_meta> create_from_data(const T& aData)
		{
			index_buffer_meta result; 
			result.mSizeOfOneElement = sizeof(first_or_only_element(aData)); 
			result.mNumElements = how_many_elements(aData);
			return result; 
		}
	};

	/**	This struct contains information for a buffer which is intended to be used as 
	*	instance buffer, i.e. vertex attributes provided to a shader per instance.
	*/
	struct instance_buffer_meta : public buffer_meta
	{
	public:
		/** Gets buffer usage flags for this kind of buffer. */
		vk::BufferUsageFlags buffer_usage_flags() const override { return vk::BufferUsageFlagBits::eVertexBuffer; }
		
		/** Create meta info from the size of one element and the number of elements. 
		 *	It is legal to omit the `pNumElements` parameter for only creating an input description
		 *	(e.g. for describing the pipeline layout)
		 */
		static instance_buffer_meta create_from_element_size(size_t aSizeElement, size_t aNumElements = 0) 
		{ 
			instance_buffer_meta result; 
			result.mSizeOfOneElement = aSizeElement;
			result.mNumElements = aNumElements;
			return result; 
		}

		/** Create meta info from the total size of all elements. */
		static instance_buffer_meta create_from_total_size(size_t aTotalSize, size_t aNumElements) 
		{ 
			instance_buffer_meta result; 
			result.mSizeOfOneElement = aTotalSize / aNumElements;
			result.mNumElements = aNumElements;
			return result; 
		}

		/** Create meta info from a STL-container like data structure or a single struct.
		*	Container types must provide a `size()` method and the index operator.
		*/
		template <typename T>
		static std::enable_if_t<!std::is_pointer_v<T>, instance_buffer_meta> create_from_data(const T& aData)
		{
			instance_buffer_meta result; 
			result.mSizeOfOneElement = sizeof(first_or_only_element(aData)); 
			result.mNumElements = how_many_elements(aData);
			return result; 
		}

		/** Describe which part of an element's member gets mapped to which shader locaton. */
		instance_buffer_meta& describe_member(size_t aOffset, vk::Format aFormat, content_description aContent = content_description::unspecified)
		{
			// insert already in the right place
			buffer_element_member_meta newElement{ aOffset, aFormat, aContent };
			auto it = std::lower_bound(std::begin(mOrderedMemberDescriptions), std::end(mOrderedMemberDescriptions), newElement,
				[](const buffer_element_member_meta& first, const buffer_element_member_meta& second) -> bool { 
					return first.mOffset < second.mOffset;
				});
			mOrderedMemberDescriptions.insert(it, newElement);
			return *this;
		}

		/** If the vertex buffer is not a data structure which contains interleaved data, 
		 *	this method can be used to describe its only data member. It is assumed that
		 *	the only data member has the same size as `mSizeOfOneElement`.
		 */
		template <typename M>
		instance_buffer_meta& describe_only_member(const M& aMember, content_description aContent = content_description::unspecified)
		{
			assert(sizeof(aMember) == mSizeOfOneElement);
			return describe_member(0, format_for<M>(), aContent);
		}

#if defined(_MSC_VER) && defined(__cplusplus)
		/** Describe which part of an element's member gets mapped to which shader locaton,
		 *	and let the compiler figure out offset and format.
		 */
		template <class T, class M> 
		instance_buffer_meta& describe_member(M T::* aMember, content_description aContent = content_description::unspecified)
		{
			return describe_member(
				((::size_t)&reinterpret_cast<char const volatile&>((((T*)0)->*aMember))),
				format_for<M>(),
				aContent);
		}
#endif
	};

#if VK_HEADER_VERSION >= 135
	/**	This struct contains information for a buffer which is intended to be used as 
	*	geometries buffer for real-time ray tracing, containing AABB data.
	*/
	class aabb_buffer_meta : public buffer_meta
	{
	public:
		/** Gets buffer usage flags for this kind of buffer. */
		vk::BufferUsageFlags buffer_usage_flags() const override { return vk::BufferUsageFlagBits::eRayTracingKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR; }
		
		static aabb_buffer_meta create_from_num_elements(size_t aNumElements, size_t aStride = sizeof(VkAabbPositionsKHR)) 
		{ 
			aabb_buffer_meta result; 
			result.mSizeOfOneElement = aStride;
			result.mNumElements = aNumElements;
			return result; 
		}

		static aabb_buffer_meta create_from_total_size(size_t aTotalSize) 
		{ 
			aabb_buffer_meta result; 
			result.mSizeOfOneElement = sizeof(VkAabbPositionsKHR);
			assert(aTotalSize % result.mSizeOfOneElement == 0);
			result.mNumElements = aTotalSize / result.mSizeOfOneElement;
			return result; 
		}

		template <typename T>
		static std::enable_if_t<!std::is_pointer_v<T>, aabb_buffer_meta> create_from_data(const T& aData)
		{
			aabb_buffer_meta result; 
			result.mSizeOfOneElement = sizeof(first_or_only_element(aData));
			assert(sizeof(first_or_only_element(aData)) == sizeof(VkAabbPositionsKHR));
			result.mNumElements = how_many_elements(aData);
			return result; 
		}

		/** Describe which part of an element's member contains the AABB of type `avk::aabb` */
		aabb_buffer_meta& describe_member(size_t aOffset, content_description aContent = content_description::aabb)
		{
#if defined(_DEBUG)
			if (std::find_if(std::begin(mOrderedMemberDescriptions), std::end(mOrderedMemberDescriptions), [offs = aOffset](const buffer_element_member_meta& e) { return e.mOffset == offs; }) != mOrderedMemberDescriptions.end()) {
				AVK_LOG_WARNING("There is already a member described at offset " + std::to_string(aOffset));
			}
#endif
			// insert already in the right place
			buffer_element_member_meta newElement{ aOffset, vk::Format::eUndefined, aContent };
			auto it = std::lower_bound(std::begin(mOrderedMemberDescriptions), std::end(mOrderedMemberDescriptions), newElement,
				[](const buffer_element_member_meta& first, const buffer_element_member_meta& second) -> bool { 
					return first.mOffset < second.mOffset;
				});
			mOrderedMemberDescriptions.insert(it, newElement);
			return *this;
		}

		/* Describe that there is only one member and that member is the AABB */
		template <typename M>
		aabb_buffer_meta& describe_only_member(const M& aMember, content_description aContent = content_description::aabb)
		{
			assert (sizeof(aMember) == mSizeOfOneElement);
			auto frmt = format_for<M>();
			assert (content_description::aabb == aContent);
			assert (vk::Format::eUndefined == frmt);
			assert (sizeof(VkAabbPositionsKHR) == mSizeOfOneElement);
			return describe_member(0, frmt, aContent);
		}

#if defined(_MSC_VER) && defined(__cplusplus)
		/** Describe a member and let the compiler figure out offset and format. */
		template <class T, class M> 
		aabb_buffer_meta& describe_member(M T::* aMember, content_description aContent = content_description::aabb)
		{
			return describe_member(
				((::size_t)&reinterpret_cast<char const volatile&>((((T*)0)->*aMember))),
				format_for<M>(),
				aContent);
		}
#endif
	};

	/**	This struct contains information for a buffer which is intended to be used as 
	*	buffer for real-time ray tracing, that contains geometry instances.
	*/
	class geometry_instance_buffer_meta : public buffer_meta
	{
	public:
		/** Gets buffer usage flags for this kind of buffer. */
		vk::BufferUsageFlags buffer_usage_flags() const override { return vk::BufferUsageFlagBits::eRayTracingKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR; }
		
		static geometry_instance_buffer_meta create_from_num_elements(size_t aNumElements, size_t aStride = sizeof(VkAccelerationStructureInstanceKHR)) 
		{ 
			geometry_instance_buffer_meta result; 
			result.mSizeOfOneElement = aStride;
			result.mNumElements = aNumElements;
			return result; 
		}

		static geometry_instance_buffer_meta create_from_total_size(size_t aTotalSize) 
		{ 
			geometry_instance_buffer_meta result; 
			result.mSizeOfOneElement = sizeof(VkAccelerationStructureInstanceKHR);
			assert(aTotalSize % result.mSizeOfOneElement == 0);
			result.mNumElements = aTotalSize / result.mSizeOfOneElement;
			return result; 
		}

		template <typename T>
		static std::enable_if_t<!std::is_pointer_v<T>, geometry_instance_buffer_meta> create_from_data(const T& aData)
		{
			geometry_instance_buffer_meta result; 
			result.mSizeOfOneElement = sizeof(first_or_only_element(aData));
			assert(sizeof(first_or_only_element(aData)) == sizeof(VkAccelerationStructureInstanceKHR));
			result.mNumElements = how_many_elements(aData);
			return result; 
		}

		/** Describe which part of an element's member contains the VkAccelerationStructureInstanceKHR part,
		 *	or put differently, where does the transformation matrix begin in memory.
		 */
		geometry_instance_buffer_meta& describe_member(size_t aOffset, content_description aContent = content_description::geometry_instance)
		{
#if defined(_DEBUG)
			if (std::find_if(std::begin(mOrderedMemberDescriptions), std::end(mOrderedMemberDescriptions), [offs = aOffset](const buffer_element_member_meta& e) { return e.mOffset == offs; }) != mOrderedMemberDescriptions.end()) {
				AVK_LOG_WARNING("There is already a member described at offset " + std::to_string(aOffset));
			}
#endif
			// insert already in the right place
			buffer_element_member_meta newElement{ aOffset, vk::Format::eUndefined, aContent };
			auto it = std::lower_bound(std::begin(mOrderedMemberDescriptions), std::end(mOrderedMemberDescriptions), newElement,
				[](const buffer_element_member_meta& first, const buffer_element_member_meta& second) -> bool { 
					return first.mOffset < second.mOffset;
				});
			mOrderedMemberDescriptions.insert(it, newElement);
			return *this;
		}

		/* Describe that there is only one member and that member is the geometry instance */
		template <typename M>
		geometry_instance_buffer_meta& describe_only_member(const M& aMember, content_description aContent = content_description::geometry_instance)
		{
			assert (sizeof(aMember) == mSizeOfOneElement);
			auto frmt = format_for<M>();
			assert (content_description::geometry_instance == aContent);
			assert (vk::Format::eUndefined == frmt);
			assert (sizeof(VkAccelerationStructureInstanceKHR) == mSizeOfOneElement);
			return describe_member(0, frmt, aContent);
		}

#if defined(_MSC_VER) && defined(__cplusplus)
		/** Describe a member and let the compiler figure out offset and format. */
		template <class T, class M> 
		geometry_instance_buffer_meta& describe_member(M T::* aMember, content_description aContent = content_description::geometry_instance)
		{
			return describe_member(
				((::size_t)&reinterpret_cast<char const volatile&>((((T*)0)->*aMember))),
				format_for<M>(),
				aContent);
		}
#endif
	};
#endif

}
