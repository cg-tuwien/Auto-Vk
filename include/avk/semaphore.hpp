#pragma once
#include "avk/avk.hpp"

namespace avk
{
	// Forward declaration:
	class queue;

	struct semaphore_value_info;

	/** A synchronization object which allows GPU->GPU synchronization */
	class semaphore_t
	{
		friend class root;
		
	public:
		semaphore_t();
		semaphore_t(semaphore_t&&) noexcept = default;
		semaphore_t(const semaphore_t&) = delete;
		semaphore_t& operator=(semaphore_t&&) noexcept = default;
		semaphore_t& operator=(const semaphore_t&) = delete;
		~semaphore_t();

		/** Set a custom deleter function.
		 *	This is often used for resource cleanup, e.g. a buffer which can be deleted when this semaphore is destroyed.
		 */
		template <typename F>
		semaphore_t& set_custom_deleter(F&& aDeleter) noexcept
		{
			if (mCustomDeleter.has_value()) {
				// There is already a custom deleter! Make sure that this stays alive as well.
				mCustomDeleter = [
					existingDeleter = std::move(mCustomDeleter.value()),
					additionalDeleter = std::forward<F>(aDeleter)
				]() {
					additionalDeleter();
					existingDeleter();
				};
			}
			else {
				mCustomDeleter = std::forward<F>(aDeleter);
			}
			return *this;
		}

		semaphore_t& handle_lifetime_of(any_owning_resource_t aResource, uint64_t aDeleteResourceAtValue = std::numeric_limits<uint64_t>::max());

		const auto& create_info() const	{ return mCreateInfo; }
		auto& create_info()				{ return mCreateInfo; }
		const auto& handle() const { return mSemaphore.get(); }
		const auto* handle_addr() const { return &mSemaphore.get(); }

		// timeline semaphore specific functions

		/** @brief Destroys outdated resources which are handled by this timeline semaphore.
		 */
		void cleanup_expired_resources();

		/** @brief returns the current value of the timeline semaphore */
		const uint64_t query_current_value() const;

		/** @brief sets the timeline semaphore to the specified value */
		void signal(uint64_t aNewValue) const;

		/** @brief Waits on host until the timiline semaphore reaches the given value or the timeout(in nanoseconds) happens.
		 *  @return Value of type vk::Result containing information about whether the wait operation succeeded, or the timeout has been triggered.
		 */
		void wait_until_signaled(uint64_t aSignalValue, std::optional<uint64_t> aTimeout = {}) const;

	private:
		// The semaphore config struct:
		vk::SemaphoreCreateInfo mCreateInfo;
		// The semaphore handle:
		vk::UniqueHandle<vk::Semaphore, DISPATCH_LOADER_CORE_TYPE> mSemaphore;

		/** A custom deleter function called upon destruction of this semaphore */
		std::optional<avk::unique_function<void()>> mCustomDeleter;

		std::forward_list<std::tuple<any_owning_resource_t, uint64_t>> mLifetimeHandledResources;
	};

	/**
	 * @brief This is an explicit template specialization of owning_resource as defined in cpp_utils.hpp
	 * It is a complete duplicate of the class that contains an additional overload for operator= (defined at the bottom).
	 *
	 * The definition of an explicit template specialization is required since operator= overloads cannot be defined as non-member functions.
	 * (see https://en.cppreference.com/w/cpp/language/operators)
	 * As far as I can tell, defining an explicit template specialization for a class requires duplicating the entire class even if only minor changes are made.
	 * 
	 *
	 * The overload allows instantiating semaphore_value_info structs with the expression `sem = val`.
	 * Where the type of sem is `owning_resource<semaphore_t>` and the type of val is `uint64_t`.
	 */
	template<>
	class owning_resource<semaphore_t> : public std::variant<std::monostate, semaphore_t, std::shared_ptr<semaphore_t>>
	{
	private:
		using T = semaphore_t;
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

		[[nodiscard]] const T& as_reference() const
		{
			return get();
		}

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

		semaphore_value_info operator=(uint64_t aValue);
	};


	// Typedef for a variable representing an owner of a semaphore
	using semaphore = avk::owning_resource<semaphore_t>;

	/**
	 * @brief This is an explicit template specialization of resource_argument as defined in cpp_utils.hpp
	 * It is a complete duplicate of the class that contains an additional overload for operator= (defined at the bottom).
	 * 
	 * The definition of an explicit template specialization is required since operator= overloads cannot be defined as non-member functions.
	 * (see https://en.cppreference.com/w/cpp/language/operators)
	 * As far as I can tell, defining an explicit template specialization for a class requires duplicating the entire class even if only minor changes are made.
	 * 
	 * The overload allows instantiating semaphore_value_info structs with the expression `sem = val`.
	 * Where the type of sem is `resource_argument<semaphore_t>` and the type of val is `uint64_t`.
	 */
	template<>
	class resource_argument<semaphore_t> : public std::variant<std::reference_wrapper<semaphore_t>, std::reference_wrapper<const semaphore_t>, owning_resource<semaphore_t>>
	{
	private:
		using T = semaphore_t;
	public:
		// The type of the resource:
		using value_type = T;

		// Cast the this-pointer to what it is: a std::variant-pointer (and to const)
		const std::variant<std::reference_wrapper<T>, std::reference_wrapper<const T>, owning_resource<T>>* this_as_variant() const noexcept
		{
			return static_cast<const std::variant<std::reference_wrapper<T>, std::reference_wrapper<const T>, owning_resource<T>>*>(this);
		}

		// Cast the this-pointer to what it is: a std::variant-pointer
		std::variant<std::reference_wrapper<T>, std::reference_wrapper<const T>, owning_resource<T>>* this_as_variant() noexcept
		{
			return static_cast<std::variant<std::reference_wrapper<T>, std::reference_wrapper<const T>, owning_resource<T>>*>(this);
		}

		resource_argument(const T& aResource) noexcept
			: std::variant<std::reference_wrapper<T>, std::reference_wrapper<const T>, owning_resource<T>>{ std::cref(aResource) }
		{ }

		resource_argument(T& aResource) noexcept
			: std::variant<std::reference_wrapper<T>, std::reference_wrapper<const T>, owning_resource<T>>{ std::ref(aResource) }
		{ }

		resource_argument(owning_resource<T> aResource) noexcept
			: std::variant<std::reference_wrapper<T>, std::reference_wrapper<const T>, owning_resource<T>>{ std::move(aResource) }
		{ }

		resource_argument(resource_argument&&) noexcept = default;
		resource_argument(const resource_argument& aOther) = default;
		resource_argument& operator=(resource_argument&&) noexcept = default;
		resource_argument& operator=(const resource_argument&) = default;
		~resource_argument() = default;

		bool is_ownership() const {
			return std::holds_alternative<owning_resource<T>>(*this_as_variant());
		}

		bool is_reference() const {
			return !is_ownership();
		}

		// Get a reference to the owned resource T
		T& get()
		{
			if (is_ownership()) {
				return std::get<owning_resource<T>>(*this_as_variant()).get();
			}
			if (std::holds_alternative<std::reference_wrapper<T>>(*this_as_variant())) {
				return std::get<std::reference_wrapper<T>>(*this_as_variant());
			}
			else {
				throw avk::logic_error("The resource of type '" + std::string(typeid(T).name()) + "' is stored as const reference. Cannot return it as non-const reference. Use ::get_const_reference instead!");
			}
		}

		// Get a reference to the owned resource T
		const T& get_const_reference() const
		{
			if (is_ownership()) {
				return std::get<owning_resource<T>>(*this_as_variant()).get();
			}
			if (std::holds_alternative<std::reference_wrapper<T>>(*this_as_variant())) {
				return std::get<std::reference_wrapper<T>>(*this_as_variant());
			}
			else {
				assert(std::holds_alternative<std::reference_wrapper<const T>>(*this_as_variant()));
				return std::get<std::reference_wrapper<const T>>(*this_as_variant());
			}
		}

		// Access the resource by returning a const reference to it
		const T& operator*() const
		{
			return get_const_reference();
		}

		// Access the resource by returning its address to const
		const T* operator->() const
		{
			return &get_const_reference();
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

		semaphore_value_info operator=(uint64_t aValue);
	};
}
