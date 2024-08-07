#include "Rendering/Vulkan/Wrappers/VCommandPool.hpp"

#include "Rendering/Vulkan/VDeviceManager.hpp"
#include "Rendering/Vulkan/Wrappers/VCommandBuffer.hpp"

#include <stdexcept>

namespace Engine::Rendering::Vulkan
{
	VCommandPool::VCommandPool(VDeviceManager* const with_device_manager) : HoldsVulkanDevice(with_device_manager)
	{
		VkCommandPoolCreateInfo pool_info{};
		pool_info.sType			   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_info.flags			   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		pool_info.queueFamilyIndex = this->device_manager->GetQueueFamilies().graphics_family.value();

		VkDevice logical_device = this->device_manager->GetLogicalDevice();

		if (vkCreateCommandPool(logical_device, &pool_info, nullptr, &(this->native_handle)) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create command pool!");
		}
	}

	VCommandPool::~VCommandPool()
	{
		VkDevice logical_device = this->device_manager->GetLogicalDevice();

		vkDestroyCommandPool(logical_device, this->native_handle, nullptr);
	}

	[[nodiscard]] VCommandBuffer* VCommandPool::AllocateCommandBuffer()
	{
		return new VCommandBuffer(this->device_manager, this->native_handle);
	}

} // namespace Engine::Rendering::Vulkan
