#pragma once
#include <ak/ak.hpp>

namespace ak
{
	struct usage_type
	{
		static usage_type create_unused()			{ return { false, false, false, false,  -1,  -1, false, -1 }; }
		static usage_type create_input(int loc)		{ return { true , false, false, false, loc,  -1, false, -1 }; }
		static usage_type create_color(int loc)		{ return { false, true , false, false,  -1, loc, false, -1 }; }
		static usage_type create_depth_stencil()	{ return { false, false, true , false,  -1,  -1, false, -1 }; }
		static usage_type create_preserve()			{ return { false, false, false, true ,  -1,  -1, false, -1 }; }
		
		bool as_unused() const { return !(mInput || mColor || mDepthStencil || mPreserve); }
		bool as_input() const { return mInput; }
		bool as_color() const { return mColor; }
		bool as_depth_stencil() const { return mDepthStencil; }
		bool as_preserve() const { return mPreserve; }

		bool has_input_location() const { return -1 != mInputLocation; }
		bool has_color_location() const { return -1 != mColorLocation; }
		auto input_location() const { return mInputLocation; }
		auto color_location() const { return mColorLocation; }
		bool has_resolve() const { return mResolve; }
		auto resolve_target_index() const { return mResolveAttachmentIndex; }
		
		bool mInput;
		bool mColor;
		bool mDepthStencil;
		bool mPreserve;
		int mInputLocation;
		int mColorLocation;
		bool mResolve;
		int mResolveAttachmentIndex;
	};
}
