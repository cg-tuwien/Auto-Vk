#pragma once

namespace ak
{
	class usage_desc
	{
	public:
		usage_desc(usage_desc&&) noexcept = default;
		usage_desc(const usage_desc&) = default;
		//usage_desc(const usage_desc* ud) noexcept : mDescriptions{ud->mDescriptions} {};
		usage_desc& operator=(usage_desc&&) noexcept = default;
		usage_desc& operator=(const usage_desc&) = default;
		//usage_desc& operator=(const usage_desc* ud) noexcept { mDescriptions = ud->mDescriptions; return *this; };
		virtual ~usage_desc() {}
		
		usage_desc& unused()							{ mDescriptions.emplace_back(usage_type::create_unused()); return *this; }
		usage_desc& resolve_receiver()					{ return unused(); }
		usage_desc& input(int location)					{ mDescriptions.emplace_back(usage_type::create_input(location)); return *this; }
		usage_desc& color(int location)					{ mDescriptions.emplace_back(usage_type::create_color(location)); return *this; }
		usage_desc& depth_stencil()						{ mDescriptions.emplace_back(usage_type::create_depth_stencil()); return *this; }
		usage_desc& preserve()							{ mDescriptions.emplace_back(usage_type::create_preserve()); return *this; }

		usage_desc* operator->()						{ return this; }
		
		usage_desc& operator+(usage_desc additionalUsages)
		{
			assert(additionalUsages.mDescriptions.size() >= 1);
			auto& additional = additionalUsages.mDescriptions.front();
			auto& existing = mDescriptions.back();

			existing.mInput	= existing.mInput || additional.mInput;
			existing.mColor = existing.mColor || additional.mColor;
			existing.mDepthStencil = existing.mDepthStencil || additional.mDepthStencil;
			existing.mPreserve = existing.mPreserve || additional.mPreserve;
			assert(existing.mInputLocation == -1 || additional.mInputLocation == -1);
			existing.mInputLocation = std::max(existing.mInputLocation, additional.mInputLocation);
			assert(existing.mColorLocation == -1 || additional.mColorLocation == -1);
			existing.mColorLocation	= std::max(existing.mColorLocation, additional.mColorLocation);
			existing.mResolve = existing.mResolve || additional.mResolve;
			assert(existing.mResolveAttachmentIndex == -1 || additional.mResolveAttachmentIndex == -1);
			existing.mResolveAttachmentIndex = std::max(existing.mResolveAttachmentIndex, additional.mResolveAttachmentIndex);
			
			// Add the rest:
			mDescriptions.insert(mDescriptions.end(), additionalUsages.mDescriptions.begin() + 1, additionalUsages.mDescriptions.end());
			return *this;
		}

		bool contains_unused() const		{ return std::find_if(mDescriptions.begin(), mDescriptions.end(), [](const usage_type& u) { return u.as_unused(); }) != mDescriptions.end(); }
		bool contains_input() const			{ return std::find_if(mDescriptions.begin(), mDescriptions.end(), [](const usage_type& u) { return u.as_input(); }) != mDescriptions.end(); }
		bool contains_color() const			{ return std::find_if(mDescriptions.begin(), mDescriptions.end(), [](const usage_type& u) { return u.as_color(); }) != mDescriptions.end(); }
		bool contains_resolve() const		{ return std::find_if(mDescriptions.begin(), mDescriptions.end(), [](const usage_type& u) { return u.has_resolve(); }) != mDescriptions.end(); }
		bool contains_depth_stencil() const	{ return std::find_if(mDescriptions.begin(), mDescriptions.end(), [](const usage_type& u) { return u.as_depth_stencil(); }) != mDescriptions.end(); }
		bool contains_preserve() const		{ return std::find_if(mDescriptions.begin(), mDescriptions.end(), [](const usage_type& u) { return u.as_preserve(); }) != mDescriptions.end(); }

		usage_type first_color_depth_input_usage() const
		{
			auto n = mDescriptions.size();
			for (size_t i = 0; i < n; ++i) {
				if (mDescriptions[i].mColor || mDescriptions[i].mDepthStencil || mDescriptions[i].mInput) {
					return mDescriptions[i];
				}
			}
			return mDescriptions[0];
		}

		usage_type last_color_depth_input_usage() const
		{
			auto n = mDescriptions.size();
			for (size_t i = n; i > 0; --i) {
				if (mDescriptions[i-1].mColor || mDescriptions[i-1].mDepthStencil || mDescriptions[i-1].mInput) {
					return mDescriptions[i-1];
				}
			}
			return mDescriptions[n-1];
		}

		auto num_subpasses() const { return mDescriptions.size(); }
		auto get_subpass_usage(size_t subpassId) const { return mDescriptions[subpassId]; }
		//auto is_to_be_resolved_after_subpass(size_t subpassId) const { return mDescriptions[subpassId].has_resolve(); }
		//auto get_resolve_target_index(size_t subpassId) const { return mDescriptions[subpassId].mResolveAttachmentIndex; }
		//auto has_input_location_at_subpass(size_t subpassId) const { return mDescriptions[subpassId].as_input() && mDescriptions[subpassId].mInputLocation != -1; }
		//auto input_location_at_subpass(size_t subpassId) const { return mDescriptions[subpassId].mInputLocation; }
		//auto has_color_location_at_subpass(size_t subpassId) const { return mDescriptions[subpassId].as_color() && mDescriptions[subpassId].mColorLocation != -1; }
		//auto color_location_at_subpass(size_t subpassId) const { return mDescriptions[subpassId].mColorLocation; }

	protected:
		usage_desc() = default;
		std::vector<usage_type> mDescriptions;
	};

	class unused : public usage_desc
	{
	public:
		explicit unused()											{ mDescriptions.emplace_back(usage_type::create_unused()); }
		unused(unused&&) noexcept = default;
		unused(const unused&) = default;
		unused& operator=(unused&&) noexcept = default;
		unused& operator=(const unused&) = default;
	};

	using resolve_receiver = unused;
	
	class input : public usage_desc
	{
	public:
		explicit input(int location)								{ mDescriptions.emplace_back(usage_type::create_input(location)); }
		input(input&&) noexcept = default;
		input(const input&) = default;
		input& operator=(input&&) noexcept = default;
		input& operator=(const input&) = default;
	};
	
	class color : public usage_desc
	{
	public:
		explicit color(int location)								{ mDescriptions.emplace_back(usage_type::create_color(location)); }
		color(color&&) noexcept = default;
		color(const color&) = default;
		color& operator=(color&&) noexcept = default;
		color& operator=(const color&) = default;
	};

	/** Resolve this attachment and store the resolved results to another attachment at the specified index. */
	class resolve_to : public usage_desc
	{
	public:
		/**	Indicate that this attachment shall be resolved.
		 *	@param targetLocation	The index of the attachment where to resolve this attachment into.
		 */
		explicit resolve_to(int targetLocation)						{ auto& r = mDescriptions.emplace_back(usage_type::create_unused()); r.mResolve = true; r.mResolveAttachmentIndex = targetLocation; }
		resolve_to(resolve_to&&) noexcept = default;										// ^ usage_type not applicable here, but actually it *is* unused.
		resolve_to(const resolve_to&) = default;
		resolve_to& operator=(resolve_to&&) noexcept = default;
		resolve_to& operator=(const resolve_to&) = default;
	};
	
	class depth_stencil : public usage_desc
	{
	public:
		explicit depth_stencil(int location = 0)					{ mDescriptions.emplace_back(usage_type::create_depth_stencil()); }
		depth_stencil(depth_stencil&&) noexcept = default;
		depth_stencil(const depth_stencil&) = default;
		depth_stencil& operator=(depth_stencil&&) noexcept = default;
		depth_stencil& operator=(const depth_stencil&) = default;
	};
	
	class preserve : public usage_desc
	{
	public:
		explicit preserve()											{ mDescriptions.emplace_back(usage_type::create_preserve()); }
		preserve(preserve&&) noexcept = default;
		preserve(const preserve&) = default;
		preserve& operator=(preserve&&) noexcept = default;
		preserve& operator=(const preserve&) = default;
	};
}