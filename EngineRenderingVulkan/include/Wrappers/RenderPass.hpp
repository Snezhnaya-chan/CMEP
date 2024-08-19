#pragma once

#include "ImportVulkan.hpp"
#include "InstanceOwned.hpp"
#include "framework.hpp"

namespace Engine::Rendering::Vulkan
{
	struct RenderPass final : public InstanceOwned
	{
		VkRenderPass native_handle;

		RenderPass(InstanceOwned::value_t with_instance, VkFormat with_format);
		~RenderPass();
	};
} // namespace Engine::Rendering::Vulkan
