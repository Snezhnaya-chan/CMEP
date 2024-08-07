#include "InternalEngineObject.hpp"

#include <cstddef>
#include <cstdint>

#pragma warning(push, 2)
#include "lodepng.h"
#pragma warning(pop)

#include "Rendering/Vulkan/VDeviceManager.hpp"
#include "Rendering/Vulkan/Wrappers/VCommandBuffer.hpp"
#include "Rendering/Vulkan/Wrappers/VCommandPool.hpp"

#include "Factories/TextureFactory.hpp"

#include "Engine.hpp"

// Prefixes for logging messages
#define LOGPFX_CURRENT LOGPFX_CLASS_TEXTURE_FACTORY
#include "Logging/LoggingPrefix.hpp"

namespace Engine::Factories
{
	std::shared_ptr<Rendering::Texture> TextureFactory::InitFile(
		const std::string& path,
		Rendering::Vulkan::VBuffer* staging_buffer,
		Rendering::Texture_InitFiletype filetype,
		VkFilter filtering,
		VkSamplerAddressMode sampler_address_mode
	)
	{
		FILE* file = fopen(path.c_str(), "rb");
		if (file == nullptr)
		{
			throw std::runtime_error("Could not find texture '" + path + "'!");
		}

		this->logger
			->SimpleLog(Logging::LogLevel::Debug2, LOGPFX_CURRENT "Initializing texture from file %s", path.c_str());

		std::vector<unsigned char> data;
		std::unique_ptr<Rendering::TextureData> texture_data = std::make_unique<Rendering::TextureData>();

		switch (filetype)
		{
			case Rendering::Texture_InitFiletype::FILE_PNG:
			{
				unsigned int size_x;
				unsigned int size_y;
				unsigned error = lodepng::decode(data, size_x, size_y, path);

				assert(error == 0);

				this->logger->SimpleLog(
					Logging::LogLevel::Debug1,
					LOGPFX_CURRENT "Decoded png file %s; width %u; height %u; filter %u",
					path.c_str(),
					size_x,
					size_y,
					filtering
				);

				assert(0 < size_x && size_x < 0x2fff);
				assert(0 < size_y && size_y < 0x2fff);

				this->InitRaw(
					texture_data,
					staging_buffer,
					std::move(data),
					4,
					filtering,
					sampler_address_mode,
					size_x,
					size_y
				);
				break;
			}
			default:
			{
				throw std::runtime_error("Unknown texture filetype passed to TextureFactory!");
			}
		}

		fclose(file);

		std::shared_ptr<Rendering::Texture> texture =
			std::make_shared<Rendering::Texture>(this->owner_engine, std::move(texture_data));

		return texture;
	}

	int TextureFactory::InitRaw(
		std::unique_ptr<Rendering::TextureData>& texture_data,
		Rendering::Vulkan::VBuffer* staging_buffer,
		std::vector<unsigned char> raw_data,
		int color_format,
		VkFilter filtering,
		VkSamplerAddressMode sampler_address_mode,
		unsigned int xsize,
		unsigned int ysize
	)
	{
		uint_fast8_t channel_count = 4;

		texture_data->data		= raw_data;
		texture_data->color_fmt = color_format;
		texture_data->size		= {xsize, ysize};

		auto memory_size = static_cast<VkDeviceSize>(xsize * ysize) * channel_count;

		Rendering::Vulkan::VulkanRenderingEngine* renderer = this->owner_engine->GetRenderingEngine();

		Rendering::Vulkan::VBuffer* used_staging_buffer = nullptr;

		// If no valid buffer was passed then create one here
		if (staging_buffer == nullptr)
		{
			used_staging_buffer = new Rendering::Vulkan::VBuffer(
				renderer->GetDeviceManager(),
				static_cast<size_t>(memory_size),
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			);
		}
		else
		{
			// If it was, use that one
			used_staging_buffer = staging_buffer;
		}

		if (auto* device_manager = renderer->GetDeviceManager())
		{
			used_staging_buffer->MapMemory();

			memcpy(used_staging_buffer->mapped_data, raw_data.data(), static_cast<size_t>(memory_size));

			texture_data->texture_image = new Rendering::Vulkan::VSampledImage(
				device_manager,
				{xsize, ysize},
				VK_SAMPLE_COUNT_1_BIT,
				VK_FORMAT_R8G8B8A8_UNORM,
				VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				filtering,			 // Filter for both mag and min
				sampler_address_mode // sampler address mode
			);

			// Transfer image layout to compatible with transfers
			texture_data->texture_image->TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			auto* command_buffer = device_manager->GetCommandPool()->AllocateCommandBuffer();

			command_buffer->BufferImageCopy(used_staging_buffer, texture_data->texture_image);

			delete command_buffer;

			// Transfer image layout to compatible with rendering
			texture_data->texture_image->TransitionImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			// Unmap staging memory and cleanup buffer if we created it here
			used_staging_buffer->UnmapMemory();

			if (staging_buffer == nullptr)
			{
				delete used_staging_buffer;
			}

			texture_data->texture_image->AddImageView();
		}
		return 0;
	}
} // namespace Engine::Factories
