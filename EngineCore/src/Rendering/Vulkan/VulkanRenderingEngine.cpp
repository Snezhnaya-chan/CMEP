#include "Rendering/Vulkan/VulkanRenderingEngine.hpp"

#include "Rendering/Vulkan/ImportVulkan.hpp"
#include "Rendering/Vulkan/VBuffer.hpp"
#include "Rendering/Vulkan/VCommandBuffer.hpp"
#include "Rendering/Vulkan/VCommandPool.hpp" // IWYU pragma: keep
#include "Rendering/Vulkan/VDeviceManager.hpp"
#include "Rendering/Vulkan/VImage.hpp" // IWYU pragma: keep
#include "Rendering/Vulkan/VPipeline.hpp"
#include "Rendering/Vulkan/VSwapchain.hpp"

#include "Logging/Logging.hpp"

#include "Engine.hpp"
#include "GLFW/glfw3.h"
#include "vulkan/vulkan_core.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <utility>

// Prefixes for logging messages
#define LOGPFX_CURRENT LOGPFX_CLASS_VULKAN_RENDERING_ENGINE
#include "Logging/LoggingPrefix.hpp"

namespace Engine::Rendering::Vulkan
{
	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		auto* app = reinterpret_cast<VulkanRenderingEngine*>(glfwGetWindowUserPointer(window));
		app->SignalFramebufferResizeGLFW({static_cast<uint_fast16_t>(width), static_cast<uint_fast16_t>(height)});
	}

	////////////////////////////////////////////////////////////////////////
	///////////////////////    Runtime functions    ////////////////////////
	////////////////////////////////////////////////////////////////////////

	void VulkanRenderingEngine::RecordVulkanCommandBuffer(VCommandBuffer* command_buffer, uint32_t image_index)
	{
		VkCommandBufferBeginInfo begin_info{};
		begin_info.sType			= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags			= 0;	   // Optional
		begin_info.pInheritanceInfo = nullptr; // Optional

		command_buffer->BeginCmdBuffer(0);

		auto& command_buf_handle = command_buffer->GetNativeHandle();

		VkRenderPassBeginInfo render_pass_info{};
		render_pass_info.sType			   = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_info.renderPass		   = this->vk_render_pass;
		render_pass_info.framebuffer	   = this->vk_swap_chain_framebuffers[image_index];
		render_pass_info.renderArea.offset = {0, 0};
		render_pass_info.renderArea.extent = this->swapchain->GetExtent();

		std::array<VkClearValue, 2> clear_values{};
		clear_values[0].color		 = {{0.0f, 0.0f, 0.0f, 1.0f}}; // TODO: configurable
		clear_values[1].depthStencil = {1.0f, 0};

		render_pass_info.clearValueCount = 2;
		render_pass_info.pClearValues	 = clear_values.data();

		vkCmdBeginRenderPass(command_buf_handle, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x		  = 0.0f;
		viewport.y		  = 0.0f;
		viewport.width	  = static_cast<float>(this->swapchain->GetExtent().width);
		viewport.height	  = static_cast<float>(this->swapchain->GetExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(command_buf_handle, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = this->swapchain->GetExtent();
		vkCmdSetScissor(command_buf_handle, 0, 1, &scissor);

		// Perform actual render
		// vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->graphics_pipeline_default->pipeline);
		if (this->external_callback)
		{
			this->external_callback(command_buf_handle, current_frame, this->owner_engine);
		}

		vkCmdEndRenderPass(command_buf_handle);

		command_buffer->EndCmdBuffer();
	}

	VkExtent2D VulkanRenderingEngine::ChooseVulkanSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}

		int width;
		int height;
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D actual_extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

		actual_extent.width =
			std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actual_extent.height =
			std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actual_extent;
	}

	uint32_t VulkanRenderingEngine::FindVulkanMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties mem_properties;
		vkGetPhysicalDeviceMemoryProperties(this->device_manager->GetPhysicalDevice(), &mem_properties);

		for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
		{
			if ((type_filter & (1 << i)) != 0 &&
				(mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		this->logger->SimpleLog(Logging::LogLevel::Exception, LOGPFX_CURRENT "Failed to find required memory type");
		throw std::runtime_error("failed to find required memory type!");
	}

	VkFormat VulkanRenderingEngine::FindVulkanSupportedFormat(
		const std::vector<VkFormat>& candidates,
		VkImageTiling tiling,
		VkFormatFeatureFlags features
	)
	{
		for (VkFormat format : candidates)
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(this->device_manager->GetPhysicalDevice(), format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
			{
				return format;
			}

			if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
			{
				return format;
			}
		}

		this->logger->SimpleLog(Logging::LogLevel::Exception, LOGPFX_CURRENT "Failed to find supported vkFormat");
		throw std::runtime_error("failed to find supported vkFormat!");
	}

	VkFormat VulkanRenderingEngine::FindVulkanSupportedDepthFormat()
	{
		return this->FindVulkanSupportedFormat(
			{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	////////////////////////////////////////////////////////////////////////
	////////////////////////    Init functions    //////////////////////////
	////////////////////////////////////////////////////////////////////////

	VulkanRenderingEngine::VulkanRenderingEngine(Engine* with_engine, ScreenSize with_window_size, std::string title)
		: InternalEngineObject(with_engine), window_size(with_window_size), window_title(std::move(title))
	{
		// Initialize GLFW
		if (glfwInit() == GLFW_FALSE)
		{
			throw std::runtime_error("GLFW returned GLFW_FALSE on glfwInit!");
		}
		this->logger->SimpleLog(Logging::LogLevel::Info, LOGPFX_CURRENT "GLFW initialized");

		// Create a GLFW window
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

		// Quick hack for i3wm
		// (this one is likely unnecessary and can be left commented out)
		// glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
		//
		// TODO: Fix i3wm
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		this->window = glfwCreateWindow(
			static_cast<int>(this->window_size.x),
			static_cast<int>(this->window_size.y),
			this->window_title.c_str(),
			nullptr, // glfwGetPrimaryMonitor(),
			nullptr
		);
		glfwSetWindowUserPointer(this->window, this);
		glfwSetFramebufferSizeCallback(this->window, FramebufferResizeCallback);

		uint32_t extension_count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

		this->logger
			->SimpleLog(Logging::LogLevel::Debug1, LOGPFX_CURRENT "%u vulkan extensions supported", extension_count);

		this->device_manager = std::make_shared<VDeviceManager>(this->owner_engine, this->window);

		// this->CreateVulkanMemoryAllocator();
		this->CreateVulkanSwapChain();
		this->CreateVulkanRenderPass();

		// Create command buffers
		for (auto& vk_command_buffer : this->command_buffers)
		{
			vk_command_buffer = new VCommandBuffer(this->device_manager.get(), this->device_manager->GetCommandPool());
		}

		this->CreateMultisampledColorResources();
		this->CreateVulkanDepthResources();
		this->CreateVulkanFramebuffers();
		this->CreateVulkanSyncObjects();
	}

	VulkanRenderingEngine::~VulkanRenderingEngine()
	{
		VkDevice logical_device = this->device_manager->GetLogicalDevice();

		this->logger->SimpleLog(Logging::LogLevel::Info, LOGPFX_CURRENT "Cleaning up");

		vkDeviceWaitIdle(logical_device);

		this->CleanupVulkanSwapChain();

		delete this->multisampled_color_image;
		delete this->vk_depth_buffer;

		for (size_t i = 0; i < VulkanRenderingEngine::max_frames_in_flight; i++)
		{
			vkDestroySemaphore(logical_device, this->sync_objects[i].present_ready, nullptr);
			vkDestroySemaphore(logical_device, this->sync_objects[i].image_available, nullptr);
			vkDestroyFence(logical_device, this->sync_objects[i].in_flight, nullptr);

			vkWaitForFences(logical_device, 1, &(this->sync_objects[i].acquire_ready), VK_TRUE, UINT64_MAX);
			vkDestroyFence(logical_device, this->sync_objects[i].acquire_ready, nullptr);
		}

		for (auto& vk_command_buffer : this->command_buffers)
		{
			delete vk_command_buffer;
		}

		this->logger->SimpleLog(Logging::LogLevel::Debug3, LOGPFX_CURRENT "Cleaning up default vulkan pipeline");

		vkDestroyRenderPass(logical_device, this->vk_render_pass, nullptr);

		// VMA Cleanup
		// vmaDestroyAllocator(this->vma_allocator);

		// Destroy device after VMA
		this->device_manager.reset();

		// Clean up GLFW
		glfwDestroyWindow(this->window);
		glfwTerminate();
	}

	// Rest of section moved to 'VulkanRenderingEngine_Init.cpp'
	//

	////////////////////////////////////////////////////////////////////////
	///////////////////////    Public Interface    /////////////////////////
	////////////////////////////////////////////////////////////////////////

	void VulkanRenderingEngine::DrawFrame()
	{
		VkDevice logical_device = this->device_manager->GetLogicalDevice();

		auto& frame_sync_objects = this->sync_objects[this->current_frame];

		// Wait for fence
		vkWaitForFences(logical_device, 1, &frame_sync_objects.in_flight, VK_TRUE, UINT64_MAX);
		vkWaitForFences(logical_device, 1, &frame_sync_objects.acquire_ready, VK_TRUE, UINT64_MAX);

		// Reset fence after wait is over
		// (fence has to be reset before being used again)
		vkResetFences(logical_device, 1, &frame_sync_objects.in_flight);
		vkResetFences(logical_device, 1, &frame_sync_objects.acquire_ready);

		// Index of framebuffer in this->vk_swap_chain_framebuffers
		uint32_t image_index;
		// Acquire render target
		// the render target is an image in the swap chain
		VkResult acquire_result = vkAcquireNextImageKHR(
			logical_device,
			this->swapchain->GetNativeHandle(),
			UINT64_MAX,
			frame_sync_objects.image_available,
			frame_sync_objects.acquire_ready,
			&image_index
		);

		// If it's necessary to recreate a swap chain
		if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR || this->framebuffer_resized ||
			acquire_result == VK_SUBOPTIMAL_KHR)
		{
			// Increment current_frame (clamp to max_frames_in_flight)
			// this->current_frame		  = (this->current_frame + 1) % this->max_frames_in_flight;
			this->framebuffer_resized = false;

			this->logger->SimpleLog(
				Logging::LogLevel::Warning,
				"acquire_result was VK_ERROR_OUT_OF_DATE_KHR or VK_SUBOPTIMAL_KHR, or framebuffer was resized!"
			);

			this->RecreateVulkanSwapChain();

			return;
		}

		if (acquire_result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to acquire swap chain image!");
		}

		// Reset command buffer to initial state
		vkResetCommandBuffer(this->command_buffers[this->current_frame]->GetNativeHandle(), 0);

		// Records render into command buffer
		this->RecordVulkanCommandBuffer(this->command_buffers[this->current_frame], image_index);

		VkSubmitInfo submit_info{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		// Wait semaphores
		VkSemaphore wait_semaphores[]	   = {frame_sync_objects.image_available};
		VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submit_info.waitSemaphoreCount	   = 1;
		submit_info.pWaitSemaphores		   = wait_semaphores;
		submit_info.pWaitDstStageMask	   = wait_stages;
		submit_info.commandBufferCount	   = 1;
		submit_info.pCommandBuffers		   = &this->command_buffers[this->current_frame]->GetNativeHandle();

		// Signal semaphores to be signaled once
		// all submit_info.pCommandBuffers finish executing
		VkSemaphore signal_semaphores[]	 = {frame_sync_objects.present_ready};
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores	 = signal_semaphores;

		// Submit to queue
		// passed fence will be signaled when command buffer execution is finished
		if (vkQueueSubmit(this->device_manager->GetGraphicsQueue(), 1, &submit_info, frame_sync_objects.in_flight) !=
			VK_SUCCESS)
		{
			throw std::runtime_error("Failed to submit draw command buffer!");
		}

		// Increment current frame
		// this->current_frame = 0;
		this->current_frame = (this->current_frame + 1) % VulkanRenderingEngine::max_frames_in_flight;

		VkPresentInfoKHR present_info{};
		present_info.sType				= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		// Wait for present_ready_semaphores to be signaled
		// (when signaled = image is ready to be presented)
		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores	= signal_semaphores;

		VkSwapchainKHR swap_chains[] = {this->swapchain->GetNativeHandle()};
		present_info.swapchainCount	 = 1;
		present_info.pSwapchains	 = swap_chains;
		present_info.pImageIndices	 = &image_index;
		present_info.pResults		 = nullptr; // Optional

		// Present current image to the screen
		if (vkQueuePresentKHR(this->device_manager->GetPresentQueue(), &present_info) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to present queue!");
		}
	}

	GLFWwindowData VulkanRenderingEngine::GetWindow() const
	{
		GLFWwindowData data{};
		data.window		  = this->window;
		data.window_x	  = this->window_size.x;
		data.window_y	  = this->window_size.y;
		data.window_title = this->window_title;

		return data;
	}

	void VulkanRenderingEngine::SetRenderCallback(std::function<void(VkCommandBuffer, uint32_t, Engine*)> callback)
	{
		this->external_callback = std::move(callback);
	}

	void VulkanRenderingEngine::SyncDeviceWaitIdle()
	{
		vkDeviceWaitIdle(this->device_manager->GetLogicalDevice());
	}

	void VulkanRenderingEngine::SignalFramebufferResizeGLFW(ScreenSize with_size)
	{
		this->framebuffer_resized = true;
		this->window_size		  = with_size;
	}

	// Pipelines

	VulkanPipelineSettings VulkanRenderingEngine::GetVulkanDefaultPipelineSettings()
	{
		VkPipelineInputAssemblyStateCreateInfo input_assembly{};
		input_assembly.sType				  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		input_assembly.topology				  = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		input_assembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport{};
		viewport.x		  = 0.0f;
		viewport.y		  = 0.0f;
		viewport.width	  = static_cast<float>(this->swapchain->GetExtent().width);
		viewport.height	  = static_cast<float>(this->swapchain->GetExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = this->swapchain->GetExtent();

		VkPipelineViewportStateCreateInfo viewport_state{};
		viewport_state.sType		 = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_state.viewportCount = 1;
		viewport_state.pViewports	 = &viewport;
		viewport_state.scissorCount	 = 1;
		viewport_state.pScissors	 = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType				   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable		   = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode			   = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth			   = 1.0f;
		rasterizer.cullMode				   = VK_CULL_MODE_FRONT_BIT;
		rasterizer.frontFace			   = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable		   = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; // Optional
		rasterizer.depthBiasClamp		   = 0.0f; // Optional
		rasterizer.depthBiasSlopeFactor	   = 0.0f; // Optional

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType					= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable	= VK_FALSE;
		multisampling.rasterizationSamples	= this->device_manager->GetMSAASampleCount();
		multisampling.minSampleShading		= 1.0f;		// Optional
		multisampling.pSampleMask			= nullptr;	// Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable		= VK_FALSE; // Optional

		VkPipelineColorBlendAttachmentState color_blend_attachment{};
		color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
												VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		color_blend_attachment.blendEnable		   = VK_TRUE;
		color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		color_blend_attachment.colorBlendOp		   = VK_BLEND_OP_ADD;
		color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		color_blend_attachment.alphaBlendOp		   = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo color_blending{};
		color_blending.sType			 = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blending.logicOpEnable	 = VK_FALSE;
		color_blending.logicOp			 = VK_LOGIC_OP_COPY;
		color_blending.attachmentCount	 = 1;
		color_blending.pAttachments		 = &color_blend_attachment;
		color_blending.blendConstants[0] = 0.0f; // Optional
		color_blending.blendConstants[1] = 0.0f; // Optional
		color_blending.blendConstants[2] = 0.0f; // Optional
		color_blending.blendConstants[3] = 0.0f; // Optional

		VkPipelineDepthStencilStateCreateInfo depth_stencil{};
		depth_stencil.sType					= VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depth_stencil.depthTestEnable		= VK_TRUE;
		depth_stencil.depthWriteEnable		= VK_TRUE;
		depth_stencil.depthCompareOp		= VK_COMPARE_OP_LESS;
		depth_stencil.depthBoundsTestEnable = VK_FALSE;
		depth_stencil.minDepthBounds		= 0.0f; // Optional
		depth_stencil.maxDepthBounds		= 1.0f; // Optional
		depth_stencil.stencilTestEnable		= VK_FALSE;
		depth_stencil.front					= {}; // Optional
		depth_stencil.back					= {}; // Optional

		VulkanPipelineSettings default_settings{};
		default_settings.input_assembly				= input_assembly;
		default_settings.viewport					= viewport;
		default_settings.scissor					= scissor;
		default_settings.viewport_state				= viewport_state;
		default_settings.rasterizer					= rasterizer;
		default_settings.multisampling				= multisampling;
		default_settings.color_blend_attachment		= color_blend_attachment;
		default_settings.color_blending				= color_blending;
		default_settings.depth_stencil				= depth_stencil;
		default_settings.descriptor_layout_settings = {};

		return default_settings;
	}

	VPipeline* VulkanRenderingEngine::CreateVulkanPipeline(VulkanPipelineSettings& settings)
	{
		auto* new_pipeline = new VPipeline(this->device_manager.get(), settings, this->vk_render_pass);

		return new_pipeline;
	}

	// Buffers

	VBuffer* VulkanRenderingEngine::CreateVulkanVertexBufferFromData(std::vector<RenderingVertex> vertices)
	{
		VBuffer* staging_buffer = nullptr;
		VBuffer* vertex_buffer	= nullptr;

		VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

		staging_buffer = new VBuffer(
			this->device_manager.get(),
			buffer_size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			0
		);
		vertex_buffer = new VBuffer(
			this->device_manager.get(),
			buffer_size,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			0
		);

		staging_buffer->MapMemory();

		memcpy(staging_buffer->mapped_data, vertices.data(), static_cast<size_t>(buffer_size));

		staging_buffer->UnmapMemory();

		vertex_buffer->BufferCopy(staging_buffer, buffer_size);

		delete staging_buffer;

		return vertex_buffer;
	}

	VBuffer* VulkanRenderingEngine::CreateVulkanStagingBufferWithData(void* data, VkDeviceSize data_size)
	{
		VBuffer* staging_buffer;

		staging_buffer = new VBuffer(
			this->device_manager.get(),
			data_size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			0
		);

		staging_buffer->MapMemory();

		memcpy(staging_buffer->mapped_data, data, static_cast<size_t>(data_size));

		staging_buffer->UnmapMemory();

		return staging_buffer;
	}

	// Image functions

	void VulkanRenderingEngine::CopyVulkanBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
	{
		VCommandBuffer command_buffer(this->device_manager.get(), this->device_manager->GetCommandPool());

		VkBufferImageCopy region{};
		region.bufferOffset		 = 0;
		region.bufferRowLength	 = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask	   = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel	   = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount	   = 1;

		region.imageOffset = {0, 0, 0};
		region.imageExtent = {width, height, 1};

		command_buffer.RecordCmds([&](VCommandBuffer* with_buf) {
			vkCmdCopyBufferToImage(
				with_buf->GetNativeHandle(),
				buffer,
				image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&region
			);
		});
	}
} // namespace Engine::Rendering::Vulkan
