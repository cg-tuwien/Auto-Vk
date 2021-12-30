#pragma once
#include <avk/avk.hpp>

namespace avk
{
	namespace image_layout
	{
		struct image_layout2
		{
			vk::ImageLayout mOld;
			vk::ImageLayout mNew;
		};

		struct image_layout
		{
			vk::ImageLayout mLayout;
		};

		inline image_layout2 operator>> (image_layout a, image_layout b)
		{
			return image_layout2{ a.mLayout, b.mLayout };
		}

        static constexpr auto undefined                                  = image_layout{ vk::ImageLayout::eUndefined };
        static constexpr auto general                                    = image_layout{ vk::ImageLayout::eGeneral };
        static constexpr auto color_attachment_optimal                   = image_layout{ vk::ImageLayout::eColorAttachmentOptimal };
        static constexpr auto depth_stencil_attachment_optimal           = image_layout{ vk::ImageLayout::eDepthStencilAttachmentOptimal };
        static constexpr auto depth_stencil_read_only_optimal            = image_layout{ vk::ImageLayout::eDepthStencilReadOnlyOptimal };
        static constexpr auto shader_read_only_optimal                   = image_layout{ vk::ImageLayout::eShaderReadOnlyOptimal };
        static constexpr auto transfer_src                               = image_layout{ vk::ImageLayout::eTransferSrcOptimal };
        static constexpr auto transfer_dst                               = image_layout{ vk::ImageLayout::eTransferDstOptimal };
        static constexpr auto preinitialized                             = image_layout{ vk::ImageLayout::ePreinitialized };
        static constexpr auto depth_read_only_stencil_attachment_optimal = image_layout{ vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal };
        static constexpr auto depth_attachment_stencil_read_only_optimal = image_layout{ vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal };
        static constexpr auto depth_attachment_optimal                   = image_layout{ vk::ImageLayout::eDepthAttachmentOptimal };
        static constexpr auto depth_read_only_optimal                    = image_layout{ vk::ImageLayout::eDepthReadOnlyOptimal };
        static constexpr auto stencil_attachment_optimal                 = image_layout{ vk::ImageLayout::eStencilAttachmentOptimal };
        static constexpr auto stencil_read_only_optimal                  = image_layout{ vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal };
        static constexpr auto present_src                                = image_layout{ vk::ImageLayout::ePresentSrcKHR };
        static constexpr auto video_decode_dst                           = image_layout{ vk::ImageLayout::eVideoDecodeDstKHR };
        static constexpr auto video_decode_src                           = image_layout{ vk::ImageLayout::eVideoDecodeSrcKHR };
        static constexpr auto video_decode_dpb                           = image_layout{ vk::ImageLayout::eVideoDecodeDpbKHR };
        static constexpr auto shared_present                             = image_layout{ vk::ImageLayout::eSharedPresentKHR };
        static constexpr auto fragment_density_map_optimal               = image_layout{ vk::ImageLayout::eFragmentDensityMapOptimalEXT };
        static constexpr auto fragment_shading_rate_attachment_optimal   = image_layout{ vk::ImageLayout::eFragmentShadingRateAttachmentOptimalKHR };
        static constexpr auto video_encode_dst                           = image_layout{ vk::ImageLayout::eVideoEncodeDstKHR };
        static constexpr auto video_encode_src                           = image_layout{ vk::ImageLayout::eVideoEncodeSrcKHR };
        static constexpr auto video_encode_dpb                           = image_layout{ vk::ImageLayout::eVideoEncodeDpbKHR };
        static constexpr auto read_only_optimal                          = image_layout{ vk::ImageLayout::eReadOnlyOptimalKHR };
        static constexpr auto attachment_optimal                         = image_layout{ vk::ImageLayout::eAttachmentOptimalKHR };
	}
}
