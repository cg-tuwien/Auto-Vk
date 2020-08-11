#pragma once
#include <avk/avk.hpp>

namespace avk
{
	/**	Make an array by only specifying its type and having its size automatically set.
	 *	Source: https://stackoverflow.com/questions/54125515/partial-template-argument-deduction-or-workaround-for-stdarray
	 */
	template<typename Type, typename ... T>
	constexpr auto make_array(T&&... t) -> std::array<Type, sizeof...(T)>
	{
		return { {std::forward<T>(t)...} };
	}

	template<class T> struct identity { using type = T; };
	template<class D, class... Ts>
	struct ret : identity<D> {};
	template<class... Ts>
	struct ret<void, Ts...> : std::common_type<Ts...> {};
	template<class D, class... Ts>
	using ret_t = typename ret<D, Ts...>::type;

	/** Make a vector.
	 *	Source: https://stackoverflow.com/questions/36994727/how-do-i-write-a-make-vector-similar-to-stdmake-tuple
	 */
	template<class D = void, class... Ts>
	std::vector<ret_t<D, Ts...>> make_vector(Ts&&... args) {
		std::vector<ret_t<D, Ts...>>  ret;
		ret.reserve(sizeof...(args));
		using expander = int[];
		(void) expander{ ((void)ret.emplace_back(std::forward<Ts>(args)), 0)..., 0 };
		return ret;
	}

	template<class D = void, class... Ts>
	std::vector<ret_t<D, Ts...>> move_into_vector(Ts&&... args) {
		std::vector<ret_t<D, Ts...>>  ret;
		ret.reserve(sizeof...(args));
		using expander = int[];
		(void) expander{ ((void)ret.emplace_back(std::move(std::forward<Ts>(args))), 0)..., 0 };
		return ret;
	}

	/** Makes a Vulkan-compatible version integer based on the three given numbers */
	static constexpr uint32_t make_version(uint32_t major, uint32_t minor, uint32_t patch)
	{
		return (((major) << 22) | ((minor) << 12) | (patch));
	}

	/** Find Case Insensitive Sub String in a given substring 
	 *  Credits: https://thispointer.com/implementing-a-case-insensitive-stringfind-in-c/
	 */
	static size_t find_case_insensitive(std::string data, std::string toFind, size_t startingPos)
	{
		// Convert complete given String to lower case
		std::transform(data.begin(), data.end(), data.begin(), ::tolower);
		// Convert complete given Sub String to lower case
		std::transform(toFind.begin(), toFind.end(), toFind.begin(), ::tolower);
		// Find sub string in given string
		return data.find(toFind, startingPos);
	}

	/**	Load a binary file into memory.
	 *	Adapted from: http://www.cplusplus.com/reference/istream/istream/read/
	 */
	static std::vector<char> load_binary_file(std::string path)
	{
		std::vector<char> buffer;
		std::ifstream is(path.c_str(), std::ifstream::binary);
		if (!is) {
			throw avk::runtime_error("Couldn't load file '" + path + "'");
		}

		// get length of file:
		is.seekg(0, is.end);
		size_t length = is.tellg();
		is.seekg(0, is.beg);

		buffer.resize(length);

		// read data as a block:
		is.read(buffer.data(), length);

		if (!is) {
			is.close();
			throw avk::runtime_error(
				"Couldn't read file '" + path + "' into buffer. " +
				"load_binary_file could only read " + std::to_string(is.gcount()) + " bytes instead of " + std::to_string(length)
			);
		}

		is.close();
		return buffer;
	}


	static std::string trim_spaces(std::string_view s)
	{
		if (s.empty()) {
			return "";
		}
		auto pos1 = s.find_first_not_of(' ');
		auto pos2 = s.find_last_not_of(' ');
		return std::string(s.substr(pos1, pos2-pos1+1));
	}

#ifdef USE_BACKSPACES_FOR_PATHS
	const char SEP_TO_USE = '\\';
	const char SEP_NOT_TO_USE = '/';
#else
	const char SEP_TO_USE = '/';
	const char SEP_NOT_TO_USE = '\\';
#endif

	static std::string clean_up_path(std::string_view path)
	{
		auto cleaned_up = trim_spaces(path);
		int consecutive_sep_cnt = 0;
		for (int i = 0; i < cleaned_up.size(); ++i) {
			if (cleaned_up[i] == SEP_NOT_TO_USE) {
				cleaned_up[i] = SEP_TO_USE;
			}
			if (cleaned_up[i] == SEP_TO_USE) {
				consecutive_sep_cnt += 1;
			}
			else {
				consecutive_sep_cnt = 0;
			}
			if (consecutive_sep_cnt > 1) {
				cleaned_up = cleaned_up.substr(0, i) + (i < cleaned_up.size() - 1 ? cleaned_up.substr(i + 1) : "");
				consecutive_sep_cnt -= 1;
				--i;
			}
		}
		return cleaned_up;
	}

	static std::string extract_file_name(std::string_view path)
	{
		auto cleaned_path = clean_up_path(path);
		auto last_sep_idx = cleaned_path.find_last_of(SEP_TO_USE);
		if (std::string::npos == last_sep_idx) {
			return cleaned_path;
		}
		return cleaned_path.substr(last_sep_idx + 1);
	}

	static std::string extract_base_path(std::string_view path)
	{
		auto cleaned_path = clean_up_path(path);
		auto last_sep_idx = cleaned_path.find_last_of(SEP_TO_USE);
		if (std::string::npos == last_sep_idx) {
			return cleaned_path;
		}
		return cleaned_path.substr(0, last_sep_idx + 1);
	}

	static std::string combine_paths(std::string_view first, std::string_view second)
	{
		return clean_up_path(std::string(first) + SEP_TO_USE + std::string(second));
	}

	static std::string transform_path_for_comparison(std::string_view _Path)
	{
		auto pathCleaned = clean_up_path(_Path);
#if defined(_WIN32)
		std::transform(pathCleaned.begin(),  pathCleaned.end(),  pathCleaned.begin(),  [](auto c){ return std::tolower(c); });
#endif
		return pathCleaned;
	}

	static bool are_paths_equal(std::string_view first, std::string_view second)
	{
		auto firstPathTransformed = transform_path_for_comparison(first);
		auto secondPathTransformed = transform_path_for_comparison(second);
		return firstPathTransformed == secondPathTransformed;
	}
	
	
	template <typename V, typename F>
	bool has_flag(V value, F flag)
	{
		return (value & flag) == flag;
	}

	template <typename InQuestion, typename... Existing>
	constexpr bool has_type(std::tuple<Existing...>) {
	    return std::disjunction_v<std::is_same<InQuestion, Existing>...>;
	}
	
	// SFINAE test for detecting if a type has a `.size()` member and iterators
	template <typename T>
	class has_size_and_iterators
	{
	private:
		typedef char NoType[1];
		typedef char YesType[2];

		template<typename C> static auto Test(void*)
			-> std::tuple<YesType, decltype(std::declval<C const>().size()), decltype(std::declval<C const>().begin()), decltype(std::declval<C const>().end())>;

		  template<typename> static NoType& Test(...);

		public:
			static bool const value = sizeof(Test<T>(0)) != sizeof(NoType);
	};


	template<typename T> 
	typename std::enable_if<has_size_and_iterators<T>::value, uint32_t>::type how_many_elements(const T& t) {
		return static_cast<uint32_t>(t.size());
	}

	template<typename T> 
	typename std::enable_if<!has_size_and_iterators<T>::value, uint32_t>::type how_many_elements(const T& t) {
		return 1u;
	}

	template<typename T> 
	typename std::enable_if<has_size_and_iterators<T>::value, const typename T::value_type&>::type first_or_only_element(const T& t) {
		return t[0];
	}

	template<typename T> 
	typename std::enable_if<!has_size_and_iterators<T>::value, const T&>::type first_or_only_element(const T& t) {
		return t;
	}



	// SFINAE test for detecting if a type can be dereferenced, by testing for availability of `.operator*()`
	template <typename T>
	class is_dereferenceable
	{
	private:
		typedef char NoType[1];
		typedef char YesType[2];

		template<typename C> static auto Test(void*)
			-> std::tuple<YesType, decltype(std::declval<C const>().operator*())>;

		  template<typename> static NoType& Test(...);

		public:
			static bool const value = sizeof(Test<T>(0)) != sizeof(NoType);
	};

	
	// SFINAE test for detecting if a type has a `.resize()` method
	template <typename T>
	class has_resize
	{
	private:
		typedef char NoType[1];
		typedef char YesType[2];

		template<typename C> static auto Test(void*)
			-> std::tuple<YesType, decltype(std::declval<C const>().resize())>;

		  template<typename> static NoType& Test(...);

		public:
			static bool const value = sizeof(Test<T>(0)) != sizeof(NoType);
	};
	


	template <typename T>
	class owning_resource : public std::variant<std::monostate, T, std::shared_ptr<T>>
	{
	public:
		using value_type = T;

		owning_resource() : std::variant<std::monostate, T, std::shared_ptr<T>>() {}
		owning_resource(T&& r) noexcept : std::variant<std::monostate, T, std::shared_ptr<T>>(std::move(r)) {}
		owning_resource(owning_resource<T>&&) noexcept = default;
		owning_resource(const T&) = delete;
		owning_resource(const owning_resource<T>& other)
		{
			if (other.has_value()) {
				if (!other.is_shared_ownership_enabled()) {
					//
					//	Why are you getting this exception?
					//	The most obvious reason would be that you have intentionally
					//	tried to copy a resource which might not be allowed unless
					//	"shared ownership" has been enabled.
					//	If you intended to enable shared ownership, call: `enable_shared_ownership()`
					//	which will move the resource internally into a shared pointer.
					//
					//	Attention:
					//	A common source of unintentionally causing this exception is
					//	the usage of a resource in an `std::initializer_list`. The problem
					//	with that is that it looks like it does not support move-only types.
					//	An alternative to initializer lists would be to use `ak::make_vector`.
					//
					throw avk::logic_error("You are trying to copy-construct a resource of type '" + std::string(typeid(T).name()) + "' which does not have shared ownership enabled. This call will fail now. You can try to use owning_resource::enable_shared_ownership().");
				}
				*this_as_variant() = std::get<std::shared_ptr<T>>(other);
			}
			else {
				assert(!has_value());
			}
		}
		owning_resource<T>& operator=(T&& r) noexcept { *this_as_variant() = std::move(r); return *this; }
		owning_resource<T>& operator=(const T&) = delete;
		owning_resource<T>& operator=(owning_resource<T>&& r) noexcept = default;
		owning_resource<T>& operator=(const owning_resource<T>& other)
		{
			if (other.has_value()) {
				if (!other.is_shared_ownership_enabled()) {
					//
					//	Why are you getting this exception?
					//	The most obvious reason would be that you have intentionally
					//	tried to copy a resource which might not be allowed unless
					//	"shared ownership" has been enabled.
					//	If you intended to enable shared ownership, call: `enable_shared_ownership()`
					//	which will move the resource internally into a shared pointer.
					//
					//	Attention:
					//	A common source of unintentionally causing this exception is
					//	the usage of a resource in an `std::initializer_list`. The problem
					//	with that is that it looks like it does not support move-only types.
					//	An alternative to initializer lists would be to use `ak::make_vector`.
					//
					throw avk::logic_error("Can only copy assign owning_resources which have shared ownership enabled.");
				}
				*this_as_variant() = std::get<std::shared_ptr<T>>(other);
			}
			else {
				assert(!has_value());
			}
			return *this;
		}

		bool is_shared_ownership_enabled() const noexcept { return std::holds_alternative<std::shared_ptr<T>>(*this); }
		bool has_value() const noexcept { return !std::holds_alternative<std::monostate>(*this); }
		bool holds_item_directly() const noexcept { return std::holds_alternative<T>(*this); }
		const std::variant<std::monostate, T, std::shared_ptr<T>>* this_as_variant() const noexcept { return static_cast<const std::variant<std::monostate, T, std::shared_ptr<T>>*>(this); }
		std::variant<std::monostate, T, std::shared_ptr<T>>* this_as_variant() noexcept { return static_cast<std::variant<std::monostate, T, std::shared_ptr<T>>*>(this); }

		void enable_shared_ownership()
		{
			if (is_shared_ownership_enabled()) {
				return; // Already established
			}
			if (std::holds_alternative<std::monostate>(*this)) {
				throw avk::logic_error("This owning_resource is uninitialized, i.e. std::monostate.");
			}
			*this_as_variant() = std::make_shared<T>(std::move(std::get<T>(*this)));
		}

		operator const T&() const
		{ 
			if (is_shared_ownership_enabled()) { return *std::get<std::shared_ptr<T>>(*this_as_variant()); }
			if (holds_item_directly()) { return std::get<T>(*this_as_variant()); }
			throw avk::logic_error("This owning_resource is uninitialized, i.e. std::monostate.");
		}

		operator T&() 
		{ 
			if (is_shared_ownership_enabled()) { return *std::get<std::shared_ptr<T>>(*this_as_variant()); }
			if (holds_item_directly()) { return std::get<T>(*this_as_variant()); }
			throw avk::logic_error("This owning_resource is uninitialized, i.e. std::monostate.");
		}
		
		const T& operator*() const
		{
			return this->operator const T&();
		}
		
		T& operator*()
		{
			return this->operator T&();
		}
		
		const T* operator->() const
		{
			return &this->operator const T&();
		}
		
		T* operator->()
		{
			return &this->operator T&();
		}
	};
	


	template<typename T>
	class unique_function : protected std::function<T>
	{
	    template<typename Fn, typename En = void>
	    struct wrapper;

	    // specialization for CopyConstructible Fn
	    template<typename Fn>
	    struct wrapper<Fn, std::enable_if_t< std::is_copy_constructible<Fn>::value >>
	    {
	        Fn fn;

	        template<typename... Args>
	        auto operator()(Args&&... args) noexcept { return fn(std::forward<Args>(args)...); }
	    };

	    // specialization for MoveConstructible-only Fn
	    template<typename Fn>
	    struct wrapper<Fn, std::enable_if_t< !std::is_copy_constructible<Fn>::value
											 && std::is_move_constructible<Fn>::value >>
	    {
	        Fn fn;

	        wrapper(Fn fn) : fn(std::move(fn)) { }

	        wrapper(wrapper&&) noexcept = default;
	        wrapper& operator=(wrapper&&) noexcept = default;

	        // these two functions are instantiated by std::function and are never called
	        wrapper(const wrapper& rhs) noexcept : fn(const_cast<Fn&&>(rhs.fn)) { throw avk::logic_error("never called"); } // hack to initialize fn for non-DefaultContructible types
	        wrapper& operator=(const wrapper&) noexcept { throw avk::logic_error("never called"); }

			~wrapper() = default;

	        template<typename... Args>
	        auto operator()(Args&&... args) noexcept { return fn(std::forward<Args>(args)...); }
	    };

	    using base = std::function<T>;

	public:
		unique_function() = default;
		unique_function(std::nullptr_t) noexcept : base(nullptr) {}
		unique_function(const unique_function&) noexcept = default;
		unique_function(unique_function&&) noexcept = default;

		template<class Fn> 
		unique_function(Fn f) noexcept : base( wrapper<Fn>{ std::move(f) }) {}

		~unique_function() = default;

		unique_function& operator=(const unique_function&) noexcept = default;
		unique_function& operator=(unique_function&&) noexcept = default;
		unique_function& operator=(std::nullptr_t) noexcept
		{
			base::operator=(nullptr); return *this;
		}

		template<typename Fn>
	    unique_function& operator=(Fn&& f) noexcept
	    { base::operator=(wrapper<Fn>{ std::forward<Fn>(f) }); return *this; }

		template<typename Fn>
	    unique_function& operator=(std::reference_wrapper<Fn> f) noexcept
	    { base::operator=(wrapper<Fn>{ std::move(f) }); return *this; }

		template<class Fn> 
		Fn* target() noexcept
		{ return &base::template target<wrapper<Fn>>()->fn; }

		template<class Fn> 
		const Fn* target() const noexcept // TODO/ATTENTION/NOTE: changed this to const Fn* in order to allow `const unique_function<void(command_buffer_t&, pipeline_stage, std::optional<read_memory_access>)>& aToTest` in `ak::sync` (i.e. the CONST&). Not sure if this is totally okay or has any side effects.
		{ return &base::template target<wrapper<Fn>>()->fn; }

		template<class Fn> 
		const std::type_info& target_type() const noexcept
		{ return typeid(*target<Fn>()); }

		using base::swap;
	    using base::operator();
		using base::operator bool;
	};

		
	template< class R, class... Args >
	static void swap( unique_function<R(Args...)> &lhs, unique_function<R(Args...)> &rhs )
	{
		lhs.swap(rhs);
	}

	template< class R, class... ArgTypes >
	static bool operator==( const unique_function<R(ArgTypes...)>& f, std::nullptr_t ) noexcept
	{
		return !f;
	}

	template< class R, class... ArgTypes >
	static bool operator==( std::nullptr_t, const unique_function<R(ArgTypes...)>& f ) noexcept
	{
		return !f;
	}

	template< class R, class... ArgTypes >
	static bool operator!=( const unique_function<R(ArgTypes...)>& f, std::nullptr_t ) noexcept
	{
		return static_cast<bool>(f);
	}

	template< class R, class... ArgTypes >
	static bool operator!=( std::nullptr_t, const unique_function<R(ArgTypes...)>& f ) noexcept
	{
		return static_cast<bool>(f);
	}

	/*	Combines multiple hash values.
	 *  Inspiration and implementation largely taken from: https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x/54728293#54728293
	 *  TODO: Should a larger magic constant be used since we're only supporting x64 and hence, size_t will always be 64bit?!
	 */
	template <typename T, typename... Rest>
	void hash_combine(std::size_t& seed, const T& v, const Rest&... rest) noexcept
	{
		seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		(hash_combine(seed, rest), ...);
	}

	/**	Returns true if `_Element` is contained within `_Vector`, also provides 
	 *	the option to return the position where the element has been found.
	 *	@param	_Vector			Where to look for `_Element`
	 *	@param	_Element		The element to find
	 *	@param	_OutPosition	(Optional) If the address of an iterator is set, it will be assigned the position of the element
	 */
	template <typename T>
	bool contains_element(const std::vector<T>& _Vector, const T& _Element, typename std::vector<T>::const_iterator* _OutPosition = nullptr)
	{
		auto it = std::find(std::begin(_Vector), std::end(_Vector), _Element);
		if (nullptr != _OutPosition) {
			*_OutPosition = it;
		}
		return _Vector.end() != it;
	}

	/** Returns the index of the first occurence of `_Element` within `_Vector`
	 *	If the element is not contained, `_Vector.size()` will be returned.
	 */
	template <typename T>
	size_t index_of(const std::vector<T>& _Vector, const T& _Element)
	{
		auto it = std::find(std::begin(_Vector), std::end(_Vector), _Element);
		return std::distance(std::begin(_Vector), it);
	}

	/** Inserts a copy of `_Element` into `_Vector` if `_Element` is not already contained in `_Vector` 
	 *	@param	_Vector				The collection where `_Element` shall be inserted
	 *	@param	_Element			The element to be inserted into `_Vector`, if it is not already contained. `_Element` must be copy-constructible.
	 *	@param	_PositionOfElement	(Optional) If the address of an iterator is set, it will be assigned the position of the 
	 *								newly inserted element into `_Vector`.
	 *	@return	True if the element was inserted into the vector, meaning it was not contained before. False otherwise.
	 */
	template <typename T>
	bool add_to_vector_if_not_already_contained(std::vector<T>& _Vector, const T& _Element, typename std::vector<T>::const_iterator* _PositionOfElement = nullptr)
	{
		if (!contains_element(_Vector, _Element, _PositionOfElement)) {
			_Vector.push_back(_Element);
			if (nullptr != _PositionOfElement) {
				*_PositionOfElement = std::prev(_Vector.end());
			}
			return true;
		}
		return false;
	}

	/** Convert a "FourCC" to a std::string */
	inline std::string fourcc_to_string(unsigned int fourcc)
	{
		char fourccBuf[8];
		fourccBuf[3] = static_cast<char>(0x000000FF & fourcc);
		fourccBuf[2] = static_cast<char>(0x000000FF & (fourcc >> 8));
		fourccBuf[1] = static_cast<char>(0x000000FF & (fourcc >> 16));
		fourccBuf[0] = static_cast<char>(0x000000FF & (fourcc >> 24));

		// convert 000000000 to spaces
		for (int i = 0; i < 4; i++)
		{
			if (0 == fourccBuf[i])
				fourccBuf[i] = ' ';
		}

		fourccBuf[4] = 0;
		return std::string(fourccBuf);
	}

	template <typename T>
	struct handle_wrapper
	{
		handle_wrapper()
			: mHandle{ nullptr }
		{ }
		
		handle_wrapper(T aHandle)
			: mHandle{ std::move(aHandle) }
		{ }

		handle_wrapper(handle_wrapper&& aOther) noexcept
			: mHandle{ std::move(aOther.mHandle) }
		{
			aOther.reset();
		}
		
		handle_wrapper(const handle_wrapper& aOther)
			: mHandle{ aOther.mHandle }
		{ }
		
		handle_wrapper& operator=(handle_wrapper&& aOther) noexcept
		{
			mHandle = std::move(aOther.mHandle);
			aOther.reset();
			return *this;
		}

		handle_wrapper& operator=(const handle_wrapper& aOther)
		{
			mHandle = aOther.mHandle;
			return *this;
		}

		void reset()
		{
			mHandle = nullptr;
		}

		T mHandle;
	};
}
