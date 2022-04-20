#pragma once
#include <avk/avk.hpp>

namespace avk
{
	struct subpass_usage_type
	{
		static subpass_usage_type create_unused()			{ return { false, false, false, false,  -1,  -1, false, -1, {} }; }
		static subpass_usage_type create_input(int loc)		{ return { true , false, false, false, loc,  -1, false, -1, {} }; }
		static subpass_usage_type create_color(int loc)		{ return { false, true , false, false,  -1, loc, false, -1, {} }; }
		static subpass_usage_type create_depth_stencil()	{ return { false, false, true , false,  -1,  -1, false, -1, {} }; }
		static subpass_usage_type create_preserve()			{ return { false, false, false, true ,  -1,  -1, false, -1, {} }; }
		
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

		/**	Declare which aspect of the image shall be used.
		 *	This can be handy with depth_stencil attachments, where it can make sense to set the
		 *	aspect to either vk::ImageAspectFlagBits::eDepth or vk::ImageAspectFlagBits::eStencil.
		 */
		subpass_usage_type& use_aspect(vk::ImageAspectFlags aAspect)
		{
			mAspectToBeUsed = aAspect;
			return *this;
		}
		
		/**	Declare the resolve mode for a the depth aspect of a depth_stencil attachment.
		 */
		subpass_usage_type& set_depth_resolve_mode(vk::ResolveModeFlagBits aResolveMode)
		{
			mResolveModeDepth = aResolveMode;
			return *this;
		}

		/**	Declare the resolve mode for a the stencil aspect of a depth_stencil attachment.
		 */
		subpass_usage_type& set_stencil_resolve_mode(vk::ResolveModeFlagBits aResolveMode)
		{
			mResolveModeStencil = aResolveMode;
			return *this;
		}

		bool mInput;
		bool mColor;
		bool mDepthStencil;
		bool mPreserve;
		int mInputLocation;
		int mColorLocation;
		bool mResolve;
		int mResolveAttachmentIndex;

		vk::ImageAspectFlags mAspectToBeUsed;
		vk::ResolveModeFlags mResolveModeDepth;
		vk::ResolveModeFlags mResolveModeStencil;
	};
}
