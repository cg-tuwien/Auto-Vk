#pragma once
#include <avk/avk.hpp>

namespace avk
{
	/**	Make an array by only specifying its type and having its size automatically set.
	 *	Source: https://stackoverflow.com/questions/54125515/partial-template-argument-deduction-or-workaround-for-stdarray
	 */
	template<typename Type, typename ... T>
	constexpr auto make_array(T&&... t)->std::array<Type, sizeof...(T)>
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
	std::vector<ret_t<D, Ts...>> make_vector(Ts&&... args)
	{
		std::vector<ret_t<D, Ts...>>  ret;
		ret.reserve(sizeof...(args));
		using expander = int[];
		(void)expander
		{
			((void)ret.emplace_back(std::forward<Ts>(args)), 0)..., 0
		};
		return ret;
	}

	template<class D = void, class... Ts>
	std::vector<ret_t<D, Ts...>> move_into_vector(Ts&&... args)
	{
		std::vector<ret_t<D, Ts...>>  ret;
		ret.reserve(sizeof...(args));
		using expander = int[];
		(void)expander
		{
			((void)ret.emplace_back(std::move(std::forward<Ts>(args))), 0)..., 0
		};
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
		return std::string(s.substr(pos1, pos2 - pos1 + 1));
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
				cleaned_up = cleaned_up.substr(0, i) + (i < cleaned_up.size() - 1 ? cleaned_up.substr(static_cast<size_t>(i) + 1) : "");
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
		std::transform(pathCleaned.begin(), pathCleaned.end(), pathCleaned.begin(), [](auto c) { return std::tolower(c); });
#endif
		return pathCleaned;
	}

	static bool are_paths_equal(std::string_view first, std::string_view second)
	{
		auto firstPathTransformed = transform_path_for_comparison(first);
		auto secondPathTransformed = transform_path_for_comparison(second);
		return firstPathTransformed == secondPathTransformed;
	}

	// Returns true if the given enum value contains the flag in question
	template <typename V, typename F>
	bool has_flag(V value, F flag)
	{
		return (value & flag) == flag;
	}

	// Returns true if the value inQuestion is equal to one of the given possibilities
	template <typename X, typename... Y>
	bool equals_one_of(const X& inQuestion, const Y&... possibilities)
	{
		return (... || (inQuestion == possibilities));
	}
	
	// Returns true if the given 
	template <typename InQuestion, typename... Existing>
	constexpr bool has_type(std::tuple<Existing...>)
	{
		return std::disjunction_v<std::is_same<InQuestion, Existing>...>;
	}

	template <typename V, typename F>
	V exclude(V original, F toExclude)
	{
		typedef std::underlying_type_t<V> EnumType;
		return static_cast<V>(static_cast<EnumType>(original) & (~static_cast<EnumType>(toExclude)));
	}

	template <typename V, typename F>
	bool is_included(V toTest, F inQuestion)
	{
		return (toTest & inQuestion) == inQuestion;
	}

	// SFINAE test for detecting if a type has a `.size()` member and iterators
	template <typename T>
	class has_size_and_iterators
	{
	private:
		typedef char NoType[1];
		typedef char YesType[2];

		template<typename C> static auto Test(void*)
			->std::tuple<YesType, decltype(std::declval<C const>().size()), decltype(std::declval<C const>().begin()), decltype(std::declval<C const>().end())>;

		template<typename> static NoType& Test(...);

	public:
		static bool const value = sizeof(Test<T>(0)) != sizeof(NoType);
	};

	// SFINAE test for detecting if a type has a `.size()` member and iterators
	template <typename T>
	class has_nested_value_types
	{
	private:
		typedef char NoType[1];
		typedef char YesType[2];

		template<typename C> static auto Test(void*)
			->std::tuple<YesType, typename C::value_type::value_type>;

		template<typename> static NoType& Test(...);

	public:
		static bool const value = sizeof(Test<T>(0)) != sizeof(NoType);
	};


	template<typename T>
	typename std::enable_if<has_size_and_iterators<T>::value, uint32_t>::type how_many_elements(const T& t)
	{
		return static_cast<uint32_t>(t.size());
	}

	template<typename T>
	typename std::enable_if<!has_size_and_iterators<T>::value, uint32_t>::type how_many_elements(const T& t)
	{
		return 1u;
	}

	template<typename T>
	typename std::enable_if<has_size_and_iterators<T>::value, const typename T::value_type&>::type first_or_only_element(const T& t)
	{
		return t[0];
	}

	template<typename T>
	typename std::enable_if<!has_size_and_iterators<T>::value, const T&>::type first_or_only_element(const T& t)
	{
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
			->std::tuple<YesType, decltype(std::declval<C const>().operator*())>;

		template<typename> static NoType& Test(...);

	public:
		static bool const value = sizeof(Test<T>(0)) != sizeof(NoType);
	};


	// A concept which requires a type to have a .resize(size_t)
	template <typename T>
	concept has_resize = requires (T x)
	{
		x.resize(size_t{1});
	};

	// is_same test for a variadic arguments pack
	template <class T, class... Ts>
	struct are_same : std::conjunction<std::is_same<T, Ts>...> {};

	// A concept which requires a type to be non-const
	template <typename T>
	concept non_const = !std::is_const_v<T>;
	// A concept which requires a type to have a subscript operator
	template <typename T>
	concept has_subscript_operator = requires (T x)
	{
		x[size_t{1}];
	};

	// A concept which requires a type to have ::value_type
	template <typename T>
	concept has_value_type = requires
	{
		typename T::value_type; // Meaning: "Nested type T::value_type exists"
	};

	// A concept which requires a type to have a .handle()
	template <typename T>
	concept has_handle = requires (T x)
	{
		x.handle();
	};

	// A concept which requires a type to have iterators
	template <typename T>
	concept has_iterators = requires (T x)
	{
		std::begin(x);
		std::end(x);
	};
	
	// This class represents a/the owner of a specific resource T.
	//
	// The resource is either held locally on the stack, or -- as an additional features -- moved onto
	// the heap by the means of a shared pointer. If a resource lives on the heap, copy constructor
	// and copy assignment operator will be enabled. If it doesn't. they will throw an exception.
	//
	// Resource types T are expected to be move-only types, otherwise handling their lifetime with
	// this class does probably not really make any sense.
	template <typename T> requires non_const<T>
	class owning_resource : public std::variant<std::monostate, T, std::shared_ptr<T>>
	{
	public:
		// The type of the resource being owned and handled
		using value_type = T;

		// Cast the this-pointer to what it is: a std::variant-pointer (and to const)
		const std::variant<std::monostate, T, std::shared_ptr<T>>* this_as_variant() const noexcept
		{
			return static_cast<const std::variant<std::monostate, T, std::shared_ptr<T>>*>(this);
		}

		// Cast the this-pointer to what it is: a std::variant-pointer
		std::variant<std::monostate, T, std::shared_ptr<T>>* this_as_variant() noexcept
		{
			return static_cast<std::variant<std::monostate, T, std::shared_ptr<T>>*>(this);
		}

		// Construct an owning_resource instance and initialize it to std::monostate.
		owning_resource()
			: std::variant<std::monostate, T, std::shared_ptr<T>>{}
		{}

		// Construct an owning_resource by rvalue reference of a resource T.
		// The original resource that is moved from might not be modified by the move.
		// By default, an owning_resource is created with shared ownership enabled.
		owning_resource(T&& aResource, bool aCreateWithSharedOwnershipEnabled = true) noexcept
		{
			if (aCreateWithSharedOwnershipEnabled) {
				*this_as_variant() = std::make_shared<T>(std::move(aResource));
			}
			else {
				*this_as_variant() = std::move(aResource);
			}
		}

		owning_resource<T>& operator=(T&& aResource) = delete;
		//// Move-assign an owning_resource from an rvalue reference of a resource T.
		//// The original resource that is moved from might not be modified by the move.
		//// By default, an owning_resource is created with shared ownership enabled.
		//owning_resource<T>& operator=(T&& aResource) noexcept
		//{
		//	if (aCreateWithSharedOwnershipEnabled) {
		//		*this_as_variant() = std::make_shared<T>(std::move(aResource));
		//	}
		//	else {
		//		*this_as_variant() = std::move(aResource);
		//	}
		//	return *this;
		//}

		// Move-construct an owning_resource from another owning_resource.
		// The other owning_reference is reset to std::monostate.
		owning_resource(owning_resource<T>&& aOther) noexcept
			: std::variant<std::monostate, T, std::shared_ptr<T>>{ std::move(aOther) }
		{
			if (aOther.has_value()) {
				aOther = owning_resource<T>{};
			}
		}

		// Move-assign an owning_resource from another owning_resource.
		// The other owning_reference is reset to std::monostate.
		owning_resource<T>& operator=(owning_resource<T>&& aOther) noexcept
		{
			*this_as_variant() = std::move(aOther);
			if (aOther.has_value()) {
				aOther = owning_resource<T>{};
			}
			return *this;
		}

		// Resource types are expected to be move-only types. Therefore, an owning_resource
		// can not be created by copying a resource.
		owning_resource(const T&) = delete;

		// Resource types are expected to be move-only types. Therefore, an owning_resource
		// can not be assigned a copy of a resource.
		owning_resource<T>& operator=(const T&) = delete;

		// owning_resources not always can be copy-constructed. But when they can, they
		// can only due to a preceeding call to enable_shared_ownership, which moves the
		// resource onto the heap.
		owning_resource(const owning_resource<T>& aOther)
		{
			if (aOther.has_value()) {
				if (!aOther.is_shared_ownership_enabled()) {
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
				*this_as_variant() = std::get<std::shared_ptr<T>>(aOther);
			}
			else {
				assert(!has_value());
			}
		}

		// owning_resources not always can be copy-assigned. But when they can, they
		// can only due to a preceeding call to enable_shared_ownership, which moves the
		// resource onto the heap.
		owning_resource<T>& operator=(const owning_resource<T>& aOther)
		{
			if (aOther.has_value()) {
				if (!aOther.is_shared_ownership_enabled()) {
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
				*this_as_variant() = std::get<std::shared_ptr<T>>(aOther);
			}
			else {
				assert(!has_value());
			}
			return *this;
		}

		// Nothing wrong with default destruction
		~owning_resource() = default;

		// Enable shared ownership of this resource. That means: Transfer the resource
		// from the stack to the heap and manage its lifetime through a shared pointer.
		// (Unless it already has been.)
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

		// Has this owning_resource instance shared ownership enabled.
		// Or put differently: Is the resource T living on the heap and
		// and referenced via a shared pointer?
		bool is_shared_ownership_enabled() const noexcept
		{
			return std::holds_alternative<std::shared_ptr<T>>(*this);
		}

		// Does this owning_resource store the resource on the stack?
		bool holds_item_directly() const noexcept
		{
			return std::holds_alternative<T>(*this);
		}

		// Does this owning_resource actually store a resource?
		// (Or does it refer to std::monostate?)
		bool has_value() const noexcept
		{
			return !std::holds_alternative<std::monostate>(*this);
		}

		// Get a const reference to the owned resource T
		const T& get() const
		{
			if (holds_item_directly()) { return std::get<T>(*this_as_variant()); }
			if (is_shared_ownership_enabled()) { return *std::get<std::shared_ptr<T>>(*this_as_variant()); }
			throw avk::logic_error("This owning_resource is uninitialized, i.e. std::monostate.");
		}

		// Get a reference to the owned resource T
		T& get()
		{
			if (is_shared_ownership_enabled()) { return *std::get<std::shared_ptr<T>>(*this_as_variant()); }
			if (holds_item_directly()) { return std::get<T>(*this_as_variant()); }
			throw avk::logic_error("This owning_resource is uninitialized, i.e. std::monostate.");
		}

		//[[nodiscard]] const T& as_const_reference() const
		//{
		//	return get();
		//}

		[[nodiscard]] T& as_reference()
		{
			return get();
		}

		// Access the resource by returning a reference to it
		const T& operator*() const
		{
			return get();
		}

		// Access the resource by returning a reference to it
		T& operator*()
		{
			return get();
		}

		// Access the resource by returning its address
		const T* operator->() const
		{
			return &get();
		}

		// Access the resource by returning its address
		T* operator->()
		{
			return &get();
		}
	};

	// A type for passing resources as arguments. Can be used to express that
	// ownership shall be passed along with it, or only a reference to it.
	template <typename T>
	class resource_argument : public std::variant<T*, owning_resource<T>>
	{
	public:
		// The type of the resource:
		using value_type = T;
		
		// Cast the this-pointer to what it is: a std::variant-pointer (and to const)
		const std::variant<T*, owning_resource<T>>* this_as_variant() const noexcept
		{
			return static_cast<std::variant<T*, owning_resource<T>>*>(this);
		}

		// Cast the this-pointer to what it is: a std::variant-pointer
		std::variant<T*, owning_resource<T>>* this_as_variant() noexcept
		{
			return static_cast<std::variant<T*, owning_resource<T>>*>(this);
		}
		
		resource_argument(T& aResource) noexcept
		{
			*this_as_variant() = &aResource;
		}

		resource_argument(owning_resource<T> aResource) noexcept
		{
			*this_as_variant() = std::move(aResource);
		}

		resource_argument(resource_argument&&) noexcept = default;
		resource_argument(const resource_argument& aOther) = default;
		resource_argument& operator=(resource_argument&&) noexcept = default;
		resource_argument& operator=(const resource_argument&) = default;
		~resource_argument() = default;

		bool is_ownership() {
			return std::holds_alternative<owning_resource<T>>(*this_as_variant());
		}

		bool is_reference() {
			return !is_ownership();
		}

		// Get a reference to the owned resource T
		T& get()
		{
			if (is_reference()) {
				assert(std::holds_alternative<T*>(*this_as_variant()));
				return *std::get<T*>(*this_as_variant());
			}
			assert(is_ownership());
			return std::get<owning_resource<T>>(*this_as_variant()).as_reference();
		}

		// Access the resource by returning a reference to it
		T& operator*()
		{
			return get();
		}

		// Access the resource by returning its address
		T* operator->()
		{
			return &get();
		}

		// Get the ownership, i.e. the owned resource:
		owning_resource<T>& get_ownership()
		{
			if (is_reference()) {
				throw avk::logic_error("The resource of type '" + std::string(typeid(T).name()) + "' is stored as reference. Cannot get its parent (owning) resource.");
			}
			assert(is_ownership());
			return std::get<owning_resource<T>>(*this_as_variant());
		}

		// Get the ownership or an empty ownership object.
		// This method does not throw.
		owning_resource<T> move_ownership_or_get_empty()
		{
			if (is_reference()) {
				return owning_resource<T>{};
			}
			assert(is_ownership());
			return std::move(std::get<owning_resource<T>>(*this_as_variant()));
		}
	};
	
	template<typename T>
	class unique_function : protected std::function<T>
	{
		template<typename Fn, typename En = void>
		struct wrapper 
		{
			// this struct is not used, only its specializations, but defining it stops the MSVC++ compiler from issuing warning C4348.
		};

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

			wrapper(Fn fn) noexcept : fn(std::move(fn)) {}

			wrapper(wrapper&&) noexcept = default;
			wrapper& operator=(wrapper&&) noexcept = default;

			// these two functions are instantiated by std::function and are never called
			wrapper(const wrapper& rhs) : fn(const_cast<Fn&&>(rhs.fn)) { throw avk::logic_error("never called"); } // hack to initialize fn for non-DefaultContructible types
			wrapper& operator=(const wrapper&) { throw avk::logic_error("never called"); }

			~wrapper() = default;

			template<typename... Args>
			auto operator()(Args&&... args) noexcept { return fn(std::forward<Args>(args)...); }
		};

		using base = std::function<T>;

	public:
		unique_function() noexcept = default;
		unique_function(std::nullptr_t) noexcept : base(nullptr) {}
		unique_function(const unique_function&) = default;
		unique_function(unique_function&&) noexcept = default;

		template<class Fn>
		unique_function(Fn f) noexcept : base(wrapper<Fn>{ std::move(f) }) {}

		~unique_function() = default;

		unique_function& operator=(const unique_function&) = default;
		unique_function& operator=(unique_function&&) noexcept = default;
		unique_function& operator=(std::nullptr_t) noexcept
		{
			base::operator=(nullptr); return *this;
		}

		template<typename Fn>
		unique_function& operator=(Fn&& f) noexcept
		{
			base::operator=(wrapper<Fn>{ std::forward<Fn>(f) }); return *this;
		}

		template<typename Fn>
		unique_function& operator=(std::reference_wrapper<Fn> f) noexcept
		{
			base::operator=(wrapper<Fn>{ std::move(f) }); return *this;
		}

		template<class Fn>
		Fn* target() noexcept
		{
			return &base::template target<wrapper<Fn>>()->fn;
		}

		template<class Fn>
		const Fn* target() const noexcept // TODO/ATTENTION/NOTE: changed this to const Fn* in order to allow `const unique_function<void(command_buffer_t&, pipeline_stage, std::optional<read_memory_access>)>& aToTest` in `ak::old_sync` (i.e. the CONST&). Not sure if this is totally okay or has any side effects.
		{
			return &base::template target<wrapper<Fn>>()->fn;
		}

		template<class Fn>
		const std::type_info& target_type() const noexcept
		{
			return typeid(*target<Fn>());
		}

		using base::swap;
		using base::operator();
		using base::operator bool;
	};


	template< class R, class... Args >
	static void swap(unique_function<R(Args...)> &lhs, unique_function<R(Args...)> &rhs)
	{
		lhs.swap(rhs);
	}

	template< class R, class... ArgTypes >
	static bool operator==(const unique_function<R(ArgTypes...)>& f, std::nullptr_t) noexcept
	{
		return !f;
	}

	template< class R, class... ArgTypes >
	static bool operator==(std::nullptr_t, const unique_function<R(ArgTypes...)>& f) noexcept
	{
		return !f;
	}

	template< class R, class... ArgTypes >
	static bool operator!=(const unique_function<R(ArgTypes...)>& f, std::nullptr_t) noexcept
	{
		return static_cast<bool>(f);
	}

	template< class R, class... ArgTypes >
	static bool operator!=(std::nullptr_t, const unique_function<R(ArgTypes...)>& f) noexcept
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
		seed ^= std::hash<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
		(hash_combine(seed, rest), ...);
	}

	/**	Returns true if `aElement` is contained within `aContainer`, also provides
	 *	the option to return the position where the element has been found.
	 *	@param	aContainer		The container to search `aElement` in.
	 *	@param	aElement		The element to be searched
	 *	@param	aOutPosition	(Optional) If the address of an iterator is set, it will be assigned the position of the element
	 */
	template <typename T>
	bool contains(const T& aContainer, const typename T::value_type& aElement, typename T::const_iterator* aOutPosition = nullptr)
	{
		auto it = std::find(std::begin(aContainer), std::end(aContainer), aElement);
		if (nullptr != aOutPosition) {
			*aOutPosition = it;
		}
		return std::end(aContainer) != it;
	}

	/** Returns the index of the first occurence of `_Element` within `_Vector`
	 *	If the element is not contained, `_Vector.size()` will be returned.
	 */
	template <typename T>
	size_t index_of(const std::vector<T>& aVector, const T& aElement)
	{
		auto it = std::find(std::begin(aVector), std::end(aVector), aElement);
		return std::distance(std::begin(aVector), it);
	}

	/** Inserts a copy of `_Element` into `_Vector` if `_Element` is not already contained in `_Vector`
	 *	@param	aVector				The collection where `_Element` shall be inserted
	 *	@param	aElement			The element to be inserted into `_Vector`, if it is not already contained. `_Element` must be copy-constructible.
	 *	@param	aPositionOfElement	(Optional) If the address of an iterator is set, it will be assigned the position of the
	 *								newly inserted element into `_Vector`.
	 *	@return	True if the element was inserted into the vector, meaning it was not contained before. False otherwise.
	 */
	template <typename T>
	bool add_to_vector_if_not_already_contained(std::vector<T>& aVector, const T& aElement, typename std::vector<T>::const_iterator* aPositionOfElement = nullptr)
	{
		if (!contains(aVector, aElement, aPositionOfElement)) {
			aVector.push_back(aElement);
			if (nullptr != aPositionOfElement) {
				*aPositionOfElement = std::prev(aVector.end());
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
		for (int i = 0; i < 4; i++) {
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
		{
		}

		handle_wrapper(T aHandle)
			: mHandle{ std::move(aHandle) }
		{
		}

		handle_wrapper(handle_wrapper&& aOther) noexcept
		{
			std::swap(mHandle, aOther.mHandle);
		}

		handle_wrapper(const handle_wrapper& aOther)
			: mHandle{ aOther.mHandle }
		{
		}

		handle_wrapper& operator=(handle_wrapper&& aOther) noexcept
		{
			std::swap(mHandle, aOther.mHandle);
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


#pragma region swap + dispose and handle help functions

	/**  This concept captures those objects which have a size() method, like vectors
	 */
	template <typename T>
	concept has_size = requires (T x)
	{
		x.size();
	};

	/** This concept captures those objects that are explicitely convertible to boolean
	 *
	 *	Note: most vulkan resources contain an explicit boolean converter.
	 */
	template <class T>
	concept boolean_convertible = requires(T(&f)()) {
		static_cast<bool>(f());
	};

	/** Passes the content of aLhs to the lifetime handler and subsequently moves the content of aRhs into aLhs.
	 *
	 *  aLhs's initial content will be passed to aLifeTimeHandler
	 *  aLhs will contain the original aRhs's content after execution.
	 *  aRhs will be in an unspecified state after execution.
	 *
	 *  @param aLhs                 left hand side of the assignment - reference to be assigned to.
	 *  @param aRhs                 right hand side of the assignment
	 *  @param aLifeTimeHandler     life time handler for original aLhs content
	 */
	template<typename T, typename F>
	void assign_and_lifetime_handle_previous(T& aLhs, T&& aRhs, F&& aLifeTimeHandler)
	{
		aLifeTimeHandler(std::move(aLhs));
		aLhs = std::move(aRhs);
	}

	/** Passes the content of aLhs to the lifetime handler and subsequently moves the content of aRhs into aLhs.
	 *
	 *  aLhs's initial content will be passed to aLifeTimeHandler
	 *  aLhs will contain the original aRhs's content after execution.
	 *  aRhs will be in an unspecified state after execution.
	 *
	 *  life-time handle will not be invoked if the size of aLhs is zero.
	 *
	 *  @param aLhs                 left hand side of the assignment - reference to be assigned to.
	 *  @param aRhs                 right hand side of the assignment
	 *  @param aLifeTimeHandler     life time handler for original aLhs content
	 */
	template<typename T, typename F> requires has_size<T>
	void assign_and_lifetime_handle_previous(T& aLhs, T&& aRhs, F&& aLifeTimeHandler)
	{
		if (aLhs.size() > 0) {
			aLifeTimeHandler(std::move(aLhs));
		}
		aLhs = std::move(aRhs);
	}

	/** Passes the content of aLhs to the lifetime handler and subsequently moves the content of aRhs into aLhs.
	 *
	 *  aLhs's initial content will be passed to aLifeTimeHandler
	 *  aLhs will contain the original aRhs's content after execution.
	 *  aRhs will be in an unspecified state after execution.
	 *
	 *  life-time handle will not be invoked if aLhs contains no value.
	 *
	 *  @param aLhs                 left hand side of the assignment - reference to be assigned to.
	 *  @param aRhs                 right hand side of the assignment
	 *  @param aLifeTimeHandler     life time handler for original aLhs content
	 */
	template<typename T, typename F>
	void assign_and_lifetime_handle_previous(owning_resource<T>& aLhs, owning_resource<T>&& aRhs, F&& aLifeTimeHandler)
	{
		if (aLhs.has_value()) {
			aLifeTimeHandler(std::move(aLhs));
		}
		aLhs = std::move(aRhs);
	}

	/** Passes the content of aLhs to the lifetime handler and subsequently moves the content of aRhs into aLhs.
	 *
	 *  aLhs's initial content will be passed to aLifeTimeHandler
	 *  aLhs will contain the original aRhs's content after execution.
	 *  aRhs will be in an unspecified state after execution.
	 *
	 *  life-time handle will not be invoked if aLhs explicitely converts to false.
	 *
	 *  @param aLhs                 left hand side of the assignment - reference to be assigned to.
	 *  @param aRhs                 right hand side of the assignment
	 *  @param aLifeTimeHandler     life time handler for original aLhs content
	 */
	template<typename T, typename F> requires boolean_convertible<T>
	void assign_and_lifetime_handle_previous(T& aLhs, T&& aRhs, F&& aLifeTimeHandler)
	{
		if (!!aLhs) {
			aLifeTimeHandler(std::move(aLhs));
		}
		aLhs = std::move(aRhs);
	}

	/** Passes the content held by optional aLhs to the lifetime handler and subsequently moves the content of aRhs within aLhs.
	 *
	 *  aLhs's initial value will be passed to aLifeTimeHandler
	 *  aLhs will contain the original aRhs's content after execution.
	 *  aRhs will be in an unspecified state after execution.
	 *
	 *  life-time handle will not be invoked if aLhs contains no value.
	 *
	 *  @param aLhs                 left hand side of the assignment - reference to be assigned to.
	 *  @param aRhs                 right hand side of the assignment
	 *  @param aLifeTimeHandler     life time handler for original aLhs content
	 */
	template<typename T, typename F>
	void assign_and_lifetime_handle_previous(std::optional<T>& aLhs, T&& aRhs, F&& aLifeTimeHandler)
	{
		if (!aLhs.has_value()) {
			aLhs = std::move(aRhs);
		}
		else {
			assign_and_lifetime_handle_previous<T, F>(aLhs.value(), std::forward<T>(aRhs), std::forward<F>(aLifeTimeHandler));
		}
	}
#pragma endregion

#pragma region lambda overload
	// A "lambda overload" pattern which can be used to create one lambda for different parameters
	//
	// Example:
	//
	//  std::variant<Fluid, LightItem, HeavyItem, FragileItem> package;
	//
	//	std::visit(lambda_overload{
	//		[](Fluid&) { cout << "fluid\n"; },
	//		[](LightItem&) { cout << "light item\n"; },
	//		[](HeavyItem&) { cout << "heavy item\n"; },
	//		[](FragileItem&) { cout << "fragile\n"; }
	//	}, package);
	//
	//	All credits go to Bartlomiej Filipek: https://www.bfilipek.com/2018/09/visit-variants.html
	//
	template<class... Ts> struct lambda_overload : Ts... { using Ts::operator()...; };
	template<class... Ts> lambda_overload(Ts...) -> lambda_overload<Ts...>;
#pragma endregion

	// A concept which requires a type to have a .has_value()
	template <typename T>
	concept has_has_value = requires (T& x)
	{
		x.has_value();
	};

	// A concept which requires a type to not have a .has_value()
	template <typename T>
	concept not_has_value = !has_has_value<T>;

	// Add the resource R as to the list of resources to be lifetime-handled by object LH
	template <typename LH, typename R> requires has_has_value<R>
	void let_it_handle_lifetime_of(LH& lh, R& r)
	{
		if (r.has_value()) {
			if (r.is_shared_ownership_enabled()) {
				lh.handle_lifetime_of(r);
			}
			else {
				lh.handle_lifetime_of(std::move(r));
			}
		}
	}

	// Add the resource R as to the list of resources to be lifetime-handled by object LH
	template <typename LH, typename R> requires not_has_value<R>
	void let_it_handle_lifetime_of(LH& lh, R& r)
	{
		if (r.is_shared_ownership_enabled()) {
			lh.handle_lifetime_of(r);
		}
		else {
			lh.handle_lifetime_of(std::move(r));
		}
	}
}
