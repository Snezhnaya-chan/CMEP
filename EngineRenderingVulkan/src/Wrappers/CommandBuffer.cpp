#include "Wrappers/CommandBuffer.hpp"

#include "ImportVulkan.hpp"
#include "Wrappers/Buffer.hpp"
#include "Wrappers/Image.hpp"
#include "Wrappers/Instance.hpp"
#include "Wrappers/InstanceOwned.hpp"
#include "Wrappers/LogicalDevice.hpp"

namespace Engine::Rendering::Vulkan
{
	CommandBuffer::CommandBuffer(InstanceOwned::value_t with_instance, vk::CommandPool from_pool)
		: InstanceOwned(with_instance), owning_pool(from_pool)
	{
		vk::CommandBufferAllocateInfo alloc_info(from_pool, vk::CommandBufferLevel::ePrimary, 1, {});

		LogicalDevice* logical_device = instance->GetLogicalDevice();

		native_handle = logical_device->GetHandle().allocateCommandBuffers(alloc_info)[0];
	}

	CommandBuffer::~CommandBuffer()
	{
		LogicalDevice* logical_device = instance->GetLogicalDevice();

		logical_device->GetHandle().freeCommandBuffers(owning_pool, native_handle);
	}

	void CommandBuffer::BeginCmdBuffer(VkCommandBufferUsageFlags usage_flags)
	{
		ResetBuffer();

		VkCommandBufferBeginInfo begin_info{};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = usage_flags; // VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(native_handle, &begin_info);
	}

	void CommandBuffer::EndCmdBuffer()
	{
		vkEndCommandBuffer(native_handle);
	}

	void CommandBuffer::ResetBuffer(VkCommandBufferResetFlags flags)
	{
		vkResetCommandBuffer(native_handle, flags);
	}

	void CommandBuffer::RecordCmds(std::function<void(CommandBuffer*)> const& lambda)
	{
		LogicalDevice* device = instance->GetLogicalDevice();

		// TODO: Check if we should really pass non-zero here
		BeginCmdBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		// TODO: Don't pass this here
		lambda(this);

		EndCmdBuffer();
		QueueSubmit(device->GetGraphicsQueue());

		device->GetGraphicsQueue().WaitQueueIdle();
	}

	void CommandBuffer::QueueSubmit(vk::Queue to_queue)
	{
		VkSubmitInfo submit_info{};
		submit_info.sType			   = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers	   = reinterpret_cast<VkCommandBuffer*>(&native_handle);

		vkQueueSubmit(to_queue, 1, &submit_info, VK_NULL_HANDLE);
	}

	void CommandBuffer::BufferBufferCopy(Buffer* from_buffer, Buffer* to_buffer, std::vector<VkBufferCopy> regions)
	{
		RecordCmds([&](CommandBuffer* handle) {
			vkCmdCopyBuffer(
				handle->native_handle,
				from_buffer->GetHandle(),
				to_buffer->GetHandle(),
				static_cast<uint32_t>(regions.size()),
				regions.data()
			);
		});
	}

	void CommandBuffer::BufferImageCopy(Buffer* from_buffer, Image* to_image)
	{
		ImageSize image_size = to_image->GetSize();

		// Sane defaults
		VkBufferImageCopy region{};
		region.bufferOffset		 = 0;
		region.bufferRowLength	 = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask	   = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel	   = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount	   = 1;

		region.imageOffset = {0, 0, 0};
		region.imageExtent = {image_size.x, image_size.y, 1};

		RecordCmds([&](CommandBuffer* with_buf) {
			vkCmdCopyBufferToImage(
				with_buf->native_handle,
				from_buffer->GetHandle(),
				static_cast<VkImage>(to_image->GetHandle()),
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&region
			);
		});
	}

	void CommandBuffer::BeginRenderPass(const VkRenderPassBeginInfo* with_info)
	{
		vkCmdBeginRenderPass(native_handle, with_info, VK_SUBPASS_CONTENTS_INLINE);
	}

	void CommandBuffer::EndRenderPass()
	{
		vkCmdEndRenderPass(native_handle);
	}

} // namespace Engine::Rendering::Vulkan
