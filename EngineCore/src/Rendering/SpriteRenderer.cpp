#include <assert.h>
#include <cstring>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Rendering/SpriteRenderer.hpp"
#include "Rendering/Texture.hpp"
#include "Object.hpp"

#include "Engine.hpp"

namespace Engine::Rendering
{
	SpriteRenderer::SpriteRenderer(Engine* engine) : IRenderer(engine)
	{
		VulkanRenderingEngine* renderer = this->owner_engine->GetRenderingEngine();

		VulkanPipelineSettings pipeline_settings = renderer->getVulkanDefaultPipelineSettings();
		pipeline_settings.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		//pipeline_settings.rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;

		pipeline_settings.descriptorLayoutSettings.binding.push_back(0);
		pipeline_settings.descriptorLayoutSettings.descriptorCount.push_back(1);
		pipeline_settings.descriptorLayoutSettings.types.push_back(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		pipeline_settings.descriptorLayoutSettings.stageFlags.push_back(VK_SHADER_STAGE_VERTEX_BIT);

		pipeline_settings.descriptorLayoutSettings.binding.push_back(1);
		pipeline_settings.descriptorLayoutSettings.descriptorCount.push_back(1);
		pipeline_settings.descriptorLayoutSettings.types.push_back(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		pipeline_settings.descriptorLayoutSettings.stageFlags.push_back(VK_SHADER_STAGE_FRAGMENT_BIT);

		this->pipeline = renderer->createVulkanPipeline(pipeline_settings, "game/shaders/vulkan/spriterenderer_vert.spv", "game/shaders/vulkan/spriterenderer_frag.spv");

	}

	SpriteRenderer::~SpriteRenderer()
	{
		this->logger->SimpleLog(Logging::LogLevel::Debug3, "Cleaning up sprite renderer");
		VulkanRenderingEngine* renderer = this->owner_engine->GetRenderingEngine();

		vkDeviceWaitIdle(renderer->GetLogicalDevice());
		
		if(this->vbo != nullptr)
		{
			renderer->cleanupVulkanBuffer(this->vbo);
		}
		renderer->cleanupVulkanPipeline(this->pipeline);
	}

	void SpriteRenderer::Update(glm::vec3 pos, glm::vec3 size, glm::vec3 rotation, uint_fast16_t screenx, uint_fast16_t screeny, glm::vec3 parent_position, glm::vec3 parent_rotation, glm::vec3 parent_size)
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

	void SpriteRenderer::UpdateTexture(std::shared_ptr<Rendering::Texture> texture)
	{
		this->texture = texture;

		this->has_updated_mesh = false;
	}

	void SpriteRenderer::UpdateMesh()
	{
		this->has_updated_mesh = true;

		VulkanRenderingEngine* renderer = this->owner_engine->GetRenderingEngine();

		if (this->vbo == nullptr)
		{
			std::array<RenderingVertex, 6> vertices = {};
			vertices[0] = { glm::vec3(0.0, 1.0, 0.0), glm::vec3(1.f, 0.f, 0.f), glm::vec2(0.0, 1.0) };
			vertices[1] = { glm::vec3(1.0, 1.0, 0.0), glm::vec3(1.f, 0.f, 0.f), glm::vec2(1.0, 1.0) };
			vertices[2] = { glm::vec3(0.0, 0.0, 0.0), glm::vec3(1.f, 0.f, 0.f), glm::vec2(0.0, 0.0) };
			vertices[3] = { glm::vec3(1.0, 1.0, 0.0), glm::vec3(1.f, 0.f, 0.f), glm::vec2(1.0, 1.0) };
			vertices[4] = { glm::vec3(1.0, 0.0, 0.0), glm::vec3(1.f, 0.f, 0.f), glm::vec2(1.0, 0.0) };
			vertices[5] = { glm::vec3(0.0, 0.0, 0.0), glm::vec3(1.f, 0.f, 0.f), glm::vec2(0.0, 0.0) };
	
			std::vector<RenderingVertex> generated_mesh{};
			generated_mesh.insert(generated_mesh.end(), vertices.begin(), vertices.end());

			this->vbo = renderer->createVulkanVertexBufferFromData(generated_mesh);
		}

		glm::mat4 Projection = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f);

		if(this->_parent_size.x == 0.0f && this->_parent_size.y == 0.0f && this->_parent_size.z == 0.0f)
		{
			this->_parent_size = glm::vec3(1,1,1);
		}

		glm::quat ModelRotation = glm::quat(glm::radians(this->_rotation));
		glm::quat ParentRotation = glm::quat(glm::radians(this->_parent_rotation));
		glm::mat4 Model = 
						glm::scale(
							glm::translate(
								glm::scale(
									glm::translate(glm::mat4(1.0f), 
									this->_parent_pos)
										*
									glm::toMat4(
									ParentRotation),
								this->_parent_size),
							this->_pos)
								*
							glm::toMat4(
							ModelRotation),
						this->_size);

		this->matMVP = Projection * Model;

		VulkanTextureImage* textureImage = this->texture->GetTextureImage();
		for (size_t i = 0; i < renderer->GetMaxFramesInFlight(); i++)
		{
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = pipeline->uniformBuffers[i]->buffer;
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(glm::mat4);

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = textureImage->image->imageView;
			imageInfo.sampler = textureImage->textureSampler;

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
	}

	void SpriteRenderer::Render(VkCommandBuffer commandBuffer, uint32_t currentFrame)
	{
		if (!this->has_updated_mesh)
		{
			this->UpdateMesh();
		}

		VulkanRenderingEngine* renderer = this->owner_engine->GetRenderingEngine();
		vkMapMemory(renderer->GetLogicalDevice(),
			pipeline->uniformBuffers[currentFrame]->allocationInfo.deviceMemory,
			pipeline->uniformBuffers[currentFrame]->allocationInfo.offset,
			pipeline->uniformBuffers[currentFrame]->allocationInfo.size,
			0,
			&(pipeline->uniformBuffers[currentFrame]->mappedData));

		memcpy(this->pipeline->uniformBuffers[currentFrame]->mappedData, &this->matMVP, sizeof(glm::mat4));
		vkUnmapMemory(renderer->GetLogicalDevice(), pipeline->uniformBuffers[currentFrame]->allocationInfo.deviceMemory);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline->vkPipelineLayout, 0, 1, &this->pipeline->vkDescriptorSets[currentFrame], 0, nullptr);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline->pipeline);
		VkBuffer vertexBuffers[] = { this->vbo->buffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

		vkCmdDraw(commandBuffer, 6, 1, 0, 0);
	}
}