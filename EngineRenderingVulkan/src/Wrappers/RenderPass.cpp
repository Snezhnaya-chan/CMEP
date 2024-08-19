#include "Wrappers/RenderPass.hpp"

#include "ImportVulkan.hpp"
#include "Wrappers/Instance.hpp"
#include "Wrappers/InstanceOwned.hpp"
#include "Wrappers/LogicalDevice.hpp"
#include "vulkan/vulkan_core.h"

#include <array>
#include <stdexcept>

namespace Engine::Rendering::Vulkan
{
	RenderPass::RenderPass(InstanceOwned::value_t with_instance, vk::Format with_format) : InstanceOwned(with_instance)
	{
		const auto& physical_device = instance->GetPhysicalDevice();

		VkAttachmentDescription color_attachment{};
		color_attachment.format			= static_cast<VkFormat>(with_format);
		color_attachment.samples		= static_cast<VkSampleCountFlagBits>(instance->GetMSAASamples());
		color_attachment.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp		= VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout	= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depth_attachment{};
		depth_attachment.format			= static_cast<VkFormat>(physical_device.FindSupportedDepthFormat());
		depth_attachment.samples		= static_cast<VkSampleCountFlagBits>(instance->GetMSAASamples());
		depth_attachment.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attachment.storeOp		= VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
		depth_attachment.finalLayout	= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription color_attachment_resolve{};
		color_attachment_resolve.format			= static_cast<VkFormat>(with_format);
		color_attachment_resolve.samples		= VK_SAMPLE_COUNT_1_BIT;
		color_attachment_resolve.loadOp			= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment_resolve.storeOp		= VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment_resolve.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment_resolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment_resolve.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment_resolve.finalLayout	= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference color_attachment_ref{};
		color_attachment_ref.attachment = 0;
		color_attachment_ref.layout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depth_attachment_ref{};
		depth_attachment_ref.attachment = 1;
		depth_attachment_ref.layout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference color_attachment_resolve_ref{};
		color_attachment_resolve_ref.attachment = 2;
		color_attachment_resolve_ref.layout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint		= VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount	= 1;
		subpass.pColorAttachments		= &color_attachment_ref;
		subpass.pDepthStencilAttachment = &depth_attachment_ref;
		subpass.pResolveAttachments		= &color_attachment_resolve_ref;

		std::array<VkAttachmentDescription, 3> attachments =
			{color_attachment, depth_attachment, color_attachment_resolve};

		VkSubpassDependency dependency{};
		dependency.srcSubpass	= VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass	= 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
								  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask	 = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
								  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo render_pass_info{};
		render_pass_info.sType			 = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		render_pass_info.pAttachments	 = attachments.data();
		render_pass_info.subpassCount	 = 1;
		render_pass_info.pSubpasses		 = &subpass;
		render_pass_info.dependencyCount = 1;
		render_pass_info.pDependencies	 = &dependency;

		LogicalDevice* logical_device = instance->GetLogicalDevice();

		if (vkCreateRenderPass(logical_device->GetHandle(), &render_pass_info, nullptr, &this->native_handle) !=
			VK_SUCCESS)
		{
			throw std::runtime_error("failed to create render pass!");
		}
	}

	RenderPass::~RenderPass()
	{
		LogicalDevice* logical_device = instance->GetLogicalDevice();

		vkDestroyRenderPass(logical_device->GetHandle(), this->native_handle, nullptr);
	}
} // namespace Engine::Rendering::Vulkan
