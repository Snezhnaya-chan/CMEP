#include "Rendering/Vulkan/VulkanImage.hpp"

#include "Rendering/Vulkan/HoldsVulkanDevice.hpp"
#include "Rendering/Vulkan/VulkanCommandBuffer.hpp"
#include "Rendering/Vulkan/VulkanDeviceManager.hpp"

#include <stdexcept>

namespace Engine::Rendering
{
	VulkanImage::VulkanImage(
		VulkanDeviceManager* const with_device_manager,
		VmaAllocator with_allocator,
		VulkanImageSize size,
		VkSampleCountFlagBits num_samples,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties
	)
		: HoldsVulkanDevice(with_device_manager), HoldsVMA(with_allocator), image_format(format)
	{

		VkImageCreateInfo image_info{};
		image_info.sType		 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image_info.imageType	 = VK_IMAGE_TYPE_2D;
		image_info.extent		 = VkExtent3D{size.x, size.y, 1};
		image_info.mipLevels	 = 1;
		image_info.arrayLayers	 = 1;
		image_info.format		 = format;
		image_info.tiling		 = tiling;
		image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		image_info.usage		 = usage;
		image_info.sharingMode	 = VK_SHARING_MODE_EXCLUSIVE;
		image_info.samples		 = num_samples;
		image_info.flags		 = 0; // Optional

		VmaAllocationCreateInfo vma_alloc_info{};
		vma_alloc_info.usage		 = VMA_MEMORY_USAGE_AUTO;
		vma_alloc_info.flags		 = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
		// //VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
		// | VMA_ALLOCATION_CREATE_MAPPED_BIT;
		vma_alloc_info.requiredFlags = properties;

		if (vmaCreateImage(
				this->allocator,
				&image_info,
				&vma_alloc_info,
				&(this->image),
				&(this->allocation),
				&(this->allocation_info)
			) != VK_SUCCESS)
		{
			// this->logger->SimpleLog(Logging::LogLevel::Exception, LOGPFX_CURRENT "Failed to create image");
			throw std::runtime_error("Failed to create VulkanImage!");
		}

		vmaSetAllocationName(this->allocator, this->allocation, "VulkanImage");
	}

	VulkanImage::~VulkanImage()
	{
		if (this->image_view != nullptr)
		{
			vkDestroyImageView(this->device_manager->GetLogicalDevice(), this->image_view, nullptr);
		}

		vkDestroyImage(this->device_manager->GetLogicalDevice(), this->image, nullptr);

		vmaFreeMemory(this->allocator, this->allocation);
	}

	void VulkanImage::TransitionImageLayout(VulkanCommandPool* with_pool, VkFormat format, VkImageLayout new_layout)
	{
		VkImageMemoryBarrier barrier{};
		barrier.sType							= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout						= this->current_layout;
		barrier.newLayout						= new_layout;
		barrier.srcQueueFamilyIndex				= VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex				= VK_QUEUE_FAMILY_IGNORED;
		barrier.image							= this->image;
		barrier.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel	= 0;
		barrier.subresourceRange.levelCount		= 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount		= 1;

		VkPipelineStageFlags source_stage;
		VkPipelineStageFlags destination_stage;

		if (this->current_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			source_stage	  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (this->current_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
				 new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			source_stage	  = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else
		{
			throw std::invalid_argument("Unsupported layout transition!");
		}

		Rendering::VulkanCommandBuffer(this->device_manager, with_pool)
			.RecordCmds([&](VulkanCommandBuffer* with_buffer) {
				vkCmdPipelineBarrier(
					with_buffer->GetNativeHandle(),
					source_stage,
					destination_stage,
					0,
					0,
					nullptr,
					0,
					nullptr,
					1,
					&barrier
				);
			});

		this->current_layout = new_layout;
	}

	void VulkanImage::AddImageView(VkImageAspectFlags with_aspect_flags)
	{
		VkImageViewCreateInfo view_info{};
		view_info.sType							  = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_info.image							  = image;
		view_info.viewType						  = VK_IMAGE_VIEW_TYPE_2D;
		view_info.format						  = this->image_format;
		view_info.subresourceRange.aspectMask	  = with_aspect_flags;
		view_info.subresourceRange.baseMipLevel	  = 0;
		view_info.subresourceRange.levelCount	  = 1;
		view_info.subresourceRange.baseArrayLayer = 0;
		view_info.subresourceRange.layerCount	  = 1;

		if (vkCreateImageView(this->device_manager->GetLogicalDevice(), &view_info, nullptr, &this->image_view) !=
			VK_SUCCESS)
		{
			throw std::runtime_error("failed to create texture image view!");
		}
	}
} // namespace Engine::Rendering