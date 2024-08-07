#pragma once

#include "Rendering/Vulkan/Wrappers/HoldsVulkanDevice.hpp"
#include "Rendering/Vulkan/Wrappers/framework.hpp"

#include "vulkan/vulkan_core.h"

#include <functional>

namespace Engine::Rendering::Vulkan
{
	class VCommandBuffer final : public HoldsVulkanDevice
	{
	private:
		VkCommandPool owning_pool	  = nullptr;
		VkCommandBuffer native_handle = VK_NULL_HANDLE;

	public:
		VCommandBuffer(VDeviceManager* with_device_manager, VkCommandPool from_pool);
		~VCommandBuffer();

		void BeginCmdBuffer(VkCommandBufferUsageFlags usage_flags);
		void EndCmdBuffer();

		void ResetBuffer();

		// TODO: Remove this in the future
		void RecordCmds(std::function<void(VCommandBuffer*)> const& lambda);

		void QueueSubmit(VkQueue to_queue);

		void BufferBufferCopy(VBuffer* from_buffer, VBuffer* to_buffer, std::vector<VkBufferCopy> regions);
		void BufferImageCopy(VBuffer* from_buffer, VImage* to_image);

		[[nodiscard]] VkCommandBuffer& GetNativeHandle()
		{
			return this->native_handle;
		}

		void BeginRenderPass(const VkRenderPassBeginInfo* with_info);
		void EndRenderPass();
	};
} // namespace Engine::Rendering::Vulkan
