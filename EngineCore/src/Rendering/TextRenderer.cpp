#include <assert.h>
#include <vector>
#include <array>
#include <cstring>

#include "Rendering/TextRenderer.hpp"
#include "Rendering/Texture.hpp"
#include "Rendering/Font.hpp"
#include "Logging/Logging.hpp"

#include "Engine.hpp"

namespace Engine::Rendering
{
	TextRenderer::TextRenderer(Engine* engine) : IRenderer(engine)
	{
		VulkanRenderingEngine* renderer = this->owner_engine->GetRenderingEngine();

		VulkanPipelineSettings pipeline_settings = renderer->getVulkanDefaultPipelineSettings();
		pipeline_settings.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		pipeline_settings.descriptorLayoutSettings.binding.push_back(0);
		pipeline_settings.descriptorLayoutSettings.descriptorCount.push_back(1);
		pipeline_settings.descriptorLayoutSettings.types.push_back(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		pipeline_settings.descriptorLayoutSettings.stageFlags.push_back(VK_SHADER_STAGE_VERTEX_BIT);

		pipeline_settings.descriptorLayoutSettings.binding.push_back(1);
		pipeline_settings.descriptorLayoutSettings.descriptorCount.push_back(1);
		pipeline_settings.descriptorLayoutSettings.types.push_back(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		pipeline_settings.descriptorLayoutSettings.stageFlags.push_back(VK_SHADER_STAGE_FRAGMENT_BIT);

		this->pipeline = renderer->createVulkanPipeline(pipeline_settings, "game/shaders/vulkan/textrenderer_vert.spv", "game/shaders/vulkan/textrenderer_frag.spv");

	}

	TextRenderer::~TextRenderer()
	{
		this->logger->SimpleLog(Logging::LogLevel::Debug3, "Cleaning up text renderer");
		VulkanRenderingEngine* renderer = this->owner_engine->GetRenderingEngine();

		vkDeviceWaitIdle(renderer->GetLogicalDevice());
		
		if(this->vbo != nullptr)
		{
			renderer->cleanupVulkanBuffer(this->vbo);
		}
		renderer->cleanupVulkanPipeline(this->pipeline);
	}

	void TextRenderer::Update(glm::vec3 pos, glm::vec3 size, glm::vec3 rotation, uint_fast16_t screenx, uint_fast16_t screeny, glm::vec3 parent_position, glm::vec3 parent_rotation, glm::vec3 parent_size)
	{
		this->_pos = pos;
		this->_size = size;
		this->_rotation = rotation;

		this->_parent_pos = parent_position;
		this->_parent_rotation = parent_rotation;
		this->_parent_size = parent_size;

		this->_screenx = screenx;
		this->_screeny = screeny;

		this->has_updated_mesh = false;
	}

	int TextRenderer::UpdateFont(Rendering::Font* const font)
	{
		this->font = font;

		this->has_updated_mesh = false;

		return 0;

	}

	int TextRenderer::UpdateText(const std::string text)
	{
		this->text.assign(text);
		
		this->has_updated_mesh = false;

		return 0;
	}

	void TextRenderer::UpdateMesh()
	{
		this->has_updated_mesh = true;

		/*if (this->vao == 0)
		{
			glCreateVertexArrays(1, &this->vao);
		}

		if (this->vbo == 0)
		{
			glCreateBuffers(1, &this->vbo);
		}*/

		VulkanRenderingEngine* renderer = this->owner_engine->GetRenderingEngine();

		if (this->vbo != nullptr)
		{
			vkDeviceWaitIdle(renderer->GetLogicalDevice());
			renderer->cleanupVulkanBuffer(this->vbo);
			this->vbo = nullptr;
		}

		int fontsize = std::stoi(this->font->GetFontInfoParameter("size")->c_str(), nullptr, 10);

		assert(fontsize > 0);
		
		//std::vector<GLfloat> generated_mesh = {};
		std::vector<RenderingVertex> generated_mesh = {};

		unsigned int vbo_ = 0;
		float accu_x = 0.f;
		float accu_y = 0.f;
		for (int i = 0; i < this->text.size(); i++)
		{
			vbo_ = 0;
			if (this->text[i] == '\n')
			{
				accu_y -= 0.35f * (float)(std::round(_size.y) / fontsize);
				accu_x = 0.f;
			}
			else if (this->text[i] == ' ')
			{
				accu_x += 0.05f * (float)(std::round(_size.x) / fontsize);
			}
			else
			{
				// Get character info
				Rendering::FontChar* ch = this->font->GetChar(this->text[i]);
				assert(ch != nullptr);
				if (ch == nullptr)
				{
					Logging::GlobalLogger->SimpleLog(Logging::LogLevel::Error, "Char 0x%x is not found in set font", this->text[i]);
					continue;
				}

				// Get texture
				unsigned int texture_x = 0, texture_y = 0;
				std::shared_ptr<Texture> texture = this->font->GetPageTexture(ch->page);
				assert(texture != nullptr);
				texture->GetSize(texture_x, texture_y);
				assert(texture_x > 0 && texture_y > 0);

				// Obscure math I don't understand, achieved with trial and error and works so just leave it like this
				const float xs = ch->width / (float)this->_screenx * 2 * (float)(std::round(_size.x) / fontsize);
				const float ys = ch->height / (float)this->_screeny * 2 * (float)(std::round(_size.y) / fontsize);
				const float x = (float)this->_pos.x * 2 - 1.f + accu_x;
				const float y = (float)this->_pos.y * 2 - 1.f + accu_y;

				std::array<RenderingVertex, 6> vertices = {};
				vertices[0] = { glm::vec3(x, ys + y, 0.f),		glm::vec3(1.f, 0.f, 0.f), glm::vec2((ch->x) / (float)texture_x,             (ch->y + ch->height) / (float)texture_y) };
				vertices[1] = { glm::vec3(xs + x, ys + y, 0.f),	glm::vec3(1.f, 0.f, 0.f), glm::vec2((ch->x + ch->width) / (float)texture_x, (ch->y + ch->height) / (float)texture_y) };
				vertices[2] = { glm::vec3(x, y, 0.f),			glm::vec3(1.f, 0.f, 0.f), glm::vec2((ch->x) / (float)texture_x,             (ch->y) / (float)texture_y) };
				vertices[3] = { glm::vec3(xs + x, ys + y, 0.f),	glm::vec3(1.f, 0.f, 0.f), glm::vec2((ch->x + ch->width) / (float)texture_x, (ch->y + ch->height) / (float)texture_y) };
				vertices[4] = { glm::vec3(xs + x, y, 0.f),		glm::vec3(1.f, 0.f, 0.f), glm::vec2((ch->x + ch->width) / (float)texture_x, (ch->y) / (float)texture_y) };
				vertices[5] = { glm::vec3(x, y, 0.f),			glm::vec3(1.f, 0.f, 0.f), glm::vec2((ch->x) / (float)texture_x,             (ch->y) / (float)texture_y) };

				//std::array<GLfloat, 30> data = {
				//	x, ys + y, 0.f, /**/ (ch->x) / (float)texture_x, (ch->y) / (float)texture_y,
				//	xs + x, ys + y, 0.f, /**/ (ch->x + ch->width) / (float)texture_x, (ch->y) / (float)texture_y,
				//	x, y, 0.f, /**/ (ch->x) / (float)texture_x, (ch->y + ch->height) / (float)texture_y,

				//	xs + x, ys + y, 0.f, /**/ (ch->x + ch->width) / (float)texture_x, (ch->y) / (float)texture_y,
				//	xs + x, y, 0.f, /**/ (ch->x + ch->width) / (float)texture_x, (ch->y + ch->height) / (float)texture_y,
				//	x, y, 0.f, /**/ (ch->x) / (float)texture_x, (ch->y + ch->height) / (float)texture_y
				//};

				accu_x += xs + (6.f / this->_screenx);

				generated_mesh.insert(generated_mesh.end(), vertices.begin(), vertices.end());

				this->textureImage = texture->GetTextureImage();
			}
		}

		assert(generated_mesh.size() > 0);
		//glNamedBufferData(this->vbo, generated_mesh.size() * sizeof(GLfloat), (void*)generated_mesh.data(), GL_STATIC_DRAW);
		this->vbo_vert_count = generated_mesh.size();
		
		this->vbo = renderer->createVulkanVertexBufferFromData(generated_mesh);
		
		for (size_t i = 0; i < renderer->GetMaxFramesInFlight(); i++)
		{
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = pipeline->uniformBuffers[i]->buffer;
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(glm::mat4);

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = this->textureImage->image->imageView;
			imageInfo.sampler = this->textureImage->textureSampler;

			std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = pipeline->vkDescriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = pipeline->vkDescriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(renderer->GetLogicalDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}

		//renderer->updateVulkanDescriptorSetsVulkanTextureImage(this->pipeline, this->textureImage);
	}

	void TextRenderer::Render(VkCommandBuffer commandBuffer, uint32_t currentFrame)
	{
		if (this->text.size() == 0)
		{
			return;
		}

		if (!this->has_updated_mesh)
		{
			this->UpdateMesh();
		}

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline->vkPipelineLayout, 0, 1, &this->pipeline->vkDescriptorSets[currentFrame], 0, nullptr);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline->pipeline);
		VkBuffer vertexBuffers[] = { this->vbo->buffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

		vkCmdDraw(commandBuffer, static_cast<uint32_t>(this->vbo_vert_count), 1, 0, 0);
	}
}