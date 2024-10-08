#include "backend/MemoryAllocator.hpp"

#include "backend/Instance.hpp"

namespace Engine::Rendering::Vulkan
{
	MemoryAllocator::MemoryAllocator(Instance* with_instance, vk::raii::Device& with_device)
	{
		VmaAllocatorCreateInfo allocator_create_info = {};
		allocator_create_info.vulkanApiVersion		 = vk::ApiVersion12;
		allocator_create_info.physicalDevice = **with_instance->getPhysicalDevice();
		allocator_create_info.device		 = *with_device;
		allocator_create_info.instance		 = *with_instance->getHandle();

		vmaCreateAllocator(&allocator_create_info, &native_handle);
	}

	MemoryAllocator::~MemoryAllocator()
	{
		vmaDestroyAllocator(native_handle);
	}
} // namespace Engine::Rendering::Vulkan
