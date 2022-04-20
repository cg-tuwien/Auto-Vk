#pragma once
#include <avk/avk.hpp>

namespace avk
{
	namespace usage {}

	class subpass_usages
	{
		friend subpass_usages operator>> (subpass_usages a, subpass_usages b);
		friend subpass_usages operator+  (subpass_usages existingUsages, subpass_usages additionalUsages);

	public:
		explicit subpass_usages(subpass_usage_type aUsage) { mDescriptions.emplace_back(std::move(aUsage)); }
		subpass_usages(subpass_usages&&) noexcept = default;
		subpass_usages(const subpass_usages&) = default;
		//subpass_usages(const subpass_usages* ud) noexcept : mDescriptions{ud->mDescriptions} {};
		subpass_usages& operator=(subpass_usages&&) noexcept = default;
		subpass_usages& operator=(const subpass_usages&) = default;
		//subpass_usages& operator=(const subpass_usages* ud) noexcept { mDescriptions = ud->mDescriptions; return *this; };
		virtual ~subpass_usages() = default;

		[[nodiscard]] bool contains_unused() const			{ return std::ranges::find_if(mDescriptions.begin(), mDescriptions.end(), [](const subpass_usage_type& u) { return u.as_unused(); }) != mDescriptions.end(); }
		[[nodiscard]] bool contains_input() const			{ return std::ranges::find_if(mDescriptions.begin(), mDescriptions.end(), [](const subpass_usage_type& u) { return u.as_input(); }) != mDescriptions.end(); }
		[[nodiscard]] bool contains_color() const			{ return std::ranges::find_if(mDescriptions.begin(), mDescriptions.end(), [](const subpass_usage_type& u) { return u.as_color(); }) != mDescriptions.end(); }
		[[nodiscard]] bool contains_resolve() const			{ return std::ranges::find_if(mDescriptions.begin(), mDescriptions.end(), [](const subpass_usage_type& u) { return u.has_resolve(); }) != mDescriptions.end(); }
		[[nodiscard]] bool contains_depth_stencil() const	{ return std::ranges::find_if(mDescriptions.begin(), mDescriptions.end(), [](const subpass_usage_type& u) { return u.as_depth_stencil(); }) != mDescriptions.end(); }
		[[nodiscard]] bool contains_preserve() const		{ return std::ranges::find_if(mDescriptions.begin(), mDescriptions.end(), [](const subpass_usage_type& u) { return u.as_preserve(); }) != mDescriptions.end(); }

		[[nodiscard]] subpass_usage_type first_color_depth_input_usage() const
		{
			auto n = mDescriptions.size();
			for (size_t i = 0; i < n; ++i) {
				if (mDescriptions[i].mColor || mDescriptions[i].mDepthStencil || mDescriptions[i].mInput) {
					return mDescriptions[i];
				}
			}
			return mDescriptions[0];
		}

		[[nodiscard]] subpass_usage_type last_color_depth_input_usage() const
		{
			auto n = mDescriptions.size();
			for (size_t i = n; i > 0; --i) {
				if (mDescriptions[i-1].mColor || mDescriptions[i-1].mDepthStencil || mDescriptions[i-1].mInput) {
					return mDescriptions[i-1];
				}
			}
			return mDescriptions[n-1];
		}

		[[nodiscard]] auto num_subpasses() const { return mDescriptions.size(); }
		[[nodiscard]] auto get_subpass_usage(size_t subpassId) const { return mDescriptions[subpassId]; }
		//auto is_to_be_resolved_after_subpass(size_t subpassId) const { return mDescriptions[subpassId].has_resolve(); }
		//auto get_resolve_target_index(size_t subpassId) const { return mDescriptions[subpassId].mResolveAttachmentIndex; }
		//auto has_input_location_at_subpass(size_t subpassId) const { return mDescriptions[subpassId].as_input() && mDescriptions[subpassId].mInputLocation != -1; }
		//auto input_location_at_subpass(size_t subpassId) const { return mDescriptions[subpassId].mInputLocation; }
		//auto has_color_location_at_subpass(size_t subpassId) const { return mDescriptions[subpassId].as_color() && mDescriptions[subpassId].mColorLocation != -1; }
		//auto color_location_at_subpass(size_t subpassId) const { return mDescriptions[subpassId].mColorLocation; }

	protected:
		subpass_usages() = default;
		std::vector<subpass_usage_type> mDescriptions;
	};

	/**	To define an multiple subpass usages, use operator>> with multiple subpass_usages values!
	 *	There are multiple such subpass_usages structs prepared in the avk::usage namespace.
	 *
	 *	Example:
	 *	  // Declare that an attachment is first used as color attachment at locaton 0, then preserved, then as input attachment at location 5:
	 *	  avk::usage::color(0) >> avk::usage::preserve() >> avk::usage::input(5)
	 */
	inline subpass_usages operator>> (subpass_usages a, subpass_usages b)
	{
		a.mDescriptions.insert(
			std::end(a.mDescriptions),
			std::make_move_iterator(std::begin(b.mDescriptions)), std::make_move_iterator(std::end(b.mDescriptions))
		);
		return a;
	}

	inline subpass_usages operator+(subpass_usages existingUsages, subpass_usages additionalUsages)
	{
		assert(!additionalUsages.mDescriptions.empty());
		const auto& additional = additionalUsages.mDescriptions.front();
		auto& existing = existingUsages.mDescriptions.back();

		existing.mInput = existing.mInput || additional.mInput;
		existing.mColor = existing.mColor || additional.mColor;
		existing.mDepthStencil = existing.mDepthStencil || additional.mDepthStencil;
		existing.mPreserve = existing.mPreserve || additional.mPreserve;
		assert(existing.mInputLocation == -1 || additional.mInputLocation == -1);
		existing.mInputLocation = std::max(existing.mInputLocation, additional.mInputLocation);
		assert(existing.mColorLocation == -1 || additional.mColorLocation == -1);
		existing.mColorLocation = std::max(existing.mColorLocation, additional.mColorLocation);
		existing.mResolve = existing.mResolve || additional.mResolve;
		assert(existing.mResolveAttachmentIndex == -1 || additional.mResolveAttachmentIndex == -1);
		existing.mResolveAttachmentIndex = std::max(existing.mResolveAttachmentIndex, additional.mResolveAttachmentIndex);
		existing.mAspectToBeUsed = existing.mAspectToBeUsed | additional.mAspectToBeUsed;

		if (static_cast<bool>(existing.mResolveModeDepth) && static_cast<bool>(additional.mResolveModeDepth)) {
			AVK_LOG_WARNING(std::string("Ambiguous depth resolve modes (") 
				+ vk::to_string(existing.mResolveModeDepth)
				+ " vs "
				+ vk::to_string(additional.mResolveModeDepth) 
				+ " in operator+(subpass_usages, subpass_usages). Don't know which one shall be used. Specify the depth resolve mode only on ONE side of operator+!"
			);
		}
		else {
			existing.mResolveModeDepth = static_cast<bool>(existing.mResolveModeDepth)
				? existing.mResolveModeDepth
				: additional.mResolveModeDepth;
		}

		if (static_cast<bool>(existing.mResolveModeStencil) && static_cast<bool>(additional.mResolveModeStencil)) {
			AVK_LOG_WARNING(std::string("Ambiguous stencil resolve modes (")
				+ vk::to_string(existing.mResolveModeStencil)
				+ " vs "
				+ vk::to_string(additional.mResolveModeStencil)
				+ " in operator+(subpass_usages, subpass_usages). Don't know which one shall be used. Specify the stencil resolve mode only on ONE side of operator+!"
			);
		}
		else {
			existing.mResolveModeStencil = static_cast<bool>(existing.mResolveModeStencil)
				? existing.mResolveModeStencil
				: additional.mResolveModeStencil;
		}

		// Sanity checks:
		if (static_cast<bool>(existing.mResolveModeDepth) && !existing.as_depth_stencil()) {
			AVK_LOG_WARNING("Sanity check failed in operator+(subpass_usages, subpass_usages): Attachment is NOT used as depth_stencil attachment, but a depth resolve mode has been specified for it.");
		}
		if (static_cast<bool>(existing.mResolveModeStencil) && !existing.as_depth_stencil()) {
			AVK_LOG_WARNING("Sanity check failed in operator+(subpass_usages, subpass_usages): Attachment is NOT used as depth_stencil attachment, but a stencil resolve mode has been specified for it.");
		}

		// Add the rest:
		existingUsages.mDescriptions.insert(
			std::end(existingUsages.mDescriptions),
			std::make_move_iterator(std::begin(additionalUsages.mDescriptions) + 1), std::make_move_iterator(std::end(additionalUsages.mDescriptions))
		);
		return existingUsages;
	}

	namespace usage
	{
		/**	Declare that this attachment is unused in the current sub pass.
		 *	Note: This is the appropriate usage for resolve targets (i.e. attachments which RECEIVE the resolved results)
		 *	      in a certain sub pass. See also usage::resolve_to(int) for further information!
		 */
		static const auto unused = subpass_usages{ subpass_usage_type::create_unused() };

		/**	Declare that this attachment is used as input attachment in the current sub pass.
		 *	@param	location	Location specifies the binding location for this attachment.
		 *						Example usage in GLSL shaders: layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput inColor;
		 */
		inline static auto input(int location) { return subpass_usages{ subpass_usage_type::create_input(location) }; }

		/**	Declare that this attachment is used as color attachment in the current sub pass.
		 *	@param	location	Location specifies the binding location for this attachment.
		 *						Example usage in GLSL shaders: layout (location = 0) out vec4 outColor;
		 */
		inline static auto color(int location) { return subpass_usages{ subpass_usage_type::create_color(location) }; }

		/** Declare that this attachment's contents shall be resolved into another attachment at the end of the current sub pass.
		 *	@param	targetLocation	The index of the attachment where to resolve this attachment into.
		 *	                        Note: In contrast to input(int) and color(int), this parameter does NOT refer to a binding location.
		 *							      Instead, this parameter refers to the attachment index within its associated renderpass.
		 *							Note: The appropriate usage declaration for the resolve target attachment is usage::unused.
		 */
		inline static auto resolve_to(int targetLocation)
		{
			auto r = subpass_usage_type::create_unused(); // <-- subpass_usage_type not applicable here, but actually it *is* unused.
			r.mResolve = true;
			r.mResolveAttachmentIndex = targetLocation;
			return subpass_usages{ r };
		}

		/**	Declare that this attachment is used as (the one and only) depth stencil attachment for the current sub pass.
		 */
		static const auto depth_stencil = subpass_usages{ subpass_usage_type::create_depth_stencil() };

		/**	Declare that the contents of this attachment shall be preserved during the current sub pass,
		 *	but it is not used otherwise in the CURRENT sub pass (but possibly in future sub passes).
		 */
		static const auto preserve = subpass_usages{ subpass_usage_type::create_preserve() };
	}
}
