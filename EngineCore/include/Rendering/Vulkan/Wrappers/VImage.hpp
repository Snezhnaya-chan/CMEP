#pragma once

#include "Rendering/Vulkan/ImportVulkan.hpp"

#include "HoldsVMA.hpp"
#include "HoldsVulkanDevice.hpp"
#include "framework.hpp"
#include "vulkan/vulkan_core.h"

#include <cassert>

namespace Engine::Rendering::Vulkan
{
	class VImage : public HoldsVulkanDevice, public HoldsVMA
	{
	private:
		VmaAllocationInfo allocation_info;
		VmaAllocation allocation;

	protected:
		VkImageLayout current_layout   = VK_IMAGE_LAYOUT_UNDEFINED;
		VkFormat image_format		   = VK_FORMAT_UNDEFINED;
		VkImage native_handle		   = VK_NULL_HANDLE;
		VkImageView native_view_handle = VK_NULL_HANDLE;

		VImageSize size{};

	public:
		VImage(
			VDeviceManager* with_device_manager,
			VImageSize with_size,
			VkSampleCountFlagBits num_samples,
			VkFormat format,
			VkImageUsageFlags usage,
			VkMemoryPropertyFlags properties,
			VkImageTiling with_tiling = VK_IMAGE_TILING_OPTIMAL
		);
		~VImage();

		void TransitionImageLayout(VkImageLayout new_layout);

		void AddImageView(VkImageAspectFlags with_aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT);

		[[nodiscard]] VkImage& GetNativeHandle()
		{
			assert(this->native_handle != VK_NULL_HANDLE && "This command pool has no valid native handle!");

			return this->native_handle;
		}

		[[nodiscard]] VkImageView& GetNativeViewHandle()
		{
			return this->native_view_handle;
		}

		[[nodiscard]] VImageSize GetSize() const noexcept
		{
			return this->size;
		}
	};
} // namespace Engine::Rendering::Vulkan
